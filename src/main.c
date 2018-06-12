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
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);
	if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0)
	{
		TRACE_ERR("###### block sigpipe error!\n");

		return HB_FAILURE;
	}

	start_sip_moudle();

	return 0;
}
