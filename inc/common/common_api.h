/*
 * common_api.h
 *
 *  Created on: Jul 2, 2018
 *      Author: root
 */

#ifndef SRC_COMMON_COMMON_API_H_
#define SRC_COMMON_COMMON_API_H_

#include "my_include.h"

HB_S32 random_number(HB_S32 iStartNum, HB_S32 iEndNum);
HB_S32 udp_bind_local_port(HB_S32 *pUdpSendStreamPort, HB_S32 *pUdpListenRtcpPort);

#endif /* SRC_COMMON_COMMON_API_H_ */
