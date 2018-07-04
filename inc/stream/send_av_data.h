//////////////////////////////////////////////////////////////////////////////
// 版权所有，2009-2014，北京汉邦高科数字技术有限公司
// 本文件是未公开的，包含了汉邦高科的机密和专利内容
////////////////////////////////////////////////////////////////////////////////
// 文件名： send_av_data.h
// 作者：   root
// 版本：   1.0
// 日期：   2017-07-15
// 描述：   音视频数据发送程序头文件
// 历史记录：
////////////////////////////////////////////////////////////////////////////////

#ifndef _SEND_AV_DATA_H
#define _SEND_AV_DATA_H

//#include "rtsp/rtsp.h"
#include "stpool/stpool.h"
#include "stream_hash.h"

///////////////////////////////////////////////////////////////////////////////
// RTP方式发送数据时TCP头中的通道ID
///////////////////////////////////////////////////////////////////////////////
typedef enum _tagCHANNEL_ID
{
    VIDEO_RTP = 0, //RTP视频通道ID
    VIDEO_RTCP,	   //RTCP视频通道ID
    AUDIO_RTP,     //RTP音频通道ID
    AUDIO_RTCP     //RTCP音频通道ID
}CHANNEL_ID_E;

typedef struct _tagQUEUE_ARGS
{
	HB_S32   iDataPreBufSize;
	HB_S32   iDataLen;
	HB_CHAR  *pDataBuf;
}QUEUE_ARGS_OBJ, *QUEUE_ARGS_HANDLE;

////////////////////////////////////////////////////////////////////////////////
// 函数名：send_data_over_udp
// 描  述：通过UDP方式发送RTP数据
// 参  数：[in] fd       网络文件描述符
//         [in] rtp_peer RTP客户端的网络地址
//         [in] rtp_buf  RTP数据缓冲的地址
//         [in] rtp_size RTP数据的大小
// 返回值：成功返回发送的字节数，出错返回-1
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
int send_data_over_udp(HB_S32 fd, struct sockaddr *rtp_peer, char *rtp_buf, unsigned int rtp_size);


HB_VOID send_rtp_to_client_task_err_cb(struct sttask *ptask, long iReasons);


HB_VOID *send_rtp_to_client_thread(HB_VOID *param);
HB_VOID send_rtp_to_client_task(struct sttask *ptsk);

////////////////////////////////////////////////////////////////////////////////
// 函数名：send_video_data
// 描  述：发送音频数据
// 参  数：[in] rtp_info  RTP信息
//         [in] data_size 音频数据的大小
//         [in] data_ptr  音频数据的指针
//         [in] ts        RTP时间戳
// 返回值：成功返回0，失败返回-1
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
//int send_audio_data(rtp_info_t *rtp_info,
//	unsigned int data_size, char *data_ptr, unsigned int ts);

#endif

