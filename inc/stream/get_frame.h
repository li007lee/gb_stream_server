/*
 * get_stream.h
 *
 *  Created on: 2016年11月3日
 *      Author: lijian
 */

#ifndef GET_STREAM_H_
#define GET_STREAM_H_

#include "stream_hash.h"

#define NET_BUFFER_LEN	(8*1024)

/******************byDataType数据类型_Start******************/
#define DATA_TYPE_NONE				(-1)
#define DATA_TYPE_VODFIND			0
#define DATA_TYPE_XMLFILE			1
#define DATA_TYPE_CANCEL			2
#define DATA_TYPE_VODINFO			3
#define DATA_TYPE_VODCMD			4
#define DATA_TYPE_VODDATA			5
#define DATA_TYPE_DOWNLOADFILE		6
#define DATA_TYPE_UPLOADFILE		7
#define DATA_TYPE_UP_FILE_DATA		8
#define DATA_TYPE_REAL_XML			9
#define DATA_TYPE_VOICE_CMD			10
#define DATA_TYPE_VOICE_DATA		11
#define DATA_TYPE_SMS_CMD			12//【目前用到】
#define DATA_TYPE_SMS_MEDIA			13//【目前用到】
#define DATA_TYPE_COMPLEX			14
#define DATA_TYPE_DIRECTAV_STATION	15
#define DATA_TYPE_DIRECTAV_FRAME 	16
//下两项为补充 2015-9-23
#define DATA_TYPE_DEVICE_VOD_INFO	17  //【回放DVR录像文件命令,包括请求,快放,慢放,暂停,停止】
#define DATA_TYPE_DEVICE_VOD_DATA	18  //【回放DVR录像文件数据】
/******************byDataType数据类型_End******************/

/******************byFrameType数据类型_Start******************/
#define   FRAMETYPE_BP			0	//BP帧
#define   FRAMETYPE_KEY			1	//关键帧
#define   FRAMETYPE_HEAD		2	//编码信息头,为二进制数据,hik,dh,hb设备直播连接时,应答完结束后，第一帧为此帧，HIK/DH/HB长度为36个字节，按4字节整型分布，依次为
/*	.视频宽度
    .视频高度
    .视频帧率，注意此值以毫秒为单位，通常为40(25帧/秒)
    .音频压缩方法,符合FFMPEG定义, CODEC_ID_PCM_ALAW:65543,如果是mp3，此值为86017
    .音频采样频率，默认是8000
    .音频采样宽度,默认是16
    .采样通道,默认为1
    .音频码流率，默认8000，无意义，仅作参考
    .视频编码方法,28为H264
    */
//其它设备类型会在连接请求时，通过xml信息传送，见下。
#define   FRAMETYPE_SPECIAL		3//特殊帧,手机可直接忽略此种帧
#define   FRAMETYPE_AUDIO		4//音频帧
#define   FRAMETYPE_FRAGMENT	5//FFMPEG文件点播时,一次发送30帧的片段帧，手机可暂时忽略此种类型
#define	  FRAMETYPE_END			6//vod结束标识，手机可暂时忽略此种类型
#define	  FRAMETYPE_FILE_DATA	7//文件上传/下载数据, 手机可暂时忽略此种类型
#define	  FRAMETYPE_FILE_END	8//文件结束帧，手机可暂时忽略此种类型
/******************byFrameType数据类型_End******************/

