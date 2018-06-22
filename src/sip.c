/*
 * libevent_server.c
 *
 *  Created on: 2017年9月21日
 *      Author: lijian
 */

#include "sip.h"
#include "hash_table.h"
#include "sdp.h"
#include "stream/get_frame.h"
#include "server_config.h"
#include "sip_hash.h"
#include "stream_hash.h"
#include "osipparser2/osip_parser.h"
#include "stream/stream_moudle.h"

SIP_HASH_TABLE_HANDLE glSipHashTable = NULL;
STREAM_HASH_TABLE_HANDLE stream_hash_table = NULL;
struct bufferevent *sip_stream_msg_pair[2];


struct event_base *pEventBase;
stpool_t *gb_thread_pool;

static HB_S32 build_response_default(osip_message_t ** dest, osip_dialog_t * dialog, int status, osip_message_t * request)
{
	osip_generic_param_t *tag;
	osip_message_t *response;
	int i;

	*dest = NULL;
	if (request == NULL)
		return OSIP_BADPARAMETER;

	i = osip_message_init(&response);
	if (i != 0)
		return i;
	/* initialise osip_message_t structure */
	/* yet done... */

	response->sip_version = (char *) osip_malloc(8 * sizeof(char));
	if (response->sip_version == NULL)
	{
		osip_message_free(response);
		return OSIP_NOMEM;
	}
	sprintf(response->sip_version, "SIP/2.0");
	osip_message_set_status_code(response, status);

#ifndef MINISIZE
	/* handle some internal reason definitions. */
	if (MSG_IS_NOTIFY (request) && status == 481)
	{
		response->reason_phrase = osip_strdup("Subscription Does Not Exist");
	}
	else if (MSG_IS_SUBSCRIBE (request) && status == 202)
	{
		response->reason_phrase = osip_strdup("Accepted subscription");
	}
	else
	{
		response->reason_phrase = osip_strdup(osip_message_get_reason(status));
		if (response->reason_phrase == NULL)
		{
			if (response->status_code == 101)
				response->reason_phrase = osip_strdup("Dialog Establishement");
			else
				response->reason_phrase = osip_strdup("Unknown code");
		}
		response->req_uri = NULL;
		response->sip_method = NULL;
	}
#else
	response->reason_phrase = osip_strdup (osip_message_get_reason (status));
	if (response->reason_phrase == NULL)
	{
		if (response->status_code == 101)
		response->reason_phrase = osip_strdup ("Dialog Establishement");
		else
		response->reason_phrase = osip_strdup ("Unknown code");
	}
	response->req_uri = NULL;
	response->sip_method = NULL;
#endif

	if (response->reason_phrase == NULL)
	{
		osip_message_free(response);
		return OSIP_NOMEM;
	}

	i = osip_to_clone(request->to, &(response->to));
	if (i != 0)
	{
		osip_message_free(response);
		return i;
	}

	i = osip_to_get_tag(response->to, &tag);
	if (i != 0)
	{ /* we only add a tag if it does not already contains one! */
		if ((dialog != NULL) && (dialog->local_tag != NULL))
		/* it should contain the local TAG we created */
		{
			osip_to_set_tag(response->to, osip_strdup(dialog->local_tag));
		}
	}

	i = osip_from_clone(request->from, &(response->from));
	if (i != 0)
	{
		osip_message_free(response);
		return i;
	}

	{
		osip_list_iterator_t it;
		osip_via_t *via = (osip_via_t*) osip_list_get_first(&request->vias, &it);

		while (via != NULL)
		{
			osip_via_t *via2;

			i = osip_via_clone(via, &via2);
			if (i != 0)
			{
				osip_message_free(response);
				return i;
			}
			osip_list_add(&response->vias, via2, -1);
			via = (osip_via_t *) osip_list_get_next(&it);
		}
	}

	i = osip_call_id_clone(request->call_id, &(response->call_id));
	if (i != 0)
	{
		osip_message_free(response);
		return i;
	}
	i = osip_cseq_clone(request->cseq, &(response->cseq));
	if (i != 0)
	{
		osip_message_free(response);
		return i;
	}
#ifndef MINISIZE
	if (MSG_IS_SUBSCRIBE(request))
	{
		osip_header_t *exp;
		osip_header_t *evt_hdr;

		osip_message_header_get_byname(request, "event", 0, &evt_hdr);
		if (evt_hdr != NULL && evt_hdr->hvalue != NULL)
			osip_message_set_header(response, "Event", evt_hdr->hvalue);
		else
			osip_message_set_header(response, "Event", "presence");
		i = osip_message_get_expires(request, 0, &exp);
		if (exp == NULL)
		{
			osip_header_t *cp;

			i = osip_header_clone(exp, &cp);
			if (cp != NULL)
				osip_list_add(&response->headers, cp, 0);
		}
	}
#endif

	osip_message_set_user_agent(response, NULL);

	*dest = response;
	return OSIP_SUCCESS;
}


