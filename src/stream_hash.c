/*
 * sip_dev.c
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#include "hash_table.h"
#include "stream_hash.h"

extern struct event_base *pEventBase;

//根据sip的设备id,通道，主子码流查找设备
static HB_S32 find_dev_id_chnl_stream_key(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)el;
	SIP_NODE_HANDLE pSipNode = (SIP_NODE_HANDLE)key;
	if ((!strcmp(pStreamNode->cDevId, pSipNode->cDevId)) && (pStreamNode->iDevChnl == pSipNode->iDevChnl) && (pStreamNode->iStreamType == pSipNode->iStreamType))
	{
		printf("Find dev id key: [%s] chnl[%d] stream[%d]\n", pSipNode->cDevId, pSipNode->iDevChnl, pSipNode->iStreamType);
		return 1;
	}

	return 0;
}


//创建设备节点， 返回创建好的新节点
static STREAM_NODE_HANDLE create_stream_node(STREAM_HASH_TABLE_HANDLE pHashTable, HB_U32 uHashValue, SIP_NODE_HANDLE pSipNode)
{
	STREAM_NODE_HANDLE pStreamNode = (STREAM_NODE_HANDLE)calloc(1, sizeof(STREAM_NODE_OBJ));
	pStreamNode->iStreamNodeHashValue = uHashValue;
	strncpy(pStreamNode->cDevId, pSipNode->cDevId, sizeof(pStreamNode->cDevId));
	strncpy(pStreamNode->cDevIp, pSipNode->cStreamSourceIp, sizeof(pStreamNode->cDevIp));
	pStreamNode->iDevPort = pSipNode->iStreamSourcePort;
	rtp_info_init(&(pStreamNode->stRtpSession.rtp_info_video), 96);
	pStreamNode->pWorkBase = pEventBase;

	list_init(&(pStreamNode->listClientNodeHead));

	return pStreamNode;
}



STREAM_NODE_HANDLE insert_node_to_stream_hash_table(STREAM_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE pSipNode)
{
	TRACE_YELLOW("\nIIIIIIIIIIII  insert_node_to_stream_hash_table cDevId=[%s], cCallId=[%s], uStreamHashTableLen=[%d]\n", pSipNode->cSipDevSn, pSipNode->cCallId, pHashTable->uStreamHashTableLen);
	HB_U32 uHashValue = pHashFunc(pSipNode->cSipDevSn) % pHashTable->uStreamHashTableLen;
	TRACE_YELLOW("uHashValue=%u\n", uHashValue);
	STREAM_NODE_HANDLE pStreamNode = NULL;

	pthread_mutex_lock(&(pHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	list_attributes_seeker(&(pHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), find_dev_id_chnl_stream_key);
	pStreamNode = list_seek(&(pHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID *)pSipNode);
	if(NULL == pStreamNode)
	{
		//当前申请的设备不存在（说明是新的设备）
		//如果当前节点不存在设备，直接插入连表（说明是新设备）
		//创建设备节点
		pStreamNode = create_stream_node(pHashTable, uHashValue, pSipNode);
		list_append(&(pHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), (HB_VOID*)pStreamNode);
		printf("$$$$$$$$$$$$$$$$$$$$$$$don't have pStreamNode\n");
	}
	else
	{
		printf("$$$$$$$$$$$$$$$$$$$$$$$have pStreamNode\n");
	}

	pthread_mutex_unlock(&(pHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	return pStreamNode;
}


STREAM_NODE_HANDLE find_node_from_stream_hash_table(STREAM_HASH_TABLE_HANDLE pHashTable, HB_U32 uHashValue, SIP_NODE_HANDLE pSipNode)
{
	TRACE_YELLOW("\nFFFFFFFFFF  find_node_from_stream_hash_table cDevId=[%s], uStreamHashTableLen=[%d]\n", pSipNode->cDevId, pHashTable->uStreamHashTableLen);
	list_attributes_seeker(&(pHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), find_dev_id_chnl_stream_key);
	STREAM_NODE_HANDLE stream_node = list_seek(&(pHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), pSipNode);
	if(NULL == stream_node)
	{
		printf("do not found dev id:[%s]!\n", pSipNode->cDevId);
	}
	return stream_node;
}



HB_VOID del_node_from_stream_hash_table(STREAM_HASH_TABLE_HANDLE pHashTable, STREAM_NODE_HANDLE pStreamNode)
{
	HB_U32 uHashValue = pStreamNode->iStreamNodeHashValue;
	printf("\nDDDDDDDDDD  DelNodeFromDevHashTable cDevId=[%s], uStreamHashTableLen=[%d]\n", pStreamNode->cDevId, pHashTable->uStreamHashTableLen);

//	pthread_mutex_lock(&(pHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	list_delete(&(pHashTable->pStreamHashNodeHead[uHashValue].listStreamNodeHead), pStreamNode);
//	pthread_mutex_unlock(&(pHashTable->pStreamHashNodeHead[uHashValue].lockStreamNodeMutex));
	return;
}


//获取哈希表的状态
HB_VOID get_stream_hash_state(STREAM_HASH_TABLE_HANDLE pHashTable, HB_CHAR *pHashStateJson)
{
    HB_S32 iBacket = 0;
    HB_S32 iSum = 0;
    HB_U32 i=0;
    HB_U32 uMaxLink=0;
    HB_S32 iConflictCount = 0;
    HB_S32 iHitCount = 0;
    HB_S32 iEntryCount = 0;
    double fAvgLink, fBacketUsage;

    for(i = 0; i < pHashTable->uStreamHashTableLen; i++)
    {
    	//pthread_rwlock_rdlock (&(pHashTable->hash_node[i].dev_rwlock));
    	iEntryCount = list_size(&(pHashTable->pStreamHashNodeHead[i].listStreamNodeHead));
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

    fBacketUsage = iBacket/1.0/pHashTable->uStreamHashTableLen * 100;
    if(0 == iBacket)
    {
    	fAvgLink = 0;
    }
    else
    {
    	fAvgLink = iSum/1.0/iBacket;
    }

    sprintf(pHashStateJson, "{\"CmdType\":\"hash_state\",\"HashTableLen\":%u,\"no_repetition_bucket_nums\":%d,"
    		"\"conflict_bucket_nums\":%d,\"longest_bucket_list\":%d,\"average_bucket_list_length\":%.2f,\"iBacket usage\":\"%.2f%%\"}",
			pHashTable->uStreamHashTableLen, iHitCount, iConflictCount, uMaxLink, fAvgLink, fBacketUsage);
    printf("bucket_len = %d\n", pHashTable->uStreamHashTableLen);   ///哈希表的桶的个数
   /// printf("hash_call_count = %d/n", hash_call_count);	///建立hash表的字符串的个数
    printf("No repetition bucket nums = %d\n", iHitCount);		///建立hash表的不重复的元素的个数
    printf("Conflict bucket nums = %d\n", iConflictCount);		///存在冲突的桶的个数
    printf("longest bucket list = %d\n", uMaxLink);			///最长的链的长度
    printf("average bucket list length = %.2f\n", fAvgLink);  ///链表的平均长度
    printf("iBacket usage = %.2f%%\n", fBacketUsage);			///hash table的桶的使用率
}


///initial hash table
static HB_VOID stream_hash_table_init(STREAM_HASH_TABLE_HANDLE pHashTable)
{
	HB_U32 i = 0;
	if ((NULL == pHashTable) || (NULL == pHashTable->pStreamHashNodeHead))
	{
		printf("\n######## HashTableInit: malloc p_hash_table or hash_node failed\n");
		return;
	}
	for (i = 0; i < pHashTable->uStreamHashTableLen; i++)
	{
		pthread_mutex_init(&(pHashTable->pStreamHashNodeHead[i].lockStreamNodeMutex), NULL);
		list_init(&(pHashTable->pStreamHashNodeHead[i].listStreamNodeHead));
	}
}


//创建哈希结构并初始化
STREAM_HASH_TABLE_HANDLE stream_hash_table_create(HB_U32 uTableLen)
{
	STREAM_HASH_TABLE_HANDLE pStreamHashNode = NULL;
	pStreamHashNode = (STREAM_HASH_TABLE_HANDLE)calloc(1, sizeof(STREAM_HASH_TABLE_OBJ));
	if (NULL == pStreamHashNode)
	{
		printf("\n#########  malloc hash table filled\n");
		return NULL;
	}
	pStreamHashNode->uStreamHashTableLen = uTableLen;
	pStreamHashNode->pStreamHashNodeHead = (STREAM_HEAD_OF_HASH_TABLE_HANDLE)calloc(1, sizeof(STREAM_HEAD_OF_HASH_TABLE_OBJ) * pStreamHashNode->uStreamHashTableLen);
	if (NULL == pStreamHashNode->pStreamHashNodeHead)
	{
		free(pStreamHashNode);
		pStreamHashNode = NULL;
		printf("\n#########  malloc hash node filled\n");
		return NULL;
	}

	stream_hash_table_init(pStreamHashNode);
	return pStreamHashNode;
}



//根据call_id查找客户节点
static HB_S32 find_client_node_key(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}
	RTP_CLIENT_TRANSPORT_HANDLE pClientNode = (RTP_CLIENT_TRANSPORT_HANDLE)el;
	HB_CHAR *pCallId = (HB_CHAR *)key;
	if (!strcmp(pClientNode->cCallId, pCallId))
	{
		printf("Find client call id key: [%s]\n", pCallId);
		return 1;
	}

	return 0;
}


RTP_CLIENT_TRANSPORT_HANDLE find_client_node(STREAM_NODE_HANDLE pStreamNode, HB_CHAR *pCallId)
{
	list_attributes_seeker(&(pStreamNode->listClientNodeHead), find_client_node_key);
	RTP_CLIENT_TRANSPORT_HANDLE pClientNode = list_seek(&(pStreamNode->listClientNodeHead), pCallId);
	if(NULL != pClientNode)
	{
		printf("Found client node !\n");
		return pClientNode;
	}

	printf("not found client!\n");
	return NULL;
}

HB_U32 get_stream_hash_value(STREAM_HASH_TABLE_HANDLE pHashTable, HB_CHAR *pKey)
{
	return pHashFunc(pKey) % pHashTable->uStreamHashTableLen;
}