typedef struct
{
    HB_S32		iActLength;		//包实际长度
    HB_U8	byProtocolType;	//新增,协议类型,流媒体为0,一点通盒子为1,手机通讯时,此值固定为0
    HB_U8	byProtocolVer;	//新增,协议版本,目前固定为9,以后如果有升级,按1增加,可作为C/S端通讯版本匹配提示
    HB_U8	byDataType;		//数据类型,手机通讯时,DATA_TYPE_REAL_XML:9:交互命令,DATA_TYPE_SMS_CMD:10:云台控制命令,DATA_TYPE_SMS_MEDIA:13:流媒体数据
    HB_U8	byFrameType;	//FRAMETYPE_BP:0:视频非关键帧,FRAMETYPE_KEY:1:视频关键帧,FRAMETYPE_HEAD:2:文件头,FRAMETYPE_SPECIAL:3:特殊帧,收到此帧可直接忽略掉
							//FRAMETYPE_AUDIO:4:音频帧
    HB_S32		iTimeStampHigh;		//音/视频帧时间戳高位,目前保留
    HB_S32		iTimeStampLow;		//音/视频帧时间戳低位,目前保留
    HB_S32		iVodFilePercent;//VOD文件播放进度
    HB_S32		iVodCurFrameNo;//VOD文件当前帧,需要*2,因为最大为65535,视频文件最大可能为25*3600=90000
	HB_U8	byBlockHeadFlag;//包头标识,1为头,0为中间包
	HB_U8	byBlockEndFlag;//包尾标识,1为尾,0为中间包
	HB_U8	byReserved1;	//保留1
	HB_U8	byReserved2;	//保留2
	HB_CHAR	cBuffer[NET_BUFFER_LEN];
}NET_LAYER;

#define Net_LAYER_STRUCT_LEN	sizeof(NET_LAYER)
#define PACKAGE_EXTRA_LEN (Net_LAYER_STRUCT_LEN-NET_BUFFER_LEN)


typedef struct _DeviceInfo
{
	//int thread_off_flag; //线程退出标示 0表示没退出，1表示退出
	HB_CHAR m_acStreamIp[16];	//设备所在流媒体的ip地址
	HB_S32 m_iStreamPort;	//流媒体端口

	HB_CHAR m_acDeviceSn[128];//设备序列号
	HB_S32 m_nChannel;		//设备通道号
	HB_S32 m_nStreamType;	//码流类型（0主码流，1子码流, 4音频)

	HB_S32 m_iStreamSockfd;	//与流媒体通信的socket文件描述符
	HB_CHAR *m_pRecvBuff;		//用于接收流媒体数据的buff
	HB_CHAR *m_pRecvXmlData;  //用于存放命令通讯时，流媒体返回的结果数据
	HB_CHAR *m_pStreamData;  //用于存放BPI帧或音频帧的整包数据
	HB_S32 m_bActiveRecv; //接收流媒体数据的开关，为HB_FALSE时关闭接收

	HB_S32	m_iFrameLen;//单帧长度
	HB_S32		m_TimeStampHigh;		//音/视频帧时间戳高位
	HB_S32		m_TimeStampLow;		//音/视频帧时间戳低位
	HB_S32 m_iPreRecvLen;
	HB_S32 m_iPackageLen;//收到包的长度（包含头信息）
	pthread_t pthRecvThreadId;	//读取流的线程id

}DEVICEINFO,*DEVICEINFO_HANDLE;

#if 0
typedef struct _BoxDeviceInfo
{
	//int thread_off_flag; //线程退出标示 0表示没退出，1表示退出
	HB_CHAR box_ip[16];	//盒子的ip地址
	HB_S32 box_server_port;	//盒子服务端口
	HB_CHAR dev_id[128];//设备序列号
	HB_S32 dev_channel;		//设备通道号
	HB_S32 dev_stream_type;	//码流类型（0主码流，1子码流)
	HB_S32 box_sockfd;	//与盒子通信的socket文件描述符
	HB_S32 m_bActiveRecv; //接收流媒体数据的开关，为HB_FALSE时关闭接收
}BOX_DEVICE_INFO_OBJ,*BOX_DEVICE_INFO_HANDLE;
#endif


HB_S32 play_rtsp_video_from_hbserver(STREAM_NODE_HANDLE dev_node);
HB_S32 play_rtsp_video_from_box(STREAM_NODE_HANDLE dev_node);


#endif /* GET_STREAM_H_ */
