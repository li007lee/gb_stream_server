/*
 * hash_table.c
 *
 *  Created on: 2017年12月6日
 *      Author: root
 */

#include "my_include.h"
#include "hash_table.h"
#include "simclist.h"

/* A Simple Hash Function */
unsigned int simple_hash(char *str)
{
	register unsigned int hash;
	register unsigned char *p;

	for (hash = 0, p = (unsigned char *) str; *p; p++)
		hash = 31 * hash + *p;

	return (hash & 0x7FFFFFFF);
}

/* RS Hash Function */
unsigned int RS_hash(char *str)
{
	unsigned int b = 378551;
	unsigned int a = 63689;
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * a + (*str++);
		a *= b;
	}

	return (hash & 0x7FFFFFFF);
}

/* JS Hash Function */
unsigned int JS_hash(char *str)
{
	unsigned int hash = 1315423911;

	while (*str)
	{
		hash ^= ((hash << 5) + (*str++) + (hash >> 2));
	}

	return (hash & 0x7FFFFFFF);
}

/* P. J. Weinberger Hash Function */
unsigned int PJW_hash(char *str)
{
	unsigned int BitsInUnignedInt = (unsigned int) (sizeof(unsigned int) * 8);
	unsigned int ThreeQuarters = (unsigned int) ((BitsInUnignedInt * 3) / 4);
	unsigned int OneEighth = (unsigned int) (BitsInUnignedInt / 8);

	unsigned int HighBits = (unsigned int) (0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
	unsigned int hash = 0;
	unsigned int test = 0;

	while (*str)
	{
		hash = (hash << OneEighth) + (*str++);
		if ((test = hash & HighBits) != 0)
		{
			hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
		}
	}

	return (hash & 0x7FFFFFFF);
}

/* ELF Hash Function */
unsigned int ELF_hash(char *str)
{
	unsigned int hash = 0;
	unsigned int x = 0;

	while (*str)
	{
		hash = (hash << 4) + (*str++);
		if ((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}

	return (hash & 0x7FFFFFFF);
}

/* BKDR Hash Function */
unsigned int BKDR_hash(char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);

}

/* SDBM Hash Function */
unsigned int SDBM_hash(char *str)
{
	unsigned int hash = 0;

	while (*str)
	{
		hash = (*str++) + (hash << 6) + (hash << 16) - hash;
	}

	return (hash & 0x7FFFFFFF);
}

/* DJB Hash Function */
unsigned int DJB_hash(char *str)
{
	unsigned int hash = 5381;

	while (*str)
	{
		hash += (hash << 5) + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

/* AP Hash Function */
unsigned int AP_hash(char *str)
{
	unsigned int hash = 0;
	int i;
	for (i = 0; *str; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}

	return (hash & 0x7FFFFFFF);
}

HB_U32 (*pHashFunc)(HB_CHAR *str);

//choose which hash function to be used
HB_VOID ChooseHashFunc(HB_CHAR *pHashFuncName)
{
	if (0 == strcmp(pHashFuncName, "simple_hash"))
		pHashFunc = simple_hash;
	else if (0 == strcmp(pHashFuncName, "RS_hash"))
		pHashFunc = RS_hash;
	else if (0 == strcmp(pHashFuncName, "JS_hash"))
		pHashFunc = JS_hash;
	else if (0 == strcmp(pHashFuncName, "PJW_hash"))
		pHashFunc = PJW_hash;
	else if (0 == strcmp(pHashFuncName, "ELF_hash"))
		pHashFunc = ELF_hash;
	else if (0 == strcmp(pHashFuncName, "BKDR_hash"))
	{
		pHashFunc = BKDR_hash;
	}
	else if (0 == strcmp(pHashFuncName, "SDBM_hash"))
	{
		pHashFunc = SDBM_hash;
	}
	else if (0 == strcmp(pHashFuncName, "DJB_hash"))
		pHashFunc = DJB_hash;
	else if (0 == strcmp(pHashFuncName, "AP_hash"))
		pHashFunc = AP_hash;
	//else if (0 == strcmp(pHashFuncName, "CRC_hash"))
	//pHashFunc = CRC_hash;
	else
		pHashFunc = NULL;
}

