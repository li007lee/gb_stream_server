/*
 * common_args.h
 *
 *  Created on: Jul 3, 2018
 *      Author: root
 */

#ifndef INC_COMMON_COMMON_ARGS_H_
#define INC_COMMON_COMMON_ARGS_H_

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

#endif /* INC_COMMON_COMMON_ARGS_H_ */
