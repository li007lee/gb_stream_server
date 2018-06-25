/*
 * get_frame_from_box.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */
#include "my_include.h"
#include "rtp_pack.h"
#include "event.h"
#include "send_av_data.h"
#include "cJSON.h"
#include "server_config.h"
#include "net_api.h"
#include "hash_table.h"
#include "lf_queue.h"
#include "common_args.h"

extern STREAM_HASH_TABLE_HANDLE stream_hash_table;
extern stpool_t *gb_thread_pool;

static HB_S32 parse_sdp_info(STREAM_NODE_HANDLE rtsp_node, HB_CHAR *sdp_json)
{
	cJSON *p_json = NULL;
	cJSON *p_sub = NULL;
	p_json = cJSON_Parse(sdp_json);
	if(NULL == p_json)
	{
		return HB_FAILURE;
	}
	p_sub = cJSON_GetObjectItem(p_json, "CmdType");
	if(NULL == p_sub)
	{
		cJSON_Delete(p_json);
		return HB_FAILURE;
	}
	if(!strcmp(p_sub->valuestring, "sdp_info"))
	{
		p_sub = cJSON_GetObjectItem(p_json, "m_video");
		if(p_sub != NULL)
		{
			if(strlen(p_sub->valuestring))
			{
				strcpy(rtsp_node->stSdpInfo.m_video, p_sub->valuestring);
			}
		}
		p_sub = cJSON_GetObjectItem(p_json, "a_rtpmap_video");
		if(p_sub != NULL)
		{
			if(strlen(p_sub->valuestring))
			{
				strcpy(rtsp_node->stSdpInfo.a_rtpmap_video, p_sub->valuestring);
			}
		}

		p_sub = cJSON_GetObjectItem(p_json, "a_fmtp_video");
		if(p_sub != NULL)
		{
			if(strlen(p_sub->valuestring))
			{
				strcpy(rtsp_node->stSdpInfo.a_fmtp_video, p_sub->valuestring);
			}
		}

#if 0
		p_sub = cJSON_GetObjectItem(p_json, "m_audio");
		strcpy(rtsp_node->sdp_info.m_audio, p_sub->valuestring);
		p_sub = cJSON_GetObjectItem(p_json, "a_rtpmap_audio");
		strcpy(rtsp_node->sdp_info.a_rtpmap_audio, p_sub->valuestring);
#endif
		cJSON_Delete(p_json);
		return HB_SUCCESS;
	}
	else
	{
		cJSON_Delete(p_json);
		return HB_FAILURE;
	}
}


#if 0
static HB_S32 delet_rtp_data_list(list_t *p_stream_data_node_head)
{
	HB_S32 data_node_nums = 0;
	HB_CHAR *data_node = NULL;
	data_node_nums = list_size(p_stream_data_node_head);
	while(data_node_nums)
	{
		data_node = list_get_at(p_stream_data_node_head, 0);
		list_delete(p_stream_data_node_head, data_node);
		free(data_node);
		data_node_nums--;
	}
	list_destroy(p_stream_data_node_head);
	return 0;
}
#endif

static HB_VOID delete_rtp_data_list(lf_queue queue)
{
	QUEUE_ARGS_OBJ queue_out;
	while(nolock_queue_len(queue) > 0)
	{
		nolock_queue_pop(queue, &queue_out);
		if(queue_out.data_buf != NULL)
		{
#if JE_MELLOC_FUCTION
			je_free(queue_out.data_buf);
#else
			free(queue_out.data_buf);
#endif
			queue_out.data_buf = NULL;
		}
	}
	return;
}

static HB_S32 destroy_client_rtp_list(list_t *listClientNodeHead)
{
	HB_S32 rtp_client_nums = 0;
	RTP_CLIENT_TRANSPORT_HANDLE client_node = NULL;
	rtp_client_nums = list_size(listClientNodeHead);
	while(rtp_client_nums)
	{
		client_node = list_get_at(listClientNodeHead, 0);
		list_delete(listClientNodeHead, client_node);
		if(client_node->pSendStreamBev != NULL)
		{
			bufferevent_disable(client_node->pSendStreamBev, EV_READ|EV_WRITE);
			bufferevent_free(client_node->pSendStreamBev);
			client_node->pSendStreamBev = NULL;
		}
		if(client_node->hEventArgs != NULL)
		{
			free(client_node->hEventArgs);
			client_node->hEventArgs = NULL;
		}
		if (client_node->iUdpVideoFd > 0)
		{
			close(client_node->iUdpVideoFd);
			client_node->iUdpVideoFd = -1;
		}

		free(client_node);
		client_node = NULL;
		rtp_client_nums--;
	}
	list_destroy(listClientNodeHead);
	return 0;
}



