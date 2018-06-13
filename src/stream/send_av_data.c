//////////////////////////////////////////////////////////////////////////////
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
//#include "stream_ctrl/frame_ctrl.h"
//#include "rtp/rtp.h"
//#include "rtp/pack_rtp_node.h"
#include "server_config.h"
#include "../common/common_args.h"
#include "../common/hash_table.h"
#include "../common/lf_queue.h"
//#include "jemalloc/jemalloc.h"

//#include "common/elr_mpl.h"

//FILE *p_fp;

extern STREAM_HASH_TABLE_HANDLE stream_hash_table;

#if 0
static HB_S32 destroy_client_node_list(STREAMING_NODE_HANDLE stream_node)
{
	HB_S32 i = 0;
	HB_S32 client_list_size = 0;
	HB_S32 tmp_list_size = 0;
	client_list_size = list_size(&(stream_node->client_node_head));
	tmp_list_size = client_list_size;
	while(tmp_list_size)
	{
		RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE) list_extract_at(&(stream_node->client_node_head), i);
		bufferevent_free(client_node->bev);
		client_node->bev = NULL;
		tmp_list_size--;
	}
	list_destroy(&(stream_node->client_node_head));
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
static HB_S32 destroy_client_rtp_list(list_t *client_node_head)
{
	HB_S32 rtp_client_nums = 0;
	HB_CHAR *rtp_client_node = NULL;
	rtp_client_nums = list_size(client_node_head);
	while(rtp_client_nums)
	{
		rtp_client_node = list_get_at(client_node_head, 0);
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
		list_delete(client_node_head, rtp_client_node);
		free(client_node);
		rtp_client_nums--;
	}
	printf("\n*********************    list size=[%d]\n", list_size(client_node_head));
	list_destroy(client_node_head);
	return 0;
}
#endif

