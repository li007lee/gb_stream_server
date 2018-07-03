/*
 * sip_div.h
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#ifndef SRC_STREAM_HASH_H_
#define SRC_STREAM_HASH_H_

#include "my_include.h"
#include "simclist.h"

#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "event2/event_compat.h"

#include "lf_queue.h"
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
	struct sockaddr_in rtcp_peer; //RTCP客户端的网络地址
	port_pair cli_ports;      //客户端的端口对
	port_pair ser_ports;      //服务器端的端口对
	struct event *evUdpRtcpListenEvent;
	HB_S32 iUdpRtcpSockFd;//rtcp通信端口(接收和发送)
	HB_S32 iUdpVideoFd; //UDP的视频网络文件描述符
//	HB_S32 iUdpAudioFd; //UDP的音频网络文件描述符
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
//	struct event ev_time;
	HB_CHAR cCallId[32]; //会话ID
	struct bufferevent *pSendStreamBev;
	HB_HANDLE hEventArgs;
//	RTP_type_e enumRtpType;      //RTP传输类型
//	HB_S32 iStreamStartFlag; //流媒体传输标识，0-不传输 1-开始传输
	HB_S32 iDeleteFlag; //0-正常 ，1-不正常，需要删除
	HB_S32 iSendIframeFlag; //是否从I帧开始发送,1-不需要从I帧开始发送，0-需要从I帧开始发送
//	HB_S32 iSessionId;   //RTSP会话ID
//	HB_S32 iTcpSockFd;       //TCP的网络文件描述符
//	HB_S32 iNowSeconds;
//	HB_S32 iLostCnt;
//	tcp_info_t stTcpInfo;       //TCP信息
	udp_info_t stUdpVideoInfo; //视频的UDP信息
//	udp_info_t stUdpAudioInfo; //音频的UDP信息
	HB_U32 u32Ssrc;
}RTP_CLIENT_TRANSPORT_OBJ, *RTP_CLIENT_TRANSPORT_HANDLE;

typedef struct _tagSDP_INFO
{
	HB_S32 iGetSdpState;		 //获取sdp状态，0-还未获取，1-获取中，2-获取完毕
	HB_CHAR m_video[64];			 //sdp中视频m字段信息，例：m=video 0 RTP/AVP 96
	HB_CHAR m_audio[64];			 //sdp中音频m字段信息，例：m=audio 0 RTP/AVP 14
	HB_CHAR a_rtpmap_video[64];
	HB_CHAR a_rtpmap_audio[64];
	HB_CHAR a_fmtp_video[512];
}SDP_INFO_OBJ, *SDP_INFO_HANDLE;

typedef struct _tagSTREAM_NODE
{
	struct event_base *pWorkBase;
//	struct event stEvTimer;//定时器，定时判断当前节点内是否还有用户，若没有，则在定时器回调中释放
	HB_S32 iStreamNodeHashValue;
	HB_CHAR cDevId[128];//设备序列号
	HB_CHAR cDevIp[16];
	HB_S32 iDevPort; //设备端口
	HB_S32 iDevChnl; //设备通道号
	HB_S32 iStreamType; //码流类型 0-主码流 1-子码流

	struct bufferevent *pConnectHbServerBev; //从hb流媒体取流的bev
//	HB_U32 uGetStreamFromFlag;      //从何获取流标志，0-前端盒子 1-汉邦流媒体
	HB_U32 uRtspPlayFlag;                   //rtsp视频播放关闭标识，0-还未开启 1-已经开启 2-已经关闭
//	HB_U32 uRtspAudioEnable;  //音频使能
//	HB_U32 uSsendSocketBufferSize; //网络发送缓冲区的大小
	HB_S32 iConnectBoxSockFd; //连接前端盒子的套接字

	RTP_session_t stRtpSession;       //RTP会话结构体
	SDP_INFO_OBJ stSdpInfo;

	list_t listClientNodeHead;
	HB_U32 uGetStreamThreadStartFlag;//从汉邦服务器或盒子获取流的模块启动标识 0-未启动，1-已启动，2-启动后，出现异常退出
	HB_U32 uRtpClientNodeSendDataThreadFlag;//rtp节点发送线程退出标示，0-未启动，1-已启动, 2-启动后，出现异常退出
	HB_HANDLE hGetStreamFromSource;//从视频源接收流的缓冲区
	HB_S32	iRecvStreamDataLen; //已接收到的视频流的长度

	lf_queue queueStreamData;

}STREAM_NODE_OBJ, *STREAM_NODE_HANDLE;

//每个哈希节点中的内容
typedef struct  _tagSTREAM_HEAD_OF_HASH_TABLE
{
	pthread_mutex_t lockStreamNodeMutex;
	list_t listStreamNodeHead;
}STREAM_HEAD_OF_HASH_TABLE_OBJ, *STREAM_HEAD_OF_HASH_TABLE_HANDLE;

//哈希表
typedef struct _tagSTREAM_HASH_TABLE
{
	HB_U32 uStreamHashTableLen; //hash节点长度
	STREAM_HEAD_OF_HASH_TABLE_HANDLE pStreamHashNodeHead;
}STREAM_HASH_TABLE_OBJ, *STREAM_HASH_TABLE_HANDLE;


STREAM_HASH_TABLE_HANDLE stream_hash_table_create(HB_U32 uTableLen);
STREAM_NODE_HANDLE insert_node_to_stream_hash_table(STREAM_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE pSipNode);
STREAM_NODE_HANDLE find_node_from_stream_hash_table(STREAM_HASH_TABLE_HANDLE pHashTable, HB_U32 uHashValue, SIP_NODE_HANDLE pSipNode);
HB_VOID del_node_from_stream_hash_table(STREAM_HASH_TABLE_HANDLE pHashTable, STREAM_NODE_HANDLE pStreamNode);
RTP_CLIENT_TRANSPORT_HANDLE find_client_node(STREAM_NODE_HANDLE pStreamNode, HB_CHAR *pCallId);
HB_VOID get_stream_hash_state(STREAM_HASH_TABLE_HANDLE pHashTable, HB_CHAR *pHashStateJson);
HB_U32 get_stream_hash_value(STREAM_HASH_TABLE_HANDLE pHashTable, HB_CHAR *pKey);


#endif /* SRC_SERVER_HASH_AND_LIST_SIP_DEV_H_ */