static HB_S32 parase_invite(osip_message_t ** sip, SIP_DEV_ARGS_HANDLE sip_dev_info)
{
	osip_body_t *body = NULL;
	osip_message_get_body(*sip, 0, &body);

	HB_S32 i = 0;
	HB_CHAR *media_type = NULL;
	sdp_t* sdp = NULL;
	HB_CHAR *p_username = NULL;
	HB_CHAR *p_session = NULL;
	HB_CHAR *p_version = NULL;
	HB_CHAR *p_network = NULL;
	HB_CHAR *p_addrtype = NULL;
	HB_CHAR *p_address = NULL;
	HB_S32 count;

//	char body[1024] = { 0 };
//	strncpy(body, "v=0\r\n"
//					"o=34020000002000000001 0 0 IN IP4 192.168.116.19\r\n"
//					"s=Play\r\n"
//					"c=IN IP4 192.168.116.19\r\n"
//					"t=0 0\r\n"
//					"m=video 19756 RTP/AVP 96 98\r\n"
//					"a=streamMode:SUB\r\n"
//					"a=recvonly\r\n"
//					"a=rtpmap:96 PS/90000\r\n"
//					"a=rtpmap:98 H264/90000\r\n"
//					"a=sms:223.223.199.58:700/5a12caf8\r\n"
//					"y=0201002353\r\n", sizeof(body));
//	printf("body :\n[%s]\n", body->body);

	sdp = sdp_parse(body->body);
	count = sdp_media_count(sdp);
	if (count < 0)
	{
		sdp_destroy(sdp);
		return -1;
	}
	sdp_origin_get(sdp, (const HB_CHAR **)&p_username, (const HB_CHAR **)&p_session, (const HB_CHAR **)&p_version, (const HB_CHAR **)&p_network, (const HB_CHAR **)&p_addrtype, (const HB_CHAR **)&p_address);

	strncpy(sip_dev_info->st_sip_dev_id, p_username, sizeof(sip_dev_info->st_sip_dev_id));
	strncpy(sip_dev_info->st_push_ip, p_address, sizeof(sip_dev_info->st_push_ip));

	const HB_CHAR *ssrc = sdp_y_get(sdp);
	if (NULL != ssrc)
	{
		strncpy(sip_dev_info->st_y, ssrc, sizeof(sip_dev_info->st_y));
	}

	osip_call_id_t *call_id = osip_message_get_call_id(*sip);
	if (NULL != call_id)
	{
//		printf("call_id:[%s]\n", call_id->number);
		strncpy(sip_dev_info->call_id, call_id->number, sizeof(sip_dev_info->call_id));
	}

	for (i = 0; i < count; i++)
	{
		media_type = sdp_media_type(sdp, i);
		if (!strcmp(media_type, "video"))
		{
			HB_S32 push_port = 0;
			HB_S32 num = 0;
			sdp_media_port(sdp, i, &push_port, &num);
			sip_dev_info->st_push_port = push_port;

			HB_CHAR *video_stream_server_info = NULL;
			HB_CHAR p_stream_port[8] = { 0 };
			video_stream_server_info = sdp_media_attribute_find(sdp, i, "sms");
			if(NULL != video_stream_server_info)
			{
				sscanf(video_stream_server_info, "%[^:]:%[^/]/%s", sip_dev_info->st_stram_server_ip, p_stream_port, sip_dev_info->st_dev_id);
				sip_dev_info->st_stream_server_port = atoi(p_stream_port);
			}

			HB_CHAR *video_stream_type = NULL;
			video_stream_type = sdp_media_attribute_find(sdp, i, "streamMode");
			if ((NULL != video_stream_type) && (NULL != strstr(video_stream_type, "SUB")))
			{
				//子码流
				sip_dev_info->st_stream_type = 1;
			}
			else
			{
				//默认主码流
				sip_dev_info->st_stream_type = 0;
			}
		}
	}

//	printf("dev_number:[%s], dev_id:[%s], stream_type:[%d], stream_ip:[%s], stream_port[%d], push_ip:[%s], push_port:[%d], SSRC:[%s]\n",
//					sip_dev_info->st_sip_dev_id, sip_dev_info->st_dev_id, sip_dev_info->st_stream_type, sip_dev_info->st_stram_server_ip,
//					sip_dev_info->st_stream_server_port, sip_dev_info->st_push_ip, sip_dev_info->st_push_port, sip_dev_info->st_y);

	sdp_destroy(sdp);

	return HB_SUCCESS;
}

