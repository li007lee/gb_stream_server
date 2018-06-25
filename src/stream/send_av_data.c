﻿//////////////////////////////////////////////////////////////////////////////
// 版权所有，2012-2016，北京汉邦高科数字技术有限公司
// 本文件是未公开的，包含了汉邦高科的机密和专利内容
////////////////////////////////////////////////////////////////////////////////
// 文件名： send_av_data.c
// 作者：  root
// 版本：   1.0
// 日期：   2016-11-18
// 描述：
// 历史记录：
////////////////////////////////////////////////////////////////////////////////
#include "my_include.h"
#include "event.h"
#include "send_av_data.h"
#include "server_config.h"
#include "common_args.h"
#include "hash_table.h"
#include "lf_queue.h"
#include "ps.h"
//#include "jemalloc/jemalloc.h"

//#include "common/elr_mpl.h"

extern STREAM_HASH_TABLE_HANDLE stream_hash_table;

#if 0
static HB_S32 destroy_client_node_list(STREAMING_NODE_HANDLE stream_node)
{
	HB_S32 i = 0;
	HB_S32 client_list_size = 0;
	HB_S32 tmp_list_size = 0;
	client_list_size = list_size(&(stream_node->listClientNodeHead));
	tmp_list_size = client_list_size;
	while(tmp_list_size)
	{
		RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE) list_extract_at(&(stream_node->listClientNodeHead), i);
		bufferevent_free(client_node->bev);
		client_node->bev = NULL;
		tmp_list_size--;
	}
	list_destroy(&(stream_node->listClientNodeHead));
	return 0;
}
#endif

//static unsigned int g_send_socket_buffer_size = 0; //网络发送缓冲区的大小
////////////////////////////////////////////////////////////////////////////////
// 函数名：send_data_over_udp
// 描  述：通过UDP方式发送RTP数据
// 参  数：[in] fd       网络文件描述符
//         [in] rtp_peer RTP客户端的网络地址
//         [in] rtp_buf  RTP数据缓冲的地址
//         [in] rtp_size RTP数据的大小
// 返回值：成功返回发送的字节数，出错返回-1
// 说  明：
////////////////////////////////////////////////////////////////////////////////
int send_data_over_udp(HB_S32 fd, struct sockaddr *rtp_peer, char *rtp_buf, unsigned int rtp_size)
{
	return sendto(fd, rtp_buf, rtp_size, 0, rtp_peer, sizeof(struct sockaddr));
}

#if 0
////////////////////////////////////////////////////////////////////////////////
// 函数名：add_tcp_header
// 描  述：向RTP流中添加TCP的头
// 参  数：[in/out] rtp_buf     rtp数据缓冲区
//         [in]     rtp_size    rtp数据大小
//         [in]     channel_id  通道ID
// 返回值：无
// 说  明：
////////////////////////////////////////////////////////////////////////////////
void add_tcp_header(char *rtp_buf, unsigned short rtp_size, unsigned int channel_id)
{
	rtp_buf[0] = '$';
	rtp_buf[1] = channel_id;
	rtp_buf[2] = (unsigned char)((rtp_size&0xff00)>>8);
	rtp_buf[3] = (unsigned char)((rtp_size&0xff));
}
#endif

#if 0
static HB_S32 destroy_client_rtp_list(list_t *listClientNodeHead)
{
	HB_S32 rtp_client_nums = 0;
	HB_CHAR *rtp_client_node = NULL;
	rtp_client_nums = list_size(listClientNodeHead);
	while(rtp_client_nums)
	{
		rtp_client_node = list_get_at(listClientNodeHead, 0);
		RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE)rtp_client_node;
		if(client_node->bev != NULL)
		{
			bufferevent_disable(client_node->bev, EV_READ|EV_WRITE);
			bufferevent_free(client_node->bev);
			client_node->bev = NULL;
		}
		if(client_node->event_args != NULL)
		{
			free(client_node->event_args);
			client_node->event_args = NULL;
		}
		list_delete(listClientNodeHead, rtp_client_node);
		free(client_node);
		rtp_client_nums--;
	}
	printf("\n*********************    list size=[%d]\n", list_size(listClientNodeHead));
	list_destroy(listClientNodeHead);
	return 0;
}
#endif

