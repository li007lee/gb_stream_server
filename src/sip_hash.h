/*
 * sip_div.h
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#ifndef SRC_SIP_HASH_H_
#define SRC_SIP_HASH_H_

#include "my_include.h"
#include "common/simclist.h"
#include "common/common_args.h"

typedef struct _tagSIP_NODE
{
	CMD_TYPE cmd_type;
	HB_CHAR ssrc[32]; //sip sdp 中的y值
	HB_CHAR	sip_dev_id[32]; //设备编号
	HB_CHAR dev_id[128];//设备序列号
	HB_CHAR call_id[32]; //会话ID
	HB_S32 chnl;	//设备通道号
	HB_CHAR	push_ip[16]; //视频流推送到的ip
	HB_S32 push_port; //视频流推送到的端口
	HB_S32 stream_type; //主子码流
	HB_CHAR	stream_client_ip[16]; //设备所在流媒体ip
	HB_S32	stream_client_port;	//设备所在流媒体端口
	HB_S32 sip_node_hash_value;
}SIP_NODE_OBJ, *SIP_NODE_HANDLE;


//每个哈希节点中的内容
typedef struct  _tagSIP_HEAD_OF_HASH_TABLE
{
	pthread_mutex_t dev_mutex;
	list_t sip_node_head;
}SIP_HEAD_OF_HASH_TABLE_OBJ, *SIP_HEAD_OF_HASH_TABLE_HANDLE;

//哈希表
typedef struct _tagSIP_HASH_TABLE
{
	HB_U32 hash_table_len; //hash节点长度
	SIP_HEAD_OF_HASH_TABLE_HANDLE sip_hash_node_head;
}SIP_HASH_TABLE_OBJ, *SIP_HASH_TABLE_HANDLE;

//创建哈希结构
SIP_HASH_TABLE_HANDLE SipHashTableCreate(HB_U32 table_len);
///initial hash table
HB_VOID SipHashTableInit(SIP_HASH_TABLE_HANDLE p_hash_table);
SIP_NODE_HANDLE InsertNodeToSipHashTable(SIP_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE p_sip_dev_info);
SIP_NODE_HANDLE FindNodeFromSipHashTable(SIP_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE p_sip_dev_info);
//获取哈希表的状态
HB_VOID GetSipHashState(SIP_HASH_TABLE_HANDLE pHashTable, HB_CHAR *hash_state_json);
HB_VOID DelNodeFromSipHashTable(SIP_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE sip_node);

#endif /* SRC_SIP_HASH_H_ */