static HB_S32 disable_client_rtp_list_bev(list_t *listClientNodeHead)
{
	HB_S32 rtp_client_nums = 0;
	HB_CHAR *rtp_client_node = NULL;
	rtp_client_nums = list_size(listClientNodeHead);
	while(rtp_client_nums)
	{
		rtp_client_node = list_get_at(listClientNodeHead, rtp_client_nums-1);
		RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE)rtp_client_node;
		if(client_node->pSendStreamBev != NULL)
		{
			bufferevent_disable(client_node->pSendStreamBev, EV_READ|EV_WRITE);
		}
		rtp_client_nums--;
	}
	return 0;
}


// 异常事件回调函数
static HB_VOID client_event_cb(struct bufferevent *get_push_stream_bev, HB_S16 events, HB_VOID *args)
{
	STREAM_NODE_HANDLE p_stream_node = (STREAM_NODE_HANDLE)args;
	HB_S32 hash_value = 0;
	if (events & (BEV_EVENT_EOF|BEV_EVENT_ERROR))
	{
		if (events & BEV_EVENT_ERROR)
		{
			printf("\n######client_event_cb connect error !\n");
		}
		else
		{
			printf("\n#######client_event_cb normal closed!\n");
		}
	}
	else if (events & BEV_EVENT_TIMEOUT)//超时事件
	{
		printf("\n######client_event_cb  timeout !\n");
	}
	else
	{
		printf("\n######client_event_cb other err !\n");
	}

	//发生异常
	bufferevent_free(get_push_stream_bev);
	get_push_stream_bev = NULL;
	hash_value = p_stream_node->iStreamNodeHashValue;
	//printf("\n@@@@@@@@@@@@@@@@@@  recv_stream_cb() err  ret=%d\n", ret);
	if(0 == p_stream_node->uRtpClientNodeSendDataThreadFlag)//rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 0");
		pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
		pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		destroy_client_rtp_list(&(p_stream_node->listClientNodeHead));//释放rtp客户队列
		delete_rtp_data_list(p_stream_node->queueStreamData);//释放视频队列
		nolock_queue_destroy(&(p_stream_node->queueStreamData));
		if(p_stream_node->hGetStreamFromSource != NULL)
		{
			free(p_stream_node->hGetStreamFromSource);
			p_stream_node->hGetStreamFromSource = NULL;
		}
		list_destroy(&(p_stream_node->listClientNodeHead));
		free(p_stream_node);
		p_stream_node=NULL;
		return;
	}
	else if(1 == p_stream_node->uRtpClientNodeSendDataThreadFlag)//如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 1");
		pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(p_stream_node->listClientNodeHead));
		list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
		pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		p_stream_node->uGetStreamThreadStartFlag = 2;//接受数据流模块已退出
		return;
	}
	else if(2 == p_stream_node->uRtpClientNodeSendDataThreadFlag) //rtp发送线程启动后，异常标志置位,这里只置位接收模块标志，摘除释放工作由rtp发送线程执行
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 2");
		pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(p_stream_node->listClientNodeHead));
		list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
		pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		p_stream_node->uGetStreamThreadStartFlag = 2;//接受数据流模块已退出
	}
	return;
}


