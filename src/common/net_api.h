//
//  thread_depend.h
//  MapPort
//
//  Created by MoK on 15/1/21.
//  Copyright (c) 2015ๅนด MoK. All rights reserved.
//

#ifndef __MapPort__thread_depend__
#define __MapPort__thread_depend__



//通过域名解析出相应的ip，超过timeout秒解析不出来，则返回失败-1，成功返回值大于0
HB_S32 from_domain_to_ip(HB_CHAR *srv_ip, HB_CHAR *srv_domain, HB_S32 timeout);

////////////////////////////////////////////////////////////////////////////////
// 函数名：create_socket_connect_domain
// 描述：tcp网络连接
// 参数：
//  ［IN］psockfd - 套接字指针。
//  ［IN］domain – 要连接的域名
//    [IN] domain_port - 要连接域名的端口
//   [IN] timeout - 超时时间
// 返回值：-1失败,0成功
// 说明：
//通过域名和端口创建socket并且连接到对端，timeout秒后仍然没有连接上，则返回失败-1，成功返回0
////////////////////////////////////////////////////////////////////////////////
HB_S32 create_socket_connect_domain(HB_S32 *psockfd, HB_CHAR *domain, HB_S32 domain_port, HB_S32 timeout);


////////////////////////////////////////////////////////////////////////////////
// 函数名：create_socket_connect_ipaddr
// 描述：tcp网络连接
// 参数：
//  ［IN］psockfd - 套接字指针。
//  ［IN］ipaddr – 要连接的IP地址
//    [IN] port - 要连接的端口
//   [IN] timeout - 超时时间
// 返回值：-1失败,0成功
// 说明：
//通过IP和端口创建socket并且连接到对端，timeout秒后仍然没有连接上，则返回失败
////////////////////////////////////////////////////////////////////////////////
HB_S32 create_socket_connect_ipaddr(HB_S32 *psockfd, HB_CHAR *ipaddr, HB_S32 port, HB_S32 timeout);




////////////////////////////////////////////////////////////////////////////////
// 函数名：send_data
// 描述：tcp网络数据发送
// 参数：
//  ［IN］psockfd - 套接字指针。
//  ［IN］send_buf -发送缓冲区首地址
//    [IN] data_len - 要发送的数据长度
//   [IN] timeout_s - 超时时间 -秒
//   [IN] timeout_ms - 超时时间 -微秒
// 返回值：-1超时，-2失败，0成功
// 说明：
//发送指定长度datalen的数据，如果timeout秒后还为发送，则返回超时失败-1，
//成功表示全部发送完成则返回0，否则失败返回非0
////////////////////////////////////////////////////////////////////////////////
HB_S32 send_data(HB_S32 *psockfd, HB_VOID *send_buf, HB_S32 data_len, HB_S32 timeout_s, HB_S32 timeout_ms);

HB_S32 send_data_ep(HB_S32 epfd, struct epoll_event *events, HB_S32 maxevents, HB_VOID *send_buf, HB_S32 data_len, HB_S32 timeout_s, HB_S32 timeout_ms);

////////////////////////////////////////////////////////////////////////////////
// 函数名：recv_data_once
// 描述：tcp网络数据接收
// 参数：
//  ［IN］psockfd - 套接字指针。
//  ［IN］send_buf -接收缓冲区首地址
//    [IN] data_len - 接收缓冲区长度
//   [IN] timeout - 超时时间
// 返回值：-1超时，-2失败，0对端close，大于0成功
// 说明：
//接收数据，接收缓冲区为recv_buf,缓冲区长度为recv_buf_len,超过timeout秒仍没接收
//到数据，则返回失败(-3 -2 其他错我，-1-超时，0-对端关闭close，大于0-成功接收)
////////////////////////////////////////////////////////////////////////////////
HB_S32 recv_data_once(HB_S32 *psockfd, HB_VOID *recv_buf, HB_S32 recv_buf_len, HB_S32 timeout);

HB_S32 recv_data_once_ep(HB_S32 epfd, struct epoll_event *events, HB_S32 maxevents, HB_VOID *recv_buf, HB_S32 recv_buf_len, HB_S32 timeout);


////////////////////////////////////////////////////////////////////////////////
// 函数名：recv_data_once_peek
// 描述：tcp网络数据接收,但不清除接收缓存,用于检查是否接收到某些数据
// 参数：
//  ［IN］psockfd - 套接字指针。
//  ［IN］send_buf -接收缓冲区首地址
//    [IN] data_len - 接收缓冲区长度
//   [IN] timeout - 超时时间
// 返回值：-1超时，-2失败，0对端close，大于0成功
// 说明：
//接收数据，接收缓冲区为recv_buf,缓冲区长度为recv_buf_len,超过timeout秒仍没接收
//到数据，则返回失败(-3 -2 其他错我，-1-超时，0-对端关闭close，大于0-成功接收)
////////////////////////////////////////////////////////////////////////////////
HB_S32 recv_data_once_peek(HB_S32 *psockfd, HB_VOID *recv_buf, HB_S32 recv_buf_len, HB_S32 timeout);

