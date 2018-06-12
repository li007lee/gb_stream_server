/*
 * get_stream_from_hbserver.c
 *
 *  Created on: 2016年11月23日
 *      Author: root
 */
#include "../sip_hash.h"
#include "my_include.h"
#include "get_frame.h"
//#include "rtp/rtp_api.h"
#include "send_av_data.h"
#include "server_config.h"
#include "../common/common_args.h"
//#include "common/mp4_encode.h"
#include "event.h"
//#include "jemalloc/jemalloc.h"
#include "../common/hash_table.h"
#include "../common/lf_queue.h"
#include "event2/bufferevent.h"

extern STREAM_HASH_TABLE_HANDLE stream_hash_table;

HB_S32 SendDataToStream(struct bufferevent *bev, HB_CHAR *pCmd, HB_S32 nLength, HB_S32 nDataType)
{
	HB_S32 iSendBytes = 0;
	HB_S32 i;
	HB_CHAR *pSrcOffset;
	HB_S32 iSplit;				//如果大于8K，拆分的包数
	HB_S32 iLastBlockLength;	//拆分后，前面包有效数据长度为8K，最后一包的长度

	NET_LAYER _NetLayer;
	_NetLayer.byProtocolType = 0;			//协议类型
	_NetLayer.byProtocolVer = 9;			//协议版本
	_NetLayer.byDataType = nDataType;		//数据类型,手机通讯时,DATA_TYPE_REAL_XML:9:交互命令,DATA_TYPE_SMS_CMD:12:云台控制命令,DATA_TYPE_SMS_MEDIA:13:流媒体数据
	_NetLayer.byFrameType = 0;			//帧类型
	_NetLayer.iTimeStampHigh = 0;			//时间戳
	_NetLayer.iTimeStampLow = 0;			//时间戳
	_NetLayer.iVodFilePercent = 0;		//VOD进度,默认值
	_NetLayer.iVodCurFrameNo = 0;			//VOD当前帧,默认值

	if (nLength % NET_BUFFER_LEN == 0)
	{
		iSplit = nLength / NET_BUFFER_LEN;
		iLastBlockLength = NET_BUFFER_LEN;
	}
	else
	{
		iSplit = (nLength + NET_BUFFER_LEN) / NET_BUFFER_LEN;
		iLastBlockLength = nLength % NET_BUFFER_LEN;
	}

	for (i = 0; i < iSplit; i++)
	{
		pSrcOffset = pCmd + i * NET_BUFFER_LEN;
		if (i == iSplit - 1)			//最后一包
		{
			_NetLayer.iActLength = PACKAGE_EXTRA_LEN + iLastBlockLength;
			memcpy(&_NetLayer.cBuffer, pSrcOffset, iLastBlockLength);
			if (iSplit == 1)
			{
				_NetLayer.byBlockHeadFlag = HB_TRUE;
				_NetLayer.byBlockEndFlag = HB_TRUE;
			}
			else
			{
				_NetLayer.byBlockHeadFlag = HB_FALSE;
				_NetLayer.byBlockEndFlag = HB_TRUE;
			}
		}
		else			//前面的包
		{
			_NetLayer.iActLength = Net_LAYER_STRUCT_LEN;
			memcpy(_NetLayer.cBuffer, pSrcOffset, NET_BUFFER_LEN);
			if (i == 0)
			{
				_NetLayer.byBlockHeadFlag = HB_TRUE;
				_NetLayer.byBlockEndFlag = HB_FALSE;
			}
			else
			{
				_NetLayer.byBlockHeadFlag = HB_FALSE;
				_NetLayer.byBlockEndFlag = HB_FALSE;
			}
		}

		bufferevent_write(bev, (HB_VOID*) &_NetLayer, _NetLayer.iActLength);

		TRACE_LOG("send data : [%s]\n", _NetLayer.cBuffer);
	}
	return iSendBytes;
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
	while (nolock_queue_len(queue) > 0)
	{
		nolock_queue_pop(queue, &queue_out);
		if (queue_out.data_buf != NULL)
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

static HB_S32 destroy_client_rtp_list(list_t *client_node_head)
{
	HB_S32 rtp_client_nums = 0;
	RTP_CLIENT_TRANSPORT_HANDLE rtp_client_node = NULL;
	rtp_client_nums = list_size(client_node_head);
	while (rtp_client_nums)
	{
		rtp_client_node = list_get_at(client_node_head, 0);
		list_delete(client_node_head, rtp_client_node);
		if (rtp_client_node->bev != NULL)
		{
			bufferevent_disable(rtp_client_node->bev, EV_READ | EV_WRITE);
			bufferevent_free(rtp_client_node->bev);
			rtp_client_node->bev = NULL;
		}
		if (rtp_client_node->event_args != NULL)
		{
			free(rtp_client_node->event_args);
			rtp_client_node->event_args = NULL;
		}
		free(rtp_client_node);
		rtp_client_node = NULL;
		rtp_client_nums--;
	}
//	list_destroy(client_node_head);
	return 0;
}

static HB_S32 disable_client_rtp_list_bev(list_t *client_node_head)
{
	HB_S32 rtp_client_nums = 0;
	HB_CHAR *rtp_client_node = NULL;
	rtp_client_nums = list_size(client_node_head);
	while (rtp_client_nums)
	{
		rtp_client_node = list_get_at(client_node_head, rtp_client_nums - 1);
		RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE) rtp_client_node;
		if (client_node->bev != NULL)
		{
			bufferevent_disable(client_node->bev, EV_READ | EV_WRITE);
		}
		rtp_client_nums--;
	}
	return 0;
}

void recv_stream_cb(struct bufferevent *connect_hbserver_bev, HB_VOID *event_args)
{
	GET_STREAM_ARGS_HANDLE get_stream_args = (GET_STREAM_ARGS_HANDLE) (event_args);
	DEV_NODE_HANDLE dev_node = get_stream_args->dev_node;
	NET_LAYER stPackage;
	struct evbuffer *src = bufferevent_get_input(connect_hbserver_bev);			//获取输入缓冲区
	struct sttask *ptsk;
	HB_S32 data_len = 0;
	HB_S32 ret = 0;
	HB_S32 hash_value = 0;

	for (;;)
	{
		if (evbuffer_get_length(src) < 28)
		{
			//printf("head len too small!\n");
			return;
		}
		memset(&stPackage, 0, sizeof(stPackage));
		evbuffer_copyout(src, (void*) (&stPackage), 28);
		if (evbuffer_get_length(src) < stPackage.iActLength)
		{
			//数据长度不足
			return;
		}
		//printf("\nBBBBBBBBBBBBBBBBBB  [%s]\n", dev_node->dev_id);
		memset(&stPackage, 0, sizeof(stPackage));
		bufferevent_read(connect_hbserver_bev, &stPackage, 28);
		bufferevent_read(connect_hbserver_bev, dev_node->get_stream_from_source + dev_node->recv_stream_data_len, stPackage.iActLength - 28);
		dev_node->recv_stream_data_len += stPackage.iActLength - 28;
		if (stPackage.byBlockEndFlag) //尾包
		{
			if ((0 == list_size(&(dev_node->client_node_head))) || 2 == dev_node->rtp_client_node_send_data_thread_flag)
			{
				break;
			}

			if ((0 == dev_node->rtp_client_node_send_data_thread_flag) && (1 == stPackage.byFrameType))
			{
#if USE_PTHREAD_POOL
				ptsk = stpool_task_new(get_stream_args->gb_thread_pool, "send_rtp_to_client_task", send_rtp_to_client_task,
								send_rtp_to_client_task_err_cb, get_stream_args);
				HB_S32 error = stpool_task_set_p(ptsk, get_stream_args->gb_thread_pool);
				if (error)
				{
					printf("***Err: %d(%s). (try eCAP_F_CUSTOM_TASK)\n", error, stpool_strerror(error));
					hash_value = get_stream_args->hash_value;
					destroy_client_rtp_list(&(dev_node->client_node_head)); //释放rtp客户队列
					delete_rtp_data_list(dev_node->stream_data_queue); //释放视频队列
					nolock_queue_destroy(&(dev_node->stream_data_queue));
					if (dev_node->get_stream_from_source != NULL)
					{
						free(dev_node->get_stream_from_source);
						dev_node->get_stream_from_source = NULL;
					}
					free(dev_node);
					dev_node = NULL;
					free(get_stream_args);
					get_stream_args = NULL;
					return;
				}
				else
				{
					stpool_task_set_userflags(ptsk, 0x1);
					stpool_task_queue(ptsk);
					dev_node->rtp_client_node_send_data_thread_flag = 1; //发送rtp数据线程已经启动
				}
#else
				pthread_t data_send_pthread_id;
				if(pthread_create(&data_send_pthread_id, NULL, send_rtp_to_client_thread, (HB_VOID *)pth_args) != 0)
				{
					pthread_mutex_lock(&(sip_hash_table->hash_node[p_stream_node->dev_node_hash_value].dev_mutex));
					destroy_client_rtp_list(&(p_stream_node->client_node_head)); //释放rtp客户队列
					list_remove(&(dev_node->streaming_node_head), (HB_VOID*)p_stream_node);
					if(0 == list_size(&(dev_node->streaming_node_head)))
					{
						list_destroy(&(dev_node->streaming_node_head));
						list_remove(&(sip_hash_table->hash_node[p_stream_node->dev_node_hash_value].dev_node_head), dev_node);
						free(dev_node);
						dev_node = NULL;
					}
					delete_rtp_data_list(p_stream_node->stream_data_queue); //释放视频队列
					lf_queue_destroy(&(p_stream_node->stream_data_queue));
					if(p_stream_node->get_stream_from_source != NULL)
					{
						free(p_stream_node->get_stream_from_source);
						p_stream_node->get_stream_from_source = NULL;
					}
					free(p_stream_node);
					p_stream_node=NULL;
					pthread_mutex_unlock(&(sip_hash_table->hash_node[p_stream_node->dev_node_hash_value].dev_mutex));
					return;
				}
				else
				{
					pthread_mutex_lock(&(sip_hash_table->hash_node[p_stream_node->dev_node_hash_value].dev_mutex));
					p_stream_node->rtp_client_node_send_data_thread_flag = 1; //发送rtp数据线程已经启动
					pthread_mutex_unlock(&(sip_hash_table->hash_node[p_stream_node->dev_node_hash_value].dev_mutex));
				}
#endif
			}

			if (0 == dev_node->rtp_client_node_send_data_thread_flag || 13 != stPackage.byDataType) //如果第一个I帧还没有来或者非音视频帧，则直接丢弃
			{
				dev_node->recv_stream_data_len = 0;
				return;
			}

#if 1
			BOX_CTRL_CMD_OBJ cmd_head;
			memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
			strncpy(cmd_head.header, "hBzHbox@", 8);
			cmd_head.cmd_length = dev_node->recv_stream_data_len;

			if (0 == stPackage.byFrameType)
			{
				cmd_head.data_type = BP_FRAME;
			}
			else if (1 == stPackage.byFrameType)
			{
				cmd_head.data_type = I_FRAME;
			}
			else if (4 == stPackage.byFrameType)
			{
				cmd_head.data_type = AUDIO_FRAME;
			}

			cmd_head.v_pts = (((((HB_U64) (stPackage.iTimeStampHigh)) << 32) + stPackage.iTimeStampLow)) * 90;
			//		if ((stPackage.byFrameType == 0) || (stPackage.byFrameType == 1))
			//		{
			//printf("cmd_head.v_pts=%ld------->FrameType------>%d------>DataLen========%d\n", cmd_head.v_pts, stPackage.byFrameType, p_stream_node->recv_stream_data_len);
			//		}
			if (nolock_queue_len(dev_node->stream_data_queue) > 512)
			{
				break;
			}
			data_len = (cmd_head.cmd_length);
			HB_S32 rtp_data_buf_pre_size = (((data_len / MAX_RTP_PACKET_SIZE) + 1) * 20) + MAX_RTP_PACKET_SIZE;
			//HB_S32 rtp_data_buf_pre_size = 10240;
			HB_S32 rtp_data_buf_len = data_len + rtp_data_buf_pre_size;
			QUEUE_ARGS_OBJ queue_arg;
#if JE_MELLOC_FUCTION
			queue_arg.data_buf = (HB_CHAR*)je_malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#else
			queue_arg.data_buf = (HB_CHAR*) malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#endif
			queue_arg.data_len = data_len + sizeof(BOX_CTRL_CMD_OBJ);
			queue_arg.data_pre_buf_size = rtp_data_buf_pre_size;
			memcpy(queue_arg.data_buf + rtp_data_buf_pre_size, &cmd_head, sizeof(BOX_CTRL_CMD_OBJ));
			memcpy(queue_arg.data_buf + rtp_data_buf_pre_size + 32, dev_node->get_stream_from_source, dev_node->recv_stream_data_len);
			nolock_queue_push(dev_node->stream_data_queue, &queue_arg);
			dev_node->recv_stream_data_len = 0;
#endif
			return;
		}
	}

	//发生异常
	bufferevent_free(connect_hbserver_bev);
	connect_hbserver_bev = NULL;
	hash_value = get_stream_args->hash_value;
	//printf("\n@@@@@@@@@@@@@@@@@@  recv_stream_cb() err  ret=%d\n", ret);
	if (0 == dev_node->rtp_client_node_send_data_thread_flag)	//rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_cb()   rtp_client_node_send_data_thread_flag = 0");
		destroy_client_rtp_list(&(dev_node->client_node_head));	//释放rtp客户队列
		delete_rtp_data_list(dev_node->stream_data_queue);	//释放视频队列
		nolock_queue_destroy(&(dev_node->stream_data_queue));
		if (dev_node->get_stream_from_source != NULL)
		{
			free(dev_node->get_stream_from_source);
			dev_node->get_stream_from_source = NULL;
		}
		free(dev_node);
		dev_node = NULL;
		free(get_stream_args);
		get_stream_args = NULL;
		return;
	}
	else if (1 == dev_node->rtp_client_node_send_data_thread_flag)	//如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_cb()   rtp_client_node_send_data_thread_flag = 1");
		pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		disable_client_rtp_list_bev(&(dev_node->client_node_head));
		pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		return;
	}
	else if (2 == dev_node->rtp_client_node_send_data_thread_flag) //rtp发送线程启动后，异常标志置位,这里只置位接收模块标志，摘除释放工作由rtp发送线程执行
	{
		TRACE_ERR("recv_stream_cb()   rtp_client_node_send_data_thread_flag = 2");
		pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		disable_client_rtp_list_bev(&(dev_node->client_node_head));
		dev_node->get_stream_thread_start_flag = 2; //接受数据流模块已退出
		pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
	}
	return;
}

