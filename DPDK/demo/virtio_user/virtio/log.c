#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_dev.h>
#include <rte_bus.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include "type.h"

static char* uint2str(uint16_t ether_type) {
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

void show_packet(FILE *log, struct rte_ether_addr src_mac, 
    struct rte_ether_addr dst_mac, uint16_t ether_type, 
    uint32_t src_ip, uint32_t dst_ip, char *msg)
{
	char smac[RTE_ETHER_ADDR_FMT_SIZE];
	char dmac[RTE_ETHER_ADDR_FMT_SIZE];

    char *type = uint2str(ether_type);
	rte_ether_format_addr(smac, RTE_ETHER_ADDR_FMT_SIZE, &src_mac);
	rte_ether_format_addr(dmac, RTE_ETHER_ADDR_FMT_SIZE, &dst_mac);
    if(!src_ip || !dst_ip) {
        fprintf(log, "%s frame: [NULL | %s] => [NULL | %s]\n", type, smac, dmac);
        return;
    }

    char *sip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);
    char *dip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &src_ip, sip, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &dst_ip, dip, INET_ADDRSTRLEN);

	fprintf(log, "%s frame: [%s | %s] ==> [%s | %s]\n", type, sip, smac, dip, dmac);
}