/*
 * sip_dev.c
 *
 *  Created on: Jun 1, 2018
 *      Author: root
 */

#include "stpool/stpool.h"
#include "common/common_args.h"
#include "common/hash_table.h"
#include "dev_hash.h"

extern struct event_base *pEventBase;
extern stpool_t *gb_thread_pool;
extern STREAM_HASH_TABLE_HANDLE stream_hash_table;

struct timeval tv = {USER_TIMEOUT, 0};


static void timeout_cb(evutil_socket_t fd, short events, void *arg)
{
	DEV_NODE_HANDLE dev_node = (DEV_NODE_HANDLE)arg;
	HB_S32 hash_value = dev_node->dev_node_hash_value;
	pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
//	HB_S32 stream_node_num = list_size(&(dev_node->streaming_node_head));


	pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
	printf("curtain time : %ld.\n", time(NULL));
}


//根据sip的设备id查找设备
static HB_S32 find_dev_id_key(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}
	DEV_NODE_HANDLE dev_node = (DEV_NODE_HANDLE)el;
	HB_CHAR *p_dev_id = (HB_CHAR *)key;
	//printf("Gateway ip : [%s]\n", (HB_CHAR *)key);
	if (!strcmp(dev_node->dev_id, p_dev_id))
	{
		printf("Find dev id key: [%s]\n", p_dev_id);
		return 1;
	}

	return 0;
}

//根据sip的设备id,通道，主子码流查找设备
static HB_S32 find_dev_id_chnl_stream_key(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}
	DEV_NODE_HANDLE dev_node = (DEV_NODE_HANDLE)el;
	SIP_NODE_HANDLE p_sip_node = (HB_CHAR *)key;
	//printf("Gateway ip : [%s]\n", (HB_CHAR *)key);
	if ((!strcmp(dev_node->dev_id, p_sip_node->dev_id)) && (dev_node->dev_chl_num == p_sip_node->chnl) && (dev_node->stream_type == p_sip_node->stream_type))
	{
		printf("Find dev id key: [%s] chnl[%d] stream[%d]\n", p_sip_node->dev_id, p_sip_node->chnl, p_sip_node->stream_type);
		return 1;
	}

	return 0;
}


//创建设备节点， 返回创建好的新节点
static DEV_NODE_HANDLE create_dev_node(STREAM_HASH_TABLE_HANDLE pHashTable, HB_U32 mHashValue, SIP_DEV_ARGS_HANDLE p_sip_dev_info)
{
	DEV_NODE_HANDLE dev_node = (DEV_NODE_HANDLE)calloc(1, sizeof(DEV_NODE_OBJ));
	dev_node->dev_node_hash_value = mHashValue;
	strncpy(dev_node->dev_id, p_sip_dev_info->st_dev_id, sizeof(dev_node->dev_id));
	strncpy(dev_node->dev_ip, p_sip_dev_info->st_stram_server_ip, sizeof(dev_node->dev_ip));
	dev_node->dev_port = p_sip_dev_info->st_stream_server_port;
	rtp_info_init(&(dev_node->rtp_session.rtp_info_video), 96);

//	strncpy(dev_node->dev_id, "12345", sizeof(dev_node->dev_id));
//	strncpy(dev_node->dev_ip, "192.168.118.2", sizeof(dev_node->dev_ip));
//	dev_node->dev_port = 600;

	list_init(&(dev_node->client_node_head));

	event_assign(&dev_node->ev_timer, pEventBase, -1, EV_PERSIST, timeout_cb, (void*)dev_node);
	event_add(&dev_node->ev_timer, &tv);

	return dev_node;
}



DEV_NODE_HANDLE InsertNodeToDevHashTable(STREAM_HASH_TABLE_HANDLE pHashTable, SIP_DEV_ARGS_HANDLE p_sip_dev_info)
{
	TRACE_YELLOW("\nIIIIIIIIIIII  InsertNodeToDevHashTable dev_id=[%s], call_id=[%s], hash_table_len=[%d]\n", p_sip_dev_info->st_sip_dev_id, p_sip_dev_info->call_id, pHashTable->hash_table_len);
	HB_U32 mHashValue = pHashFunc(p_sip_dev_info->st_sip_dev_id) % pHashTable->hash_table_len;
	TRACE_YELLOW("mHashValue=%u\n", mHashValue);
	DEV_NODE_HANDLE dev_node = NULL;

	pthread_mutex_lock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));
	if (list_size(&(pHashTable->stream_node_head[mHashValue].dev_node_head)) < 1)
	{
		//如果当前节点不存在设备，直接插入连表（说明是新设备）
		//创建设备节点
		DEV_NODE_HANDLE dev_node = create_dev_node(pHashTable, mHashValue, p_sip_dev_info);
		list_append(&(pHashTable->stream_node_head[mHashValue].dev_node_head), (HB_VOID*)dev_node);
	}
	else
	{
		list_attributes_seeker(&(pHashTable->stream_node_head[mHashValue].dev_node_head), find_dev_id_key);
		DEV_NODE_HANDLE dev_node = list_seek(&(pHashTable->stream_node_head[mHashValue].dev_node_head), p_sip_dev_info->st_dev_id);

		//当前哈希节点已经存在设备，此处查询当前设备是不是已经存在
		if(NULL == dev_node)
		{
			//当前申请的设备不存在（说明是新的设备）
			//如果当前节点不存在设备，直接插入连表（说明是新设备）
			//创建设备节点
			DEV_NODE_HANDLE dev_node = create_dev_node(pHashTable, mHashValue, p_sip_dev_info);
			list_append(&(pHashTable->stream_node_head[mHashValue].dev_node_head), (HB_VOID*)dev_node);
		}
	}

	pthread_mutex_unlock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));
	return dev_node;
}