HB_VOID recv_stream_err_cb(struct bufferevent *connect_hbserver_bev, HB_S16 event, HB_VOID *arg)
{
	GET_STREAM_ARGS_HANDLE get_stream_args = (GET_STREAM_ARGS_HANDLE) (arg);
	DEV_NODE_HANDLE dev_node = get_stream_args->dev_node;

	HB_S32 hash_value = 0;
	hash_value = get_stream_args->hash_value;

	HB_S32 err = EVUTIL_SOCKET_ERROR();
	if (event & BEV_EVENT_TIMEOUT) //超时
	{
		TRACE_ERR("\n###########  BEV_EVENT_TIMEOUT(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}
	else if (event & BEV_EVENT_EOF)
	{
		TRACE_ERR("\n###########  connection normal closed(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}
	else if (event & BEV_EVENT_ERROR)
	{
		TRACE_ERR("\n###########  BEV_EVENT_ERROR(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}
	else
	{
		TRACE_ERR("\n###########  BEV_EVENT_OTHER_ERROR(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}

#ifdef RECONNECT
	bufferevent_free(bev);
	//增加PERSIST的定时器事件
	struct PARAM *param = malloc(sizeof(struct PARAM));
	memset(param, 0, sizeof(struct PARAM));
	param->pCmd = "reconnect";
	param->pDevInfo = pDevInfo;
//	struct event* pClientEvent = event_new(pEventBase, STDIN_FILENO, EV_READ, handle_timeout, param);
	struct event* pClientEvent = event_new(pEventBase, -1, EV_READ, handle_timeout, param);
	event_add(pClientEvent, NULL);
	evtimer_add(pClientEvent, &tReconnectTime);

#else

	//发生异常
	bufferevent_free(connect_hbserver_bev);
	connect_hbserver_bev = NULL;
	hash_value = get_stream_args->hash_value;
	if (0 == dev_node->rtp_client_node_send_data_thread_flag) //rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_err_cb()   rtp_client_node_send_data_thread_flag = 0");
		destroy_client_rtp_list(&(dev_node->client_node_head)); //释放rtp客户队列
		delete_rtp_data_list(dev_node->stream_data_queue); //释放视频队列
		nolock_queue_destroy(&(dev_node->stream_data_queue));
		if (dev_node->get_stream_from_source != NULL)
		{
			free(dev_node->get_stream_from_source);
			dev_node->get_stream_from_source = NULL;
		}
		free(dev_node);
		dev_node = NULL;
		free(get_stream_args);
		get_stream_args = NULL;
		return;
	}
	else if (1 == dev_node->rtp_client_node_send_data_thread_flag) //如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_err_cb()   rtp_client_node_send_data_thread_flag = 1");
		pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		disable_client_rtp_list_bev(&(dev_node->client_node_head));
		dev_node->get_stream_thread_start_flag = 2; //接受数据流模块已退出
		pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		return;
	}
	else if (2 == dev_node->rtp_client_node_send_data_thread_flag) //rtp发送线程启动后，异常标志置位,这里只置位接收模块标志，摘除释放工作由rtp发送线程执行
	{
		TRACE_ERR("recv_stream_err_cb()   rtp_client_node_send_data_thread_flag = 2");
		pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		disable_client_rtp_list_bev(&(dev_node->client_node_head));
		dev_node->get_stream_thread_start_flag = 2; //接受数据流模块已退出
		pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
	}
	return;
#endif
}

HB_VOID recv_cmd_from_hbserver(struct bufferevent *connect_hbserver_bev, HB_VOID *event_args)
{
	HB_S32 ret = 0;
	NET_LAYER stPackage;
	GET_STREAM_ARGS_HANDLE get_stream_args = (GET_STREAM_ARGS_HANDLE) (event_args);
	DEV_NODE_HANDLE dev_node = get_stream_args->dev_node;
	struct evbuffer *src = bufferevent_get_input(connect_hbserver_bev); //获取输入缓冲区

	for (;;)
	{
		if (evbuffer_get_length(src) < 28)
		{
			//printf("head len too small!\n");
			return;
		}
		memset(&stPackage, 0, sizeof(stPackage));
		evbuffer_copyout(src, (void*) (&stPackage), 28);
		if (evbuffer_get_length(src) < stPackage.iActLength)
		{
			//数据长度不足
			return;
		}

		memset(&stPackage, 0, sizeof(stPackage));
		bufferevent_read(connect_hbserver_bev, &stPackage, 28);
		bufferevent_read(connect_hbserver_bev, dev_node->get_stream_from_source + dev_node->recv_stream_data_len, stPackage.iActLength - 28);
		dev_node->recv_stream_data_len += stPackage.iActLength - 28;
		if (stPackage.byBlockEndFlag) //尾包
		{
			TRACE_LOG("recv recv :[%s]\n", dev_node->get_stream_from_source);
			if (strstr(dev_node->get_stream_from_source, "SUCCESS"))
			{
				dev_node->recv_stream_data_len = 0;
				HB_CHAR pImOk[32] = { 0 };
				strncpy(pImOk, "<TYPE>ImOK</TYPE>", 17);
				SendDataToStream(connect_hbserver_bev, pImOk, strlen(pImOk), DATA_TYPE_SMS_CMD);
				ret = nolock_queue_init(&(dev_node->stream_data_queue), 0, sizeof(QUEUE_ARGS_OBJ), 512);
				if (0 == ret)
				{
					//p_stream_node->get_stream_from_source = (HB_CHAR *)malloc(1024*1024);//预先申请1M空间
					struct timeval tv_read;
					tv_read.tv_sec = 15;
					tv_read.tv_usec = 0;
					bufferevent_set_timeouts(connect_hbserver_bev, &tv_read, NULL);
					bufferevent_setwatermark(connect_hbserver_bev, EV_READ, 65536, 128 * 1024);
					bufferevent_set_max_single_read(connect_hbserver_bev, 65536);
					bufferevent_set_max_single_write(connect_hbserver_bev, 65536);
					bufferevent_setcb(connect_hbserver_bev, recv_stream_cb, NULL, recv_stream_err_cb, get_stream_args);
					return;
				}
				else
				{
					printf("lf_queue_init error: %d\n", ret);
					break;
				}
			}
			else
			{
				printf("\n###################  get stream from ydt err!\n");
				break;
			}
		}
	}

	HB_S32 hash_value = 0;
	hash_value = get_stream_args->hash_value;
	bufferevent_free(connect_hbserver_bev);
	connect_hbserver_bev = NULL;
	free(get_stream_args);
	get_stream_args = NULL;
	TRACE_ERR("recv recv :[%s]\n", dev_node->get_stream_from_source);
	destroy_client_rtp_list(&(dev_node->client_node_head)); //释放rtp客户队列
	if (dev_node->get_stream_from_source != NULL)
	{
		free(dev_node->get_stream_from_source);
		dev_node->get_stream_from_source = NULL;
	}
	free(dev_node);
	dev_node = NULL;
}

HB_VOID connect_event_cb(struct bufferevent *connect_hbserver_bev, HB_S16 event, HB_VOID *arg)
{
	GET_STREAM_ARGS_HANDLE get_stream_args = (GET_STREAM_ARGS_HANDLE) (arg);
	DEV_NODE_HANDLE dev_node = get_stream_args->dev_node;

	HB_S32 hash_value = 0;
	hash_value = get_stream_args->hash_value;

	HB_S32 err = EVUTIL_SOCKET_ERROR();
	if (event & BEV_EVENT_TIMEOUT) //超时
	{
		TRACE_ERR("\n###########  BEV_EVENT_TIMEOUT(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}
	else if (event & BEV_EVENT_EOF)
	{
		TRACE_ERR("\n###########  connection closed(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}
	else if (event & BEV_EVENT_ERROR)
	{
		TRACE_ERR("\n###########  BEV_EVENT_ERROR(%d) : %s !\n", err, evutil_socket_error_to_string(err));
	}
	else if (event & BEV_EVENT_CONNECTED)
	{
		HB_CHAR cmd_buf[1024] = { 0 };
		TRACE_LOG("the client has connected to server\n");
#if 0
		snprintf(cmd_buf, sizeof(cmd_buf),
						"<TYPE>StartStream</TYPE><DVRName>%s</DVRName><ChnNo>%d</ChnNo><StreamType>%d</StreamType><TType>0</TType>", dev_node->dev_id,
						dev_node->dev_chl_num, dev_node->stream_type);
#endif
#if 1
		snprintf(cmd_buf, sizeof(cmd_buf),
						"<TYPE>StartStream</TYPE><DVRName>%s</DVRName><ChnNo>%d</ChnNo><StreamType>%d</StreamType><TType>0</TType>", "12345",
						dev_node->dev_chl_num, dev_node->stream_type);
#endif
		SendDataToStream(connect_hbserver_bev, cmd_buf, strlen(cmd_buf), DATA_TYPE_SMS_CMD);
		dev_node->get_stream_from_source = (HB_CHAR *) malloc(1024 * 1024); //预先申请1M空间
		struct timeval tv_read;
		tv_read.tv_sec = 15;
		tv_read.tv_usec = 0;
		bufferevent_set_timeouts(connect_hbserver_bev, &tv_read, NULL);
		bufferevent_setcb(connect_hbserver_bev, recv_cmd_from_hbserver, NULL, recv_stream_err_cb, (HB_VOID*) (get_stream_args));
		bufferevent_enable(connect_hbserver_bev, EV_READ | EV_PERSIST);

		return;
	}

#ifdef RECONNECT
	bufferevent_free(bev);
	//增加PERSIST的定时器事件
	struct PARAM *param = malloc(sizeof(struct PARAM));
	memset(param, 0, sizeof(struct PARAM));
	param->pCmd = "reconnect";
	param->pDevInfo = pDevInfo;
//	struct event* pClientEvent = event_new(pEventBase, STDIN_FILENO, EV_READ, handle_timeout, param);
	struct event* pClientEvent = event_new(pEventBase, -1, EV_READ, handle_timeout, param);
	event_add(pClientEvent, NULL);
	evtimer_add(pClientEvent, &tReconnectTime);

#else

	//以下是处理异常情况
	bufferevent_disable(connect_hbserver_bev, EV_READ | EV_WRITE);
	bufferevent_free(connect_hbserver_bev);
	connect_hbserver_bev = NULL;
	free(get_stream_args);
	get_stream_args = NULL;
	do
	{
		//从汉邦服务器或者盒子获取流的线程还没有启动,所有节点资源在这里直接释放
		printf("\nbbb\n");
//		pthread_mutex_lock(&(sip_hash_table->hash_node[hash_value].dev_mutex));
//		list_remove(&(dev_node->streaming_node_head), (HB_VOID*)p_stream_node);
//		if(0 == list_size(&(dev_node->streaming_node_head)))
//		{
//			list_destroy(&(dev_node->streaming_node_head));
//			list_remove(&(sip_hash_table->hash_node[hash_value].dev_node_head), dev_node);
//			free(dev_node);
//			dev_node = NULL;
//		}
//		pthread_mutex_unlock(&(sip_hash_table->hash_node[hash_value].dev_mutex));
		destroy_client_rtp_list(&(dev_node->client_node_head)); //释放rtp客户队列
//		free(p_stream_node);
//		p_stream_node=NULL;
	} while (0);
	printf("\n *****connect_event_cb() !\n");
#endif
}

HB_S32 play_rtsp_video_from_hbserver(GET_STREAM_ARGS_HANDLE get_stream_args)
{
	if ((1 == get_stream_args->dev_node->rtsp_play_flag) || (NULL == get_stream_args->dev_node))
	{
		free(get_stream_args);
		get_stream_args = NULL;
		return HB_FAILURE;
	}

	HB_S32 ret = 0;
	HB_S32 connect_socket = -1;
	DEV_NODE_HANDLE dev_node = get_stream_args->dev_node;

	struct bufferevent *connect_hbserver_bev = NULL;
	struct sockaddr_in stServerAddr;

	//初始化server_addr
	memset(&stServerAddr, 0, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
//	stServerAddr.sin_port = htons(dev_node->dev_port);
//	inet_aton(dev_node->dev_ip, &stServerAddr.sin_addr);
	stServerAddr.sin_port = htons(600);
	inet_aton("192.168.118.2", &stServerAddr.sin_addr);

	//创建接收视频流数据的bev，并且与rtsp客户端请求视频创建的bev放在同一个base里面，可以减少锁的数量，因为同一个base里，bev都是串行的
	connect_hbserver_bev = bufferevent_socket_new(get_stream_args->work_base, -1,
					BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS); //第二个参数传-1,表示以后设置文件描述符
	if (NULL == connect_hbserver_bev)
	{
		free(get_stream_args);
		get_stream_args = NULL;
		return HB_FAILURE;
	}
	dev_node->connect_hbserver_bev = connect_hbserver_bev;
	struct timeval tv_read;
	tv_read.tv_sec = 15;
	tv_read.tv_usec = 0;
	bufferevent_set_timeouts(connect_hbserver_bev, &tv_read, NULL);
	connect_socket = bufferevent_getfd(connect_hbserver_bev);
	HB_S32 nRecvBufLen = 65536; //设置为64K
	setsockopt(connect_socket, SOL_SOCKET, SO_RCVBUF, (const HB_CHAR*) &nRecvBufLen, sizeof(HB_S32));
	bufferevent_setwatermark(connect_hbserver_bev, EV_READ, 29, 0);

	//设置bufferevent各回调函数
	bufferevent_setcb(connect_hbserver_bev, NULL, NULL, connect_event_cb, (HB_VOID*) (get_stream_args));
	//启用读取或者写入事件
	if (-1 == bufferevent_enable(connect_hbserver_bev, EV_READ))
	{
		free(get_stream_args);
		get_stream_args = NULL;
		return HB_FAILURE;
	}

	//调用bufferevent_socket_connect函数
	if (-1 == bufferevent_socket_connect(connect_hbserver_bev, (struct sockaddr*) &stServerAddr, sizeof(struct sockaddr_in)))
	{
		free(get_stream_args);
		get_stream_args = NULL;
		return HB_FAILURE;
	}

	dev_node->rtsp_play_flag = 1; //rtsp播放功能已启动

	return HB_SUCCESS;
}

