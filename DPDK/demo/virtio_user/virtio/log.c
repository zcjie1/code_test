#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include "common.h"

static char* uint2str(uint16_t ether_type)
{
    switch(ether_type) {
        case SD_ETHER_TYPE_IPV4:
            return "IPv4";
        case SD_ETHER_TYPE_IPV6:
            return "IPv6";
        case SD_ETHER_TYPE_ARP:
            return "ARP";
        case SD_ETHER_TYPE_VLAN:
            return "VLAN";
        case SD_ETHER_TYPE_QINQ:
        case SD_ETHER_TYPE_QINQ1:
        case SD_ETHER_TYPE_QINQ2:
        case SD_ETHER_TYPE_QINQ3:
            return "QINQ";
        default:
            return "EXPERIMENTAL";
    }
}

static void get_elapsed_time(char *buffer, size_t size)
{
    struct timespec current_time;
    curr_time(&current_time);

    long seconds = current_time.tv_sec - start_time.tv_sec;
    long nanoseconds = current_time.tv_nsec - start_time.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += NS_PER_S;
    }
    snprintf(buffer, size, "%ld.%09ld", seconds, nanoseconds);
}

void show_packet(FILE *log, struct rte_ether_addr src_mac, 
    struct rte_ether_addr dst_mac, uint16_t ether_type, 
    uint32_t src_ip, uint32_t dst_ip, char *msg)
{
	char smac[RTE_ETHER_ADDR_FMT_SIZE];
	char dmac[RTE_ETHER_ADDR_FMT_SIZE];
    char time_buffer[TIME_BUFFER_SIZE];

    get_elapsed_time(time_buffer, TIME_BUFFER_SIZE);

    char *type = uint2str(ether_type);
	rte_ether_format_addr(smac, RTE_ETHER_ADDR_FMT_SIZE, &src_mac);
	rte_ether_format_addr(dmac, RTE_ETHER_ADDR_FMT_SIZE, &dst_mac);
    if(!src_ip || !dst_ip) {
        fprintf(log, "[%s] %s frame: [NULL | %s] => [NULL | %s]\n", time_buffer, type, smac, dmac);
        return;
    }

    char *sip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);
    char *dip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &src_ip, sip, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &dst_ip, dip, INET_ADDRSTRLEN);

	fprintf(log, "[%s] %s frame: <%s | %s> ==> <%s | %s> ", time_buffer, type, sip, smac, dip, dmac);
    fprintf(log, "%s\n", msg);

    free(sip);
    free(dip);
}

void show_result(FILE *log)
{
    uint64_t rx_num = result.rx_num;
    uint64_t rx_bytes = result.rx_bytes;

    struct timespec current_time;
    curr_time(&current_time);

    long seconds = current_time.tv_sec - start_time.tv_sec;
    long nanoseconds = current_time.tv_nsec - start_time.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += NS_PER_S;
    }
    long millionseconds = nanoseconds / 1000000;
    double real_seconds = seconds + (millionseconds / 1000);

    double rate = (double)rx_bytes / real_seconds;
    rate = rate / 1000; // MB

    fprintf(log, "\n==================Packets statistics=======================");
	fprintf(log, "\nPackets receive: %24""lu"
			"\nBytes receive: %26""lu"
            "\nTransmission Rate: %22"".2f""MB/s",
			rx_num,
			rx_bytes,
			rate);
	fprintf(log, "\n==========================================================\n\n");
	fflush(stdout);
}