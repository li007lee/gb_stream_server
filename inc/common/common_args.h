/*
 * common_args.h
 *
 *  Created on: Jul 3, 2018
 *      Author: root
 */

#ifndef INC_COMMON_COMMON_ARGS_H_
#define INC_COMMON_COMMON_ARGS_H_

#include "server_config.h"

typedef enum _CMD_TYPE
{
	PLAY=1,
	STOP
}CMD_TYPE;

typedef struct _SIP_STREAM_MSG_ARGS
{
	CMD_TYPE enumCmdType;
	HB_CHAR cCallId[32];
}SIP_STREAM_MSG_ARGS_OBJ, *SIP_STREAM_MSG_ARGS_HANDLE;


typedef struct _GLOBLE_ARGS
{
	HB_S32	iGbListenPort;	//当前服务器监听端口
	HB_S32	iUseRtcpFlag; //是否使用rtcp 1 使用, 0不使用
	HB_CHAR cLocalIp[16];
	HB_CHAR	cNetworkCardName[32];
	HB_S32	 iMaxConnections; //最大用户连接数，视频并发数

#ifdef RECV_STREAM_FROM_BOX
	HB_CHAR cBoxIp[16];
	HB_S32	 iBoxPort;
#endif

}GLOBLE_ARGS_OBJ;

extern GLOBLE_ARGS_OBJ glGlobleArgs;

#endif /* INC_COMMON_COMMON_ARGS_H_ */
