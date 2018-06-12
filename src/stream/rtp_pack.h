/*
 * rtp_pack.h
 *
 *  Created on: Jun 5, 2018
 *      Author: root
 */

#ifndef SRC_SERVER_RTP_PACK_H_
#define SRC_SERVER_RTP_PACK_H_

#include "my_include.h"

#define RTP_VERSION       2
#define MAX_RTP_PACKET_SIZE (1448) //1500(max) - 20(IP header) - 20(TCP header) - 12or14(RTP header)

///////////////////////////////////////////////////////////////////////////////////////////
// NAL信息结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct _NAL_msg
{
	HB_BOOL eoFrame;       //是否是EOF
	unsigned char type; //NAL类型
	char *start;        //起始地址指针
	char *end;          //结束地址指针
	unsigned long size; //数据的大小
} NAL_msg;

///////////////////////////////////////////////////////////////////////////////////////////
// RTP头定义结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct _RTP_header
{
	/* byte 0 */
	#if (BYTE_ORDER == LITTLE_ENDIAN)
		unsigned char csrc_len:4;  //CSRC计数
		unsigned char extension:1; //扩展位
		unsigned char padding:1;   //填充位
		unsigned char version:2;   //版本号
	#elif (BYTE_ORDER == BIG_ENDIAN)
		unsigned char version:2;   //版本号
		unsigned char padding:1;   //填充位
	    unsigned char extension:1; //扩展位
	    unsigned char csrc_len:4;  //CSRC计数
    #else
    	#error Neither big nor little
    #endif
	/* byte 1 */
	#if (BYTE_ORDER == LITTLE_ENDIAN)
		unsigned char payload:7; //负载类型
		unsigned char marker:1;	 //最后一包数据的标识位
	#elif (BYTE_ORDER == BIG_ENDIAN)
		unsigned char marker:1;  //最后一包数据的标识位
		unsigned char payload:7; //负载类型
	#endif
	/* bytes 2, 3 */
	unsigned short seq_no;  //序列号
	/* bytes 4-7 */
	unsigned int timestamp; //时间戳
	/* bytes 8-11 */
	unsigned int ssrc;	    //SSRC，区分是哪一路RTP流
} RTP_header;

///////////////////////////////////////////////////////////////////////////////////////////
// RTP信息定义结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct __RTP_INFO
{
	NAL_msg    nal;    //NAL信息
	RTP_header rtp_hdr;//RTP头
	int hdr_len;       //RTP头的长度

	char *pRTP; //指向RTP包地址的指针
	char *start;//指向负载的起始地址的指针
	char *end;  //指向负载的结束地址的指针

	unsigned short seq_num;
	unsigned int timestamp; //时间戳
	unsigned int ssrc;	    //SSRC，区分是哪一路RTP流

	unsigned int s_bit;    //开始位
	unsigned int e_bit;    //结束位
	unsigned int FU_flag;  //是否是采用FU打包的标识
	unsigned int beginNAL; //是否是一个NAL的开始的标识
}rtp_info_t;


///////////////////////////////////////////////////////////////////////////////////////////
// RTP会话结构体
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct _RTP_session
{
	//RTP_transport transport;   //RTP传输信息
	rtp_info_t rtp_info_video; //视频的RTP信息
	rtp_info_t rtp_info_audio; //音频的RTP信息

	unsigned int start_seq;     //起始的序列号
	unsigned int start_rtptime; //起始的RTP时间戳

	//unsigned char started;      //是否开始音视频流传输的标识
	unsigned int  ssrc;         //SSRC标识，区分是哪一路RTP流

	struct timespec EAGAIN_start; //产生EAGAIN的起始时间
	struct timespec EAGAIN_end;   //产生EAGAIN的结束时间
}RTP_session_t;


HB_VOID rtp_info_init(rtp_info_t *rtp_info, HB_S32 payload);

#endif /* SRC_SERVER_RTP_PACK_H_ */
