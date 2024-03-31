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
    return 0; // 실패, 큐가 비어있음
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
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE); // 메시지 버퍼 초기화
        fputs("Input message (Q to send, bye to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "bye\n")) {
            write(sock, "ECHO_CLOSE\n", strlen("ECHO_CLOSE\n"));
            break; // 서버 응답 후 종료
        } else if (!strcmp(message, "Q\n")) {
            write(sock, "SEND\n", strlen("SEND\n"));
            while (dequeue(message)) {
                strcat(message, "\n"); // 메시지 끝에 구분자 추가
                write(sock, message, strlen(message));
            }

            write(sock, "RECV\n", strlen("RECV\n"));
            // 서버로부터 응답 받기 시작
            while(1) {
                char response[BUF_SIZE];
                int read_len = read(sock, response, BUF_SIZE-1);
                if (read_len == -1) {
                    error_handling("read() error!");
                } else if (read_len == 0) {
                    break; // 서버가 연결을 닫음
                }
                response[read_len] = '\0'; // NULL로 문자열 종료
                printf("Server: %s\n", response);
                if (strstr(response, "RECV\n") != NULL) { // "RECV\n" 수신 시 읽기 중단
                    break;
                }
            }
        } else {
            enqueue(message);
        }
    }

    close(sock);
    return 0;
}
