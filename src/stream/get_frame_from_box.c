/*
 * get_frame_from_box.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */
#include "my_include.h"
#include "event.h"
#include "lf_queue.h"
#include "cJSON.h"


#include "stream/rtp_pack.h"
#include "stream/send_av_data.h"
#include "server_config.h"
#include "net_api.h"
#include "hash_table.h"
#include "common_api.h"

extern STREAM_HASH_TABLE_HANDLE glStreamHashTable;
extern stpool_t *glGbThreadPool;

static HB_S32 parse_sdp_info(STREAM_NODE_HANDLE pStreamNode, HB_CHAR *pSdpJson)
{
	cJSON *pJson = NULL;
	cJSON *pSub = NULL;
	pJson = cJSON_Parse(pSdpJson);
	if(NULL == pJson)
	{
		return HB_FAILURE;
	}
	pSub = cJSON_GetObjectItem(pJson, "CmdType");
	if(NULL == pSub)
	{
		cJSON_Delete(pJson);
		return HB_FAILURE;
	}
	if(!strcmp(pSub->valuestring, "sdp_info"))
	{
		pSub = cJSON_GetObjectItem(pJson, "m_video");
		if(pSub != NULL)
		{
			if(strlen(pSub->valuestring))
			{
				strcpy(pStreamNode->stSdpInfo.m_video, pSub->valuestring);
			}
		}
		pSub = cJSON_GetObjectItem(pJson, "a_rtpmap_video");
		if(pSub != NULL)
		{
			if(strlen(pSub->valuestring))
			{
				strcpy(pStreamNode->stSdpInfo.a_rtpmap_video, pSub->valuestring);
			}
		}

		pSub = cJSON_GetObjectItem(pJson, "a_fmtp_video");
		if(pSub != NULL)
		{
			if(strlen(pSub->valuestring))
			{
				strcpy(pStreamNode->stSdpInfo.a_fmtp_video, pSub->valuestring);
			}
		}

#if 0
		pSub = cJSON_GetObjectItem(pJson, "m_audio");
		strcpy(pStreamNode->sdp_info.m_audio, pSub->valuestring);
		pSub = cJSON_GetObjectItem(pJson, "a_rtpmap_audio");
		strcpy(pStreamNode->sdp_info.a_rtpmap_audio, pSub->valuestring);
#endif
		cJSON_Delete(pJson);
		return HB_SUCCESS;
	}
	else
	{
		cJSON_Delete(pJson);
		return HB_FAILURE;
	}
}


static HB_VOID clear_rtp_data_from_queue(lf_queue queue)
{
	QUEUE_ARGS_OBJ stQueueOut;
	while(nolock_queue_len(queue) > 0)
	{
		nolock_queue_pop(queue, &stQueueOut);
		if(stQueueOut.pDataBuf != NULL)
		{
#if JE_MELLOC_FUCTION
			je_free(stQueueOut.pDataBuf);
#else
			free(stQueueOut.pDataBuf);
#endif
			stQueueOut.pDataBuf = NULL;
		}
	}
	return;
}


// 异常事件回调函数
static HB_VOID client_event_cb(struct bufferevent *pPushStreamBev, HB_S16 events, HB_VOID *args)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)args;
	HB_U32 uHashValue = 0;
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
	bufferevent_free(pPushStreamBev);
	pPushStreamBev = NULL;
	uHashValue = pStreamNode->uStreamNodeHashValue;
	if(0 == pStreamNode->uRtpClientNodeSendDataThreadFlag)//rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 0");
//		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		list_remove(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
		destory_client_list(&(pStreamNode->listClientNodeHead));//释放rtp客户队列
//		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		clear_rtp_data_from_queue(pStreamNode->queueStreamData);//释放视频队列
		nolock_queue_destroy(&(pStreamNode->queueStreamData));
		if(pStreamNode->hGetStreamFromSource != NULL)
		{
			free(pStreamNode->hGetStreamFromSource);
			pStreamNode->hGetStreamFromSource = NULL;
		}
		free(pStreamNode);
		pStreamNode=NULL;
	}
	else //if((1 == pStreamNode->uRtpClientNodeSendDataThreadFlag) || (2 == pStreamNode->uRtpClientNodeSendDataThreadFlag))//如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = %d\n", pStreamNode->uRtpClientNodeSendDataThreadFlag);
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(pStreamNode->listClientNodeHead));
//		list_delete(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		pStreamNode->uGetStreamThreadStartFlag = 2;//接受数据流模块已退出
	}

	return;
}


