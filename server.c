#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <string.h>     // memset
#include <unistd.h>     // close
#include <arpa/inet.h>  // sockaddr_in, htons, etc.
#include <pthread.h>    // pthreads for multithreading

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int client_sockets[MAX_CLIENTS];  // shared list of connected clients
int client_count = 0;
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;  // protects the list above

// Send a message to every connected client
void broadcast_message(const char *message, int sender_fd){
    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] != sender_fd){ // don't echo back to the sender
            write(client_sockets[i], message, strlen(message));
        }
    }
    pthread_mutex_unlock(&client_lock);
}

// Remove a client from the shared list once they disconnect
void remove_client(int client_fd){
    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < client_count; i++){
        if (client_sockets[i] == client_fd){
            client_sockets[i] = client_sockets[client_count - 1]; // replace with last client
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);
}

// This function runs in its own thread, one per connected client
void* handle_client(void *arg){
    int client_fd = *(int*)arg;
    free(arg);  // we malloc'd this when creating the thread, clean it up

    char buffer[BUFFER_SIZE];

    while (1){
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        if(bytes_read <= 0){
            printf("Client disconnected (fd %d)\n", client_fd);
            break;
        }
        buffer[bytes_read] = '\0';
        printf("Recieved: %s\n", buffer);

        broadcast_message(buffer, client_fd);
    }

    close(client_fd);
    remove_client(client_fd);
    return NULL;
}


int main(){
    //1. Create a socket
    // AF_INET: IPv4, SOCK_STREAM: TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("socket failed");
        exit(1);
    }

    // Allow quick restart of the server without "address already in use"
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //2. Define the address to bind to
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // listen on all local interfaces
    address.sin_port = htons(PORT); // port 8080, converted to network byte order


    //3. Bind the socket to the address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(1);
    }

    //4. Start listening for incoming connections (5 = max queued connections)
    if (listen(server_fd, 5) < 0){
        perror("listen failed");
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1){
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_fd < 0){
            perror("accept failed");
            continue;
        }
    

        printf("Client connected %s:%d (fd %d)\n",
                inet_ntoa(client_address.sin_addr),
                ntohs(client_address.sin_port),
                client_fd);
        
        // Add this client to the shared list        
        pthread_mutex_lock(&client_lock);
        if (client_count < MAX_CLIENTS){
            client_sockets[client_count++] = client_fd;
        } else{
            printf("Max clients reached, rejecting connection. \n");
            close(client_fd);
            pthread_mutex_unlock(&client_lock);
            continue;
        }
        pthread_mutex_unlock(&client_lock);

        // Spin up a thread to handle this client independently
        int* client_fd_ptr = malloc(sizeof(int));
        *client_fd_ptr = client_fd;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_fd_ptr);
        pthread_detach(thread_id);  // thread cleans itself up when done
        }
    close(server_fd);
    return 0;
    }