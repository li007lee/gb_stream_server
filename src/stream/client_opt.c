/*
 * client_opt.c
 *
 *  Created on: Jul 4, 2018
 *      Author: root
 */

#include "stream/client_opt.h"
#include "event2/event.h"

//禁用客户节点中的发送视频流事件
HB_VOID disable_client_rtp_list_bev(list_t *listClientNodeHead)
{
	HB_S32 iClientNums = 0;
	iClientNums = list_size(listClientNodeHead);
	while (iClientNums)
	{
		RTP_CLIENT_TRANSPORT_HANDLE pClientNode = list_get_at(listClientNodeHead, iClientNums - 1);
		if (pClientNode->pSendStreamBev != NULL)
		{
			bufferevent_disable(pClientNode->pSendStreamBev, EV_READ | EV_WRITE);
		}
		iClientNums--;
	}
}



//销毁整个客户链表
HB_VOID destory_client_list(list_t *plistClientNodeHead)
{
	printf("destory_client_list()\n");
	HB_S32 iClientNums = 0;
	RTP_CLIENT_TRANSPORT_HANDLE pClientNode = NULL;
	iClientNums = list_size(plistClientNodeHead);
	while (iClientNums)
	{
		pClientNode = list_get_at(plistClientNodeHead, 0);
		list_delete(plistClientNodeHead, pClientNode);
		if (pClientNode->pSendStreamBev != NULL)
		{
			bufferevent_disable(pClientNode->pSendStreamBev, EV_READ | EV_WRITE);
			bufferevent_free(pClientNode->pSendStreamBev);
			pClientNode->pSendStreamBev = NULL;
		}
		if (pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent != NULL)
		{
			event_del(pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent);
			pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent = NULL;
		}
		if (pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent != NULL)
		{
			event_del(pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent);
			pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent = NULL;
		}
		if (pClientNode->hEventArgs != NULL)
		{
			free(pClientNode->hEventArgs);
			pClientNode->hEventArgs = NULL;
		}
		if (pClientNode->stUdpVideoInfo.iUdpVideoFd > 0)
		{
			close(pClientNode->stUdpVideoInfo.iUdpVideoFd);
			pClientNode->stUdpVideoInfo.iUdpVideoFd = -1;
		}
		if (pClientNode->stUdpVideoInfo.iUdpRtcpSockFd > 0)
		{
			close(pClientNode->stUdpVideoInfo.iUdpRtcpSockFd);
			pClientNode->stUdpVideoInfo.iUdpRtcpSockFd = -1;
		}
		free(pClientNode);
		pClientNode = NULL;
		iClientNums--;
	}
	list_destroy(plistClientNodeHead);
	printf("destory_client_list() OK!\n");
}


//从客户链表中删除一个客户节点
HB_VOID del_one_client_node(list_t *plistClientNodeHead, RTP_CLIENT_TRANSPORT_HANDLE pClientNode)
{
	printf("del_one_client_node() client from list\n");
	list_delete(plistClientNodeHead, pClientNode);
	if (pClientNode->pSendStreamBev != NULL)
	{
		bufferevent_free(pClientNode->pSendStreamBev);
		pClientNode->pSendStreamBev = NULL;
	}
	if (pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent != NULL)
	{
		event_del(pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent);
		pClientNode->stUdpVideoInfo.evUdpSendRtcpEvent = NULL;
	}
	if (pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent != NULL)
	{
		event_del(pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent);
		pClientNode->stUdpVideoInfo.evUdpRtcpListenEvent = NULL;
	}
	if (pClientNode->hEventArgs != NULL)
	{
		free(pClientNode->hEventArgs);
		pClientNode->hEventArgs = NULL;
	}
	if (pClientNode->stUdpVideoInfo.iUdpVideoFd > 0)
	{
		close(pClientNode->stUdpVideoInfo.iUdpVideoFd);
		pClientNode->stUdpVideoInfo.iUdpVideoFd = -1;
	}
	if (pClientNode->stUdpVideoInfo.iUdpRtcpSockFd > 0)
	{
		close(pClientNode->stUdpVideoInfo.iUdpRtcpSockFd);
		pClientNode->stUdpVideoInfo.iUdpRtcpSockFd = -1;
	}
	printf("del_one_client_node() client from list ok\n");
}