#if 1
static HB_VOID client_read_cb(struct bufferevent *get_push_stream_bev, HB_VOID *args)
{
	STREAM_NODE_HANDLE p_stream_node = (STREAM_NODE_HANDLE)args;
	HB_S32 hash_value = 0;
	HB_S32 ret = 0;
	struct sttask *ptsk;
	BOX_CTRL_CMD_OBJ cmd_head;
	memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
	HB_S32 data_len = 0;
	struct evbuffer *evbuf = NULL;
	evbuf = bufferevent_get_input(get_push_stream_bev);
	for(;;)
	{
		if(evbuffer_get_length(evbuf) < 32)
		{
			return;
		}
		memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
		ret = evbuffer_copyout(evbuf, (HB_VOID*)(&cmd_head), sizeof(BOX_CTRL_CMD_OBJ));
		//printf("\n#########  cmd_head:[%s]  len:[%d]\n", cmd_head.header, ret);
		if(strncmp(cmd_head.header, "hBzHbox@", 8))
		{
			ret = -101;
			break;
		}
		data_len = (cmd_head.cmd_length);
		if(evbuffer_get_length(evbuf) < (data_len + sizeof(BOX_CTRL_CMD_OBJ)))
		{
			return;
		}

		if(0 == p_stream_node->uRtpClientNodeSendDataThreadFlag)//如果rtp发送线程没有启动，则启动rtp包发送线程
		{
#if USE_PTHREAD_POOL
			ptsk = stpool_task_new(gb_thread_pool, "send_rtp_to_client_task", send_rtp_to_client_task, NULL, p_stream_node);
			HB_S32 error = stpool_task_set_p(ptsk, gb_thread_pool);
			if (error)
			{
				printf("***Err: %d(%s). (try eCAP_F_CUSTOM_TASK)\n", error, stpool_strerror(error));
				ret = -102;
				break;
			}
			else
			{
				stpool_task_set_userflags(ptsk, 0x1);
				stpool_task_queue(ptsk);
				p_stream_node->uRtpClientNodeSendDataThreadFlag = 1; //发送rtp数据线程已经启动
			}
#else
			pthread_t data_send_pthread_id;
			if(pthread_create(&data_send_pthread_id, NULL, send_rtp_to_client_thread, (HB_VOID *)pth_args) != 0)
			{
				pthread_rwlock_wrlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].dev_mutex));

				pthread_rwlock_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].dev_mutex));
				return;
			}
			else
			{
				p_stream_node->uRtpClientNodeSendDataThreadFlag = 1; //发送rtp数据线程已经启动
			}
#endif
		}
//		if(2 == p_stream_node->uRtpClientNodeSendDataThreadFlag || 2 == p_stream_node->rtsp_play_flag)
		if((0 == list_size(&(p_stream_node->listClientNodeHead))) || (2 == p_stream_node->uRtpClientNodeSendDataThreadFlag))
		{
			break;
		}
		if(nolock_queue_len(p_stream_node->queueStreamData) > 512)
		{
			break;
		}
		HB_S32 rtp_data_buf_pre_size = (((data_len/MAX_RTP_PACKET_SIZE)+1)*20)+MAX_RTP_PACKET_SIZE;
		//HB_S32 rtp_data_buf_pre_size = 10240;
		HB_S32 rtp_data_buf_len = data_len + rtp_data_buf_pre_size;
		QUEUE_ARGS_OBJ queue_arg;
#if JE_MELLOC_FUCTION
		queue_arg.data_buf = (HB_CHAR*)je_malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#else
		queue_arg.data_buf = (HB_CHAR*)malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#endif
		queue_arg.data_len = data_len + sizeof(BOX_CTRL_CMD_OBJ);
		queue_arg.data_pre_buf_size = rtp_data_buf_pre_size;
		ret = bufferevent_read(get_push_stream_bev, (HB_VOID*)(queue_arg.data_buf+rtp_data_buf_pre_size), (data_len + sizeof(BOX_CTRL_CMD_OBJ)));
		nolock_queue_push(p_stream_node->queueStreamData, &queue_arg);
		return;
	}
	//发生异常
	bufferevent_free(get_push_stream_bev);
	get_push_stream_bev = NULL;
	hash_value = p_stream_node->iStreamNodeHashValue;
	//printf("\n@@@@@@@@@@@@@@@@@@  recv_stream_cb() err  ret=%d\n", ret);
	if(0 == p_stream_node->uRtpClientNodeSendDataThreadFlag)//rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 0");
		pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
		pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		destroy_client_rtp_list(&(p_stream_node->listClientNodeHead));//释放rtp客户队列
		delete_rtp_data_list(p_stream_node->queueStreamData);//释放视频队列
		nolock_queue_destroy(&(p_stream_node->queueStreamData));
		if(p_stream_node->hGetStreamFromSource != NULL)
		{
			free(p_stream_node->hGetStreamFromSource);
			p_stream_node->hGetStreamFromSource = NULL;
		}
		list_destroy(&(p_stream_node->listClientNodeHead));
		free(p_stream_node);
		p_stream_node=NULL;
		return;
	}
	else if((1 == p_stream_node->uRtpClientNodeSendDataThreadFlag) || (2 == p_stream_node->uRtpClientNodeSendDataThreadFlag))//如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = %d\n", p_stream_node->uRtpClientNodeSendDataThreadFlag);
		pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(p_stream_node->listClientNodeHead));
		list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
		pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
		p_stream_node->uGetStreamThreadStartFlag = 2;//接受数据流模块已退出
		return;
	}

	return;
}
#endif


