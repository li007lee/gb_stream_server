/*
 * sip_dev.c
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#include "sip_hash.h"

#include "common_args.h"
#include "hash_table.h"

static HB_S32 find_call_id(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}

	SIP_NODE_HANDLE pSipNode = (SIP_NODE_HANDLE)el;
	HB_CHAR *pCallId = (HB_CHAR *)key;
	if (!strcmp(pSipNode->cCallId, pCallId))
	{
		printf("Find dev call id: [%s]\n", pCallId);
		return 1;
	}

	return 0;
}


SIP_NODE_HANDLE insert_node_to_sip_hash_table(SIP_HASH_TABLE_HANDLE pSipHashTable, SIP_DEV_ARGS_HANDLE pSipDevInfo)
{
	printf("\nIIIIIIIIIIII  InsertNodeToSipHashTable dev_id=[%s], call_id=[%s], uHashTableLen=[%d]\n", pSipDevInfo->st_sip_dev_id, pSipDevInfo->call_id, pSipHashTable->uHashTableLen);
	HB_U32 uHashValue = pHashFunc(pSipDevInfo->call_id) % pSipHashTable->uHashTableLen;
	SIP_NODE_HANDLE pSipNode = NULL;

	pthread_mutex_lock(&(pSipHashTable->pSipHashNodeHead[uHashValue].lockSipNodeMutex));
	list_attributes_seeker(&(pSipHashTable->pSipHashNodeHead[uHashValue].listSipNodeHead), find_call_id);
	pSipNode = list_seek(&(pSipHashTable->pSipHashNodeHead[uHashValue].listSipNodeHead), pSipDevInfo->call_id);
	//当前哈希节点已经存在设备，此处查询当前设备是不是已经存在
	if(NULL == pSipNode)
	{
		//当前申请的设备不存在（说明是新的设备）
		//如果当前节点不存在设备，直接插入连表（说明是新设备）
		pSipNode = (SIP_NODE_HANDLE)calloc(1, sizeof(SIP_NODE_OBJ));
		strncpy(pSipNode->cSipDevSn, pSipDevInfo->st_sip_dev_id, sizeof(pSipNode->cSipDevSn));
		strncpy(pSipNode->cDevId, pSipDevInfo->st_dev_id, sizeof(pSipNode->cDevId));
		strncpy(pSipNode->cCallId, pSipDevInfo->call_id, sizeof(pSipNode->cCallId));
		strncpy(pSipNode->cStreamServerIp, pSipDevInfo->st_stram_server_ip, sizeof(pSipNode->cStreamServerIp));
		pSipNode->iStreamServerPort = pSipDevInfo->st_stream_server_port;
		pSipNode->iDevChnl = pSipDevInfo->st_dev_chnl;
		pSipNode->iStreamType = pSipDevInfo->st_stream_type;
		strncpy(pSipNode->cPushIp, pSipDevInfo->st_push_ip, sizeof(pSipNode->cPushIp));
		pSipNode->iPushPort = pSipDevInfo->st_push_port;
		pSipNode->iSipNodeHashValue = uHashValue;
		strncpy(pSipNode->cSsrc, pSipDevInfo->st_y, sizeof(pSipNode->cSsrc));
		list_append(&(pSipHashTable->pSipHashNodeHead[uHashValue].listSipNodeHead), (HB_VOID*)pSipNode);

	}
	pthread_mutex_unlock(&(pSipHashTable->pSipHashNodeHead[uHashValue].lockSipNodeMutex));
	return pSipNode;
}


SIP_NODE_HANDLE find_node_from_sip_hash_table(SIP_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE pSipDevInfo)
{
	printf("\nFFFFFFFFFF  FindNodeFromSipHashTable call_id=[%s], uHashTableLen=[%d]\n", pSipDevInfo->call_id, pHashTable->uHashTableLen);
	HB_U32 uHashValue = pHashFunc(pSipDevInfo->call_id) % pHashTable->uHashTableLen;
	printf("mHashValue=%u\n", uHashValue);

	SIP_NODE_HANDLE pSipNode = NULL;

	pthread_mutex_lock(&(pHashTable->pSipHashNodeHead[uHashValue].lockSipNodeMutex));

	list_attributes_seeker(&(pHashTable->pSipHashNodeHead[uHashValue].listSipNodeHead), find_call_id);
	pSipNode = list_seek(&(pHashTable->pSipHashNodeHead[uHashValue].listSipNodeHead), pSipDevInfo->call_id);

	//当前哈希节点已经存在设备，此处查询当前设备是不是已经存在
	if(NULL != pSipNode)
	{
		printf("FOUND FOUND FOUND FOUND FOUND CALL ID : [%s]\n", pSipNode->cCallId);
	}
	else
	{
		//当前申请的设备不存在
		printf("do not found call id:[%s]!\n", pSipDevInfo->call_id);
	}

	pthread_mutex_unlock(&(pHashTable->pSipHashNodeHead[uHashValue].lockSipNodeMutex));
	return pSipNode;
}


HB_VOID del_node_from_sip_hash_table(SIP_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE pSipNode)
{
	printf("\nFFFFFFFFFF  FindNodeFromSipHashTable call_id=[%s], uHashTableLen=[%d]\n", pSipNode->cCallId, pHashTable->uHashTableLen);
	HB_U32 mHashValue = pSipNode->iSipNodeHashValue;
	printf("mHashValue=%u\n", mHashValue);

	pthread_mutex_lock(&(pHashTable->pSipHashNodeHead[mHashValue].lockSipNodeMutex));
	list_delete(&(pHashTable->pSipHashNodeHead[mHashValue].listSipNodeHead), pSipNode);
	pthread_mutex_unlock(&(pHashTable->pSipHashNodeHead[mHashValue].lockSipNodeMutex));
	return;
}



//获取哈希表的状态
HB_VOID get_sip_hash_state(SIP_HASH_TABLE_HANDLE pHashTable, HB_CHAR *pHashStateJson)
{
    HB_S32 iBacket = 0;
    HB_S32 iSum = 0;
    HB_U32 i=0;
    HB_U32 uMaxLink=0;
    HB_S32 iConflictCount = 0;
    HB_S32 iHitCount = 0;
    HB_S32 iEntryCount = 0;
    double fAvgLink, fBacketUsage;

    for(i = 0; i < pHashTable->uHashTableLen; i++)
    {
    	//pthread_rwlock_rdlock (&(pHashTable->hash_node[i].dev_rwlock));
    	iEntryCount = list_size(&(pHashTable->pSipHashNodeHead[i].listSipNodeHead));
        if (iEntryCount > 0)
        {
        	iBacket++;
        	iSum += iEntryCount;
            if (iEntryCount > uMaxLink)
            {
            	uMaxLink = iEntryCount;
            }
            if (iEntryCount > 1)
            {
            	iConflictCount++;
            }
            else
            {
            	iHitCount++;
            }
        }
        //pthread_rwlock_unlock (&(pHashTable->hash_node[i].dev_rwlock));
    }

    fBacketUsage = iBacket/1.0/pHashTable->uHashTableLen * 100;
    if(0 == iBacket)
    {
    	fAvgLink = 0;
    }
    else
    {
    	fAvgLink = iSum/1.0/iBacket;
    }

    sprintf(pHashStateJson, "{\"CmdType\":\"hash_state\",\"HashTableLen\":%u,\"no_repetition_bucket_nums\":%d,"
    		"\"conflict_bucket_nums\":%d,\"longest_bucket_list\":%d,\"average_bucket_list_length\":%.2f,\"backet usage\":\"%.2f%%\"}",
			pHashTable->uHashTableLen, iHitCount, iConflictCount, uMaxLink, fAvgLink, fBacketUsage);
    printf("bucket_len = %d\n", pHashTable->uHashTableLen);   ///哈希表的桶的个数
   /// printf("hash_call_count = %d/n", hash_call_count);	///建立hash表的字符串的个数
    printf("No repetition bucket nums = %d\n", iHitCount);		///建立hash表的不重复的元素的个数
    printf("Conflict bucket nums = %d\n", iConflictCount);		///存在冲突的桶的个数
    printf("longest bucket list = %d\n", uMaxLink);			///最长的链的长度
    printf("average bucket list length = %.2f\n", fAvgLink);  ///链表的平均长度
    printf("backet usage = %.2f%%\n", fBacketUsage);			///hash table的桶的使用率
}


///initial hash table
static HB_VOID sip_hash_table_init(SIP_HASH_TABLE_HANDLE hHashTable)
{
	HB_U32 i = 0;
	if ((NULL == hHashTable) || (NULL == hHashTable->pSipHashNodeHead))
	{
		printf("\n######## HashTableInit: malloc p_hash_table or hash_node failed\n");
		return;
	}
	for (i = 0; i < hHashTable->uHashTableLen; i++)
	{
		pthread_mutex_init(&(hHashTable->pSipHashNodeHead[i].lockSipNodeMutex), NULL);
		list_init(&(hHashTable->pSipHashNodeHead[i].listSipNodeHead));
	}
}


//创建哈希结构并初始化
SIP_HASH_TABLE_HANDLE sip_hash_table_create(HB_U32 uTableLen)
{
	SIP_HASH_TABLE_HANDLE pSipHashNode = NULL;
	pSipHashNode = (SIP_HASH_TABLE_HANDLE)malloc(sizeof(SIP_HASH_TABLE_OBJ));
	if (NULL == pSipHashNode)
	{
		printf("\n#########  malloc hash table filled\n");
		return NULL;
	}
	pSipHashNode->uHashTableLen = uTableLen;
	pSipHashNode->pSipHashNodeHead = (SIP_HEAD_OF_HASH_TABLE_HANDLE) malloc(sizeof(SIP_HEAD_OF_HASH_TABLE_OBJ) * pSipHashNode->uHashTableLen);
	if (NULL == pSipHashNode->pSipHashNodeHead)
	{
		free(pSipHashNode);
		pSipHashNode = NULL;
		printf("\n#########  malloc hash node filled\n");
		return NULL;
	}

	sip_hash_table_init(pSipHashNode);

	return pSipHashNode;
}