static HB_S32 parase_ack_bye(osip_message_t *pSipMsg, SIP_DEV_ARGS_HANDLE pSipDevInfo)
{
	osip_call_id_t *pCallId = osip_message_get_call_id(pSipMsg);
	if (NULL != pCallId)
	{
		printf("call id:[%s]\n", pCallId->number);
		strncpy(pSipDevInfo->call_id, pCallId->number, sizeof(pSipDevInfo->call_id));
	}
	else
	{
		printf("get call_id failed!\n");
		return HB_FAILURE;
	}

//	printf("sip_dev_id:[%s], call_id:[%s]\n", sip_dev_info->st_sip_dev_id, sip_dev_info->call_id);

	return HB_SUCCESS;
}

static CONTENT_TYPE_E get_content_type(osip_message_t * pSipMsg)
{
	if (MSG_IS_INVITE(pSipMsg))
	{
		return INVITE;
	}
	else if (MSG_IS_ACK(pSipMsg))
	{
		return ACK;
	}
	else if (MSG_IS_BYE(pSipMsg))
	{
		return BYE;
	}
	if (MSG_IS_REGISTER(pSipMsg))
	{
		return REGISTER;
	}

	return NO_TYPE;
}

/*
 *	Function: 接收到客户端连接的回调函数
 *
 *	@param pListener : 监听句柄
 *  @param iAcceptSockfd : 收到连接后分配的文件描述符（与客户端连接的文件描述符）
 *  @param pClientAddr : 客户端地址结构体
 *
 *	Retrun: 无
 */
