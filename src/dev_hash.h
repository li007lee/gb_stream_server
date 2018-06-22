/*
 * sip_div.h
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#ifndef SRC_SERVER_HASH_AND_LIST_SIP_DEV_H_
#define SRC_SERVER_HASH_AND_LIST_SIP_DEV_H_

#include "my_include.h"
#include "common/simclist.h"
#include "common/common_args.h"

#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "event2/event_compat.h"

#include "common/lf_queue.h"
#include "stream/rtp_pack.h"
#include "stpool/stpool.h"

#include "sip_hash.h"

#define USER_TIME_OUT	30 //用户连接超时

///////////////////////////////////////////////////////////////////////////////////////////
// RTP和RTCP协议的定义
///////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	rtp_proto = 0,
	rtcp_proto
}rtp_protos;

///////////////////////////////////////////////////////////////////////////////////////////
// RTP传输类型的定义
///////////////////////////////////////////////////////////////////////////////////////////
typedef enum _RTP_type
{
	RTP_no_transport = 0,
	RTP_rtp_avp,    //UDP
	RTP_rtp_avp_tcp //TCP
}RTP_type_e;

///////////////////////////////////////////////////////////////////////////////////////////
// RTP和RTCP端口对结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	int RTP;
	int RTCP;
}port_pair;

///////////////////////////////////////////////////////////////////////////////////////////
// UDP信息定义结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct _udp_info
{
	struct sockaddr_in rtp_peer; //RTP客户端的网络地址
	port_pair cli_ports;      //客户端的端口对
	port_pair ser_ports;      //服务器端的端口对
}udp_info_t;

///////////////////////////////////////////////////////////////////////////////////////////
// TCP信息定义结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct _tcp_info
{
	//port_pair interleaved; //interleaved选项中指定的端口对
	int RTP;
	int RTCP;
}tcp_info_t;

typedef struct _tagRTP_CLIENT_TRANSPORT
{
	HB_CHAR call_id[32]; //会话ID
	struct bufferevent *bev;
	HB_VOID *event_args;
	RTP_type_e type;      //RTP传输类型
	HB_S32 stream_start; //流媒体传输标识，0-不传输 1-开始传输
	HB_S32 delete_flag; //0-正常 ，1-不正常，需要删除
	HB_S32 send_Iframe_flag; //是否从I帧开始发送,1-不需要从I帧开始发送，0-需要从I帧开始发送
	long int session_id;   //RTSP会话ID
	HB_S32 rtp_fd;       //TCP的网络文件描述符
	HB_S32 now_seconds;
	HB_S32 lost_cnt;
	HB_S32 rtp_fd_video; //UDP的视频网络文件描述符
	HB_S32 rtp_fd_audio; //UDP的音频网络文件描述符
	tcp_info_t tcp;       //TCP信息
	udp_info_t udp_video; //视频的UDP信息
	udp_info_t udp_audio; //音频的UDP信息
}RTP_CLIENT_TRANSPORT_OBJ, *RTP_CLIENT_TRANSPORT_HANDLE;

typedef struct _tagSDP_INFO
{
	HB_S32			      get_sdp_state;		 //获取sdp状态，0-还未获取，1-获取中，2-获取完毕
	HB_CHAR			   m_video[64];			 //sdp中视频m字段信息，例：m=video 0 RTP/AVP 96
	HB_CHAR				m_audio[64];			 //sdp中音频m字段信息，例：m=audio 0 RTP/AVP 14
	HB_CHAR				a_rtpmap_video[64];
	HB_CHAR				a_rtpmap_audio[64];
	HB_CHAR				a_fmtp_video[512];
}SDP_INFO_OBJ, *SDP_INFO_HANDLE;

typedef struct _tagSTREAM_NODE
{
	struct event_base *work_base;
	struct event ev_timer;//定时器，定时判断当前节点内是否还有用户，若没有，则在定时器回调中释放
	HB_S32 dev_node_hash_value;
	HB_CHAR dev_id[128];//设备序列号
	HB_CHAR dev_ip[16];
	HB_U32 dev_port; //设备端口

	struct bufferevent *connect_hbserver_bev; //从hb流媒体取流的bev
	HB_U32 get_stream_from_flag;      //从何获取流标志，0-前端盒子 1-汉邦流媒体
	HB_U32 rtsp_play_flag;                   //rtsp视频播放关闭标识，0-还未开启 1-已经开启 2-已经关闭
	HB_U32 rtsp_audio_enable;  //音频使能
	HB_U32 send_socket_buffer_size; //网络发送缓冲区的大小
	HB_S32 connect_box_sock_fd; //连接前端盒子的套接字
	HB_U32 dev_chl_num; //设备通道号
	HB_U32 stream_type; //码流类型 0-主码流 1-子码流

	RTP_session_t rtp_session;       //RTP会话结构体
	SDP_INFO_OBJ sdp_info;

	list_t client_node_head;
	HB_U32 get_stream_thread_start_flag;//从汉邦服务器或盒子获取流的模块启动标识 0-未启动，1-已启动，2-启动后，出现异常退出
	HB_U32 rtp_client_node_send_data_thread_flag;//rtp节点发送线程退出标示，0-未启动，1-已启动, 2-启动后，出现异常退出
	HB_VOID *get_stream_from_source;//
	HB_S32	recv_stream_data_len; //已接收到的视频流的长度

	lf_queue stream_data_queue;

}STREAM_NODE_OBJ, *STREAM_NODE_HANDLE;

//每个哈希节点中的内容
typedef struct  _tagSTREAM_HEAD_OF_HASH_TABLE
{
	pthread_mutex_t stream_mutex;
	list_t stream_node_head;
}STREAM_HEAD_OF_HASH_TABLE_OBJ, *STREAM_HEAD_OF_HASH_TABLE_HANDLE;

//哈希表
typedef struct _tagSTREAM_HASH_TABLE
{
	HB_U32 hash_table_len; //hash节点长度
	STREAM_HEAD_OF_HASH_TABLE_HANDLE stream_node_head;
}STREAM_HASH_TABLE_OBJ, *STREAM_HASH_TABLE_HANDLE;


typedef struct _tagLIST_ARGS
{
//	HB_CHAR dev_sip_id[32];
//	HB_CHAR dev_id[128];
	HB_U32 dev_chl_num; //设备通道号
	HB_U32 stream_type; //码流类型 0-主码流 1-子码流
}LIST_ARGS_OBJ, *LIST_ARGS_HANDLE;


typedef struct _tagRTSP_CMD_ARGS
{
	HB_S32 hash_value;
	stpool_t *rtsp_pool;
	STREAM_NODE_HANDLE dev_node;
	RTP_CLIENT_TRANSPORT_HANDLE client_node;
	HB_CHAR reply_msg[2048];
	HB_S32    reply_msg_len;
	HB_S32    reply_code;
}RTSP_CMD_ARGS_OBJ, *RTSP_CMD_ARGS_HANDLE;

STREAM_HASH_TABLE_HANDLE DevHashTableCreate(HB_U32 table_len);
HB_VOID DevHashTableInit(STREAM_HASH_TABLE_HANDLE p_hash_table);
STREAM_NODE_HANDLE InsertNodeToDevHashTable(STREAM_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE p_sip_node);
STREAM_NODE_HANDLE FindDevFromDevHashTable(STREAM_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE sip_node);
HB_VOID DelNodeFromDevHashTable(STREAM_HASH_TABLE_HANDLE pHashTable, STREAM_NODE_HANDLE dev_node);
RTP_CLIENT_TRANSPORT_HANDLE FindClientNode(STREAM_NODE_HANDLE dev_node, HB_CHAR *call_id);
HB_VOID GetDevHashState(STREAM_HASH_TABLE_HANDLE pHashTable, HB_CHAR *hash_state_json);


#endif /* SRC_SERVER_HASH_AND_LIST_SIP_DEV_H_ */


