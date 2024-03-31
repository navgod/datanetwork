#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAXBUF 1024
#define QUEUE_SIZE 10
#define PORT 8081

void error_handling(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

char messageQueue[QUEUE_SIZE][MAXBUF]; // 10개 까지 수용
int queueStart = 0, queueEnd = 0;

void enqueue(char *message) {
    if ((queueEnd + 1) % 10 != queueStart) { 
        strcpy(messageQueue[queueEnd], message);
        queueEnd = (queueEnd + 1) % 10;
    }
}

int dequeue(char *message) {
    if (queueStart != queueEnd) { 
        strcpy(message, messageQueue[queueStart]);
        queueStart = (queueStart + 1) % 10;
        return 1; 
    }
    return 0; // 실패, 큐가 비어있음
}

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen error");

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            continue; 

        int pid = fork();
        if (pid == -1) {
            close(clnt_sock);
            continue; 
        } else if (pid == 0) {
            close(serv_sock);

            char buf[MAXBUF];
            int state = 0; // 0: 일반, 1: 메시지 수신 중

            while (read(clnt_sock, buf, MAXBUF) != -1) {
                if (state == 0 && strcmp(buf, "SEND\n") == 0) {
                    state = 1; // "SEND\n" 메시지 수신, 수신 모드로 전환
                    continue;
                } else if (state == 1 && strcmp(buf, "RECV\n") == 0) {
                    state = 0; // "RECV\n" 메시지 수신, 큐에 저장된 메시지 전송
                    while (dequeue(buf)) { // 큐가 비어있을 때까지 메시지 전송
                        write(clnt_sock, buf, strlen(buf));
                    }
                    continue;
                } else if (strcmp(buf, "ECHO_CLOSE\n") == 0) {
                    write(clnt_sock, "ECHO_CLOSE\n", strlen("ECHO_CLOSE\n")); // 응답 후 종료
                    break;
                }

                if (state == 1) {
                    enqueue(buf);
                }
            }

            close(clnt_sock);
            exit(EXIT_FAILURE);
        } else {
            close(clnt_sock);
        }
    }

    close(serv_sock);
    return 0;
}
