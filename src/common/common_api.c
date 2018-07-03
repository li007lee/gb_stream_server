/*
 * common_api.c
 *
 *  Created on: Jul 2, 2018
 *      Author: root
 */

#include "common_api.h"

//获取start_num与end_num之间的随机数
HB_S32 random_number(HB_S32 iStartNum, HB_S32 iEndNum)
{
	HB_S32 ret_num = 0;
	srand((unsigned)time(0));
	ret_num = rand() % (iEndNum - iStartNum) + iStartNum;
	return ret_num;
}



