#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include<sys/types.h>

#define MAX_USER 32
#define MAX_QUEUE 32
#define SEND_BUFFER_LENGTH 1024
#define MAX_MESSAGE_LENGTH 1048576
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int client[MAX_USER];
int occupied[MAX_USER];

struct Info {
    int fd_recv;
    int id;
};
struct Message_queue {
    int head;
    int tail;
    int length;
    char message[MAX_QUEUE][MAX_MESSAGE_LENGTH];
} message_queue[MAX_USER];

void *read_chat(void*data){
    struct Info *info = (struct Info *) data;
    char message[MAX_MESSAGE_LENGTH];
    while (1) {
        for (; message_queue[info->id].length != 0;) {
            pthread_mutex_lock(&mutex);
            stpcpy(message, message_queue[info->id].message[message_queue[info->id].head]);
            message_queue[info->id].head = (message_queue[info->id].head + 1) % MAX_QUEUE;
            message_queue[info->id].length--;
            pthread_mutex_unlock(&mutex);
            unsigned long remain = strlen(message);
            long sent = 0;
            while (remain > 0) {
                sent = send(client[info->id], message + sent, remain, 0);
                if (sent == -1) {
                    perror("send");
                    exit(-1);
                }
                remain -= sent;
            }
        }
//        printf("Thread %d output ended with message %s.\n", info->id, message);
//        fflush(stdout);
    }
}
void *write_chat(void *data) {
    struct Info *info = (struct Info *) data;
    char buffer[SEND_BUFFER_LENGTH];
    ssize_t length;
    long head = 8;
    char message[MAX_MESSAGE_LENGTH];
    sprintf(message, "Client %d:", info->id);
    while (1) {
        length = recv(info->fd_recv, buffer, SEND_BUFFER_LENGTH - 12, 0);

        if (length <= 0) {
            occupied[info->id] = 0;
            close(client[info->id]);
            return 0;
        }
        printf("Thread %d received %s, sized %zd\n", info->id, buffer, length);
        fflush(stdout);
        int i;
        long signal = 0;
        long number = 0;
        for (i = 0; i < length; i++) {
            if (buffer[i] == '\n') {
                number = i - signal + 1;
                strncpy(message + head, buffer + signal, number);

                //printf("%d,%d,%d",message_queue[info->id].head,message_queue[info->id].tail,message_queue[info->id].length);
                fflush(stdout);
                for (int j = 0; j < MAX_USER; j++) {
                    if (occupied[j] && j != info->id) {
                        if (message_queue->length == MAX_QUEUE) {
                            perror("full queue");
                            exit(2);
                        }
                        printf("To be sent:%s\n", message);
                        pthread_mutex_lock(&mutex);
                        strcpy(message_queue[j].message[message_queue[j].tail], message);
                        message_queue[j].tail = (message_queue[j].tail + 1) % MAX_QUEUE;
                        message_queue[j].length++;
                        pthread_mutex_unlock(&mutex);
                    }
                }

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
                    if (message_queue->length == MAX_QUEUE) {
                        perror("full queue");
                        exit(2);
                    }
                    strcpy(message_queue[j].message[message_queue[j].tail], message);
                    message_queue[j].tail = (message_queue[j].tail + 1) % MAX_QUEUE;
                    message_queue[j].length++;
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }
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
    pthread_t thread[MAX_USER*2];
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
                pthread_create(&thread[2*i], NULL, read_chat, (void *) &info[i]);
                pthread_create(&thread[2*i+1], NULL, write_chat, (void *) &info[i]);
                break;
            }
        }
        if (i == MAX_USER) {
            perror("max user");
            return 1;
        }
    }
}
