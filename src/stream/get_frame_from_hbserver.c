/*
 * get_stream_from_hbserver.c
 *
 *  Created on: 2016年11月23日
 *      Author: root
 */
#include "my_include.h"
#include "event.h"
#include "lf_queue.h"
#include "event2/bufferevent.h"

#include "server_config.h"
#include "sip_hash.h"
#include "stream/get_frame.h"
#include "stream/send_av_data.h"

extern STREAM_HASH_TABLE_HANDLE glStreamHashTable;
extern stpool_t *glGbThreadPool;

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


static HB_VOID clear_rtp_data_from_queue(lf_queue pQueue)
{
	QUEUE_ARGS_OBJ stQueueOut;
	while (nolock_queue_len(pQueue) > 0)
	{
		nolock_queue_pop(pQueue, &stQueueOut);
		if (stQueueOut.pDataBuf != NULL)
		{
#if JE_MELLOC_FUCTION
			je_free(queue_out.pDataBuf);
#else
			free(stQueueOut.pDataBuf);
#endif
			stQueueOut.pDataBuf = NULL;
		}
	}
	return;
}


void recv_stream_cb(struct bufferevent *pConnectHbServerBev, HB_VOID *event_args)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)event_args;
	NET_LAYER stPackage;
	struct evbuffer *pEvbufferSrc = bufferevent_get_input(pConnectHbServerBev);			//获取输入缓冲区
	struct sttask *pStpoolTask;
	HB_S32 iDataLen = 0;
	HB_U32 uHashValue = 0;

	for (;;)
	{
		if (evbuffer_get_length(pEvbufferSrc) < 28)
		{
			//printf("head len too small!\n");
			return;
		}
		memset(&stPackage, 0, sizeof(stPackage));
		evbuffer_copyout(pEvbufferSrc, (void*) (&stPackage), 28);
		if (evbuffer_get_length(pEvbufferSrc) < stPackage.iActLength)
		{
			//数据长度不足
			return;
		}
		memset(&stPackage, 0, sizeof(stPackage));
		bufferevent_read(pConnectHbServerBev, &stPackage, 28);
		bufferevent_read(pConnectHbServerBev, pStreamNode->hGetStreamFromSource + pStreamNode->iRecvStreamDataLen, stPackage.iActLength - 28);
		pStreamNode->iRecvStreamDataLen += stPackage.iActLength - 28;
		if (stPackage.byBlockEndFlag) //尾包
		{
			if ((0 == list_size(&(pStreamNode->listClientNodeHead))) || 2 == pStreamNode->uRtpClientNodeSendDataThreadFlag)
			{
				break;
			}

			if ((0 == pStreamNode->uRtpClientNodeSendDataThreadFlag) && (1 == stPackage.byFrameType))
			{
				pStpoolTask = stpool_task_new(glGbThreadPool, "send_rtp_to_client_task", send_rtp_to_client_task,
								send_rtp_to_client_task_err_cb, pStreamNode);
				HB_S32 iError = stpool_task_set_p(pStpoolTask, glGbThreadPool);
				if (iError)
				{
					printf("***Err: %d(%s). (try eCAP_F_CUSTOM_TASK)\n", iError, stpool_strerror(iError));
					uHashValue = pStreamNode->uStreamNodeHashValue;
					pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
					destory_client_list(&(pStreamNode->listClientNodeHead)); //释放rtp客户队列
					clear_rtp_data_from_queue(pStreamNode->queueStreamData); //释放视频队列
					nolock_queue_destroy(&(pStreamNode->queueStreamData));
					if (pStreamNode->hGetStreamFromSource != NULL)
					{
						free(pStreamNode->hGetStreamFromSource);
						pStreamNode->hGetStreamFromSource = NULL;
					}
					free(pStreamNode);
					pStreamNode = NULL;
					pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
					return;
				}
				else
				{
					stpool_task_set_userflags(pStpoolTask, 0x1);
					stpool_task_queue(pStpoolTask);
					pStreamNode->uRtpClientNodeSendDataThreadFlag = 1; //发送rtp数据线程已经启动
				}
			}

			if (0 == pStreamNode->uRtpClientNodeSendDataThreadFlag || 13 != stPackage.byDataType) //如果第一个I帧还没有来或者非音视频帧，则直接丢弃
			{
				pStreamNode->iRecvStreamDataLen = 0;
				return;
			}

#if 1
			BOX_CTRL_CMD_OBJ stCmdHead;
			memset(&stCmdHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
			strncpy(stCmdHead.header, "hBzHbox@", 8);
			stCmdHead.cmd_length = pStreamNode->iRecvStreamDataLen;

			if (0 == stPackage.byFrameType)
			{
				stCmdHead.data_type = BP_FRAME;
			}
			else if (1 == stPackage.byFrameType)
			{
				stCmdHead.data_type = I_FRAME;
			}
			else if (4 == stPackage.byFrameType)
			{
				stCmdHead.data_type = AUDIO_FRAME;
			}

			stCmdHead.v_pts = (((((HB_U64) (stPackage.iTimeStampHigh)) << 32) + stPackage.iTimeStampLow)) * 90;
			//		if ((stPackage.byFrameType == 0) || (stPackage.byFrameType == 1))
			//		{
			//printf("stCmdHead.v_pts=%ld------->FrameType------>%d------>DataLen========%d\n", stCmdHead.v_pts, stPackage.byFrameType, pStreamNode->iRecvStreamDataLen);
			//		}
			if (nolock_queue_len(pStreamNode->queueStreamData) > 512)
			{
				break;
			}
			iDataLen = (stCmdHead.cmd_length);
			HB_S32 iRtpDataBufPreSize = (((iDataLen / MAX_RTP_PACKET_SIZE) + 1) * 20) + MAX_RTP_PACKET_SIZE;//20为（rtp头）
			//HB_S32 rtp_data_buf_pre_size = 10240;
			HB_S32 rtp_data_buf_len = iDataLen + iRtpDataBufPreSize;
			QUEUE_ARGS_OBJ stQueueArg;
#if JE_MELLOC_FUCTION
			stQueueArg.pDataBuf = (HB_CHAR*)je_malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#else
			stQueueArg.pDataBuf = (HB_CHAR*) malloc(rtp_data_buf_len + sizeof(BOX_CTRL_CMD_OBJ));
#endif
			stQueueArg.iDataLen = iDataLen + sizeof(BOX_CTRL_CMD_OBJ);
			stQueueArg.iDataPreBufSize = iRtpDataBufPreSize;
			memcpy(stQueueArg.pDataBuf + iRtpDataBufPreSize, &stCmdHead, sizeof(BOX_CTRL_CMD_OBJ));
			memcpy(stQueueArg.pDataBuf + iRtpDataBufPreSize + 32, pStreamNode->hGetStreamFromSource, pStreamNode->iRecvStreamDataLen);
			nolock_queue_push(pStreamNode->queueStreamData, &stQueueArg);
			pStreamNode->iRecvStreamDataLen = 0;
#endif
			return;
		}
	}

	//发生异常
	bufferevent_free(pConnectHbServerBev);
	pConnectHbServerBev = NULL;
	uHashValue = pStreamNode->uStreamNodeHashValue;
	//printf("\n@@@@@@@@@@@@@@@@@@  recv_stream_cb() err  ret=%d\n", ret);
	if (0 == pStreamNode->uRtpClientNodeSendDataThreadFlag)	//rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = 0");
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		list_delete(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), pStreamNode);
		destory_client_list(&(pStreamNode->listClientNodeHead));	//释放rtp客户队列
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		clear_rtp_data_from_queue(pStreamNode->queueStreamData);	//释放视频队列
		nolock_queue_destroy(&(pStreamNode->queueStreamData));
		if (pStreamNode->hGetStreamFromSource != NULL)
		{
			free(pStreamNode->hGetStreamFromSource);
			pStreamNode->hGetStreamFromSource = NULL;
		}
		free(pStreamNode);
		pStreamNode = NULL;
		return;
	}
	else if ((1 == pStreamNode->uRtpClientNodeSendDataThreadFlag) || (2 == pStreamNode->uRtpClientNodeSendDataThreadFlag))	//如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{
		TRACE_ERR("recv_stream_cb()   uRtpClientNodeSendDataThreadFlag = %d\n", pStreamNode->uRtpClientNodeSendDataThreadFlag);
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(pStreamNode->listClientNodeHead));
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		pStreamNode->uGetStreamThreadStartFlag = 2; //接受数据流模块已退出
	}

	return;
}

