#ifndef __RAW_PACKET_H__
#define __RAW_PACKET_H__

#include <stdint.h>
#include <rte_mbuf.h>


/**
 * 申请用于服务器模式的原始数据包资源
 * 
 * @param mbuf_pool 内存池句柄，用于分配rte_mbuf结构
 * @param mbufs 指向rte_mbuf结构数组的指针，用于存储分配的内存缓冲区
 * @param count 要分配的rte_mbuf结构的数量
 * 
 * @return 返回0表示分配成功，非0表示分配失败
 */
int
raw_packet_alloc_server(struct rte_mempool *mbuf_pool, 
        struct rte_mbuf **mbufs, unsigned int count);


/**
 * 服务器端释放并回收内存缓冲区
 * 
 * @param m 待释放的mbuf结构指针。
 */
void
raw_packet_free_server(struct rte_mbuf **m, unsigned int count);

/**
 * 客户端分配内存缓冲区以创建原始数据包
 * 
 * @param port_id 端口ID。
 * @param queue_id 队列ID。
 * @param rx_pkts 待分配的mbuf结构指针数组。
 * @param nb_pkts 待分配数据包的数量。
 * 
 * @return 返回实际分配的空闲数据包数量
 */
int
raw_packet_alloc_client(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **rx_pkts, const uint16_t nb_pkts);

/**
 * 客户端释放并回收内存缓冲区。
 * 
 * @param port_id 端口ID。
 * @param queue_id 队列ID。
 * @param rx_pkts 待释放的mbuf结构指针数组。
 * @param nb_pkts 数组中数据包的数量。
 * 
 */
void 
raw_packet_free_client(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **tx_pkts, const uint16_t nb_pkts);

/**
 * 发送一组数据包到指定的端口和队列。
 * 
 * @param port_id 端口ID。
 * @param queue_id 队列ID。
 * @param tx_pkts 发送的mbuf结构指针数组。
 * @param nb_pkts 数组中数据包的数量。
 * 
 * @return 返回实际发送的数据包数量
 */
int 
raw_packet_send(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **tx_pkts, const uint16_t nb_pkts);

/**
 * 从指定的端口和队列接收一组数据包。
 * 
 * @param port_id 端口ID。
 * @param queue_id 队列ID。
 * @param rx_pkts 接收的mbuf结构指针数组。
 * @param nb_pkts 数组中数据包的数量。
 * 
 * @return 返回实际接收到的数据包数量
 */
int 
raw_packet_recv(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **rx_pkts, const uint16_t nb_pkts);

#endif // !__RAW_PACKET_H__