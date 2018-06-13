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
#include "../common/common_args.h"
#include "../dev_hash.h"

extern STREAM_HASH_TABLE_HANDLE stream_hash_table;
extern struct bufferevent *sip_stream_msg_pair[2];

HB_VOID stream_read_cb(struct bufferevent *buf_bev, HB_VOID *arg)
{
	struct bufferevent *write_to_sip_bev = sip_stream_msg_pair[1];
	SIP_NODE_OBJ sip_node;
	memset(&sip_node, 0, sizeof(SIP_NODE_OBJ));
	bufferevent_read(buf_bev, &sip_node, sizeof(SIP_NODE_OBJ));

	printf("recv recv from sip moudle!\n");

	switch (sip_node.cmd_type)
	{
		case READY:
		{
//			printf("recv READY cmd!\n");
//			bufferevent_write(write_to_sip_bev, &sip_node, sizeof(SIP_NODE_OBJ));
		}
			break;
		case PLAY:
		{
			DEV_NODE_HANDLE dev_node = InsertNodeToDevHashTable(stream_hash_table, &sip_node);
			HB_S32 hash_value = dev_node->dev_node_hash_value;
			pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
			//查找客户节点
			RTP_CLIENT_TRANSPORT_HANDLE client_node = FindClientNode(dev_node, sip_node.call_id);
			if (NULL == client_node)
			{
				client_node = (RTP_CLIENT_TRANSPORT_HANDLE) calloc(1, sizeof(RTP_CLIENT_TRANSPORT_OBJ));
				strncpy(client_node->call_id, sip_node.call_id, sizeof(client_node->call_id));
				client_node->udp_video.cli_ports.RTP = sip_node.push_port;
				client_node->rtp_fd_video = socket(AF_INET, SOCK_DGRAM, 0);
				if (client_node->rtp_fd_video < 0)
				{
					fprintf(stderr, "Socket Error:%s\n", strerror(errno));
				}

				bzero(&(client_node->udp_video.rtp_peer), sizeof(struct sockaddr_in));
				client_node->udp_video.rtp_peer.sin_family = AF_INET;
//						client_node->udp_video.rtp_peer.sin_addr.s_addr=htonl(sip_node->push_ip);
				client_node->udp_video.rtp_peer.sin_port = htons(client_node->udp_video.cli_ports.RTP);
				inet_aton(sip_node.push_ip, &(client_node->udp_video.rtp_peer.sin_addr));

				list_append(&(dev_node->client_node_head), client_node);
			}

//			printf("client ip[%s], port[%d]\n", htonl(client_node->udp_video.rtp_peer.sin_addr.s_addr), htons(client_node->udp_video.rtp_peer.sin_port));
			play_rtsp_video_from_hbserver(dev_node);
			pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
			printf("succeed!\n");
		}
			break;
		case STOP:
		{
			printf("\n*************************** recv del client from sip\n");
			DEV_NODE_HANDLE dev_node = FindDevFromDevHashTable(stream_hash_table, &sip_node);
			if (NULL != dev_node)
			{
				printf("111111111111111111sip_node.call_id=[%s]\n", sip_node.call_id);
				RTP_CLIENT_TRANSPORT_HANDLE client_node = FindClientNode(dev_node, sip_node.call_id);
				if (NULL != client_node)
				{
					printf("del del del del del client !\n");
					client_node->delete_flag = 1;
				}
			}
		}
			break;
		default:
			break;
	}
}