//获取start_num与end_num之间的随机数
static HB_S32 random_number(HB_S32 start_num, HB_S32 end_num)
{
	HB_S32 ret_num = 0;
	srand((unsigned)time(0));
	ret_num = rand() % (end_num - start_num) + start_num;
	return ret_num;
}

static HB_S32 recv_box_frame(HB_S32 arg_epfd, struct epoll_event *arg_events, HB_S32 arg_maxevents, HB_VOID *args)
{
	STREAM_NODE_HANDLE p_stream_node = (STREAM_NODE_HANDLE)args;
	//RTP_CLIENT_TRANSPORT_HANDLE rtp_client_node = pth_args->client_node;

	TRACE_LOG("Start Recv Stream from box [ip:%s port:%d  ---  cDevId:%s dev_channel:%d]!\n",p_stream_node->cDevIp, p_stream_node->iDevPort, p_stream_node->cDevId, p_stream_node->iDevChnl);
	//pthread_t data_send_pthread_id;
	//HB_CHAR *node_data_buf = NULL;
	//HB_S32 send_data_task_flag = 0;
	//HB_S32 set_sps_pps_to_data_base_flag = 0;
	//HB_U32 cur_node_data_size = 0;
	//HB_CHAR *tmp_buf = NULL;
	//HB_S32 frame_type = 0;
	//HB_U64 itime = 0;
	HB_S32 ret = -1;
	BOX_CTRL_CMD_OBJ cmd_msg;
	HB_S32 listen_socket = -1;
	HB_S32 accept_socket = -1;
	HB_S32 sin_size;
	struct sockaddr_in client_addr;
	sin_size = sizeof(client_addr);
	HB_S32 tmp_server_port = 0;
	HB_S32 epfd=-1;
	struct epoll_event ev;
	struct epoll_event events[1];
	HB_S32 max_events = sizeof(events)/sizeof(struct epoll_event);
	HB_S32 retry_times = 0;

RETRY:
	tmp_server_port = random_number(40000, 60000);//随机生成一个接收私有协议视频帧的服务端口

	while(1)
	{
		ret = check_port(tmp_server_port);
		if(0 == ret)
		{
			break;
		}
		tmp_server_port += 1;
		usleep(10000);
	}
	listen_socket = setup_listen_socket(tmp_server_port);
	if(listen_socket < 0)
	{
		if(retry_times <3)
		{
			sleep(1);
			retry_times ++;
			goto RETRY;
		}
		TRACE_ERR("listen err!");
		ret = -1;
		goto END;
	}

	HB_S32 i, nfds;
	epfd=epoll_create(1);
	if(epfd <= 0)
	{
		goto END;
	}

    //设置与要处理的事件相关的文件描述符
    ev.data.fd=listen_socket;
    //设置要处理的事件类型
    ev.events=EPOLLIN;
    //注册epoll事件
    if(epoll_ctl(epfd,EPOLL_CTL_ADD,listen_socket,&ev) != 0)
    {
    	goto END;
    }

    HB_S32 cmd_host_len = 0;
	HB_CHAR json_cmd[512] = {0};
	snprintf(json_cmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(json_cmd)-sizeof(BOX_CTRL_CMD_OBJ), \
			"{\"cmdType\":\"server_info\",\"serverIp\":\"%s\",\"serverPort\":%d}", RTSP_SERVER_ADDR, tmp_server_port);
	memset(&cmd_msg, 0, sizeof(BOX_CTRL_CMD_OBJ));
	memcpy(cmd_msg.header, "hBzHbox@", 8);
	cmd_msg.cmd_type = BOX_SERVER_INFO;
	cmd_host_len = strlen(json_cmd+sizeof(BOX_CTRL_CMD_OBJ));
	cmd_msg.cmd_length = htonl(cmd_host_len);
	struct epoll_event tmp_ev;
	tmp_ev.events=EPOLLOUT;
	tmp_ev.data.fd = p_stream_node->iConnectBoxSockFd;
	epoll_ctl(arg_epfd,EPOLL_CTL_MOD,p_stream_node->iConnectBoxSockFd,&tmp_ev);
	memcpy(json_cmd, &cmd_msg, sizeof(BOX_CTRL_CMD_OBJ));
	if(send_data_ep(arg_epfd, arg_events, arg_maxevents, json_cmd, cmd_host_len+sizeof(BOX_CTRL_CMD_OBJ), 5, 0) != 0)
	{
		close(listen_socket);
		close_sockfd(&(p_stream_node->iConnectBoxSockFd));
		ret = -3;
		goto END;
	}
	close_sockfd(&(p_stream_node->iConnectBoxSockFd));
	close_sockfd(&arg_epfd);


	nfds=epoll_wait(epfd,events,2,5000);
	if(nfds <= 0)
	{
		ret = -4;
		goto END;
	}
	for(i=0;i<nfds;++i)
	{
		if(events[i].data.fd==listen_socket)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
		{
			accept_socket = accept(listen_socket, (struct sockaddr *)&client_addr, (socklen_t *)&sin_size);
			close_sockfd(&listen_socket);
			set_socket_non_blocking(accept_socket);
			printf("\nBBBBBBBBBBBBB   accept_socket=%d\n", accept_socket);
#if 0
			ev.data.fd=accept_socket;
			//设置用于注测的读操作事件
			ev.events=EPOLLIN;
			//注册ev
			epoll_ctl(epfd,EPOLL_CTL_ADD,accept_socket,&ev);
#endif
			break;
		}
	}

	struct bufferevent *rtsp_sockfd_bev = NULL;
	HB_S32 nRecvBufLen = 131072; //设置为128K
	setsockopt(accept_socket, SOL_SOCKET, SO_RCVBUF, (const HB_CHAR*)&nRecvBufLen, sizeof(HB_S32));
	//创建接收视频流数据的bev，并且与rtsp客户端请求视频创建的bev放在同一个base里面，可以减少锁的数量，因为同一个base里，bev都是串行的
	rtsp_sockfd_bev= bufferevent_socket_new(p_stream_node->pWorkBase, accept_socket, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS);
	if(NULL == rtsp_sockfd_bev)
    {
    	printf("\n###########  bufferevent_socket_new  ERR!\n");
    	ret = -6;
    	goto END;
    }
	bufferevent_set_max_single_read(rtsp_sockfd_bev, 131072);
	struct timeval tv_read;
	tv_read.tv_sec  = 15;//10秒超时时间,接收到新的连接后，10秒内没收到任何数据，则用accept_client_event_cb回调函数处理
	tv_read.tv_usec = 0;
	bufferevent_set_timeouts(rtsp_sockfd_bev, &tv_read, NULL);
	bufferevent_setwatermark(rtsp_sockfd_bev, EV_READ, 131072, 256*1024);

	p_stream_node->uGetStreamThreadStartFlag = 1; //接收流媒体数据模块启动
	//list_init(&(p_stream_node->stream_data_node_head));
   ret = nolock_queue_init(&(p_stream_node->queueStreamData), 0, sizeof(QUEUE_ARGS_OBJ), 512);
   if (ret < 0)
    {
        printf("lf_queue_init error: %d\n", ret);
        goto END;
    }

	bufferevent_setcb(rtsp_sockfd_bev, client_read_cb, NULL, client_event_cb, p_stream_node);
	bufferevent_enable(rtsp_sockfd_bev, EV_READ | EV_PERSIST);

END:
	if(epfd > 0)
	{
		close_sockfd(&epfd);
	}
	if(ret < 0)//出现异常
	{
		printf("\nAAAAAAAAAA    666\n");
		return -1;
	}
	return 0;
}



