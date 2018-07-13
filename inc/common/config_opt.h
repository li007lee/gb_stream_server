/*
 * udp_opt.h
 *
 *  Created on: 2017年3月27日
 *      Author: root
 */

#ifndef CONFIG_OPT_H_
#define CONFIG_OPT_H_

#include "my_include.h"

FILE *open_config_file(HB_CHAR *pFileName);
HB_S32 close_config_file(FILE *fp);
HB_VOID get_string_value(FILE *fp, HB_U8 *pSectionName, HB_U8 *pKeyName, HB_U8 *pDefaultVal, HB_U8 *pOutput, HB_U32 u32OutputSize);
HB_S32 get_int_value(FILE *fp, HB_U8 *pSectionName, HB_U8 *pKeyName, HB_U32 u32DefaultVal);

#endif /* CONFIG_OPT_H_ */
