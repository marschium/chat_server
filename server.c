#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>


#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

#define MAX_CLIENTS 64
#define BUF_SIZE 4096

typedef int client_id;

int clients[MAX_CLIENTS] = {0};
pthread_t threads[MAX_CLIENTS] = {0};
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

client_id add_client(int fd) {            
    pthread_mutex_lock(&clients_mutex);
    int id = -1;
    for(client_id i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == 0) {
            clients[i] = fd;
            id = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return id;
}

void remove_client(client_id id) {
    pthread_mutex_lock(&clients_mutex);
    if (id < MAX_CLIENTS) {
        close(clients[id]);
        clients[id] = 0;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// TODO better casting for args
void* handle_client(void* d) {
    int id = (int) d;
    int fd = clients[id];
    char* buf = malloc(sizeof(char) * BUF_SIZE);
    printf("starting handler for %d\n", id);
    while (1) {
        size_t rbytes = recv(fd, buf, BUF_SIZE, 0);
        if (rbytes > 0) {
            size_t str_end = MIN(rbytes + 1, BUF_SIZE - 1);
            buf[str_end] = 0;
            printf("msg from %d - %s", id, buf);

            pthread_mutex_lock(&clients_mutex);
            for(int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] != 0 && i != id) {                
                    send(clients[i], buf, str_end, 0);
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }
        else {
            printf("client %d disconnected", id);
            break;
        }
    }
    free(buf);
    remove_client(id);
    return NULL;
}

int main(int argc, char** argv) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in listen_addr, connect_addr;
    socklen_t connect_addr_len;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(8001);

    int _ = bind(socket_fd, (struct sockaddr*) &listen_addr, sizeof(listen_addr));
    _ = listen(socket_fd, 32);

    while (1) {
        connect_addr_len = sizeof(connect_addr);
        int client_fd = accept(socket_fd, (struct sockaddr*) &connect_addr, &connect_addr_len);
        char* client_addr = inet_ntoa(connect_addr.sin_addr);
        printf("client connected from %s\n", client_addr);
        int client_id = add_client(client_fd);
        if (client_id >= 0) {
            printf("client accepted\n");
            pthread_t tid;
            int res = pthread_create(&threads[client_id], NULL, &handle_client, (void*) client_id);
            // TODO res
        }
        else {
            printf("client declined\n");
            close(client_fd);
        }
    }

    return 0;
}