static HB_VOID udp_recv_cb(const HB_S32 iUdpSockFd, HB_S16 iWhich, HB_HANDLE hArg)
{
	SIP_DEV_ARGS_OBJ stSipDevInfo;
	osip_message_t *pSipMsg;
	struct bufferevent *pWriteToStreamBev = sip_stream_msg_pair[0];
	struct sockaddr_in stServerSinAddr;
	socklen_t iServerAddrSize = sizeof(stServerSinAddr);
	HB_CHAR cRecvBuf[4096] = { 0 };

	memset(&stSipDevInfo, 0, sizeof(SIP_DEV_ARGS_OBJ));

	/* Recv the data, store the address of the sender in server_sin */
	if (recvfrom(iUdpSockFd, cRecvBuf, sizeof(cRecvBuf) - 1, 0, (struct sockaddr *) &stServerSinAddr, &iServerAddrSize) == -1)
	{
		perror("recvfrom()");
		event_loopbreak();
		return;
	}

	TRACE_YELLOW("FROM TO PARSE: [%s]\n", cRecvBuf);
	osip_message_init(&pSipMsg);
	osip_message_parse(pSipMsg, cRecvBuf, strlen(cRecvBuf));

//	char *pMessageMethod = osip_message_get_method(pSipMsg);
//	printf("message_method:[%s]\n", message_method);

	switch (get_content_type(pSipMsg))
	{
//		case REGISTER:
		case INVITE:
		{
			HB_S32 iMessageLen = 0;
			HB_CHAR *pTmpBufDest = NULL;
			HB_CHAR cResponseBody[4096] = { 0 }; //回应消息体
			osip_message_t *pResponseMsg;
			osip_contact_t *pContact = NULL;
			HB_CHAR cTmpContact[128] = { 0 };
			if (HB_SUCCESS == parase_invite(&pSipMsg, &stSipDevInfo))
			{
				SIP_NODE_HANDLE pSipNode = insert_node_to_sip_hash_table(glSipHashTable, &stSipDevInfo);

				build_response_default(&pResponseMsg, NULL, 200, pSipMsg);
				osip_message_set_content_type(pResponseMsg, "application/sdp");
				osip_message_get_contact(pSipMsg, 0, &pContact);
				snprintf(cTmpContact, sizeof(cTmpContact), "<sip:%s@%s:5060>", pContact->url->username, LOCAL_IP);
				osip_message_set_contact(pResponseMsg, cTmpContact);
				printf("contact : [%s]\n", cTmpContact);
				snprintf(cResponseBody, sizeof(cResponseBody), "v=0\r\n"
								"o=%s 0 0 IN IP4 %s\r\n"
								"s=Play\r\n"
								"c=IN IP4 %s\r\n"
								"t=0 0\r\n"
								"m=video 0 RTP/AVP 96\r\n"
								"a=streamMode:%d\r\n"
								"a=recvonly\r\n"
								"a=rtpmap:96 PS/90000\r\n"
								"y=%s\r\n\r\n", pSipNode->cSipDevSn, LOCAL_IP, LOCAL_IP, pSipNode->iStreamType, pSipNode->cSsrc);
				osip_message_set_body(pResponseMsg, cResponseBody, strlen(cResponseBody));
				osip_message_to_str(pResponseMsg, &pTmpBufDest, (size_t *) &iMessageLen);

				if (-1 == sendto(iUdpSockFd, (HB_VOID *) pTmpBufDest, strlen(pTmpBufDest), 0, (struct sockaddr *) &stServerSinAddr, iServerAddrSize))
				{
					perror("sendto()");
					event_loopbreak();
				}
				osip_message_free(pResponseMsg);
				pResponseMsg = NULL;
				TRACE_GREEN("response len=%d, buf=[%s]\n", iMessageLen, pTmpBufDest);
			}
			break;
		}
		case ACK:
		{
			if (HB_SUCCESS == parase_ack_bye(pSipMsg, &stSipDevInfo))
			{
				SIP_NODE_HANDLE pSipNode = find_node_from_sip_hash_table(glSipHashTable, &stSipDevInfo);
				if (NULL == pSipNode)
				{
					break;
				}
				pSipNode->enumCmdType = PLAY;
				bufferevent_write(pWriteToStreamBev, pSipNode, sizeof(SIP_NODE_OBJ));
				bufferevent_setcb(sip_stream_msg_pair[1], stream_read_cb, NULL, NULL, NULL);
				bufferevent_enable(sip_stream_msg_pair[1], EV_READ);
			}
		}
			break;
		case BYE:
		{
			HB_S32 iMessageLen = 0;
			HB_CHAR *pTmpBufDest = NULL;
			osip_message_t *pResponseMsg;
			if (HB_SUCCESS == parase_ack_bye(pSipMsg, &stSipDevInfo))
			{
				build_response_default(&pResponseMsg, NULL, 200, pSipMsg);
				osip_message_to_str(pResponseMsg, &pTmpBufDest, (size_t *) &iMessageLen);
				if (-1 == sendto(iUdpSockFd, (HB_VOID *) pTmpBufDest, strlen(pTmpBufDest), 0, (struct sockaddr *) &stServerSinAddr, iServerAddrSize))
				{
					perror("sendto()");
					event_loopbreak();
				}
				osip_message_free(pResponseMsg);
				pResponseMsg = NULL;
				TRACE_GREEN("response bye len=%d, buf=[%s]\n", iMessageLen, pTmpBufDest);

				SIP_NODE_HANDLE pSipNode = find_node_from_sip_hash_table(glSipHashTable, &stSipDevInfo);
				if (NULL != pSipNode)
				{
					pSipNode->enumCmdType = STOP;
					bufferevent_write(pWriteToStreamBev, pSipNode, sizeof(SIP_NODE_OBJ));
					del_node_from_sip_hash_table(glSipHashTable, pSipNode);
					free(pSipNode);
					pSipNode = NULL;
					bufferevent_setcb(sip_stream_msg_pair[1], stream_read_cb, NULL, NULL, NULL);
					bufferevent_enable(sip_stream_msg_pair[1], EV_READ);
				}
			}
		}
			break;
		default:
			break;
	}
	osip_message_free(pSipMsg);
	pSipMsg = NULL;
	return;
}