static HB_S32 destroy_client_rtp_list(list_t *listClientNodeHead)
{
	HB_S32 rtp_client_nums = 0;
	RTP_CLIENT_TRANSPORT_HANDLE rtp_client_node = NULL;
	rtp_client_nums = list_size(listClientNodeHead);
	while (rtp_client_nums)
	{
		//printf("\n*********************    list size=[%d]\n", list_size(listClientNodeHead));
		rtp_client_node = list_get_at(listClientNodeHead, 0);
		list_delete(listClientNodeHead, rtp_client_node);
//		list_delete_at(listClientNodeHead, 0);
		if (rtp_client_node->pSendStreamBev != NULL)
		{
			//bufferevent_disable(rtp_client_node->bev, EV_READ|EV_WRITE);
			bufferevent_free(rtp_client_node->pSendStreamBev);
			rtp_client_node->pSendStreamBev = NULL;
		}
		if (rtp_client_node->hEventArgs != NULL)
		{
			free(rtp_client_node->hEventArgs);
			rtp_client_node->hEventArgs = NULL;
		}
		if (rtp_client_node->iUdpVideoFd > 0)
		{
			close(rtp_client_node->iUdpVideoFd);
			rtp_client_node->iUdpVideoFd = -1;
		}
		free(rtp_client_node);
		rtp_client_node = NULL;
		rtp_client_nums--;
	}
	list_destroy(listClientNodeHead);
	return 0;
}

static HB_VOID delete_rtp_data_list(lf_queue queue)
{
	QUEUE_ARGS_OBJ queue_out;
	while (nolock_queue_len(queue) > 0)
	{
		nolock_queue_pop(queue, &queue_out);
		if (queue_out.data_buf != NULL)
		{
#if JE_MELLOC_FUCTION
			je_free(queue_out.data_buf);
#else
			free(queue_out.data_buf);
#endif
			queue_out.data_buf = NULL;
		}
	}
	printf("\n***************     queue size = [%d]\n", nolock_queue_len(queue));
	return;
}

HB_VOID send_rtp_to_client_task_err_cb(struct sttask *ptask, long reasons)
{
	// Try to free the customed task in its error handler
	if (0x1 & stpool_task_get_userflags(ptask))
	{
		printf("\nMMMMMMMMMMMMMMMMMMMMM  send_rtp_to_client_task_err_cb. [%d]\n", sizeof(struct sttask));
		stpool_task_detach(ptask);
		stpool_task_delete(ptask);
		return;
	}
}

