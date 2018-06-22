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
#include "common_args.h"
#include "../stream_hash.h"

extern STREAM_HASH_TABLE_HANDLE stream_hash_table;
extern struct bufferevent *sip_stream_msg_pair[2];

HB_VOID stream_read_cb(struct bufferevent *buf_bev, HB_VOID *arg)
{
	struct bufferevent *write_to_sip_bev = sip_stream_msg_pair[1];
	SIP_NODE_OBJ sip_node;
	memset(&sip_node, 0, sizeof(SIP_NODE_OBJ));
	bufferevent_read(buf_bev, &sip_node, sizeof(SIP_NODE_OBJ));

	printf("recv recv from sip moudle!\n");

	switch (sip_node.enumCmdType)
	{
		case PLAY:
		{
			STREAM_NODE_HANDLE p_stream_node = InsertNodeToStreamHashTable(stream_hash_table, &sip_node);
			HB_S32 hash_value = p_stream_node->iStreamNodeHashValue;
			pthread_mutex_lock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
			//查找客户节点
			RTP_CLIENT_TRANSPORT_HANDLE client_node = find_client_node(p_stream_node, sip_node.cCallId);
			if (NULL == client_node)
			{
				client_node = (RTP_CLIENT_TRANSPORT_HANDLE) calloc(1, sizeof(RTP_CLIENT_TRANSPORT_OBJ));
				strncpy(client_node->cCallId, sip_node.cCallId, sizeof(client_node->cCallId));
				client_node->stUdpVideoInfo.cli_ports.RTP = sip_node.iPushPort;
				client_node->iUdpVideoFd = socket(AF_INET, SOCK_DGRAM, 0);
				if (client_node->iUdpVideoFd < 0)
				{
					fprintf(stderr, "Socket Error:%s\n", strerror(errno));
				}

				bzero(&(client_node->stUdpVideoInfo.rtp_peer), sizeof(struct sockaddr_in));
				client_node->stUdpVideoInfo.rtp_peer.sin_family = AF_INET;
//						client_node->udp_video.rtp_peer.sin_addr.s_addr=htonl(sip_node->push_ip);
				client_node->stUdpVideoInfo.rtp_peer.sin_port = htons(client_node->stUdpVideoInfo.cli_ports.RTP);
				inet_aton(sip_node.cPushIp, &(client_node->stUdpVideoInfo.rtp_peer.sin_addr));
				printf("#######################append client  cCallId:[%s]\n", sip_node.cCallId);
				list_append(&(p_stream_node->listClientNodeHead), client_node);
			}

//			printf("client ip[%s], port[%d]\n", htonl(client_node->udp_video.rtp_peer.sin_addr.s_addr), htons(client_node->udp_video.rtp_peer.sin_port));
			play_rtsp_video_from_hbserver(p_stream_node);
//			play_rtsp_video_from_box(p_stream_node);
			pthread_mutex_unlock(&(stream_hash_table->pStreamHashNodeHead[hash_value].lockStreamNodeMutex));
			printf("succeed!\n");
		}
			break;
		case STOP:
		{
			printf("\n*************************** recv del client from sip\n");
			STREAM_NODE_HANDLE p_stream_node = find_node_from_stream_hash_table(stream_hash_table, &sip_node);
			if (NULL != p_stream_node)
			{
				printf("111111111111111111sip_node.cCallId=[%s]\n", sip_node.cCallId);
				RTP_CLIENT_TRANSPORT_HANDLE client_node = find_client_node(p_stream_node, sip_node.cCallId);
				if (NULL != client_node)
				{
					printf("del del del del del client !\n");
					client_node->iDeleteFlag = 1;
				}
			}
		}
			break;
		default:
			break;
	}
}
