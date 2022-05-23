#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_USER 32
#define SEND_BUFFER_LENGTH 1024
#define MAX_MESSAGE_LENGTH 1048576
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int client[MAX_USER];
int occupied[MAX_USER];

struct Info {
    int fd_recv;
    int id;
};

void *handle_chat(void *data) {
    struct Info *info = (struct Info *) data;
    char buffer[SEND_BUFFER_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
    sprintf(message, "Client %d: ", info->id);
    ssize_t length;
    long head = 8;
    while (1) {
        length = recv(info->fd_recv, buffer, SEND_BUFFER_LENGTH - 12, 0);
        if (length <= 0) {
            occupied[info->id] = 0;
            close(client[info->id]);
            return 0;
        }
        int i;
        long signal = 0;
        long number = 0;
        for (i = 0; i < length; i++) {
            if (buffer[i] == '\n') {
                number = i - signal + 1;
                strncpy(message + head, buffer + signal, number);
                pthread_mutex_lock(&mutex);
                for (int j = 0; j < MAX_USER; j++) {
                    if (occupied[j] && j != info->id) {
                        long remain = head + number;
                        long sent = 0;
                        while (remain > 0) {
                            sent = send(client[j], message + sent, remain, 0);
                            if (sent == -1) {
                                perror("send");
                                exit(-1);
                            }
                            remain -= sent;
                        }
                    }
                }
                pthread_mutex_unlock(&mutex);
                signal = i + 1;
                head = 8;
            }
        }
        if (signal != length) {
            number = length - signal;
            strncpy(message + head, buffer + signal, number);
            pthread_mutex_lock(&mutex);
            for (int j = 0; j < MAX_USER; j++) {
                if (occupied[j] && j != info->id) {
                    long remain = head + number;
                    long sent = 0;
                    while (remain > 0) {
                        sent = send(client[j], message + sent, remain, 0);
                        if (sent == -1) {
                            perror("send");
                            exit(-1);
                        }
                        remain -= sent;
                    }
                    send(client[j],"\n",1,0);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2)) {
        perror("listen");
        return 1;
    }
    pthread_t thread[MAX_USER];
    struct Info info[MAX_USER];
    for (;;) {
        int temp_client = accept(fd, NULL, NULL);
        if (temp_client == -1) {
            perror("accept");
            return 1;
        }
        int i;
        for (i = 0; i < MAX_USER; i++) {
            if (occupied[i] == 0) {
                occupied[i] = 1;
                client[i] = temp_client;
                info[i].fd_recv = temp_client;
                info[i].id = i;
                pthread_create(&thread[i], NULL, handle_chat, (void *) &info[i]);
                break;
            }
        }
        if (i == MAX_USER) {
            perror("max user");
            return 1;
        }
    }
        return 0;

//    int fd1 = accept(fd, NULL, NULL);
//    int fd2 = accept(fd, NULL, NULL);
//    if (fd1 == -1 || fd2 == -1) {
//        perror("accept");
//        return 1;
//    }
//    pthread_t thread1, thread2;
//    struct Pipe pipe1;
//    struct Pipe pipe2;
//    pipe1.fd_send = fd1;
//    pipe1.fd_recv = fd2;
//    pipe2.fd_send = fd2;
//    pipe2.fd_recv = fd1;
//    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1);
//    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
//    pthread_join(thread1, NULL);
//    pthread_join(thread2, NULL);
    return 0;
}