static HB_VOID *get_box_stream_thread(HB_VOID *args)
{
	TRACE_DBG("#######  get_box_stream_thread  start!");
	HB_S32 ret = 0;
	STREAM_NODE_HANDLE p_stream_node = (STREAM_NODE_HANDLE)args;
	p_stream_node->uGetStreamThreadStartFlag = 1;
	HB_S32 epfd = -1;
	struct epoll_event ev;
	struct epoll_event events[1];

	BOX_CTRL_CMD_OBJ cmd_buf;
	memset(&cmd_buf, 0, sizeof(BOX_CTRL_CMD_OBJ));
	HB_CHAR json_cmd[512] = {0};
	HB_CHAR *recv_msg = NULL;

	do
	{
		if (create_socket_connect_ipaddr(&(p_stream_node->iConnectBoxSockFd),
				p_stream_node->cDevIp, p_stream_node->iDevPort, 5) < 0)
		{
			TRACE_ERR("connect to media stream failed!\n");
			break;
		}
		set_socket_non_blocking(p_stream_node->iConnectBoxSockFd);
		epfd=epoll_create(1);
		if(epfd <= 0)
		{
			break;
		}
	    //设置与要处理的事件相关的文件描述符
	    ev.data.fd=p_stream_node->iConnectBoxSockFd;
	    //设置要处理的事件类型
	    ev.events=EPOLLOUT;
	    //注册epoll事件
	    if(epoll_ctl(epfd,EPOLL_CTL_ADD,p_stream_node->iConnectBoxSockFd,&ev) != 0)
	    {
	    	break;
	    }
		//连接前端盒子成功，发送要观看的设备信息json消息。
		//snprintf(json_cmd, sizeof(json_cmd),
		//		"{\"CmdType\":\"open_video\",\"DevId\":\"%s\",\"DevChnl\":%d,\"DevStreamType\":%d}",
		//		rtsp_node->cDevId, rtsp_node->iDevChnl, rtsp_node->iStreamType);
#if PRIVATE_SERVER_MODE
		snprintf(json_cmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(json_cmd)-sizeof(BOX_CTRL_CMD_OBJ),
				"{\"CmdType\":\"open_video\",\"DevId\":\"%s\",\"DevChnl\":%d,\"DevStreamType\":%d}",
				p_stream_node->cDevId+8, p_stream_node->iDevChnl-1, p_stream_node->iStreamType);
#endif
#if YDT_SERVER_MODE
		snprintf(json_cmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(json_cmd)-sizeof(BOX_CTRL_CMD_OBJ),
				"{\"CmdType\":\"open_video\",\"DevId\":\"%s\",\"DevChnl\":%d,\"DevStreamType\":%d}",
				p_stream_node->cDevId, rtsp_node->iDevChnl, rtsp_node->iStreamType);
#endif

		memcpy(cmd_buf.header, "hBzHbox@", 8);
		cmd_buf.cmd_type = BOX_PLAY_VIDEO;
		cmd_buf.cmd_length = strlen(json_cmd+sizeof(BOX_CTRL_CMD_OBJ));
		memcpy(json_cmd, &cmd_buf, sizeof(BOX_CTRL_CMD_OBJ));
		ret = send_data_ep(epfd, events, 1, json_cmd, cmd_buf.cmd_length+sizeof(BOX_CTRL_CMD_OBJ), 5, 0);
		if(ret != 0)
		{
			TRACE_ERR("*************  02 send_data err!");
			break;
		}
		printf("\n>>>>>>>>>>>>>>  send to box open_video msg :[%s]\n", json_cmd+sizeof(BOX_CTRL_CMD_OBJ));

#if 0
		recv_box_frame(epfd, events, 1, pth_args);
		free(pth_args);
		pth_args = NULL;
		return NULL;
#endif
#if 1
		memset(&cmd_buf, 0, sizeof(BOX_CTRL_CMD_OBJ));
		ev.events=EPOLLIN;
		epoll_ctl(epfd,EPOLL_CTL_MOD,p_stream_node->iConnectBoxSockFd,&ev);
		if ((ret = recv_data_ep(epfd, events, 1, &cmd_buf, sizeof(BOX_CTRL_CMD_OBJ), 10)) <= 0)
		{
			TRACE_ERR("******## 00 recv msg from box failed! ret = %d\n", ret);
			break;
		}
		if(cmd_buf.cmd_length > 0)
		{
			recv_msg = (HB_CHAR*)malloc(cmd_buf.cmd_length+1);
			if(NULL == recv_msg)
			{
				break;
			}
			memset(recv_msg, 0, cmd_buf.cmd_length+1);
			if ((ret = recv_data_ep(epfd, events, 1, recv_msg, cmd_buf.cmd_length, 10)) <= 0)
			{
				TRACE_ERR("******## 01 recv msg from box failed! ret = %d\n", ret);
				free(recv_msg);
				recv_msg = NULL;
				break;
			}
			printf("\n<<<<<<<<<<<<<< 123   recv len = %d   recv sdp_info from box:[%s]\n", cmd_buf.cmd_length, recv_msg);
		}

		if (CMD_OK == cmd_buf.cmd_code)
		{
			if(recv_msg != NULL)
			{
				printf("\n<<<<<<<<<<<<<<    recv len = %d   recv sdp_info from box:[%s]\n", cmd_buf.cmd_length, recv_msg);
				parse_sdp_info(p_stream_node, recv_msg);
				free(recv_msg);
				recv_msg = NULL;
			}
			//printf("\n\nKKKKKKKKKKKKKKKKKKKK\n\n");
			recv_box_frame(epfd, events, 1, p_stream_node);
			return NULL;
		}
		else
		{
			if(NULL == recv_msg)
			{
				free(recv_msg);
				recv_msg = NULL;
			}
			TRACE_ERR("Recv wrong message from box! Exit!\n");
		}
#endif
	}while(0);


	TRACE_ERR("\n###### get_box_stream_thread()   Err!\n");
	close_sockfd(&(p_stream_node->iConnectBoxSockFd));
	if(epfd > 0)
	{
		close_sockfd(&epfd);
	}

	HB_S32 hash_value = 0;
	hash_value = p_stream_node->iStreamNodeHashValue;
	pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
	list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
	pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
	destroy_client_rtp_list(&(p_stream_node->listClientNodeHead));//释放rtp客户队列
	list_destroy(&(p_stream_node->listClientNodeHead));
	free(p_stream_node);
	p_stream_node=NULL;

	printf("\nAAAAAAAAAA    333\n");
	return NULL;
}


