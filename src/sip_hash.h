/*
 * sip_div.h
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#ifndef SRC_SIP_HASH_H_
#define SRC_SIP_HASH_H_

#include "my_include.h"
#include "simclist.h"
#include "common_args.h"

typedef struct _tagSIP_NODE
{
	CMD_TYPE enumCmdType;
	HB_CHAR cSsrc[32]; //sip sdp 中的y值
	HB_CHAR	cSipDevSn[32]; //设备编号
	HB_CHAR cDevId[128];//设备序列号
	HB_CHAR cCallId[32]; //会话ID
	HB_S32 iDevChnl;	//设备通道号
	HB_CHAR	cPushIp[16]; //视频流推送到的ip
	HB_S32 iPushPort; //视频流推送到的端口
	HB_S32 iStreamType; //主子码流
	HB_CHAR	cStreamServerIp[16]; //设备所在流媒体ip
	HB_S32	iStreamServerPort;	//设备所在流媒体端口
	HB_S32 iSipNodeHashValue;
}SIP_NODE_OBJ, *SIP_NODE_HANDLE;


//每个哈希节点中的内容
typedef struct  _tagSIP_HEAD_OF_HASH_TABLE
{
	pthread_mutex_t lockSipNodeMutex;
	list_t listSipNodeHead;
}SIP_HEAD_OF_HASH_TABLE_OBJ, *SIP_HEAD_OF_HASH_TABLE_HANDLE;

//哈希表
typedef struct _tagSIP_HASH_TABLE
{
	HB_U32 uHashTableLen; //hash节点长度
	SIP_HEAD_OF_HASH_TABLE_HANDLE pSipHashNodeHead;
}SIP_HASH_TABLE_OBJ, *SIP_HASH_TABLE_HANDLE;

//创建哈希结构
SIP_HASH_TABLE_HANDLE sip_hash_table_create(HB_U32 uTableLen);
SIP_NODE_HANDLE insert_node_to_sip_hash_table(SIP_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE pSipDevInfo);
SIP_NODE_HANDLE find_node_from_sip_hash_table(SIP_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE pSipDevInfo);
//获取哈希表的状态
HB_VOID get_sip_hash_state(SIP_HASH_TABLE_HANDLE pHashTable, HB_CHAR *pHashStateJson);
HB_VOID del_node_from_sip_hash_table(SIP_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE pSipNode);

#endif /* SRC_SIP_HASH_H_ */