////////////////////////////////////////////////////////////////////////////////
// 函数名：recv_data
// 描述：tcp网络数据接收
// 参数：
//  ［IN］psockfd - 套接字指针。
//  ［IN］send_buf -接收缓冲区首地址
//    [IN] recv_buf_len - 指定接收数据的长度
//   [IN] timeout - 超时时间
// 返回值：-1超时，-2失败，0对端close，大于0成功
// 说明：
//接收数据，接收缓冲区为recv_buf,缓冲区长度为recv_buf_len,超过timeout秒仍没接收
//到数据，则返回失败(-1-超时，小于0-其他错误，0-对端关闭socket，大于0-成功接收)
////////////////////////////////////////////////////////////////////////////////
HB_S32 recv_data(HB_S32 *psockfd, HB_VOID *recv_buf, HB_S32 recv_data_len, HB_S32 timeout);

HB_S32 recv_data_ep(HB_S32 epfd, struct epoll_event *events, HB_S32 maxevents, HB_VOID *recv_buf, HB_S32 recv_data_len, HB_S32 timeout);


////////////////////////////////////////////////////////////////////////////////
// 函数名：close_sockfd
// 描述：关闭tcp网络套接字
// 参数：
//  ［IN］psockfd - 套接字指针。
// 返回值：1
// 说明：
//关闭tcp网络套接字，并初始化为-1
////////////////////////////////////////////////////////////////////////////////
HB_S32 close_sockfd(HB_S32 *sockfd);

HB_S32 set_socket_non_blocking(HB_S32 sockfd);

HB_S32 setup_listen_socket(HB_U16 port);

////////////////////////////////////////////////////////////////////////////////
// 函数名：make_socket_non_blocking
// 描  述：设置网络文件描述符为非阻塞模式
// 参  数：[in] sock   网络文件描述符
// 返回值：成功返回1，出错返回0
// 说  明：
////////////////////////////////////////////////////////////////////////////////
HB_S32 make_socket_non_blocking(int sock);


////////////////////////////////////////////////////////////////////////////////
// 函数名：increase_send_buffer_to
// 描  述：增大网络发送缓冲区的大小
// 参  数：[in] sock           网络文件描述符
//         [in] requested_size 缓冲区的大小
// 返回值：成功返回缓冲区的大小，出错返回0
// 说  明：
////////////////////////////////////////////////////////////////////////////////
unsigned int increase_send_buffer_to(int socket, unsigned int requested_size);

////////////////////////////////////////////////////////////////////////////////
// 函数名：increase_receive_buffer_to
// 描  述：增大网络接收缓冲区的大小
// 参  数：[in] sock           网络文件描述符
//         [in] requested_size 网络缓冲区的大小
// 返回值：成功返回缓冲区的大小，出错返回0
// 说  明：
////////////////////////////////////////////////////////////////////////////////
unsigned int increase_receive_buffer_to(int socket, unsigned int requested_size);

HB_S32 check_port(HB_S32 port);
HB_S32 get_dev_ip(HB_CHAR *eth, HB_CHAR *ipaddr);
HB_S32 get_dev_mac(HB_CHAR *eth, HB_CHAR *mac);
////////////////////////////////////////////////////////////////////////////////
// 函数名：get_host_ip
// 描  述：获取主机IP
// 参  数：无
// 返回值：成功返回主机的IP地址，出错返回NULL
// 说  明：
////////////////////////////////////////////////////////////////////////////////
HB_CHAR *get_host_ip(HB_VOID);


////////////////////////////////////////////////////////////////////////////////
// 函数名：get_udp_server_port
// 描  述：获取服务器端的UDP端口
// 参  数：无
// 返回值：成功返回获取的UDP端口
// 说  明：
////////////////////////////////////////////////////////////////////////////////
int get_udp_server_port(void);

////////////////////////////////////////////////////////////////////////////////
// 函数名：setup_datagram_socket
// 描  述：设置报式套接字
// 参  数：[in] port              端口号
//         [in] make_non_blocking 是否是非阻塞模式的标识
// 返回值：成功返回网络文件描述符，出错返回ERR_GENERIC
// 说  明：
////////////////////////////////////////////////////////////////////////////////
int setup_datagram_socket(unsigned short port, HB_BOOL make_non_blocking);

#endif /* defined(__MapPort__thread_depend__) */
