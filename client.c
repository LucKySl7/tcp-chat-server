#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Runs in its own thread: waits for and prints messages from the server
void* receive_messages(void* arg) {
    int sock_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_read = read(sock_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            printf("Disconnected from server.\n");
            exit(0); // server closed or errored
        }
        buffer[bytes_read] = '\0';
        printf("\n%s\n", buffer);
        printf("Enter message (or 'quit'): ");
        fflush(stdout); // force prompt to print immediately
    }
    return NULL;
}

int main() {
    // 1. Create a socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    // 2. Define the server's address to connect to
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert "127.0.0.1" into binary address format
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("invalid address");
        exit(1);
    }

    // 3. Connect to the server
    if (connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("connect failed");
        exit(1);
    }

    printf("Connected to server.\n");

    // Start a thread dedicated to receiving messages
    pthread_t receiver_thread;
    pthread_create(&receiver_thread, NULL, receive_messages, &sock_fd);
    pthread_detach(receiver_thread); // let it clean itself up

    // Main thread's only job now: read user input and send it
    char input[BUFFER_SIZE];
    while (1) {
        printf("Enter message (or 'quit'): ");
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0'; // strip trailing newline

        if (strcmp(input, "quit") == 0) break;

        write(sock_fd, input, strlen(input));
    }

    close(sock_fd);
    return 0;
}