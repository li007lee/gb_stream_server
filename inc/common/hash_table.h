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

HB_VOID ChooseHashFunc(HB_CHAR *pHashFuncName);


#endif /* HASH_TABLE_H_ */