#if 1
static HB_VOID client_read_cb(struct bufferevent *pPushStreamBev, HB_VOID *args)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)args;
	HB_U32 uHashValue = 0;
	HB_S32 iRet = 0;
	struct sttask *pTask;
	BOX_CTRL_CMD_OBJ stCmdHead;
	memset(&stCmdHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
	HB_S32 iDataLen = 0;
	struct evbuffer *evbuf = NULL;
	evbuf = bufferevent_get_input(pPushStreamBev);
	for(;;)
	{
		if(evbuffer_get_length(evbuf) < 32)
		{
			return;
		}
		memset(&stCmdHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
		iRet = evbuffer_copyout(evbuf, (HB_VOID*)(&stCmdHead), sizeof(BOX_CTRL_CMD_OBJ));
		//printf("\n#########  stCmdHead:[%s]  len:[%d]\n", stCmdHead.header, iRet);
		if(strncmp(stCmdHead.header, "hBzHbox@", 8))
		{
			iRet = -101;
			break;
		}
		iDataLen = (stCmdHead.cmd_length);
		if(evbuffer_get_length(evbuf) < (iDataLen + sizeof(BOX_CTRL_CMD_OBJ)))
		{
			return;
		}

		if(0 == pStreamNode->uRtpClientNodeSendDataThreadFlag)//如果rtp发送线程没有启动，则启动rtp包发送线程
		{
			pTask = stpool_task_new(glGbThreadPool, "send_rtp_to_client_task", send_rtp_to_client_task, NULL, pStreamNode);
			HB_S32 error = stpool_task_set_p(pTask, glGbThreadPool);
			if (error)
			{
				printf("***Err: %d(%s). (try eCAP_F_CUSTOM_TASK)\n", error, stpool_strerror(error));
				iRet = -102;
				break;
			}
			else
			{
				stpool_task_set_userflags(pTask, 0x1);
				stpool_task_queue(pTask);
				pStreamNode->uRtpClientNodeSendDataThreadFlag = 1; //发送rtp数据线程已经启动
			}
		}
//		if(2 == pStreamNode->uRtpClientNodeSendDataThreadFlag || 2 == pStreamNode->rtsp_play_flag)
		if((0 == list_size(&(pStreamNode->listClientNodeHead))) || (2 == pStreamNode->uRtpClientNodeSendDataThreadFlag))
		{
			break;
		}
		if(nolock_queue_len(pStreamNode->queueStreamData) > 512)
		{
			break;
		}
		HB_S32 rtp_data_buf_pre_size = (((iDataLen/MAX_RTP_PACKET_SIZE)+1)*20)+MAX_RTP_PACKET_SIZE;
		HB_S32 rtp_data_buf_len = iDataLen + rtp_data_buf_pre_size;
		QUEUE_ARGS_OBJ stQueueArg;
#if JE_MELLOC_FUCTION
		stQueueArg.pDataBuf = (HB_CHAR*)je_malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#else
		stQueueArg.pDataBuf = (HB_CHAR*)malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#endif
		stQueueArg.iDataLen = iDataLen + sizeof(BOX_CTRL_CMD_OBJ);
		stQueueArg.iDataPreBufSize = rtp_data_buf_pre_size;
		iRet = bufferevent_read(pPushStreamBev, (HB_VOID*)(stQueueArg.pDataBuf+rtp_data_buf_pre_size), (iDataLen + sizeof(BOX_CTRL_CMD_OBJ)));
		nolock_queue_push(pStreamNode->queueStreamData, &stQueueArg);
		return;
	}
	//发生异常
	bufferevent_free(pPushStreamBev);
	pPushStreamBev = NULL;
	uHashValue = pStreamNode->uStreamNodeHashValue;
	//printf("\n@@@@@@@@@@@@@@@@@@  recv_stream_cb() err  iRet=%d\n", iRet);
	if(0 == pStreamNode->uRtpClientNodeSendDataThreadFlag)//rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 0");
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		list_remove(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
		destory_client_list(&(pStreamNode->listClientNodeHead));//释放rtp客户队列
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		clear_rtp_data_from_queue(pStreamNode->queueStreamData);//释放视频队列
		nolock_queue_destroy(&(pStreamNode->queueStreamData));
		if(pStreamNode->hGetStreamFromSource != NULL)
		{
			free(pStreamNode->hGetStreamFromSource);
			pStreamNode->hGetStreamFromSource = NULL;
		}
		free(pStreamNode);
		pStreamNode=NULL;
		return;
	}
	else if((1 == pStreamNode->uRtpClientNodeSendDataThreadFlag) || (2 == pStreamNode->uRtpClientNodeSendDataThreadFlag))//如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = %d\n", pStreamNode->uRtpClientNodeSendDataThreadFlag);
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(pStreamNode->listClientNodeHead));
		list_remove(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		pStreamNode->uGetStreamThreadStartFlag = 2;//接受数据流模块已退出
		return;
	}

	return;
}
#endif


