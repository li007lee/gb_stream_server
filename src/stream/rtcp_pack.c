/*
 * rtcp_pack.c
 *
 *  Created on: Jul 2, 2018
 *      Author: root
 */


#include "rtcp_pack.h"


static void rtp_write_uint32(HB_U8* ptr, HB_U32 val)
{
	ptr[0] = (HB_U8)(val >> 24);
	ptr[1] = (HB_U8)(val >> 16);
	ptr[2] = (HB_U8)(val >> 8);
	ptr[3] = (HB_U8)val;
}

static void nbo_write_rtcp_header(HB_U8 *ptr, const rtcp_header_t *header)
{
	ptr[0] = (HB_U8)((header->v << 6) | (header->p << 5) | header->rc);
	ptr[1] = (HB_U8)(header->pt);
	ptr[2] = (HB_U8)(header->length >> 8);
	ptr[3] = (HB_U8)(header->length & 0xFF);
}

HB_U64 rtpclock()
{
	HB_U64 v;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	v = tv.tv_sec;
	v *= 1000;
	v += tv.tv_usec / 1000;

	return v;
}


HB_U64 clock2ntp(HB_U64 clock)
{
	HB_U64 ntp;

	// high 32 bits in seconds
	ntp = ((clock/1000)+0x83AA7E80) << 32; // 1/1/1970 -> 1/1/1900

	// low 32 bits in picosecond
	// ms * 2^32 / 10^3
	// 10^6 = 2^6 * 15625
	// => ms * 1000 * 2^26 / 15625
	ntp |= ((clock % 1000) * 1000) * 0x4000000 / 15625;

	return ntp;
}



int rtcp_sr_pack(RTP_CLIENT_TRANSPORT_HANDLE pClientNode, HB_U8* ptr, int bytes)
{
	HB_U32 i, timestamp = 162007200;
	HB_U64 ntp;
	rtcp_header_t header;

	static int count = 16436;
	static int iSendBytes = 20971520;

	header.v = 2;
	header.p = 0;
	header.pt = RTCP_SR;
	header.rc = 0;
	header.length = (24/*sizeof(rtcp_sr_t)*/ + header.rc*24/*sizeof(rtcp_rb_t)*/)/4; // see 6.4.1 SR: Sender Report RTCP Packet

	if((HB_U32)bytes < (header.length+1) * 4)
		return (header.length+1) * 4;

	nbo_write_rtcp_header(ptr, &header);

	// RFC3550 6.4.1 SR: Sender Report RTCP Packet (p32)
	// Note that in most cases this timestamp will not be equal to the RTP
	// timestamp in any adjacent data packet. Rather, it must be calculated from the corresponding
	// NTP timestamp using the relationship between the RTP timestamp counter and real time as
	// maintained by periodically checking the wallclock time at a sampling instant.
//	ntp = rtpclock();
//	ntp = 0x09a7ec8009a7fa90;
//	timestamp = iCurTime;


//	count += 1232;
//	iSendBytes += 321561;
//	ntp = clock2ntp(ntp);
	ntp = 0x09a7ec8009a7fa90;
	rtp_write_uint32(ptr+4, pClientNode->u32Ssrc);
	rtp_write_uint32(ptr+8, (HB_U32)(ntp >> 32));
	rtp_write_uint32(ptr+12, (HB_U32)(ntp & 0xFFFFFFFF));
	rtp_write_uint32(ptr+16, timestamp);
	rtp_write_uint32(ptr+20, count); // send packets
	rtp_write_uint32(ptr+24, iSendBytes); // send bytes

	ptr += 28;
#if 0
	// report block
	for(i = 0; i < header.rc; i++, ptr += 24/*sizeof(rtcp_rb_t)*/)
	{
		uint64_t delay;
		int lost_interval;
		int cumulative;
		uint32_t fraction;
		uint32_t expected, extseq;
		uint32_t expected_interval;
		uint32_t received_interval;
		uint32_t lsr, dlsr;
		struct rtp_member *sender;

		sender = rtp_member_list_get(ctx->senders, i);
		if(0 == sender->rtp_packets || sender->ssrc == ctx->self->ssrc)
			continue; // don't receive any packet

		extseq = sender->rtp_seq_cycles + sender->rtp_seq; // 32-bits sequence number
		assert(extseq >= sender->rtp_seq_base);
		expected = extseq - sender->rtp_seq_base + 1;
		expected_interval = expected - sender->rtp_expected0;
		received_interval = sender->rtp_packets - sender->rtp_packets0;
		lost_interval = (int)(expected_interval - received_interval);
		if(lost_interval < 0 || 0 == expected_interval)
			fraction = 0;
		else
			fraction = (lost_interval << 8)/expected_interval;

		cumulative = expected - sender->rtp_packets;
		if(cumulative > 0x007FFFFF)
		{
			cumulative = 0x007FFFFF;
		}
		else if(cumulative < 0)
		{
			// 'Clamp' this loss number to a 24-bit signed value:
			// live555 RTCP.cpp RTCPInstance::enqueueReportBlock line:799
			cumulative = 0;
		}

		delay = rtpclock() - sender->rtcp_clock; // now - Last SR time
		lsr = ((sender->rtcp_sr.ntpmsw&0xFFFF)<<16) | ((sender->rtcp_sr.ntplsw>>16) & 0xFFFF);
		// in units of 1/65536 seconds
		// 65536/1000000 == 1024/15625
		dlsr = (uint32_t)(delay/1000.0f * 65536);

		nbo_w32(ptr, sender->ssrc);
		ptr[4] = (unsigned char)fraction;
		ptr[5] = (unsigned char)((cumulative >> 16) & 0xFF);
		ptr[6] = (unsigned char)((cumulative >> 8) & 0xFF);
		ptr[7] = (unsigned char)(cumulative & 0xFF);
		nbo_w32(ptr+8, extseq);
		nbo_w32(ptr+12, (uint32_t)sender->jitter);
		nbo_w32(ptr+16, lsr);
		nbo_w32(ptr+20, 0==lsr ? 0 : dlsr);

		sender->rtp_expected0 = expected; // update source prior data
		sender->rtp_packets0 = sender->rtp_packets;
	}
#endif

	return (header.length+1) * 4;
}