static HB_VOID get_box_stream_task(struct sttask *ptsk)
{
	printf("\n@@@@@@@@@@@@  get_box_stream_task  start!\n");
	HB_S32 ret = 0;
	STREAM_NODE_HANDLE p_stream_node = (STREAM_NODE_HANDLE)(ptsk->task_arg);
	HB_S32 hash_value = 0;
	hash_value = p_stream_node->iStreamNodeHashValue;
	HB_S32 epfd = -1;
	struct epoll_event ev;
	struct epoll_event events[1];

	BOX_CTRL_CMD_OBJ cmd_buf;
	memset(&cmd_buf, 0, sizeof(BOX_CTRL_CMD_OBJ));
	HB_CHAR json_cmd[512] = {0};
	HB_CHAR *recv_msg = NULL;

	do
	{
		if (create_socket_connect_ipaddr(&(p_stream_node->iConnectBoxSockFd),\
				p_stream_node->cDevIp, p_stream_node->iDevPort, 5) < 0)
		{
			TRACE_ERR("connect to media stream failed!\n");
			break;
		}

		epfd=epoll_create(1);
		if(epfd <= 0)
		{
			break;
		}
	    //设置与要处理的事件相关的文件描述符
	    ev.data.fd=p_stream_node->iConnectBoxSockFd;
	    //设置要处理的事件类型
	    ev.events=EPOLLOUT;
	    //注册epoll事件
	    if(epoll_ctl(epfd,EPOLL_CTL_ADD,p_stream_node->iConnectBoxSockFd,&ev) != 0)
	    {
	    	break;
	    }
		//连接前端盒子成功，发送要观看的设备信息json消息。
		//snprintf(json_cmd, sizeof(json_cmd),
		//		"{\"CmdType\":\"open_video\",\"DevId\":\"%s\",\"DevChnl\":%d,\"DevStreamType\":%d}",
		//		rtsp_node->cDevId, rtsp_node->iDevChnl, rtsp_node->iStreamType);

		snprintf(json_cmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(json_cmd)-sizeof(BOX_CTRL_CMD_OBJ),
				"{\"cmdType\":\"open_video\",\"devId\":\"%s\",\"devChnl\":%d,\"devStreamType\":%d}",
				p_stream_node->cDevId+8, p_stream_node->iDevChnl, p_stream_node->iStreamType);  //+8是跳过"YDT-BOX-"的长度

		HB_S32 cmd_host_len = 0;
		memcpy(cmd_buf.header, "hBzHbox@", 8);
		cmd_buf.cmd_type = BOX_PLAY_VIDEO;
		cmd_host_len = strlen(json_cmd+sizeof(BOX_CTRL_CMD_OBJ));
		cmd_buf.cmd_length = htonl(cmd_host_len);
		memcpy(json_cmd, &cmd_buf, sizeof(BOX_CTRL_CMD_OBJ));
		ret = send_data_ep(epfd, events, 1, json_cmd, cmd_host_len+sizeof(BOX_CTRL_CMD_OBJ), 5, 0);
		if(ret != 0)
		{
			TRACE_ERR("*************  01 send_data err!");
			break;
		}

		printf("\n>>>>>>>>>>>>>>  send to box open_video msg :[%s]\n", json_cmd);

		memset(&cmd_buf, 0, sizeof(BOX_CTRL_CMD_OBJ));
		ev.events=EPOLLIN;
		epoll_ctl(epfd,EPOLL_CTL_MOD,p_stream_node->iConnectBoxSockFd,&ev);
		if ((ret = recv_data_ep(epfd, events, 1, &cmd_buf, sizeof(BOX_CTRL_CMD_OBJ), 10)) <= 0)
		{
			TRACE_ERR("******## 00 recv msg from box failed! ret = %d\n", ret);
			break;
		}

		cmd_host_len = ntohl(cmd_buf.cmd_length);
		if(cmd_host_len > 0)
		{
			recv_msg = (HB_CHAR*)malloc(cmd_host_len+1);
			if(NULL == recv_msg)
			{
				break;
			}
			memset(recv_msg, 0, cmd_host_len+1);
			if ((ret = recv_data_ep(epfd, events, 1, recv_msg, cmd_host_len, 10)) <= 0)
			{
				TRACE_ERR("******## 01 recv msg from box failed! ret = %d\n", ret);
				free(recv_msg);
				recv_msg = NULL;
				break;
			}
		}

		if (CMD_OK == cmd_buf.cmd_code)
		{
			if(recv_msg != NULL)
			{
				printf("\n<<<<<<<<<<<<<<    recv len = %d   recv sdp_info from box:[%s]\n", cmd_host_len, recv_msg);
				parse_sdp_info(p_stream_node, recv_msg);
				free(recv_msg);
				recv_msg = NULL;
			}
			ret = recv_box_frame(epfd, events, 1, p_stream_node);
			if(0 == ret)
			{
				return;
			}
		}
		else
		{
			if(NULL == recv_msg)
			{
				free(recv_msg);
				recv_msg = NULL;
			}
			//printf("recv from stream : [%s]\n", stDeviceInfo->m_pRecvXmlData);
			TRACE_ERR("Recv wrong message from box! Exit!\n");
		}
	}while(0);


	TRACE_ERR("\n###### get_box_stream_task()   Err!\n");
	close_sockfd(&(p_stream_node->iConnectBoxSockFd));
	if(epfd > 0)
	{
		close_sockfd(&epfd);
	}

	//从汉邦服务器或者盒子获取流的线程还没有启动,所有节点资源在这里直接释放
	printf("\nbbb\n");
	pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
	list_remove(&(stream_hash_table->pStreamHashNodeHead[hash_value].listStreamNodeHead), (HB_VOID*)p_stream_node);
	pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
	destroy_client_rtp_list(&(p_stream_node->listClientNodeHead));//释放rtp客户队列
	free(p_stream_node);
	p_stream_node=NULL;

	return;
}


