/*
 * server_config.h
 *
 *  Created on: 2017年5月30日
 *      Author: root
 */

#ifndef INCLUDE_SERVER_CONFIG_H_
#define INCLUDE_SERVER_CONFIG_H_

#define CONFIGURE_PATH "../conf/config.ini" //配置文件路徑

#define STREAM_DATA_FRAME_BUFFER_NUMS   (100)//视频数据缓冲帧数
#define CLIENT_MAXIMUM_BUFFER   (2097152)  //客户最大缓冲2M

//以下是线程池的配置
#define MIN_THREADS     (500)  //最小启动线程数
#define MAX_THREADS    (2500)  //线程数上限

#define RTCP_SEND_TIMER	(15)	//用于发送rtcp消息的定时器时间 单位秒
#define RTCP_RECV_TIMEOUT	(20) //接收对端rtcp返回消息的超时时间 单位秒

#define JE_MELLOC_FUCTION 0   //是否启用jemalloc 1-启用，0-关闭

#endif /* INCLUDE_SERVER_CONFIG_H_ */