static HB_S32 recv_box_frame(HB_S32 arg_epfd, struct epoll_event *arg_events, HB_S32 arg_maxevents, HB_VOID *args)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)args;
	//RTP_CLIENT_TRANSPORT_HANDLE rtp_client_node = pth_args->client_node;

	TRACE_LOG("Start Recv Stream from box [ip:%s port:%d  ---  cDevId:%s dev_channel:%d]!\n",pStreamNode->cDevIp, pStreamNode->iDevPort, pStreamNode->cDevId, pStreamNode->iDevChnl);
	//pthread_t data_send_pthread_id;
	//HB_CHAR *node_data_buf = NULL;
	//HB_S32 send_data_task_flag = 0;
	//HB_S32 set_sps_pps_to_data_base_flag = 0;
	//HB_U32 cur_node_data_size = 0;
	//HB_CHAR *tmp_buf = NULL;
	//HB_S32 frame_type = 0;
	//HB_U64 itime = 0;
	HB_S32 iRet = -1;
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
		iRet = check_port(tmp_server_port);
		if(0 == iRet)
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
		iRet = -1;
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
	HB_CHAR cJsonCmd[512] = {0};
	snprintf(cJsonCmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(cJsonCmd)-sizeof(BOX_CTRL_CMD_OBJ), \
			"{\"cmdType\":\"server_info\",\"serverIp\":\"%s\",\"serverPort\":%d}", glGlobleArgs.cLocalIp, tmp_server_port);
	memset(&cmd_msg, 0, sizeof(BOX_CTRL_CMD_OBJ));
	memcpy(cmd_msg.header, "hBzHbox@", 8);
	cmd_msg.cmd_type = BOX_SERVER_INFO;
	cmd_host_len = strlen(cJsonCmd+sizeof(BOX_CTRL_CMD_OBJ));
	cmd_msg.cmd_length = htonl(cmd_host_len);
	struct epoll_event tmp_ev;
	tmp_ev.events=EPOLLOUT;
	tmp_ev.data.fd = pStreamNode->iConnectBoxSockFd;
	epoll_ctl(arg_epfd,EPOLL_CTL_MOD,pStreamNode->iConnectBoxSockFd,&tmp_ev);
	memcpy(cJsonCmd, &cmd_msg, sizeof(BOX_CTRL_CMD_OBJ));
	if(send_data_ep(arg_epfd, arg_events, arg_maxevents, cJsonCmd, cmd_host_len+sizeof(BOX_CTRL_CMD_OBJ), 5, 0) != 0)
	{
		close(listen_socket);
		close_sockfd(&(pStreamNode->iConnectBoxSockFd));
		iRet = -3;
		goto END;
	}
	close_sockfd(&(pStreamNode->iConnectBoxSockFd));
	close_sockfd(&arg_epfd);


	nfds=epoll_wait(epfd,events,2,5000);
	if(nfds <= 0)
	{
		iRet = -4;
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
	rtsp_sockfd_bev= bufferevent_socket_new(pStreamNode->pWorkBase, accept_socket, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS);
	if(NULL == rtsp_sockfd_bev)
    {
    	printf("\n###########  bufferevent_socket_new  ERR!\n");
    	iRet = -6;
    	goto END;
    }
	bufferevent_set_max_single_read(rtsp_sockfd_bev, 131072);
	struct timeval tv_read;
	tv_read.tv_sec  = 15;//10秒超时时间,接收到新的连接后，10秒内没收到任何数据，则用accept_client_event_cb回调函数处理
	tv_read.tv_usec = 0;
	bufferevent_set_timeouts(rtsp_sockfd_bev, &tv_read, NULL);
	bufferevent_setwatermark(rtsp_sockfd_bev, EV_READ, 131072, 256*1024);

	pStreamNode->uGetStreamThreadStartFlag = 1; //接收流媒体数据模块启动
	//list_init(&(pStreamNode->stream_data_node_head));
   iRet = nolock_queue_init(&(pStreamNode->queueStreamData), 0, sizeof(QUEUE_ARGS_OBJ), 512);
   if (iRet < 0)
    {
        printf("lf_queue_init error: %d\n", iRet);
        goto END;
    }

	bufferevent_setcb(rtsp_sockfd_bev, client_read_cb, NULL, client_event_cb, pStreamNode);
	bufferevent_enable(rtsp_sockfd_bev, EV_READ | EV_PERSIST);

END:
	if(epfd > 0)
	{
		close_sockfd(&epfd);
	}
	if(iRet < 0)//出现异常
	{
		printf("\nAAAAAAAAAA    666\n");
		return -1;
	}
	return 0;
}


