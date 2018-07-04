/*
 * rtcp_pack.h
 *
 *  Created on: Jul 2, 2018
 *      Author: root
 */

#ifndef SRC_STREAM_RTCP_PACK_H_
#define SRC_STREAM_RTCP_PACK_H_
#include "my_include.h"
#include "../stream_hash.h"

enum
{
	RTCP_SR		= 200,
	RTCP_RR		= 201,
	RTCP_SDES	= 202,
	RTCP_BYE	= 203,
	RTCP_APP	= 204,
};

typedef struct _rtcp_header_t
{
	uint32_t v:2;		// version
	uint32_t p:1;		// padding
	uint32_t rc:5;		// reception report count
	uint32_t pt:8;		// packet type
	uint32_t length:16; /* pkt len in words, w/o this word */
} rtcp_header_t;






typedef struct _rtcp_sdes_item_t // source description RTCP packet
{
	uint8_t pt; // chunk type
	uint8_t len;
	uint8_t *data;
} rtcp_sdes_item_t;

int rtcp_sr_pack(RTP_CLIENT_TRANSPORT_HANDLE pClientNode, HB_U8* ptr, int bytes);
int rtcp_sdes_pack(RTP_CLIENT_TRANSPORT_HANDLE pClientNode, HB_U8* ptr, int bytes);

#endif /* SRC_STREAM_RTCP_PACK_H_ */
