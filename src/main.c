/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "my_include.h"
#include "sip.h"
#include "common_args.h"
#include "config_opt.h"
#include "server_config.h"

GLOBLE_ARGS_OBJ glGlobleArgs;

HB_S32 get_server_config()
{
	FILE *fp = open_config_file(CONFIGURE_PATH);
	if (NULL == fp)
	{
		TRACE_ERR("open config file [%s] failed\n", CONFIGURE_PATH);
		return HB_FAILURE;
	}

	get_string_value(fp, "SERVER_CONFIG", "LocalIp", NULL, glGlobleArgs.cLocalIp, sizeof(glGlobleArgs.cLocalIp));
	printf("glGlobleArgs.cLocalIp:[%s]\n", glGlobleArgs.cLocalIp);
	get_string_value(fp, "SERVER_CONFIG", "ETHX", NULL, glGlobleArgs.cNetworkCardName, sizeof(glGlobleArgs.cNetworkCardName));
	printf("glGlobleArgs.cNetworkCardName:[%s]\n", glGlobleArgs.cNetworkCardName);

	glGlobleArgs.iGbListenPort = get_int_value(fp, "SERVER_CONFIG", "GbListenPort", 5060);
	printf("glGlobleArgs.iGbListenPort:[%d]\n", glGlobleArgs.iGbListenPort);
	glGlobleArgs.iUseRtcpFlag = get_int_value(fp, "SERVER_CONFIG", "UseRtcp", 0);
	printf("glGlobleArgs.iUseRtcpFlag:[%d]\n", glGlobleArgs.iUseRtcpFlag);
	glGlobleArgs.iMaxConnections = get_int_value(fp, "SERVER_CONFIG", "MaxConnections", 100);
	printf("glGlobleArgs.iMaxConnections:[%d]\n", glGlobleArgs.iMaxConnections);
	if (glGlobleArgs.iMaxConnections < 1)
	{
		glGlobleArgs.iMaxConnections = 1;
	}

#ifdef RECV_STREAM_FROM_BOX
	get_string_value(fp, "BOX_CONFIG", "BoxIp", NULL, glGlobleArgs.cBoxIp, sizeof(glGlobleArgs.cNetworkCardName));
	printf("glGlobleArgs.cBoxIp:[%s]\n", glGlobleArgs.cBoxIp);
	glGlobleArgs.iBoxPort = get_int_value(fp, "BOX_CONFIG", "BoxPort", 8109);
	printf("glGlobleArgs.iBoxPort:[%d]\n", glGlobleArgs.iBoxPort);
#endif

	close_config_file(fp);

	return HB_SUCCESS;
}

HB_S32 main(HB_S32 argc, HB_CHAR **argv)
{
	sigset_t stSignalMask;
	sigemptyset(&stSignalMask);
	sigaddset(&stSignalMask, SIGPIPE);
	if (pthread_sigmask(SIG_BLOCK, &stSignalMask, NULL) != 0)
	{
		TRACE_ERR("###### block sigpipe error!\n");

		return HB_FAILURE;
	}

	printf("##############Compile Time: %s %s\n", __DATE__, __TIME__);

	if (HB_FAILURE == get_server_config())
	{
		return HB_FAILURE;
	}

	start_sip_moudle();

	return 0;
}
