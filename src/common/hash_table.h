/*
 * hash_table.h
 *
 *  Created on: 2017年12月6日
 *      Author: root
 */

#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include "simclist.h"

extern HB_U32 (*pHashFunc)(HB_CHAR *str);
//
//typedef struct  _tagHEAD_OF_HASH_TABLE
//{
//	pthread_mutex_t dev_mutex;
//	list_t dev_node_head;
//}HEAD_OF_HASH_TABLE_OBJ, *HEAD_OF_HASH_TABLE_HANDLE;
//
////哈希表
//typedef struct _tagHASH_TABLE
//{
//	HB_U32 hash_table_len;	//hash节点长度
//	HEAD_OF_HASH_TABLE_HANDLE hash_node;
//}HASH_TABLE_OBJ, *HASH_TABLE_HANDLE;


HB_VOID ChooseHashFunc(HB_CHAR *pHashFuncName);


#endif /* HASH_TABLE_H_ */
