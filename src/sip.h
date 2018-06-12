/*
 * libevent_server.h
 *
 *  Created on: 2017年9月21日
 *      Author: root
 */

#ifndef SRC_SIP_H_
#define SRC_SIP_H_

#include "my_include.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "event2/event_compat.h"

#include "osipparser2/osip_message.h"
#include "osipparser2/osip_parser.h"
#include "osipparser2/osip_port.h"
#include "osip2/osip_dialog.h"

#define CMD_MAX_LEN	(512) //RTSP服务器发来命令的最大长度

extern struct event_base *pEventBase;

typedef enum _tagCONTENT_TYPE
{
	NO_TYPE=0,
    REGISTER=1,
	INVITE,
	ACK,
	CANCEL,
	MESSAGE,
	INFO,
	OPTIONS,
	BYE,
}CONTENT_TYPE_E;


HB_S32 start_sip_moudle();

#endif /* SRC_SIP_H_ */
