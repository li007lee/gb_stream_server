/*
 * server_config.h
 *
 *  Created on: 2017年5月30日
 *      Author: root
 */

#ifndef INCLUDE_SERVER_CONFIG_H_
#define INCLUDE_SERVER_CONFIG_H_

//#define RTSP_SERVER_PORT (8557)  //rtsp服务监听端口

#define CPU_NUMS (2)
#define PRIVATE_SERVER_MODE 1   //私服模式 ，1-开启 ，0-关闭（不能与一点通服务模式同时开启）
#define YDT_SERVER_MODE 0          //一点通服务模式，1-开启 ，0-关闭（不能与私服模式同时开启）
#define LOCAL_IP "172.16.3.100"
//#define LOCAL_IP "192.168.118.14"
#define RTSP_SERVER_ADDR "172.16.3.100"  //rtsp服务地址
//#define RTSP_SERVER_ADDR "192.168.118.14"  //rtsp服务地址
#define RTSP_SERVER_PORT (8556)  //rtsp服务监听端口
#define PROXY_RTSP_SERVER_PORT (8558)  //rtsp代理服务监听端口
#define DATA_BASE_FILE "../../data/dev_manager.db" //数据库文件
#define UDP_SURPORT 0      //是否支持UDP 1-开启UDP， 0-关闭UDP


//#define AUTHENTICATE  ture //是否需要用户名密码认证
#define STREAM_DATA_FRAME_BUFFER_NUMS   (100)//视频数据缓冲帧数
//#define STREAM_DATA_NODE_MAXIMUM_NUMBER   (150)//视频数据节点最大数
#define CLIENT_MAXIMUM_BUFFER   (2097152)  //客户最大缓冲2M

//以下是线程池的配置
#define USE_PTHREAD_POOL  1    //是否启用线程池 0-关闭，1-启用
#define MAX_THREADS    (2500)   //线程数上限
#define MIN_THREADS     (500)     //最小启动线程数

#define MAX_RTP_PACKET_SIZE (1448) //1500(max) - 20(IP header) - 20(TCP header) - 12or14(RTP header)
#define RTCP_SEND_TIMER	(15)	//用于发送rtcp消息的定时器时间 单位秒
#define RTCP_RECV_TIMEOUT	(20) //接收对端rtcp返回消息的超时时间 单位秒


//以下是日志和调试的配置
#define LOG_CONF_FILE "../../log/log_conf.conf"
#define USE_LOG_FUNCTION 0    //是否启用日志功能  1-启用，0-关闭
#define USE_DEBUG_FUNCTION 1  //是否启用调试功能  1-启用，0-关闭

#define JE_MELLOC_FUCTION 0   //是否启用jemalloc 1-启用，0-关闭

//是否开启宏函数
#define USE_MACRO_FUNCTION 0  //是否开启宏函数 0-不启用宏函数 1-启用宏函数


#endif /* INCLUDE_SERVER_CONFIG_H_ */
