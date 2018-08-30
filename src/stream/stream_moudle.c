/*
 * stream_moudle.c
 *
 *  Created on: Jun 12, 2018
 *      Author: root
 */

#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "event2/event_compat.h"

#include "stream/stream_moudle.h"
#include "stream_hash.h"
#include "common_args.h"
#include "server_config.h"
HB_VOID sip_pair_read_cb(struct bufferevent *buf_bev, HB_VOID *arg);

extern STREAM_HASH_TABLE_HANDLE glStreamHashTable;
extern struct bufferevent *sip_stream_msg_pair[2];

static HB_VOID udp_rtcp_recv_cb(const HB_S32 iUdpSockFd, HB_S16 iWhich, HB_HANDLE hArg)
{

//	struct sockaddr_in stServerSinAddr;
//	socklen_t iServerAddrSize = sizeof(stServerSinAddr);
//	HB_CHAR cRecvBuf[4096] = { 0 };
//	/* Recv the data, store the address of the sender in server_sin */
//	if (recvfrom(iUdpSockFd, cRecvBuf, sizeof(cRecvBuf) - 1, 0, (struct sockaddr *) &stServerSinAddr, &iServerAddrSize) == -1)
//	{
//		perror("recvfrom()");
//		event_loopbreak();
//		return;
//	}
//	TRACE_YELLOW("######################recv recv  rtcp rtcp rtcp rtcp [%s]!!!!!!!!!!!!!!!!!!\n", cRecvBuf);
#if 1
	RTP_CLIENT_TRANSPORT_HANDLE pClientNode = (RTP_CLIENT_TRANSPORT_HANDLE)hArg;

	switch(iWhich)
	{
		case EV_TIMEOUT :
		{
			if (pClientNode != NULL)
			{
				pClientNode->iDeleteFlag = 1;
			}
			struct bufferevent *pWriteToSipBev = sip_stream_msg_pair[1];
			SIP_STREAM_MSG_ARGS_HANDLE pWriteToSipBevArg = calloc(1, sizeof(SIP_STREAM_MSG_ARGS_OBJ));
			pWriteToSipBevArg->enumCmdType = STOP;
			strncpy(pWriteToSipBevArg->cCallId, pClientNode->cCallId, sizeof(pWriteToSipBevArg->cCallId));

			bufferevent_write(pWriteToSipBev, pWriteToSipBevArg, sizeof(SIP_STREAM_MSG_ARGS_OBJ));
			bufferevent_setcb(sip_stream_msg_pair[0], sip_pair_read_cb, NULL, NULL, (HB_VOID *)pWriteToSipBevArg);
			bufferevent_enable(sip_stream_msg_pair[0], EV_READ);
			break;
		}
		case EV_READ:
		{
			struct sockaddr_in stServerSinAddr;
			socklen_t iServerAddrSize = sizeof(stServerSinAddr);
			HB_CHAR cRecvBuf[4096] = { 0 };
			/* Recv the data, store the address of the sender in server_sin */
			if (recvfrom(iUdpSockFd, cRecvBuf, sizeof(cRecvBuf) - 1, 0, (struct sockaddr *) &stServerSinAddr, &iServerAddrSize) == -1)
			{
				perror("recvfrom()");
//				event_loopbreak();
				return;
			}
			TRACE_YELLOW("######################recv recv  rtcp rtcp rtcp rtcp [%s]!!!!!!!!!!!!!!!!!!\n", cRecvBuf);
			break;
		}
		default:
			TRACE_YELLOW("######################recv recv  rtcp rtcp rtcp rtcp lalalalalalaalalalal!!!!!!!!!!!!!!!!!!\n");
			break;

	}
#endif


}


static HB_VOID udp_send_rtcp_timer(const HB_S32 iUdpSockFd, HB_S16 iWhich, HB_HANDLE hArg)
{
	RTP_CLIENT_TRANSPORT_HANDLE pClientNode = (RTP_CLIENT_TRANSPORT_HANDLE)hArg;

	if (1 == pClientNode->iSendIframeFlag)
	{
		HB_S32 iSrLen = 0;
		HB_S32 iSdesLen = 0;

		HB_CHAR cRtcpBuf[512] = {0};
//		iSrLen = rtcp_sr_pack(pClientNode, cRtcpBuf, 512, u64Time);
		iSrLen = rtcp_sr_pack(pClientNode, cRtcpBuf, sizeof(cRtcpBuf));
		iSdesLen = rtcp_sdes_pack(pClientNode, cRtcpBuf+iSrLen, 512-iSrLen);
		sendto(pClientNode->stUdpVideoInfo.iUdpRtcpSockFd, cRtcpBuf, iSrLen+iSdesLen, 0, (struct sockaddr*)(&(pClientNode->stUdpVideoInfo.rtcp_peer)), (socklen_t)(sizeof(struct sockaddr_in)));
		printf("send rtcp to peer  [%d] to [%d]\n", pClientNode->stUdpVideoInfo.ser_ports.RTCP, pClientNode->stUdpVideoInfo.cli_ports.RTCP);
		struct timeval tv = {RTCP_RECV_TIMEOUT, 0};
		if (NULL == pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent)
		{
			struct event_base *pWorkBase = event_get_base(pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent);
			pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent = event_new(pWorkBase, pClientNode->stUdpVideoInfo.iUdpRtcpSockFd, EV_READ | EV_PERSIST, udp_rtcp_recv_cb, (HB_VOID *)pClientNode);
			event_add(pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent, &tv);
		}
	}
}


