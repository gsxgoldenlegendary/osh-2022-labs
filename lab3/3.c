#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define SEND_BUFFER_LENGTH 1024
#define MAX_MESSAGE_LENGTH 1048576
#define MAX_USER 32

int occupied[MAX_USER];
int client[MAX_USER];

void handle_chat(int id) {
    char buffer[SEND_BUFFER_LENGTH];
    ssize_t length;
    int head = 8;
    bool first_time = true;
    char message[MAX_MESSAGE_LENGTH];
    sprintf(message, "Client %d:", id);
    while (1) {
        length = recv(client[id], buffer, SEND_BUFFER_LENGTH - 12, 0);
        if (length <= 0) {
            if (first_time) {
                occupied[id] = 0;
                close(client[id]);
                return;
            }
            return;
        }
        first_time = false;
        int signal = 0;
        int number = 0;
        for (int i = 0; i < length; i++) {
            if (buffer[i] == '\n') {
                number = i - signal + 1;
                strncpy(message + head, buffer + signal, number);
                for (int j = 0; j < MAX_USER; j++) {
                    if (occupied[j] && j != id) {
                        int remain = head + number;
                        int sended = 0;
                        while (remain > 0) {
                            sended = send(client[j], message + sended, remain, 0);
                            if (sended == -1) {
                                perror("send");
                                exit(-1);
                            }
                            remain -= sended;
                        }
                    }
                }
                signal = i + 1;
                head = 10;
            }
        }
        if (signal != length) {
            number = length - signal;
            strncpy(message + head, buffer + signal, number);
            for (int j = 0; j < MAX_USER; j++) {
                if (occupied[j] && j != id) {
                    int remain = head + number;
                    int sended = 0;
                    while (remain > 0) {
                        sended = send(client[j], message + sended, remain, 0);
                        if (sended == -1) {
                            perror("send");
                            exit(-1);
                        }
                        remain -= sended;
                    }
                }
            }
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
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd, MAX_USER)) {
        perror("listen");
        return 1;
    }
    int i;
    int fd_num = fd;
    fd_set clients;
    while (1) {
        FD_ZERO(&clients);
        FD_SET(fd, &clients);
        for (i = 0; i < MAX_USER; i++) {
            if (occupied[i]) { FD_SET(client[i], &clients); }
        }
        if (select(fd_num + 1, &clients, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(fd, &clients)) {
                int temp_client = accept(fd, NULL, NULL);
                if (temp_client == -1) {
                    perror("accept");
                    return 1;
                }
                fcntl(temp_client, F_SETFL, fcntl(temp_client, F_GETFL, 0) | O_NONBLOCK);
                if (fd_num < temp_client) {
                    fd_num = temp_client;
                }
                for (i = 0; i < MAX_USER; i++) {
                    if (!occupied[i]) {
                        occupied[i] = 1;
                        client[i] = temp_client;
                        break;
                    }
                }
            } else {
                for (i = 0; i < MAX_USER; i++) {
                    if (occupied[i] && FD_ISSET(client[i], &clients)) {
                        handle_chat(i);
                    }
                }
            }
        }
    }
}

