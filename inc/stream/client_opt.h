/*
 * client_opt.h
 *
 *  Created on: Jul 4, 2018
 *      Author: root
 */

#ifndef SRC_STREAM_CLIENT_OPT_H_
#define SRC_STREAM_CLIENT_OPT_H_

#include "my_include.h"

#include "simclist.h"

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
	struct event *evUdpSendRtcpEvent;
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

HB_VOID disable_client_rtp_list_bev(list_t *listClientNodeHead);
HB_VOID del_one_client_node(list_t *plistClientNodeHead, RTP_CLIENT_TRANSPORT_HANDLE pClientNode);
HB_VOID destory_client_list(list_t *plistClientNodeHead);

#endif /* SRC_STREAM_CLIENT_OPT_H_ */