HB_VOID send_rtp_to_client_task(struct sttask *ptsk)
{
	PS_FRAME_INFO_OBJ ps_info;
//	ps_init(25);
	struct evbuffer *evbuf = NULL;
	STREAM_NODE_HANDLE p_stream_node = (STREAM_NODE_HANDLE) (ptsk->task_arg);
	printf("\n@@@@@@@@@@@  send_rtp_to_client_task TASK start!dev_addr=%p\n", p_stream_node);
	HB_S32 ret = 0;
	HB_S32 rtp_data_nums = 0;
	HB_U32 send_interval = 0;
	HB_CHAR *video_data_node = NULL;
	HB_S32 rtp_data_buf_pre_size = 0;
	BOX_CTRL_CMD_OBJ cmd_head;
	memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
	HB_U32 cur_node_data_size = 0;
	HB_S32 set_sps_pps_to_data_base_flag = 0;
	HB_S32 hash_value = 0;
	hash_value = p_stream_node->iStreamNodeHashValue;
	HB_CHAR cDevId[128] = { 0 };

	struct timeval now;
	struct timespec outtime;

	HB_U64 add_time = 0;
	HB_U64 itime = 0;
	HB_U64 timestamp_old = 0;	//上一帧时间戳
	HB_U64 timestamp_current = 0;	//当前帧时间戳
	HB_S32 err_timestamp_times = 0;	//时间戳后来的比先来的小，这样的错误次数计数，调试打印用

	memset(&ps_info, 0, sizeof(PS_FRAME_INFO_OBJ));
	memcpy(cDevId, p_stream_node->cDevId, 127);
//	int ps_fd = open("./test.ps", O_RDWR|O_CREAT, 0777);

	char *p_ps_data = (char*) malloc(512 * 1400);
	int ps_data_len = 0;

	for (;;)
	{
//		printf("\n111111111   [%d]\n", list_size(&(p_stream_node->listClientNodeHead)));
		video_data_node = NULL;
		memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
		//客户节点为0或发送线程标识为退出状态,表示此通道可以关闭
#if 1
		if(2 == p_stream_node->uGetStreamThreadStartFlag)	//接收数据模块异常退出
		{
			ret = -101;
			break;
		}
		if((0 == list_size(&(p_stream_node->listClientNodeHead))))
		{
			p_stream_node->uRtpClientNodeSendDataThreadFlag = 2;
			ret = -102;
			break;
		}

		rtp_data_nums = nolock_queue_len(p_stream_node->queueStreamData);
		if(rtp_data_nums > 15)
		{
			if(rtp_data_nums >= (STREAM_DATA_FRAME_BUFFER_NUMS))
			{
				send_interval = 20000;
			}
			else if (rtp_data_nums < (STREAM_DATA_FRAME_BUFFER_NUMS) && (rtp_data_nums > 15))
			{
				send_interval = 40000;
			}
			else if (rtp_data_nums <= 15)
			{
				send_interval = 60000;
			}
		}
		else
		{
			usleep(50000);
			continue;
		}

#endif
//		printf("\n#############    list size = [%d]\n", list_size(&(p_stream_node->listClientNodeHead)));
		//video_data_node = list_get_at(&(p_stream_node->stream_data_node_head), 0);
		QUEUE_ARGS_OBJ queue_out;
		ret = nolock_queue_pop(p_stream_node->queueStreamData, &queue_out);
		video_data_node = queue_out.data_buf;
		rtp_data_buf_pre_size = queue_out.data_pre_buf_size;
		if (video_data_node != NULL)
		{
			memcpy(&cmd_head, video_data_node + rtp_data_buf_pre_size, 32);
#if 1
			if (0 == timestamp_old)
			{
				add_time = 0;
				timestamp_old = (HB_U64) (cmd_head.v_pts);
				if (0 == timestamp_old)
				{
					add_time = 3600;
				}
			}
			else
			{
				timestamp_current = (HB_U64) (cmd_head.v_pts);
				if (0 == timestamp_current)
				{
					add_time = 3600;
				}
				else
				{
					if (((HB_S64) timestamp_current - (HB_S64) timestamp_old) < 0)
					{
						err_timestamp_times++;
					}
					else if (0 == ((HB_S64) timestamp_current - (HB_S64) timestamp_old))
					{
						add_time = 3600;
					}
					else
					{
						add_time = (timestamp_current - timestamp_old);
					}
					timestamp_old = timestamp_current;
				}
				//printf("\n**************  timestamp_current=%lu   timestamp_old=%lu  timestamp=%d:%d\n", timestamp_current, timestamp_old, stDeviceInfo->m_TimeStampHigh, pPackage->iTimeStampLow);

			}
#endif

			itime += add_time;
			if (add_time <= 0)
			{
				add_time = 3600;
			}

//			itime += 3600;
//			printf("\n***************  v_pts=%lu rtsp_time_stamp =%lu, add_time=%lu   \n", (HB_U64) (cmd_head.v_pts), itime, add_time);
			cur_node_data_size = 0;
			if (I_FRAME == cmd_head.data_type) //I帧
			{
				ps_process(&ps_info, video_data_node + rtp_data_buf_pre_size + 32, cmd_head.cmd_length, 1, p_ps_data+rtp_data_buf_pre_size, &ps_data_len, itime, add_time);
//    			write(ps_fd, p_ps_data+rtp_data_buf_pre_size, ps_data_len);
				cur_node_data_size = pack_ps_rtp_and_add_node(p_stream_node, p_ps_data, ps_data_len, itime, rtp_data_buf_pre_size, 1);
			}
			else if (BP_FRAME == cmd_head.data_type) //P帧
			{
				ps_process(&ps_info, video_data_node + rtp_data_buf_pre_size + 32, cmd_head.cmd_length, 0, p_ps_data + rtp_data_buf_pre_size, &ps_data_len, itime, add_time);
//				write(ps_fd, p_ps_data + rtp_data_buf_pre_size, ps_data_len);
				cur_node_data_size = pack_ps_rtp_and_add_node(p_stream_node, p_ps_data, ps_data_len, itime, rtp_data_buf_pre_size, 0);
			}
			else
			{
				//printf("\n#########   audio ..... \n");
#if JE_MELLOC_FUCTION
				je_free(video_data_node);
#else
				free(video_data_node);
#endif
				video_data_node = NULL;
				continue;
			}

//			if (cur_node_data_size <= 0)
//			{
//				printf("\n#########   cur_node_data_size=[%d] \n", cur_node_data_size);
//#if JE_MELLOC_FUCTION
//				je_free(video_data_node);
//#else
//				free(video_data_node);
//#endif
//				video_data_node = NULL;
//				ret = -103;
//				p_stream_node->uRtpClientNodeSendDataThreadFlag = 2;
//				break;
//			}
			//printf("\n***************pack_video_rtp_and_add_node() pack_video_rtp_and_add_node = [%d]  \n", pack_video_rtp_and_add_node);
			//pthread_mutex_lock(&(p_stream_node->client_rtp_mutex));
			//list_delete(&(p_stream_node->stream_data_node_head), video_data_node);
			//je_free(video_data_node);
			free(video_data_node);
			video_data_node = NULL;
			usleep(send_interval);
			continue;


#if 0
			HB_S32 i = 0;
			HB_S32 client_list_size = 0;
			HB_S32 tmp_list_size = 0;
			client_list_size = list_size(&(p_stream_node->listClientNodeHead));
			tmp_list_size = client_list_size;
			while (tmp_list_size)
			{
				RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE) list_get_at(&(p_stream_node->listClientNodeHead), i);
				if (RTP_rtp_avp_tcp == client_node->enumRtpType && 1 == client_node->iStreamStartFlag)    		//TCP传输
				{
					//printf("\n111111111   [%d]\n", list_size(&(p_stream_node->listClientNodeHead)));
					ret = 0;
					if (0 == client_node->iSendIframeFlag && I_FRAME == cmd_head.data_type)
					{
						if (client_node->iDeleteFlag != 1)
						{
							if (NULL != client_node->bev)
							{
//        						ret = bufferevent_write(client_node->bev, video_data_node, cur_node_data_size);
								evbuf = bufferevent_get_output(client_node->bev);
							}
							else
							{
								tmp_list_size--;
								continue;
							}
							//printf("\n#############    send buf size = [%d]\n", evbuffer_get_length(evbuf));
							if (0 == ret)
							{
								client_node->iSendIframeFlag = 1;
								if (evbuffer_get_length(evbuf) > CLIENT_MAXIMUM_BUFFER)
								{
									ret = -1;
								}
							}
							else
							{
								ret = -2;
							}
						}
						else
						{
							ret = -3;
						}
					}
					else if (1 == client_node->iSendIframeFlag)
					{
						if (client_node->iDeleteFlag != 1)
						{
							if (NULL != client_node->bev)
							{
								//printf("\nPPPPPPPPPP max_single write = %d  read = %d\n", bufferevent_get_max_single_write(client_node->bev),bufferevent_get_max_single_read(client_node->bev));
//        						ret = bufferevent_write(client_node->bev, video_data_node, cur_node_data_size);
								evbuf = bufferevent_get_output(client_node->bev);
							}
							else
							{
								tmp_list_size--;
								continue;
							}
							if (0 == ret)
							{
								//printf("\n#############    send buf size = [%d]\n", evbuffer_get_length(evbuf));
								if (evbuffer_get_length(evbuf) > CLIENT_MAXIMUM_BUFFER)
								{
									ret = -4;
								}
								else
								{
									if (AUDIO_FRAME == cmd_head.data_type)
									{
										send_interval = 1;
									}
								}
							}
							else
							{
								ret = -5;
							}
						}
						else
						{
							ret = -6;
						}
					}

					if (ret != 0)
					{
						if (client_node->bev != NULL)
						{
							bufferevent_disable(client_node->bev, EV_READ | EV_WRITE);
							bufferevent_free(client_node->bev);
						}

						list_remove(&(p_stream_node->listClientNodeHead), client_node);
						free(client_node);
#if 0
						//pthread_rwlock_wrlock(&(dev_hash_table->hash_node[hash_value].dev_rwlock));
						if(1 == p_stream_node->uGetStreamThreadStartFlag)//接收流模块正在工作，这里只是摘除节点，由接收流模块进行释放
						{
							if(0 == list_size(&(p_stream_node->listClientNodeHead)))    		//客户队列为0
							{
								list_remove(&(p_stream_node->streaming_node_head), p_stream_node);
								if(0 == list_size(&(p_stream_node->streaming_node_head)))
								{
									list_remove(&(dev_hash_table->hash_node[hash_value].p_stream_node_head), p_stream_node);
									free(p_stream_node);
									p_stream_node = NULL;
								}
								p_stream_node->uRtpClientNodeSendDataThreadFlag = 2;
								pthread_mutex_unlock(&(dev_hash_table->hash_node[hash_value].dev_mutex));
								//pthread_rwlock_unlock(&(p_stream_node->client_rwlock));
								TRACE_ERR("###################send rtp data thread exit 03 [ret = %d]####################\n", ret);
								return;
							}

						}
						else //接收流模块异常退出，stream_node节点已由接收流模块摘除，这里释放strem_node节点及此节点以下的资源
						{
							//pthread_rwlock_wrlock(&(dev_hash_table->hash_node[hash_value].dev_rwlock));
							delet_rtp_data_list(&(p_stream_node->stream_data_node_head));
							//pthread_rwlock_unlock(&(dev_hash_table->hash_node[hash_value].dev_rwlock));
							destroy_client_rtp_list(&(p_stream_node->listClientNodeHead));//释放rtp客户队列
							free(p_stream_node);
							p_stream_node = NULL;
							pthread_mutex_unlock(&(dev_hash_table->hash_node[hash_value].dev_mutex));
							//pthread_rwlock_unlock(&(p_stream_node->client_rwlock));
							TRACE_ERR("###################send rtp data thread exit 04 [ret = %d]####################\n", ret);
							return;
						}
#endif
					}
					else
					{
						i++;
					}
				}
				tmp_list_size--;
			}
#endif

#if JE_MELLOC_FUCTION
			je_free(video_data_node);
#else
			free(video_data_node);
#endif
			video_data_node = NULL;
		}
		usleep(send_interval);
		continue;

	}