HB_S32 play_rtsp_video_from_box(STREAM_NODE_HANDLE pStreamNode)
{
	if (1 == pStreamNode->uRtspPlayFlag)
	{
		return HB_FAILURE;
	}

#if USE_PTHREAD_POOL
	struct sttask *pTask;
	pTask = stpool_task_new(gb_thread_pool, "get_box_stream_task", get_box_stream_task, NULL, pStreamNode);
	HB_S32 error = stpool_task_set_p(pTask, gb_thread_pool);
	if (error)
	{
		printf("***Err: %d(%s). (try eCAP_F_CUSTOM_TASK)\n", error, stpool_strerror(error));
		return HB_FAILURE;
	}
	else
	{
		stpool_task_set_userflags(pTask, 0x1);
		stpool_task_queue(pTask);
		pStreamNode->uRtspPlayFlag = 1; //rtsp播放功能已启动
	}

#else

	pthread_attr_t attr;
	pthread_t get_stream_pthread_id;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&get_stream_pthread_id, &attr, get_box_stream_thread, (HB_VOID *)event_args) != 0) //直接从盒子获取单帧数据
	{
		pthread_attr_destroy(&attr);
		return HB_FAILURE;
	}
	pthread_attr_destroy(&attr);
#endif
	return HB_SUCCESS;
}