static HB_VOID get_box_stream_task(struct sttask *ptsk)
{
	printf("\n@@@@@@@@@@@@  get_box_stream_task  start!\n");
	HB_S32 iRet = 0;
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)(ptsk->task_arg);
	HB_U32 uHashValue = 0;
	uHashValue = pStreamNode->uStreamNodeHashValue;
	HB_S32 epfd = -1;
	struct epoll_event ev;
	struct epoll_event events[1];

	BOX_CTRL_CMD_OBJ cmd_buf;
	memset(&cmd_buf, 0, sizeof(BOX_CTRL_CMD_OBJ));
	HB_CHAR cJsonCmd[512] = {0};
	HB_CHAR *pRecvMsg = NULL;

	do
	{
		if (create_socket_connect_ipaddr(&(pStreamNode->iConnectBoxSockFd),\
				pStreamNode->cDevIp, pStreamNode->iDevPort, 5) < 0)
		{
			TRACE_ERR("connect to media stream failed! ip:[%s],port:[%d]\n", pStreamNode->cDevIp, pStreamNode->iDevPort);
			break;
		}

		epfd=epoll_create(1);
		if(epfd <= 0)
		{
			break;
		}
	    //设置与要处理的事件相关的文件描述符
	    ev.data.fd=pStreamNode->iConnectBoxSockFd;
	    //设置要处理的事件类型
	    ev.events=EPOLLOUT;
	    //注册epoll事件
	    if(epoll_ctl(epfd,EPOLL_CTL_ADD,pStreamNode->iConnectBoxSockFd,&ev) != 0)
	    {
	    	break;
	    }
		//连接前端盒子成功，发送要观看的设备信息json消息。
		snprintf(cJsonCmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(cJsonCmd)-sizeof(BOX_CTRL_CMD_OBJ),
				"{\"cmdType\":\"open_video\",\"devId\":\"%s\",\"devChnl\":0,\"devStreamType\":0}",
				"251227033954859-DS-2DC2204IW-D3%2fW20170528CCCH769439538");  //+8是跳过"YDT-BOX-"的长度


//		snprintf(cJsonCmd+sizeof(BOX_CTRL_CMD_OBJ), sizeof(cJsonCmd)-sizeof(BOX_CTRL_CMD_OBJ),
//				"{\"cmdType\":\"open_video\",\"devId\":\"%s\",\"devChnl\":%d,\"devStreamType\":%d}",
//				pStreamNode->cDevId+8, pStreamNode->iDevChnl, pStreamNode->iStreamType);  //+8是跳过"YDT-BOX-"的长度

		HB_S32 cmd_host_len = 0;
		memcpy(cmd_buf.header, "hBzHbox@", 8);
		cmd_buf.cmd_type = BOX_PLAY_VIDEO;
		cmd_host_len = strlen(cJsonCmd+sizeof(BOX_CTRL_CMD_OBJ));
		cmd_buf.cmd_length = htonl(cmd_host_len);
		memcpy(cJsonCmd, &cmd_buf, sizeof(BOX_CTRL_CMD_OBJ));
		iRet = send_data_ep(epfd, events, 1, cJsonCmd, cmd_host_len+sizeof(BOX_CTRL_CMD_OBJ), 5, 0);
		if(iRet != 0)
		{
			TRACE_ERR("*************  01 send_data err!");
			break;
		}

		printf("\n>>>>>>>>>>>>>>  send to box open_video msg :[%s]\n", cJsonCmd);

		memset(&cmd_buf, 0, sizeof(BOX_CTRL_CMD_OBJ));
		ev.events=EPOLLIN;
		epoll_ctl(epfd,EPOLL_CTL_MOD,pStreamNode->iConnectBoxSockFd,&ev);
		if ((iRet = recv_data_ep(epfd, events, 1, &cmd_buf, sizeof(BOX_CTRL_CMD_OBJ), 10)) <= 0)
		{
			TRACE_ERR("******## 00 recv msg from box failed! iRet = %d\n", iRet);
			break;
		}

		cmd_host_len = ntohl(cmd_buf.cmd_length);
		if(cmd_host_len > 0)
		{
			pRecvMsg = (HB_CHAR*)malloc(cmd_host_len+1);
			if(NULL == pRecvMsg)
			{
				break;
			}
			memset(pRecvMsg, 0, cmd_host_len+1);
			if ((iRet = recv_data_ep(epfd, events, 1, pRecvMsg, cmd_host_len, 10)) <= 0)
			{
				TRACE_ERR("******## 01 recv msg from box failed! iRet = %d\n", iRet);
				free(pRecvMsg);
				pRecvMsg = NULL;
				break;
			}
		}

		if (CMD_OK == cmd_buf.cmd_code)
		{
			if(pRecvMsg != NULL)
			{
				printf("\n<<<<<<<<<<<<<<    recv len = %d   recv sdp_info from box:[%s]\n", cmd_host_len, pRecvMsg);
				parse_sdp_info(pStreamNode, pRecvMsg);
				free(pRecvMsg);
				pRecvMsg = NULL;
			}
			iRet = recv_box_frame(epfd, events, 1, pStreamNode);
			if(0 == iRet)
			{
				return;
			}
		}
		else
		{
			if(NULL == pRecvMsg)
			{
				free(pRecvMsg);
				pRecvMsg = NULL;
			}
			//printf("recv from stream : [%s]\n", stDeviceInfo->m_pRecvXmlData);
			TRACE_ERR("Recv wrong message from box! Exit!\n");
		}
	}while(0);


	TRACE_ERR("\n###### get_box_stream_task()   Err!\n");
	close_sockfd(&(pStreamNode->iConnectBoxSockFd));
	if(epfd > 0)
	{
		close_sockfd(&epfd);
	}

	//从汉邦服务器或者盒子获取流的线程还没有启动,所有节点资源在这里直接释放
	printf("\nbbb\n");
	pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	list_delete(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
	destory_client_list(&(pStreamNode->listClientNodeHead));//释放rtp客户队列
	free(pStreamNode);
	pStreamNode=NULL;
	pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));

	return;
}


HB_S32 play_rtsp_video_from_box(STREAM_NODE_HANDLE pStreamNode)
{
	if (1 == pStreamNode->uRtspPlayFlag)
	{
		return HB_FAILURE;
	}

	struct sttask *pTask;
	pTask = stpool_task_new(glGbThreadPool, "get_box_stream_task", get_box_stream_task, NULL, pStreamNode);
	HB_S32 iError = stpool_task_set_p(pTask, glGbThreadPool);
	if (iError)
	{
		printf("***Err: %d(%s). (try eCAP_F_CUSTOM_TASK)\n", iError, stpool_strerror(iError));
		return HB_FAILURE;
	}
	else
	{
		stpool_task_set_userflags(pTask, 0x1);
		stpool_task_queue(pTask);
		pStreamNode->uRtspPlayFlag = 1; //rtsp播放功能已启动
	}

	return HB_SUCCESS;
}