HB_VOID stream_read_cb(struct bufferevent *buf_bev, HB_VOID *arg)
{
//	struct bufferevent *pWriteToSipBev = sip_stream_msg_pair[1];
	STREAM_NODE_HANDLE pStreamNode = NULL;
	SIP_NODE_OBJ stSipNode;
	memset(&stSipNode, 0, sizeof(SIP_NODE_OBJ));
	bufferevent_read(buf_bev, &stSipNode, sizeof(SIP_NODE_OBJ));
	static HB_S32 iSendToZhangPort = 10000;

	switch (stSipNode.enumCmdType)
	{
		case PLAY:
		{
#if 1
			HB_CHAR cHashSource[128] = {0};
			snprintf(cHashSource, sizeof(cHashSource), "%s_%d_%d", stSipNode.cSipDevSn, stSipNode.iDevChnl, stSipNode.iStreamType);
			TRACE_YELLOW("\nIIIIIIIIIIII  insert_node_to_stream_hash_table cDevId=[%s], cCallId=[%s], uStreamHashTableLen=[%d]\n", \
							stSipNode.cSipDevSn, stSipNode.cCallId, glStreamHashTable->uStreamHashTableLen);
			HB_U32 uHashValue = get_stream_hash_value(glStreamHashTable, cHashSource);
			pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
			pStreamNode = insert_node_to_stream_hash_table(glStreamHashTable, uHashValue, &stSipNode);
			//查找客户节点
			RTP_CLIENT_TRANSPORT_HANDLE pClientNode = find_client_node(pStreamNode, stSipNode.cCallId);
			if (NULL == pClientNode)
			{
				pClientNode = (RTP_CLIENT_TRANSPORT_HANDLE) calloc(1, sizeof(RTP_CLIENT_TRANSPORT_OBJ));
				strncpy(pClientNode->cCallId, stSipNode.cCallId, sizeof(pClientNode->cCallId));
				pClientNode->u32Ssrc = stSipNode.u32Ssrc;
				pClientNode->stUdpVideoInfo.cli_ports.RTP = stSipNode.iPushPort;
				pClientNode->stUdpVideoInfo.cli_ports.RTCP = stSipNode.iPushPort + 1;
				pClientNode->stUdpVideoInfo.iUdpVideoFd = stSipNode.iUdpSendStreamSockFd;

				//设置对端rtp视频流接收端口
				bzero(&(pClientNode->stUdpVideoInfo.rtp_peer), sizeof(struct sockaddr_in));
				pClientNode->stUdpVideoInfo.rtp_peer.sin_family = AF_INET;
				pClientNode->stUdpVideoInfo.rtp_peer.sin_port = htons(pClientNode->stUdpVideoInfo.cli_ports.RTP);
				inet_aton(stSipNode.cPushIp, &(pClientNode->stUdpVideoInfo.rtp_peer.sin_addr));

				/*******************************测试专用*******************************/
//				pClientNode->stUdpVideoInfo.rtp_peer.sin_port = htons(iSendToZhangPort++);
//				inet_aton("172.16.3.200", &(pClientNode->stUdpVideoInfo.rtp_peer.sin_addr));
//				inet_aton("127.0.0.1", &(pClientNode->stUdpVideoInfo.rtp_peer.sin_addr));
				/*******************************测试专用*******************************/

				if (glGlobleArgs.iUseRtcpFlag)
				{
					pClientNode->stUdpVideoInfo.iUdpRtcpSockFd = stSipNode.iUdpRtcpSockFd;
					pClientNode->stUdpVideoInfo.ser_ports.RTCP = stSipNode.iUdpRtcpListenPort;

					//设置对端rtcp的接收端口
					bzero(&(pClientNode->stUdpVideoInfo.rtcp_peer), sizeof(struct sockaddr_in));
					pClientNode->stUdpVideoInfo.rtcp_peer.sin_family = AF_INET;
					pClientNode->stUdpVideoInfo.rtcp_peer.sin_port = htons(pClientNode->stUdpVideoInfo.cli_ports.RTCP);
					inet_aton(stSipNode.cPushIp, &(pClientNode->stUdpVideoInfo.rtcp_peer.sin_addr));

					struct timeval tv = {RTCP_SEND_TIMER, 0};
					pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent = event_new(pStreamNode->pWorkBase, pClientNode->stUdpVideoInfo.iUdpRtcpSockFd, EV_PERSIST, udp_send_rtcp_timer, (HB_VOID *)pClientNode);
					event_add(pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent, &tv);
				}

				TRACE_BLUE("#######################append client  cCallId:[%s]\n", stSipNode.cCallId);
				list_append(&(pStreamNode->listClientNodeHead), pClientNode);
			}

			play_rtsp_video_from_hbserver(pStreamNode);
//			play_rtsp_video_from_box(pStreamNode);
			pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
#endif
			printf("succeed!\n");
		}
			break;
		case STOP:
		{
			printf("\n*************************** recv del client from sip\n");
			HB_CHAR cHashSource[128] = {0};
			snprintf(cHashSource, sizeof(cHashSource), "%s_%d_%d", stSipNode.cSipDevSn, stSipNode.iDevChnl, stSipNode.iStreamType);
			HB_U32 uHashValue = get_stream_hash_value(glStreamHashTable, cHashSource);
			pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
			pStreamNode = find_node_from_stream_hash_table(glStreamHashTable, uHashValue, &stSipNode);
			if (NULL != pStreamNode)
			{
				RTP_CLIENT_TRANSPORT_HANDLE pClientNode = find_client_node(pStreamNode, stSipNode.cCallId);
				if (NULL != pClientNode)
				{
					TRACE_BLUE("#################del del del del del client !\n");
					pClientNode->iDeleteFlag = 1;
				}
				else
				{
					TRACE_BLUE("000#################Do not found client!\n");
				}
			}
			else
			{
				TRACE_BLUE("111#################Do not found dev[%s]!\n", stSipNode.cSipDevSn);
			}
			pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		}
			break;
		default:
			break;
	}
}
