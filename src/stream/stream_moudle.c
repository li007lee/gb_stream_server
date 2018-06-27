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

#include "stream_moudle.h"
#include "../stream_hash.h"

extern STREAM_HASH_TABLE_HANDLE glStreamHashTable;
//extern struct bufferevent *sip_stream_msg_pair[2];

HB_VOID stream_read_cb(struct bufferevent *buf_bev, HB_VOID *arg)
{
//	struct bufferevent *pWriteToSipBev = sip_stream_msg_pair[1];
	STREAM_NODE_HANDLE pStreamNode = NULL;
	SIP_NODE_OBJ stSipNode;
	memset(&stSipNode, 0, sizeof(SIP_NODE_OBJ));
	bufferevent_read(buf_bev, &stSipNode, sizeof(SIP_NODE_OBJ));

	printf("recv recv from sip moudle!\n");
	switch (stSipNode.enumCmdType)
	{
		case PLAY:
		{
			pStreamNode = insert_node_to_stream_hash_table(glStreamHashTable, &stSipNode);
			HB_S32 uHashValue = pStreamNode->iStreamNodeHashValue;
			pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
			//查找客户节点
			RTP_CLIENT_TRANSPORT_HANDLE client_node = find_client_node(pStreamNode, stSipNode.cCallId);
			if (NULL == client_node)
			{
				client_node = (RTP_CLIENT_TRANSPORT_HANDLE) calloc(1, sizeof(RTP_CLIENT_TRANSPORT_OBJ));
				strncpy(client_node->cCallId, stSipNode.cCallId, sizeof(client_node->cCallId));
				client_node->stUdpVideoInfo.cli_ports.RTP = stSipNode.iPushPort;
				client_node->iUdpVideoFd = socket(AF_INET, SOCK_DGRAM, 0);
				if (client_node->iUdpVideoFd < 0)
				{
					fprintf(stderr, "Socket Error:%s\n", strerror(errno));
				}

				bzero(&(client_node->stUdpVideoInfo.rtp_peer), sizeof(struct sockaddr_in));
				client_node->stUdpVideoInfo.rtp_peer.sin_family = AF_INET;
				client_node->stUdpVideoInfo.rtp_peer.sin_port = htons(client_node->stUdpVideoInfo.cli_ports.RTP);
				inet_aton(stSipNode.cPushIp, &(client_node->stUdpVideoInfo.rtp_peer.sin_addr));
				printf("#######################append client  cCallId:[%s]\n", stSipNode.cCallId);
				list_append(&(pStreamNode->listClientNodeHead), client_node);
			}

			play_rtsp_video_from_hbserver(pStreamNode);
//			play_rtsp_video_from_box(pStreamNode);
			pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
			printf("succeed!\n");
		}
			break;
		case STOP:
		{
			printf("\n*************************** recv del client from sip\n");
//			printf("\nstSipNode.cSipDevSn=[%s] \n", stSipNode.cSipDevSn);
//			printf("\nuHashValue=[%u] \n", get_stream_hash_value(glStreamHashTable, stSipNode.cSipDevSn));
//			printf("\nuStreamHashTableLen=[%u] \n", glStreamHashTable->uStreamHashTableLen);
//			HB_U32 uHashValue = pHashFunc(stSipNode.cSipDevSn) % glStreamHashTable->uStreamHashTableLen;
			HB_U32 uHashValue = get_stream_hash_value(glStreamHashTable, stSipNode.cSipDevSn);
//			printf("00000000sip_node.cCallId=[%s], cSipDevSn=[%s]\n", stSipNode.cCallId, stSipNode.cSipDevSn);
			pthread_mutex_lock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
			pStreamNode = find_node_from_stream_hash_table(glStreamHashTable, uHashValue, &stSipNode);
			if (NULL != pStreamNode)
			{
				printf("111111111111111111sip_node.cCallId=[%s]\n", stSipNode.cCallId);
				RTP_CLIENT_TRANSPORT_HANDLE client_node = find_client_node(pStreamNode, stSipNode.cCallId);
				if (NULL != client_node)
				{
					printf("del del del del del client !\n");
					client_node->iDeleteFlag = 1;
				}
			}
			pthread_mutex_unlock(&(glStreamHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
		}
			break;
		default:
			break;
	}
}
