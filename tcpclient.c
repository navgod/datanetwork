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

    char responseBuffer[BUF_SIZE] = {0}; // 서버로부터의 응답을 저장할 버퍼
    while(1) {
        char response[BUF_SIZE];
        memset(response, 0, BUF_SIZE); // 응답 버퍼 초기화
        int read_len = read(sock, response, BUF_SIZE-1);
        if (read_len == -1) {
            error_handling("read() error!");
        } else if (read_len == 0) {
            break; // 서버가 연결을 닫음
        }
        response[read_len] = '\0'; // NULL로 문자열 종료

        strcat(responseBuffer, response); // 버퍼에 응답 추가

        // "\n" 기준으로 응답 분리
        char* token = strtok(responseBuffer, "\n");
        while(token != NULL) {
            printf("Server: %s\n", token);
            if (strcmp(token, "RECV") == 0) {
                // "RECV\n"을 받았으므로 처리 중단
                memset(responseBuffer, 0, BUF_SIZE); // 버퍼 초기화
                break;
            }
            token = strtok(NULL, "\n");
        }
        if (strcmp(token, "RECV") == 0) {
            break; // "RECV\n" 수신 시 루프 종료
        }
    }

    close(sock);
    return 0;
}
