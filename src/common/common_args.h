/*
 * common_args.h
 *
 *  Created on: 2018年4月17日
 *      Author: root
 */

#ifndef INCLUDE_COMMON_COMMON_ARGS_H_
#define INCLUDE_COMMON_COMMON_ARGS_H_

#include "my_include.h"
#include "stpool/stpool.h"
#include "event.h"

typedef struct _tagSIP_DEV_ARGS
{
	HB_CHAR st_y[16]; //SSRC值
	HB_CHAR call_id[32]; //会话ID
	HB_CHAR	st_sip_dev_id[32]; //设备编号
	HB_CHAR	st_push_ip[16]; //视频流推送到的ip
	HB_S32 st_push_port; //视频流推送到的端口
	HB_CHAR st_dev_id[128];	//设备id
	HB_S32	st_dev_chnl; //设备通道号
	HB_S32	st_stream_type;//设备主子码流
	HB_CHAR	st_stram_server_ip[16]; //设备所在流媒体ip
	HB_S32	st_stream_server_port;	//设备所在流媒体端口
}SIP_DEV_ARGS_OBJ, *SIP_DEV_ARGS_HANDLE;



typedef struct _tagQUEUE_ARGS
{
	HB_S32   data_pre_buf_size;
	HB_S32   data_len;
	HB_CHAR  *data_buf;
}QUEUE_ARGS_OBJ, *QUEUE_ARGS_HANDLE;

typedef enum _CMD_TYPE
{
	PLAY=1,
	STOP
}CMD_TYPE;

#endif /* INCLUDE_COMMON_COMMON_ARGS_H_ */