HB_VOID recv_stream_err_cb(struct bufferevent *pConnectHbserverBev, HB_S16 event, HB_VOID *hArg)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)(hArg);

	HB_S32 iErrCode = EVUTIL_SOCKET_ERROR();
	if (event & BEV_EVENT_TIMEOUT) //超时
	{
		TRACE_ERR("\n###########recv_stream_err_cb  BEV_EVENT_TIMEOUT(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}
	else if (event & BEV_EVENT_EOF)
	{
		TRACE_ERR("\n###########recv_stream_err_cb  connection normal closed(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}
	else if (event & BEV_EVENT_ERROR)
	{
		TRACE_ERR("\n###########recv_stream_err_cb  BEV_EVENT_ERROR(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}
	else
	{
		TRACE_ERR("\n###########recv_stream_err_cb  BEV_EVENT_OTHER_ERROR(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}

	//发生异常
	bufferevent_free(pConnectHbserverBev);
	pConnectHbserverBev = NULL;
	HB_U32 uHashValue = 0;
	uHashValue = pStreamNode->uStreamNodeHashValue;
	if (0 == pStreamNode->uRtpClientNodeSendDataThreadFlag) //rtp发送线程还未启动
	{
		TRACE_ERR("recv_stream_err_cb()   uRtpClientNodeSendDataThreadFlag = 0");
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		list_delete(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
		destory_client_list(&(pStreamNode->listClientNodeHead)); //释放rtp客户队列
		clear_rtp_data_from_queue(pStreamNode->queueStreamData); //释放视频队列
		nolock_queue_destroy(&(pStreamNode->queueStreamData));
		if (pStreamNode->hGetStreamFromSource != NULL)
		{
			free(pStreamNode->hGetStreamFromSource);
			pStreamNode->hGetStreamFromSource = NULL;
		}
		free(pStreamNode);
		pStreamNode = NULL;
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	}
	else if ((1 == pStreamNode->uRtpClientNodeSendDataThreadFlag) || (2 == pStreamNode->uRtpClientNodeSendDataThreadFlag)) //如果rtp节点发送线程已经启动,这里只是把节点摘除，并不释放，由rtp发送线程释放
	{

		TRACE_ERR("recv_stream_err_cb()   uRtpClientNodeSendDataThreadFlag = %d\n", pStreamNode->uRtpClientNodeSendDataThreadFlag);
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		disable_client_rtp_list_bev(&(pStreamNode->listClientNodeHead));
		pStreamNode->uGetStreamThreadStartFlag = 2; //接受数据流模块已退出
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	}

	return;
}

HB_VOID recv_cmd_from_hbserver(struct bufferevent *pConnectHbserverBev, HB_VOID *event_args)
{
	HB_S32 iRet = 0;
	NET_LAYER stPackage;
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)event_args;
	struct evbuffer *pEvbufferSrc = bufferevent_get_input(pConnectHbserverBev); //获取输入缓冲区

	memset(pStreamNode->hGetStreamFromSource, 0, 2048);

	for (;;)
	{
		if (evbuffer_get_length(pEvbufferSrc) < 28)
		{
			//printf("head len too small!\n");
			return;
		}
		memset(&stPackage, 0, sizeof(stPackage));
		evbuffer_copyout(pEvbufferSrc, (void*) (&stPackage), 28);
		if (evbuffer_get_length(pEvbufferSrc) < stPackage.iActLength)
		{
			//数据长度不足
			return;
		}

		memset(&stPackage, 0, sizeof(stPackage));
		bufferevent_read(pConnectHbserverBev, &stPackage, 28);
		bufferevent_read(pConnectHbserverBev, pStreamNode->hGetStreamFromSource + pStreamNode->iRecvStreamDataLen, stPackage.iActLength - 28);
		pStreamNode->iRecvStreamDataLen += stPackage.iActLength - 28;
		if (stPackage.byBlockEndFlag) //尾包
		{
			TRACE_LOG("recv recv :[%s]\n", pStreamNode->hGetStreamFromSource);
			if (strstr(pStreamNode->hGetStreamFromSource, "SUCCESS"))
			{
				pStreamNode->iRecvStreamDataLen = 0;
				HB_CHAR pImOk[32] = { 0 };
				strncpy(pImOk, "<TYPE>ImOK</TYPE>", 17);
				SendDataToStream(pConnectHbserverBev, pImOk, strlen(pImOk), DATA_TYPE_SMS_CMD);
				iRet = nolock_queue_init(&(pStreamNode->queueStreamData), 0, sizeof(QUEUE_ARGS_OBJ), 512);
				if (0 == iRet)
				{
					struct timeval stReadTimeout;
					stReadTimeout.tv_sec = 15;
					stReadTimeout.tv_usec = 0;
					bufferevent_set_timeouts(pConnectHbserverBev, &stReadTimeout, NULL);
					bufferevent_setwatermark(pConnectHbserverBev, EV_READ, 65536, 128 * 1024);
					bufferevent_set_max_single_read(pConnectHbserverBev, 65536);
					bufferevent_set_max_single_write(pConnectHbserverBev, 65536);
					bufferevent_setcb(pConnectHbserverBev, recv_stream_cb, NULL, recv_stream_err_cb, pStreamNode);
					return;
				}
				else
				{
					printf("lf_queue_init error: %d\n", iRet);
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

	bufferevent_free(pConnectHbserverBev);
	pConnectHbserverBev = NULL;
	HB_U32 uHashValue = pStreamNode->uStreamNodeHashValue;
	pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	del_node_from_stream_hash_table(glStreamHashTable, pStreamNode);
	TRACE_ERR("recv recv :[%s]\n", pStreamNode->hGetStreamFromSource);
	destory_client_list(&(pStreamNode->listClientNodeHead)); //释放rtp客户队列
	if (pStreamNode->hGetStreamFromSource != NULL)
	{
		free(pStreamNode->hGetStreamFromSource);
		pStreamNode->hGetStreamFromSource = NULL;
	}
	free(pStreamNode);
	pStreamNode = NULL;
	pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
}

HB_VOID connect_event_cb(struct bufferevent *pConnectHbServerBev, HB_S16 iEvent, HB_VOID *arg)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE) (arg);

	HB_S32 iErrCode = EVUTIL_SOCKET_ERROR();
	if (iEvent & BEV_EVENT_TIMEOUT) //超时
	{
		TRACE_ERR("\n###########  BEV_EVENT_TIMEOUT(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}
	else if (iEvent & BEV_EVENT_EOF)
	{
		TRACE_ERR("\n###########  connection closed(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}
	else if (iEvent & BEV_EVENT_ERROR)
	{
		TRACE_ERR("\n###########  BEV_EVENT_ERROR(%d) : %s !\n", iErrCode, evutil_socket_error_to_string(iErrCode));
	}
	else if (iEvent & BEV_EVENT_CONNECTED)
	{
		HB_CHAR cmd_buf[1024] = { 0 };
		TRACE_LOG("the client has connected to server\n");
#if 1
		snprintf(cmd_buf, sizeof(cmd_buf),
						"<TYPE>StartStream</TYPE><DVRName>%s</DVRName><ChnNo>%d</ChnNo><StreamType>%d</StreamType><TType>0</TType>", pStreamNode->cDevId,
						pStreamNode->iDevChnl, pStreamNode->iStreamType);
#endif
#if 0
		snprintf(cmd_buf, sizeof(cmd_buf),
						"<TYPE>StartStream</TYPE><DVRName>%s</DVRName><ChnNo>%d</ChnNo><StreamType>%d</StreamType><TType>0</TType>", "12345",
						pStreamNode->iDevChnl, pStreamNode->iStreamType);
#endif
		SendDataToStream(pConnectHbServerBev, cmd_buf, strlen(cmd_buf), DATA_TYPE_SMS_CMD);
		pStreamNode->hGetStreamFromSource = (HB_CHAR *) calloc(1, 1024 * 1024); //预先申请1M空间
		struct timeval stReadTimeout;
		stReadTimeout.tv_sec = 15;
		stReadTimeout.tv_usec = 0;
		bufferevent_set_timeouts(pConnectHbServerBev, &stReadTimeout, NULL);
		bufferevent_setcb(pConnectHbServerBev, recv_cmd_from_hbserver, NULL, recv_stream_err_cb, (HB_VOID*)(pStreamNode));
		bufferevent_enable(pConnectHbServerBev, EV_READ | EV_PERSIST);

		return;
	}

	//以下是处理异常情况
	bufferevent_disable(pConnectHbServerBev, EV_READ | EV_WRITE);
	bufferevent_free(pConnectHbServerBev);
	pConnectHbServerBev = NULL;
	pStreamNode->pConnectHbServerBev = NULL;

	do
	{
		//从汉邦服务器或者盒子获取流的线程还没有启动,所有节点资源在这里直接释放
		printf("\nbbb\n");
		HB_U32 uHashValue = pStreamNode->uStreamNodeHashValue;
		pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		del_node_from_stream_hash_table(glStreamHashTable, pStreamNode);
		destory_client_list(&(pStreamNode->listClientNodeHead)); //释放rtp客户队列
		free(pStreamNode);
		pStreamNode=NULL;
		pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	} while (0);
	printf("\n *****connect_event_cb() !\n");
}

HB_S32 play_rtsp_video_from_hbserver(STREAM_NODE_HANDLE pStreamNode)
{
	if (1 == pStreamNode->uRtspPlayFlag)
	{
		return HB_FAILURE;
	}

	HB_S32 iConnectSockFd = -1;
	struct bufferevent *pConnectHbserverBev = NULL;
	struct sockaddr_in stServerAddr;

	//初始化server_addr
	memset(&stServerAddr, 0, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(pStreamNode->iDevPort);
	inet_aton(pStreamNode->cDevIp, &stServerAddr.sin_addr);

	//创建接收视频流数据的bev，并且与rtsp客户端请求视频创建的bev放在同一个base里面，可以减少锁的数量，因为同一个base里，bev都是串行的
	pConnectHbserverBev = bufferevent_socket_new(pStreamNode->pWorkBase, -1,
					BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS); //第二个参数传-1,表示以后设置文件描述符
	if (NULL == pConnectHbserverBev)
	{
		return HB_FAILURE;
	}

	pStreamNode->pConnectHbServerBev = pConnectHbserverBev;
	iConnectSockFd = bufferevent_getfd(pConnectHbserverBev);
	HB_S32 iRecvBufLen = 65536; //设置为64K
	setsockopt(iConnectSockFd, SOL_SOCKET, SO_RCVBUF, (const HB_CHAR*) &iRecvBufLen, sizeof(HB_S32));
	bufferevent_setwatermark(pConnectHbserverBev, EV_READ, 29, 0);

	//设置bufferevent各回调函数
	bufferevent_setcb(pConnectHbserverBev, NULL, NULL, connect_event_cb, (HB_VOID*) (pStreamNode));
	//启用读取或者写入事件
	if (-1 == bufferevent_enable(pConnectHbserverBev, EV_READ))
	{
		bufferevent_free(pConnectHbserverBev);
		pStreamNode->pConnectHbServerBev = NULL;
		return HB_FAILURE;
	}

	printf("connect to zhou stream : [%s]:[%d]\n", pStreamNode->cDevIp, pStreamNode->iDevPort);
	//调用bufferevent_socket_connect函数
	if (-1 == bufferevent_socket_connect(pConnectHbserverBev, (struct sockaddr*) &stServerAddr, sizeof(struct sockaddr_in)))
	{
		bufferevent_free(pConnectHbserverBev);
		pStreamNode->pConnectHbServerBev = NULL;
		return HB_FAILURE;
	}

	struct timeval tv_read;
	tv_read.tv_sec = 15;
	tv_read.tv_usec = 0;
	bufferevent_set_timeouts(pConnectHbserverBev, &tv_read, NULL);

	pStreamNode->uRtspPlayFlag = 1; //rtsp播放功能已启动

	return HB_SUCCESS;
}

