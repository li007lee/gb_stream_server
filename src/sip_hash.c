/*
 * sip_dev.c
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#include "sip_hash.h"

#include "common/common_args.h"
#include "common/hash_table.h"

static HB_S32 find_call_id(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}
	SIP_NODE_HANDLE sip_node = (SIP_NODE_HANDLE)el;
	HB_CHAR *p_call_id = (HB_CHAR *)key;
	if (!strcmp(sip_node->call_id, p_call_id))
	{
		printf("Find dev call id: [%s]\n", p_call_id);
		return 1;
	}

	return 0;
}


SIP_NODE_HANDLE InsertNodeToSipHashTable(SIP_HASH_TABLE_HANDLE p_sip_hash_table, SIP_DEV_ARGS_HANDLE p_sip_dev_info)
{
	printf("\nIIIIIIIIIIII  InsertNodeToSipHashTable dev_id=[%s], call_id=[%s], hash_table_len=[%d]\n", p_sip_dev_info->st_sip_dev_id, p_sip_dev_info->call_id, p_sip_hash_table->hash_table_len);
	HB_U32 mHashValue = pHashFunc(p_sip_dev_info->call_id) % p_sip_hash_table->hash_table_len;
	SIP_NODE_HANDLE sip_node = NULL;

	pthread_mutex_lock(&(p_sip_hash_table->sip_hash_node_head[mHashValue].dev_mutex));
	list_attributes_seeker(&(p_sip_hash_table->sip_hash_node_head[mHashValue].sip_node_head), find_call_id);
	sip_node = list_seek(&(p_sip_hash_table->sip_hash_node_head[mHashValue].sip_node_head), p_sip_dev_info->call_id);
	//当前哈希节点已经存在设备，此处查询当前设备是不是已经存在
	if(NULL == sip_node)
	{
		//当前申请的设备不存在（说明是新的设备）
		//如果当前节点不存在设备，直接插入连表（说明是新设备）
		sip_node = (SIP_NODE_HANDLE)calloc(1, sizeof(SIP_NODE_OBJ));
		strncpy(sip_node->sip_dev_id, p_sip_dev_info->st_sip_dev_id, sizeof(sip_node->sip_dev_id));
		strncpy(sip_node->dev_id, p_sip_dev_info->st_dev_id, sizeof(sip_node->dev_id));
		strncpy(sip_node->call_id, p_sip_dev_info->call_id, sizeof(sip_node->call_id));
		strncpy(sip_node->stream_server_ip, p_sip_dev_info->st_stram_server_ip, sizeof(sip_node->stream_server_ip));
		sip_node->stream_server_port = p_sip_dev_info->st_stream_server_port;
		sip_node->chnl = p_sip_dev_info->st_dev_chnl;
		sip_node->stream_type = p_sip_dev_info->st_stream_type;
		strncpy(sip_node->push_ip, p_sip_dev_info->st_push_ip, sizeof(sip_node->push_ip));
		sip_node->push_port = p_sip_dev_info->st_push_port;
		sip_node->sip_node_hash_value = mHashValue;
		strncpy(sip_node->ssrc, p_sip_dev_info->st_y, sizeof(sip_node->ssrc));
		list_append(&(p_sip_hash_table->sip_hash_node_head[mHashValue].sip_node_head), (HB_VOID*)sip_node);

	}
	pthread_mutex_unlock(&(p_sip_hash_table->sip_hash_node_head[mHashValue].dev_mutex));
	return sip_node;
}


SIP_NODE_HANDLE FindNodeFromSipHashTable(SIP_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE p_sip_dev_info)
{
	printf("\nFFFFFFFFFF  FindNodeFromSipHashTable call_id=[%s], hash_table_len=[%d]\n", p_sip_dev_info->call_id, pHashTable->hash_table_len);
	HB_U32 mHashValue = pHashFunc(p_sip_dev_info->call_id) % pHashTable->hash_table_len;
	printf("mHashValue=%u\n", mHashValue);

	SIP_NODE_HANDLE sip_node = NULL;

	pthread_mutex_lock(&(pHashTable->sip_hash_node_head[mHashValue].dev_mutex));

	list_attributes_seeker(&(pHashTable->sip_hash_node_head[mHashValue].sip_node_head), find_call_id);
	sip_node = list_seek(&(pHashTable->sip_hash_node_head[mHashValue].sip_node_head), p_sip_dev_info->call_id);

	//当前哈希节点已经存在设备，此处查询当前设备是不是已经存在
	if(NULL != sip_node)
	{
		printf("FOUND FOUND FOUND FOUND FOUND CALL ID : [%s]\n", sip_node->call_id);
	}
	else
	{
		//当前申请的设备不存在
		printf("do not found call id:[%s]!\n", p_sip_dev_info->call_id);
	}

	pthread_mutex_unlock(&(pHashTable->sip_hash_node_head[mHashValue].dev_mutex));
	return sip_node;
}


HB_VOID DelNodeFromSipHashTable(SIP_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE sip_node)
{
	printf("\nFFFFFFFFFF  FindNodeFromSipHashTable call_id=[%s], hash_table_len=[%d]\n", sip_node->call_id, pHashTable->hash_table_len);
	HB_U32 mHashValue = sip_node->sip_node_hash_value;
	printf("mHashValue=%u\n", mHashValue);

	pthread_mutex_lock(&(pHashTable->sip_hash_node_head[mHashValue].dev_mutex));
	list_delete(&(pHashTable->sip_hash_node_head[mHashValue].sip_node_head), sip_node);
	pthread_mutex_unlock(&(pHashTable->sip_hash_node_head[mHashValue].dev_mutex));
	return;
}



//获取哈希表的状态
HB_VOID GetSipHashState(SIP_HASH_TABLE_HANDLE pHashTable, HB_CHAR *hash_state_json)
{
    HB_S32 backet = 0;
    HB_S32 sum = 0;
    HB_U32 i=0;
    HB_U32 max_link=0;
    HB_S32 conflict_count = 0;
    HB_S32 hit_count = 0;
    HB_S32 mEntryCount = 0;
    double avg_link, backet_usage;

    for(i = 0; i < pHashTable->hash_table_len; i++)
    {
    	//pthread_rwlock_rdlock (&(pHashTable->hash_node[i].dev_rwlock));
    	mEntryCount = list_size(&(pHashTable->sip_hash_node_head[i].sip_node_head));
        if (mEntryCount > 0)
        {
            backet++;
            sum += mEntryCount;
            if (mEntryCount > max_link)
            {
                max_link = mEntryCount;
            }
            if (mEntryCount > 1)
            {
                conflict_count++;
            }
            else
            {
            	hit_count++;
            }
        }
        //pthread_rwlock_unlock (&(pHashTable->hash_node[i].dev_rwlock));
    }

    backet_usage = backet/1.0/pHashTable->hash_table_len * 100;
    if(0 == backet)
    {
    	avg_link = 0;
    }
    else
    {
    	avg_link = sum/1.0/backet;
    }

    sprintf(hash_state_json, "{\"CmdType\":\"hash_state\",\"HashTableLen\":%u,\"no_repetition_bucket_nums\":%d,"
    		"\"conflict_bucket_nums\":%d,\"longest_bucket_list\":%d,\"average_bucket_list_length\":%.2f,\"backet usage\":\"%.2f%%\"}",
			pHashTable->hash_table_len, hit_count, conflict_count, max_link, avg_link, backet_usage);
    printf("bucket_len = %d\n", pHashTable->hash_table_len);   ///哈希表的桶的个数
   /// printf("hash_call_count = %d/n", hash_call_count);	///建立hash表的字符串的个数
    printf("No repetition bucket nums = %d\n", hit_count);		///建立hash表的不重复的元素的个数
    printf("Conflict bucket nums = %d\n", conflict_count);		///存在冲突的桶的个数
    printf("longest bucket list = %d\n", max_link);			///最长的链的长度
    printf("average bucket list length = %.2f\n", avg_link);  ///链表的平均长度
    printf("backet usage = %.2f%%\n", backet_usage);			///hash table的桶的使用率
}


///initial hash table
HB_VOID SipHashTableInit(SIP_HASH_TABLE_HANDLE p_hash_table)
{
	HB_U32 i = 0;
	if ((NULL == p_hash_table) || (NULL == p_hash_table->sip_hash_node_head))
	{
		printf("\n######## HashTableInit: malloc p_hash_table or hash_node failed\n");
		return;
	}
	for (i = 0; i < p_hash_table->hash_table_len; i++)
	{
		pthread_mutex_init(&(p_hash_table->sip_hash_node_head[i].dev_mutex), NULL);
		list_init(&(p_hash_table->sip_hash_node_head[i].sip_node_head));
	}
}


//创建哈希结构
SIP_HASH_TABLE_HANDLE SipHashTableCreate(HB_U32 table_len)
{
	SIP_HASH_TABLE_HANDLE sip_hash_node = NULL;
	sip_hash_node = (SIP_HASH_TABLE_HANDLE) malloc(sizeof(SIP_HASH_TABLE_OBJ));
	if (NULL == sip_hash_node)
	{
		printf("\n#########  malloc hash table filled\n");
		return NULL;
	}
	sip_hash_node->hash_table_len = table_len;
	sip_hash_node->sip_hash_node_head = (SIP_HEAD_OF_HASH_TABLE_HANDLE) malloc(sizeof(SIP_HEAD_OF_HASH_TABLE_OBJ) * sip_hash_node->hash_table_len);
	if (NULL == sip_hash_node->sip_hash_node_head)
	{
		free(sip_hash_node);
		sip_hash_node = NULL;
		printf("\n#########  malloc hash node filled\n");
		return NULL;
	}
	return sip_hash_node;
}



