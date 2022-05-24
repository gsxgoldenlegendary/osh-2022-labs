#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>


#define MAX_MESSAGE_LENGTH 1048576
#define SEND_BUFFER_LENGTH 1024

struct Pipe {
    int fd_send;
    int fd_recv;
};

void *handle_chat(void *data) {
    struct Pipe *pipe = (struct Pipe *) data;
    char message[MAX_MESSAGE_LENGTH + 8] = "Message:";
    char buffer[SEND_BUFFER_LENGTH];
    ssize_t length;
    long head = 8;
    while (1) {
        length = recv(pipe->fd_send, buffer, SEND_BUFFER_LENGTH - 12, 0);
        int i;
        long signal = 0;
        long number = 0;
        for (i = 0; i < length; i++) {
            if (buffer[i] == '\n') {
                number = i - signal + 1;
                strncpy(message + head, buffer + signal, number);
                long remain = head + number;
                long sent = 0;
                while (remain > 0) {
                    sent = send(pipe->fd_recv, message + sent, remain, 0);
                    if (sent == -1) {
                        perror("send");
                        exit(-1);
                    }
                    remain -= sent;
                }
                signal = i + 1;
                head = 8;
            }
        }
        if (signal != length) {
            number = length - signal;
            strncpy(message + head, buffer + signal, number);
//            head = head + number;
            long remain = head + number;
            long sent = 0;
            while (remain > 0) {
                sent = send(pipe->fd_recv, message + sent, remain, 0);
                if (sent == -1) {
                    perror("send");
                    exit(-1);
                }
                remain -= sent;
            }
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
    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1) {
        perror("accept");
        return 1;
    }
    pthread_t thread1, thread2;
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;
    pthread_create(&thread1, NULL, handle_chat, (void *) &pipe1);
    pthread_create(&thread2, NULL, handle_chat, (void *) &pipe2);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}