/*
 * function : 启动sip模块
 *
 *  @param : none
 *
 * return : none
 */
HB_S32 start_sip_moudle()
{
	HB_S32 iUdpListenerSock = -1;
	struct sockaddr_in stListenAddr;

	parser_init();

	glSipHashTable = sip_hash_table_create(1000); //创建哈希表
	if (NULL == glSipHashTable)
	{
		TRACE_ERR("###### HashTableCreate() err!");
		return HB_FAILURE;
	}

	stream_hash_table = stream_hash_table_create(1000); //创建哈希表
	if (NULL == stream_hash_table)
	{
		TRACE_ERR("###### HashTableCreate() err!");
		return HB_FAILURE;
	}
	ChooseHashFunc("BKDR_hash"); //选择hash函数

	if (evthread_use_pthreads() != 0)
	{
		TRACE_ERR("########## evthread_use_pthreads() err !");
		return HB_FAILURE;
	}

#if USE_PTHREAD_POOL
	HB_S64 eCAPs = eCAP_F_DYNAMIC | eCAP_F_SUSPEND | eCAP_F_THROTTLE | eCAP_F_ROUTINE | eCAP_F_CUSTOM_TASK | eCAP_F_DISABLEQ | eCAP_F_PRIORITY
					| eCAP_F_WAIT_ALL;
	//创建线程池
	//"rtsp_server_pool" -线程池的名字
	//eCAPs    			  -必要能力集
	//MAX_THREADS	     -限制的最大线程数
	//MIN_THREADS	     -创建线程池后启动的线程数
	//0,	   				  -do not suspend the pool
	//1,	   				   -优先级队列数
	gb_thread_pool = stpool_create("rtsp_server_pool", eCAPs, MAX_THREADS, MIN_THREADS, 0, 1);
	//rtsp_pool = stpool_create("rtsp_server_pool", eCAPs, atoi(argv[2])+10, atoi(argv[2]), 0, 1);
	if (!gb_thread_pool)
	{
		TRACE_ERR("###### create pthread pool  err!");
		return HB_FAILURE;
	}
#endif

	iUdpListenerSock = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&stListenAddr, sizeof(stListenAddr));
	stListenAddr.sin_family = AF_INET;
	stListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	stListenAddr.sin_port = htons(5060);
	pEventBase = event_base_new();
	if (!pEventBase)
	{
		perror("event_base_new()");
		return HB_FAILURE;
	}
	//用于线程安全
	if (evthread_make_base_notifiable(pEventBase) != 0)
	{
		TRACE_ERR("###### evthread_make_base_notifiable() err!");
		return HB_FAILURE;
	}

	if (bind(iUdpListenerSock, (struct sockaddr *) &stListenAddr, sizeof(stListenAddr)))
	{
		perror("bind()");
		return HB_FAILURE;
	}

	struct event udp_event;
	event_assign(&udp_event, pEventBase, iUdpListenerSock, EV_READ | EV_PERSIST, udp_recv_cb, (void*) &udp_event);
	event_add(&udp_event, 0);

	bufferevent_pair_new(pEventBase, BEV_OPT_CLOSE_ON_FREE, sip_stream_msg_pair);
	bufferevent_setcb(sip_stream_msg_pair[1], stream_read_cb, NULL, NULL, NULL);
	bufferevent_enable(sip_stream_msg_pair[1], EV_READ);
//	bufferevent_setcb(sip_stream_msg_pair[0], sip_read_cb, NULL, NULL, NULL);
//	bufferevent_enable(sip_stream_msg_pair[0], EV_READ | EV_PERSIST);

	event_base_dispatch(pEventBase);
	event_base_free(pEventBase);

return HB_SUCCESS;
}