GET_STREAM_ARGS_HANDLE FindDevFromDevHashTable(STREAM_HASH_TABLE_HANDLE pHashTable, SIP_NODE_HANDLE sip_node)
{
	TRACE_YELLOW("\nFFFFFFFFFF  FindNodeFromDevHashTable dev_id=[%s], hash_table_len=[%d]\n", sip_node->dev_id, pHashTable->hash_table_len);
	HB_U32 mHashValue = pHashFunc(sip_node->sip_dev_id) % pHashTable->hash_table_len;
	TRACE_YELLOW("mHashValue=%u\n", mHashValue);

	GET_STREAM_ARGS_HANDLE p_common_args = (GET_STREAM_ARGS_HANDLE)calloc(1, sizeof(GET_STREAM_ARGS_OBJ));

	pthread_mutex_lock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));
	if (list_size(&(pHashTable->stream_node_head[mHashValue].dev_node_head)) < 1)
	{
		//设备不存在
		printf("no device!\n");
//		pthread_mutex_unlock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));
//		return NULL;
	}
	else
	{
		list_attributes_seeker(&(pHashTable->stream_node_head[mHashValue].dev_node_head), find_dev_id_chnl_stream_key);
		DEV_NODE_HANDLE dev_node = list_seek(&(pHashTable->stream_node_head[mHashValue].dev_node_head), sip_node);

		//当前哈希节点已经存在设备，此处查询当前设备是不是已经存在
		if(NULL != dev_node)
		{

			pthread_mutex_unlock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));

			p_common_args->dev_node = dev_node;
			p_common_args->work_base = pEventBase;
			p_common_args->gb_thread_pool = gb_thread_pool;

			return p_common_args;
		}
		else
		{
			//当前申请的设备不存在
			printf("do not found dev id:[%s]!\n", sip_node->dev_id);
//			pthread_mutex_unlock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));
//			return NULL;
		}
	}

	pthread_mutex_unlock(&(pHashTable->stream_node_head[mHashValue].dev_mutex));
	return NULL;
}


//获取哈希表的状态
HB_VOID GetDevHashState(STREAM_HASH_TABLE_HANDLE pHashTable, HB_CHAR *hash_state_json)
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
    	mEntryCount = list_size(&(pHashTable->stream_node_head[i].dev_node_head));
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
HB_VOID DevHashTableInit(STREAM_HASH_TABLE_HANDLE p_hash_table)
{
	HB_U32 i = 0;
	if ((NULL == p_hash_table) || (NULL == p_hash_table->stream_node_head))
	{
		printf("\n######## HashTableInit: malloc p_hash_table or hash_node failed\n");
		return;
	}
	for (i = 0; i < p_hash_table->hash_table_len; i++)
	{
		pthread_mutex_init(&(p_hash_table->stream_node_head[i].dev_mutex), NULL);
		list_init(&(p_hash_table->stream_node_head[i].dev_node_head));
	}
}


//创建哈希结构
STREAM_HASH_TABLE_HANDLE DevHashTableCreate(HB_U32 table_len)
{
	STREAM_HASH_TABLE_HANDLE dev_hash_node = NULL;
	dev_hash_node = (STREAM_HASH_TABLE_HANDLE)calloc(1, sizeof(STREAM_HASH_TABLE_OBJ));
	if (NULL == dev_hash_node)
	{
		printf("\n#########  malloc hash table filled\n");
		return NULL;
	}
	dev_hash_node->hash_table_len = table_len;
	dev_hash_node->stream_node_head = (DEV_HEAD_OF_HASH_TABLE_HANDLE)calloc(1, sizeof(DEV_HEAD_OF_HASH_TABLE_OBJ) * dev_hash_node->hash_table_len);
	if (NULL == dev_hash_node->stream_node_head)
	{
		free(dev_hash_node);
		dev_hash_node = NULL;
		printf("\n#########  malloc hash node filled\n");
		return NULL;
	}
	return dev_hash_node;
}














//根据call_id查找客户节点
static HB_S32 find_client_node_key(const HB_VOID *el, const HB_VOID *key)
{
	if ((el == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: el=%p, key=%p", el, key);
		return 0;
	}
	RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE)el;
	HB_CHAR *p_call_id = (HB_CHAR *)key;
	//printf("Gateway ip : [%s]\n", (HB_CHAR *)key);
	if (!strcmp(client_node->call_id, p_call_id))
	{
		printf("Find client call id key: [%s]\n", p_call_id);
		return 1;
	}

	return 0;
}


RTP_CLIENT_TRANSPORT_HANDLE FindClientNode(DEV_NODE_HANDLE dev_node, HB_CHAR call_id)
{
	list_attributes_seeker(&(dev_node->client_node_head), find_client_node_key);
	RTP_CLIENT_TRANSPORT_HANDLE client_node = list_seek(&(dev_node->client_node_head), call_id);

	if(NULL != client_node)
	{
		printf("");
		return client_node;
	}

	return NULL;
}







