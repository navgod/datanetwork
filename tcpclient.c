#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define QUEUE_SIZE 10
#define PORT 8081

void error_handling(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

char messageQueue[QUEUE_SIZE][BUF_SIZE];
int queueStart = 0, queueEnd = 0;

void enqueue(char *message) {
    if ((queueEnd + 1) % QUEUE_SIZE != queueStart) {
        strcpy(messageQueue[queueEnd], message);
        queueEnd = (queueEnd + 1) % QUEUE_SIZE;
    } else {
        printf("Queue is full. Cannot enqueue message.\n");
    }
}

int dequeue(char *message) {
    if (queueStart != queueEnd) {
        strcpy(message, messageQueue[queueStart]);
        queueStart = (queueStart + 1) % QUEUE_SIZE;
        return 1;
    }
    return 0;
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");
    else
        puts("Connected to the server...");

    while(1) {
        char message[BUF_SIZE] = {0};
        fputs("Input message (Q to send, bye to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (strcmp(message, "bye\n") == 0) {
            write(sock, "ECHO_CLOSE\n", strlen("ECHO_CLOSE\n"));
            // Wait for server's confirmation before closing.
            while(1) {
                int read_len = read(sock, message, BUF_SIZE - 1);
                if (read_len <= 0) break;
                message[read_len] = '\0';
                if (strstr(message, "ECHO_CLOSE\n") == 0) {
                    printf("Server closed connection.\n");
                    break;
                }
            }
            break;
        } else if (strcmp(message, "Q\n") == 0) {
            while (dequeue(message)) { // 큐가 비어있을 때까지 메시지 전송
                write(sock, message, strlen(message));
                write(sock, "\n", 1); // 메시지 사이에 개행 추가
            }
            write(sock, "RECV\n", 5);
            queueStart = 0; queueEnd = 0; // Reset the queue

            char buf[BUF_SIZE] = "";
            while(1) {
                memset(buf, 0, BUF_SIZE); // 버퍼 초기화
                ssize_t str_len = read(sock, buf, BUF_SIZE-1); // 메시지 읽기
                if (str_len <= 0) break; // 연결 종료 또는 오류
                buf[str_len] = '\0';
                char *token;
                char *rest = buf;

                while ((token = strtok_r(rest, "\n", &rest))) {
                    enqueue(token);
                }

                if (queueEnd > 0 && strstr(messageQueue[queueEnd - 1], "RECV") != NULL) {
                    
                    while (dequeue(buf)) { // 큐가 비어있을 때까지 메시지 전송
                        printf("Server Echo = %s\n", buf);
                    }
                    queueStart = 0; queueEnd = 0; // 큐 초기화
                    break;
                }
            }
        } else {
            message[strlen(message) - 1] = '\0'; // Remove newline
            enqueue(message);
        }
    }

    close(sock);
    return 0;
}
