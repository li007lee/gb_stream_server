/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "my_include.h"
#include "sip.h"

int main(int argc, char **argv)
{
//	signal(SIGPIPE, SIG_IGN);
	sigset_t stSignalMask;
	sigemptyset(&stSignalMask);
	sigaddset(&stSignalMask, SIGPIPE);
	if (pthread_sigmask(SIG_BLOCK, &stSignalMask, NULL) != 0)
	{
		TRACE_ERR("###### block sigpipe error!\n");

		return HB_FAILURE;
	}

	printf("##############Compile Time: %s %s\n", __DATE__, __TIME__);

	start_sip_moudle();

	return 0;
}
