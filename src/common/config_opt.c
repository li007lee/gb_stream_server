/*
 * config_opt.c
 *
 *  Created on: 2017年3月27日
 *      Author: root
 */

#include "config_opt.h"


/**********************************************************************
* 功能描述： 获取具体的字符串值
* 输入参数： fp-配置文件指针
             pszSectionName-段名, 如: GENERAL
             pszKeyName-配置项名, 如: EmployeeName
             iOutputLen-输出缓存长度
* 输出参数： pszOutput-输出缓存
* 返 回 值： 无
* 其它说明： 无
********************************************************************/
static HB_VOID get_string_content_value(FILE *fp, HB_U8 *pSectionName, HB_U8 *pKeyName, HB_U8 *pOutput, HB_U32 u32OutputSize)
{
	HB_U8  cSectionName[100]    = {0};
	HB_U8  cKeyName[100]        = {0};
	HB_U8  cContentLine[256]    = {0};
	HB_U8  cContentLineBak[256] = {0};
	HB_U32 u32ContentLineLen       = 0;
	HB_U32 u32PositionFlag         = 0;

    sprintf(cSectionName, "[%s]", pSectionName);
    strcpy(cKeyName, pKeyName);
    while (feof(fp) == 0)
    {
        memset(cContentLine, 0x00, sizeof(cContentLine));
        fgets(cContentLine, sizeof(cContentLine), fp);      // 获取段名
        // 判断是否是注释行(以;开头的行就是注释行)或以其他特殊字符开头的行
        if (cContentLine[0] == ';' || cContentLine[0] == '\r' || cContentLine[0] == '\n' || cContentLine[0] == '\0')
        {
            continue;
        }
        // 匹配段名
        if (strncasecmp(cSectionName, cContentLine, strlen(cSectionName)) == 0)
        {
            while (feof(fp) == 0)
            {
                memset(cContentLine,    0x00, sizeof(cContentLine));
                memset(cContentLineBak, 0x00, sizeof(cContentLineBak));
                fgets(cContentLine, sizeof(cContentLine), fp);     // 获取字段值

                // 判断是否是注释行(以;开头的行就是注释行)
                if (cContentLine[0] == ';')
                {
                    continue;
                }

                memcpy(cContentLineBak, cContentLine, strlen(cContentLine));

                // 匹配配置项名
                if (strncasecmp(cKeyName, cContentLineBak, strlen(cKeyName)) == 0)
                {
                	u32ContentLineLen = strlen(cContentLine);
                    for (u32PositionFlag = strlen(cKeyName); u32PositionFlag <= u32ContentLineLen; u32PositionFlag ++)
                    {
                        if (cContentLine[u32PositionFlag] == ' ')
                        {
                            continue;
                        }
                        if (cContentLine[u32PositionFlag] == '=')
                        {
                            break;
                        }

                        u32PositionFlag = u32ContentLineLen + 1;
                        break;
                    }

                    u32PositionFlag = u32PositionFlag + 1;    // 跳过=的位置

                    if (u32PositionFlag > u32ContentLineLen)
                    {
                        continue;
                    }

                    memset(cContentLine, 0x00, sizeof(cContentLine));
                    strcpy(cContentLine, cContentLineBak + u32PositionFlag);

                    // 去掉内容中的无关字符
                    for (u32PositionFlag = 0; u32PositionFlag < strlen(cContentLine); u32PositionFlag ++)
                    {
                        if (cContentLine[u32PositionFlag] == '\r' || cContentLine[u32PositionFlag] == '\n' || cContentLine[u32PositionFlag] == '\0')
                        {
                            cContentLine[u32PositionFlag] = '\0';
                            break;
                        }
                    }

                    // 将配置项内容拷贝到输出缓存中
                    strncpy(pOutput, cContentLine, u32OutputSize-1);
                    break;
                }
                else if (cContentLine[0] == '[')
                {
                    break;
                }
            }
            break;
        }
    }
}


FILE *open_config_file(char *pFileName)
{
	FILE *fp = NULL;

    // 打开配置文件
    fp = fopen(pFileName, "r");
    if (fp == NULL)
    {
        printf("GetConfigFileStringValue: open %s failed!\n", pFileName);
        return fp;
    }

	return fp;
}

HB_S32 close_config_file(FILE *fp)
{
	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return 0;
}


/**********************************************************************
* 功能描述： 从配置文件中获取字符串
* 输入参数： pSectionName-段名, 如: GENERAL
            pKeyName-配置项名, 如: EmployeeName
            pDefaultVal-默认值
			iOutputSize-输出缓存长度
* 输出参数： pOutput-输出缓存
* 返 回 值： 无
* 其它说明： 无
********************************************************************/
HB_VOID get_string_value(FILE *fp, HB_U8 *pSectionName, HB_U8 *pKeyName, HB_U8 *pDefaultVal, HB_U8 *pOutput, HB_U32 u32OutputSize)
{
    // 先对输入参数进行异常判断
    if (fp == NULL || pSectionName == NULL || pKeyName == NULL || pOutput == NULL)
    {
        printf("get_string_value: input parameter(s) is NULL!\n");
        return;
    }

    fseeko(fp, 0, SEEK_SET);

    // 获取默认值
    if (pDefaultVal == NULL)
    {
        strcpy(pOutput, "");
    }
    else
    {
        strcpy(pOutput, pDefaultVal);
    }

    // 调用函数用于获取具体配置项的值
    get_string_content_value(fp, pSectionName, pKeyName, pOutput, u32OutputSize);
}

/**********************************************************************
* 功能描述： 从配置文件中获取整型变量
* 输入参数： pSectionName-段名, 如: GENERAL
             pKeyName-配置项名, 如: EmployeeName
             iDefaultVal-默认值
* 输出参数： 无
* 返 回 值： iGetValue-获取到的整数值   -1-获取失败
* 其它说明： 无
********************************************************************/
HB_S32 get_int_value(FILE *fp, HB_U8 *pSectionName, HB_U8 *pKeyName, HB_U32 u32DefaultVal)
{
	HB_U8 cGetValue[512] = {0};
	HB_S32 iGetValue = 0;

    // 先对输入参数进行异常判断
    if (pSectionName == NULL || pKeyName == NULL)
    {
        printf("get_int_value: input parameter(s) is NULL!\n");
        return -1;
    }

    get_string_value(fp, pSectionName, pKeyName, NULL, cGetValue, sizeof(cGetValue));   //先将获取的值存放在字符型缓存中
    if (cGetValue[0] == '\0' || cGetValue[0] == ';')    // 如果是结束符或分号, 则使用默认值
    {
        iGetValue = u32DefaultVal;
    }
    else
    {
        iGetValue = atoi(cGetValue);
    }

    return iGetValue;
}