static size_t rtcp_sdes_append_item(unsigned char *ptr, size_t bytes, HB_CHAR *pCname)
{
	if(bytes >= strlen(pCname)+2)
	{
		ptr[0] = 1;
		ptr[1] = strlen(pCname);
		memcpy(ptr+2, pCname, strlen(pCname));
	}

	return strlen(pCname)+2;
}


int rtcp_sdes_pack(RTP_CLIENT_TRANSPORT_HANDLE pClientNode, HB_U8* ptr, int bytes)
{
	int n;
	rtcp_header_t header;

//	// must have CNAME
//	if(!ctx->self->sdes[RTCP_SDES_CNAME].data)
//		return 0;

	header.v = 2;
	header.p = 0;
	header.pt = RTCP_SDES;
	header.rc = 1; // self only
	header.length = 0;

	n = rtcp_sdes_append_item(ptr+8, bytes-8, "fibernetwork");
	if(bytes < 8 + n)
		return 8 + n;

	// RFC3550 6.3.9 Allocation of Source Description Bandwidth (p29)
	// Every third interval (15 seconds), one extra item would be included in the SDES packet
//	if(0 == ctx->rtcp_cycle % 3 && ctx->rtcp_cycle/3 > 0) // skip CNAME
//	{
//		if(ctx->self->sdes[ctx->rtcp_cycle/3+1].data) // skip RTCP_SDES_END
//		{
//			n += rtcp_sdes_append_item(ptr+8+n, bytes-n-8, &ctx->self->sdes[ctx->rtcp_cycle/3+1]);
//			if(n + 8 > bytes)
//				return n + 8;
//		}
//	}
//
//	ctx->rtcp_cycle = (ctx->rtcp_cycle+1) % 24; // 3 * SDES item number

	header.length = (HB_U16)((n+4+3)/4); // see 6.4.1 SR: Sender Report RTCP Packet
	nbo_write_rtcp_header(ptr, &header);
	rtp_write_uint32(ptr+4, pClientNode->u32Ssrc);

	return (header.length+1)*4;
}