static HB_S32 destroy_client_rtp_list(list_t *client_node_head)
{
	HB_S32 rtp_client_nums = 0;
	RTP_CLIENT_TRANSPORT_HANDLE rtp_client_node = NULL;
	rtp_client_nums = list_size(client_node_head);
	while (rtp_client_nums)
	{
		//printf("\n*********************    list size=[%d]\n", list_size(client_node_head));
		rtp_client_node = list_get_at(client_node_head, 0);
		list_delete(client_node_head, rtp_client_node);
		if (rtp_client_node->bev != NULL)
		{
			//bufferevent_disable(rtp_client_node->bev, EV_READ|EV_WRITE);
			bufferevent_free(rtp_client_node->bev);
			rtp_client_node->bev = NULL;
		}
		if (rtp_client_node->event_args != NULL)
		{
			free(rtp_client_node->event_args);
			rtp_client_node->event_args = NULL;
		}
		free(rtp_client_node);
		rtp_client_node = NULL;
		rtp_client_nums--;
	}
	list_destroy(client_node_head);
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

//	udp_socket = setup_datagram_socket(19750, udp_socket);
	ps_init(25);
//	int ps_fd = open("./out_file.PS", O_RDWR|O_CREAT, 0777);
	//stpool_task_detach(ptsk);
	struct evbuffer *evbuf = NULL;
	DEV_NODE_HANDLE dev_node = (DEV_NODE_HANDLE) (ptsk->task_arg);
	printf("\n@@@@@@@@@@@  send_rtp_to_client_task TASK start!dev_addr=%p\n", dev_node);
	HB_S32 ret = 0;
	HB_S32 rtp_data_nums = 0;
	HB_U32 send_interval = 0;

#if 0
	HB_CHAR *tmp_pos_udp = NULL;
	HB_U32 tmp_len_udp = 0;
	HB_U32 rtp_pack_len_udp = 0;
#endif

	HB_CHAR *video_data_node = NULL;
	HB_S32 rtp_data_buf_pre_size = 0;
	BOX_CTRL_CMD_OBJ cmd_head;
	memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
	//HB_CHAR *node_data_buf = NULL;
	HB_U32 cur_node_data_size = 0;
	HB_S32 set_sps_pps_to_data_base_flag = 0;
	HB_S32 hash_value = 0;
	hash_value = dev_node->dev_node_hash_value;
	HB_CHAR dev_id[128] = { 0 };

	struct timeval now;
	struct timespec outtime;

	HB_U64 add_time = 0;
	HB_U64 itime = 0;
	HB_U64 timestamp_old = 0;	//上一帧时间戳
	HB_U64 timestamp_current = 0;	//当前帧时间戳
	HB_S32 err_timestamp_times = 0;	//时间戳后来的比先来的小，这样的错误次数计数，调试打印用

	//Mp4Context *mp4_context = NULL;
	//mp4_context = mp4_fopen("test.mp4", "wb");

	memcpy(dev_id, dev_node->dev_id, 127);
	for (;;)
	{
//		printf("\n111111111   [%d]\n", list_size(&(dev_node->client_node_head)));
		video_data_node = NULL;
		memset(&cmd_head, 0, sizeof(BOX_CTRL_CMD_OBJ));
		//客户节点为0或发送线程标识为退出状态,表示此通道可以关闭
#if 1
		if(2 == dev_node->get_stream_thread_start_flag)	//接收数据模块异常退出
		{
			ret = -101;
			break;
		}
		if((0 == list_size(&(dev_node->client_node_head))))
		{
			dev_node->rtp_client_node_send_data_thread_flag = 2;
			ret = -102;
			break;
		}

		rtp_data_nums = nolock_queue_len(dev_node->stream_data_queue);
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
//		printf("\n#############    list size = [%d]\n", list_size(&(dev_node->client_node_head)));
		//video_data_node = list_get_at(&(p_stream_node->stream_data_node_head), 0);
		QUEUE_ARGS_OBJ queue_out;
		ret = nolock_queue_pop(dev_node->stream_data_queue, &queue_out);
		video_data_node = queue_out.data_buf;
		rtp_data_buf_pre_size = queue_out.data_pre_buf_size;
		if (video_data_node != NULL)
		{
			memcpy(&cmd_head, video_data_node + rtp_data_buf_pre_size, 32);
			//cmd_head = (BOX_CTRL_CMD_HANDLE)video_data_node;
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
//			itime += 3600;
//			printf("\n***************  v_pts=%lu rtsp_time_stamp =%lu, add_time=%lu   \n", (HB_U64) (cmd_head.v_pts), itime, add_time);
//			continue;
			cur_node_data_size = 0;
			if (I_FRAME == cmd_head.data_type) //I帧
			{
				//write_frame_to_mp4_file(mp4_context, video_data_node+32, cmd_head.cmd_length, 1);
				char *p_ps_data = (char*) malloc(512 * 1400);
				int ps_data_len = 0;
				char *es_data = (char*) malloc(512 * 1400);
				memcpy(es_data, video_data_node + rtp_data_buf_pre_size + 32, cmd_head.cmd_length);
				ps_process(es_data, cmd_head.cmd_length, 1, p_ps_data + rtp_data_buf_pre_size, &ps_data_len);
				//write(ps_fd, p_ps_data+rtp_data_buf_pre_size, ps_data_len);
				cur_node_data_size = pack_ps_rtp_and_add_node(dev_node, p_ps_data, ps_data_len, itime, rtp_data_buf_pre_size, 1);
				video_data_node = p_ps_data;
				free(es_data);
//    			write(ps_fd, video_data_node, cur_node_data_size);
//    			printf("\n***********I---ps_data_len = %d\n", ps_data_len);
				//cur_node_data_size = pack_video_rtp_and_add_node(p_stream_node, video_data_node, cmd_head.cmd_length, itime, rtp_data_buf_pre_size, 1, set_sps_pps_to_data_base_flag, dev_id);
			}
			else if (BP_FRAME == cmd_head.data_type) //P帧
			{
				char *p_ps_data = (char*) malloc(512 * 1400);
				int ps_data_len = 0;
				char *es_data = (char*) malloc(512 * 1400);
				memcpy(es_data, video_data_node + rtp_data_buf_pre_size + 32, cmd_head.cmd_length);
				ps_process(es_data, cmd_head.cmd_length, 0, p_ps_data + rtp_data_buf_pre_size, &ps_data_len);
				//write(ps_fd, p_ps_data+rtp_data_buf_pre_size, ps_data_len);
				cur_node_data_size = pack_ps_rtp_and_add_node(dev_node, p_ps_data, ps_data_len, itime, rtp_data_buf_pre_size, 0);
				video_data_node = p_ps_data;
				free(es_data);
//    			write(ps_fd, video_data_node, cur_node_data_size);
//    			printf("\n***********P---ps_data_len = %d\n", ps_data_len);
				//write_frame_to_mp4_file(mp4_context, video_data_node+32, cmd_head.cmd_length, 0);
				//cur_node_data_size = pack_video_rtp_and_add_node(p_stream_node, video_data_node, cmd_head.cmd_length, itime, rtp_data_buf_pre_size, 0, set_sps_pps_to_data_base_flag, dev_id);
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

			if (cur_node_data_size <= 0)
			{
				printf("\n#########   cur_node_data_size=[%d] \n", cur_node_data_size);
#if JE_MELLOC_FUCTION
				je_free(video_data_node);
#else
				free(video_data_node);
#endif
				video_data_node = NULL;
				ret = -103;
				dev_node->rtp_client_node_send_data_thread_flag = 2;
				break;
			}
			//printf("\n***************pack_video_rtp_and_add_node() pack_video_rtp_and_add_node = [%d]  \n", pack_video_rtp_and_add_node);
			//pthread_mutex_lock(&(p_stream_node->client_rtp_mutex));
			//list_delete(&(p_stream_node->stream_data_node_head), video_data_node);
			//je_free(video_data_node);
			usleep(send_interval);
			continue;
			HB_S32 i = 0;
			HB_S32 client_list_size = 0;
			HB_S32 tmp_list_size = 0;
			client_list_size = list_size(&(dev_node->client_node_head));
			tmp_list_size = client_list_size;
			while (tmp_list_size)
			{
				RTP_CLIENT_TRANSPORT_HANDLE client_node = (RTP_CLIENT_TRANSPORT_HANDLE) list_get_at(&(dev_node->client_node_head), i);
				if (RTP_rtp_avp_tcp == client_node->type && 1 == client_node->stream_start)    		//TCP传输
				{
					//printf("\n111111111   [%d]\n", list_size(&(p_stream_node->client_node_head)));
					ret = 0;
					if (0 == client_node->send_Iframe_flag && I_FRAME == cmd_head.data_type)
					{
						if (client_node->delete_flag != 1)
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
								client_node->send_Iframe_flag = 1;
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
					else if (1 == client_node->send_Iframe_flag)
					{
						if (client_node->delete_flag != 1)
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

						list_remove(&(dev_node->client_node_head), client_node);
						free(client_node);
#if 0
						//pthread_rwlock_wrlock(&(dev_hash_table->hash_node[hash_value].dev_rwlock));
						if(1 == p_stream_node->get_stream_thread_start_flag)//接收流模块正在工作，这里只是摘除节点，由接收流模块进行释放
						{
							if(0 == list_size(&(p_stream_node->client_node_head)))    		//客户队列为0
							{
								list_remove(&(dev_node->streaming_node_head), p_stream_node);
								if(0 == list_size(&(dev_node->streaming_node_head)))
								{
									list_remove(&(dev_hash_table->hash_node[hash_value].dev_node_head), dev_node);
									free(dev_node);
									dev_node = NULL;
								}
								p_stream_node->rtp_client_node_send_data_thread_flag = 2;
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
							destroy_client_rtp_list(&(p_stream_node->client_node_head));//释放rtp客户队列
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
	if (2 == dev_node->rtp_client_node_send_data_thread_flag) //rtp发送线程先出现异常
	{

		while (dev_node->get_stream_thread_start_flag != 2)
		{
			usleep(5000);
		}
		pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		list_remove(&(stream_hash_table->stream_node_head[hash_value].dev_node_head), dev_node);
		pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));

		delete_rtp_data_list(dev_node->stream_data_queue);
		nolock_queue_destroy(&(dev_node->stream_data_queue));
		//destroy_client_rtp_list(&(dev_node->client_node_head)); //释放rtp客户队列
		if (NULL != dev_node->get_stream_from_source)
		{
			free(dev_node->get_stream_from_source);
			dev_node->get_stream_from_source = NULL;
		}

		free(dev_node);
		dev_node = NULL;
		send_rtp_to_client_task_err_cb(ptsk, 0);
		TRACE_ERR("###################send rtp data thread exit 01 [ret = %d]####################\n", ret);
		return;
	}
	else
	{
		printf("\n######### send rtp data thread exit01 \n");
		pthread_mutex_lock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));
		list_remove(&(stream_hash_table->stream_node_head[hash_value].dev_node_head), dev_node);
		delete_rtp_data_list(dev_node->stream_data_queue);
		nolock_queue_destroy(&(dev_node->stream_data_queue));
		destroy_client_rtp_list(&(dev_node->client_node_head)); //释放rtp客户队列
		if (NULL != dev_node->get_stream_from_source)
		{
			free(dev_node->get_stream_from_source);
			dev_node->get_stream_from_source = NULL;
		}
		pthread_mutex_unlock(&(stream_hash_table->stream_node_head[hash_value].dev_mutex));

		free(dev_node);
		dev_node = NULL;

		//mp4_close(mp4_context, 25);
		TRACE_ERR("###################send rtp data thread exit 02 [ret = %d]####################\n", ret);
		return;
	}
#if 0
	if(2 == p_stream_node->get_stream_thread_start_flag)		//接收数据模块异常退出
	{
		printf("\n######### send rtp data thread exit01 \n");
		pthread_mutex_lock(&(dev_hash_table->hash_node[hash_value].dev_mutex));
		delete_rtp_data_list(p_stream_node->stream_data_queue);
		lf_queue_destroy(&(p_stream_node->stream_data_queue));
		destroy_client_rtp_list(&(p_stream_node->client_node_head));		//释放rtp客户队列
		if (NULL != p_stream_node->get_stream_from_source)
		{
			free(p_stream_node->get_stream_from_source);
			p_stream_node->get_stream_from_source = NULL;
		}
		//pthread_mutex_destroy(&(p_stream_node->client_rtp_mutex));
		//pthread_cond_destroy(&(p_stream_node->stream_data_empty));
		pthread_mutex_unlock(&(dev_hash_table->hash_node[hash_value].dev_mutex));
		sleep(1);
		free(p_stream_node);
		p_stream_node = NULL;

		//mp4_close(mp4_context, 25);
		TRACE_ERR("###################send rtp data thread exit 02 [ret = %d]####################\n", ret);
		return;
	}
	//if((0 == list_size(&(p_stream_node->client_node_head))))
	else
	{
		printf("\n######### send rtp data thread exit02 \n");
		pthread_mutex_lock(&(dev_hash_table->hash_node[hash_value].dev_mutex));
		if(2 == p_stream_node->get_stream_thread_start_flag)		//接收视频数据模块已退出
		{
			delete_rtp_data_list(p_stream_node->stream_data_queue);
			lf_queue_destroy(&(p_stream_node->stream_data_queue));
			destroy_client_rtp_list(&(p_stream_node->client_node_head));		//释放rtp客户队列
			if (NULL != p_stream_node->get_stream_from_source)
			{
				free(p_stream_node->get_stream_from_source);
				p_stream_node->get_stream_from_source = NULL;
			}
			free(p_stream_node);
			p_stream_node = NULL;
		}
		else
		{
			printf("\n######### send rtp data thread exit03 \n");
			list_remove(&(dev_node->streaming_node_head), p_stream_node);
			if(0 == list_size(&(dev_node->streaming_node_head)))
			{
				list_destroy(&(dev_node->streaming_node_head));
				list_remove(&(dev_hash_table->hash_node[hash_value].dev_node_head), dev_node);
				free(dev_node);
				dev_node = NULL;
			}
			p_stream_node->rtp_client_node_send_data_thread_flag = 2;
		}

		pthread_mutex_unlock(&(dev_hash_table->hash_node[hash_value].dev_mutex));
		//mp4_close(mp4_context, 25);
		TRACE_ERR("###################send rtp data thread exit 03 [ret = %d]####################\n", ret);
		return;
	}
#endif
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