ERR:
	TRACE_ERR("000###################send rtp data thread exit [ret = %d]####################\n", ret);
	free(p_ps_data);
	p_ps_data = NULL;
	if (2 == p_stream_node->uRtpClientNodeSendDataThreadFlag) //rtp发送线程先出现异常
	{
		while (p_stream_node->uGetStreamThreadStartFlag != 2)
		{
			usleep(5000);
		}
	}
	TRACE_ERR("111###################send rtp data thread exit [ret = %d]####################\n", ret);
	del_node_from_stream_hash_table(stream_hash_table, p_stream_node);
	delete_rtp_data_list(p_stream_node->queueStreamData);
	nolock_queue_destroy(&(p_stream_node->queueStreamData));
	destroy_client_rtp_list(&(p_stream_node->listClientNodeHead)); //释放rtp客户队列
	if (NULL != p_stream_node->hGetStreamFromSource)
	{
		free(p_stream_node->hGetStreamFromSource);
		p_stream_node->hGetStreamFromSource = NULL;
	}

	free(p_stream_node);
	p_stream_node = NULL;
	send_rtp_to_client_task_err_cb(ptsk, 0);
	TRACE_ERR("222###################send rtp data thread exit [ret = %d]####################\n", ret);

	return;
}


#if 0
////////////////////////////////////////////////////////////////////////////////
// 函数名：send_audio_data
// 描  述：发送音频数据
// 参  数：[in] rtp_info  RTP信息
//         [in] data_size 音频数据的大小
//         [in] data_ptr  音频数据的指针
//         [in] ts        RTP时间戳
// 返回值：成功返回0，失败返回-1
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
int send_audio_data(rtp_info_t *rtp_info,
				unsigned int data_size, char *data_ptr, unsigned int ts)
{
	static char stream_buf[AUDIO_STREAM_BUFFER_SIZE];
	static char *rtp_buf = NULL, *cp = NULL;
	static RTSP_session_t **head = NULL, *p = NULL;
	static RTP_header *rtp_header = NULL;
	static int ret = 0, rtp_size = 0;

	//将获取到的音频数据复制到缓冲区
	memcpy(stream_buf + RTP_HEADER_SIZE, data_ptr, data_size);

	rtp_buf = stream_buf + RTP_HEADER_SIZE - 12;
	rtp_size = data_size;

	//RTP Header赋值
	rtp_header = (RTP_header *)rtp_buf;
#if 1
	cp = (char *)rtp_header;

	rtp_header->version = rtp_info->rtp_hdr.version; //设置版本号
	rtp_header->payload = rtp_info->rtp_hdr.payload;//设置负载类型
	rtp_header->marker = 1;//最后一包数据的标识位，设置为1

	//设置序列号
	cp[2] = (rtp_info->rtp_hdr.seq_no >> 8 ) & 0xff;
	cp[3] = rtp_info->rtp_hdr.seq_no & 0xff;

	//设置时间戳
	cp[4] = (ts >> 24) & 0xff;
	cp[5] = (ts >> 16) & 0xff;
	cp[6] = (ts >> 8) & 0xff;
	cp[7] = ts & 0xff;

	//设置源标识
	cp[8] = (rtp_info->rtp_hdr.ssrc >> 24) & 0xff;
	cp[9] = (rtp_info->rtp_hdr.ssrc >> 16) & 0xff;
	cp[10] = (rtp_info->rtp_hdr.ssrc >> 8) & 0xff;
	cp[11] = rtp_info->rtp_hdr.ssrc & 0xff;
#else
	rtp_header->version = rtp_info->rtp_hdr.version;
	rtp_header->payload = rtp_info->rtp_hdr.payload;
	rtp_header->marker = 0;

	rtp_header->seq_no = htons(rtp_info->rtp_hdr.seq_no);
	rtp_header->timestamp = htonl(ts);
	rtp_header->ssrc = htonl(rtp_info->rtp_hdr.ssrc);
#endif
	rtp_size += 12; //rtp header lenth

	head = get_rtsp_session_head();
	if(NULL == head)
	{
		return -1;
	}

	for(p = (*head)->next; p != (*head); p = p->next)
	{
		//判断是否开始发送数据
		if(1 == p->rtp_session.started && p->teardown != 1)
		{
			//根据数据传输类型，选择数据发送方式
			if(RTP_rtp_avp_tcp == p->rtp_session.transport.type)
			{
				//通过TCP方式发送RTP数据
				if(p->rtp_session.transport.rtp_fd > 0)
				{
					ret = send_data_over_tcp(p->rtp_session.transport.rtp_fd,
									NULL, NULL, rtp_buf, rtp_size, AUDIO_RTP);
					if(ret < 0)
					{
						if(errno == EAGAIN)
						{
							//printf("errno == EAGAIN\n");
							continue;
						}
						else
						{
							//hb_fnc_log(HB_LOG_ERR,
							//	"send audio data over tcp error ! errno = %d", errno);
							p->rtp_session.started = 0;
							p->teardown = 1;

							continue;
						}
					}
				}
			}
			else if(RTP_rtp_avp == p->rtp_session.transport.type)
			{
				//通过UDP方式发送RTP数据
				if(p->rtp_session.transport.rtp_fd_audio > 0)
				{
					ret = sendto(p->rtp_session.transport.rtp_fd_audio, rtp_buf, rtp_size,
									0, &p->rtp_session.transport.udp_audio.rtp_peer, sizeof(struct sockaddr));
					if(ret < 0)
					{
						if(errno == EAGAIN)
						{
							//printf("errno == EAGAIN\n");
							continue;
						}
						else
						{
							//hb_fnc_log(HB_LOG_ERR,
							//	"send audio data over udp error ! errno = %d", errno);
							p->rtp_session.started = 0;
							p->teardown = 1;

							continue;
						}
					}
				}
			}
			else
			{
				//无效的传输类型
				//hb_fnc_log(HB_LOG_ERR, "transport type = %d",
				//	p->rtp_session.transport.type);
				p->rtp_session.started = 0;
				p->teardown = 1;
				continue;
			}
		}
	}

	//序列号累加
	rtp_info->rtp_hdr.seq_no++;
	//printf("rtp_info->rtp_hdr.seq_no = %d\n", rtp_info->rtp_hdr.seq_no);

	return 0;
}
#endif
