#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAXBUF 1024
#define QUEUE_SIZE 10
#define PORT 8081

void error_handling(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

char messageQueue[QUEUE_SIZE][MAXBUF];
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
        return 1; // 성공적으로 메시지를 Dequeue
    }
    return 0; // 큐가 비어있음
}

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) error_handling("socket error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen error");

    printf("Server is running on port %d\n", PORT);

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1) continue;

        int pid = fork();
        if (pid == -1) {
            close(clnt_sock);
            continue;
        } else if (pid == 0) { // 자식 프로세스
            close(serv_sock);

            char buf[MAXBUF], messageBuffer[MAXBUF] = "";
            while (1) {
                memset(buf, 0, MAXBUF); // 버퍼 초기화
                ssize_t str_len = read(clnt_sock, buf, MAXBUF-1); // 메시지 읽기
                if (str_len <= 0) break; // 연결 종료 또는 오류
                buf[str_len] = '\0';
                char *token;
                char *rest = buf;

                while ((token = strtok_r(rest, "\n", &rest))) {
                    enqueue(token);
                    printf("token = %s\n", token);
                }
                if (queueEnd > 0 && strcmp(messageQueue[queueEnd - 1], "ECHO_CLOSE\n") == 0) {
                    write(clnt_sock, "ECHO_CLOSE\n", strlen("ECHO_CLOSE\n"));
                    break; // 연결 종료
                }

                puts(messageQueue[queueEnd - 1]);
                puts("----");
                // 마지막 메시지 확인
                if (queueEnd > 0 && strstr(messageQueue[queueEnd - 1], "RECV\n") != NULL) {
                    
                    while (dequeue(buf)) { // 큐가 비어있을 때까지 메시지 전송
                        write(clnt_sock, buf, strlen(buf));
                        write(clnt_sock, "\n", 1); // 메시지 사이에 개행 추가
                        printf("buf = %s\n", buf);
                    }
                    write(clnt_sock, "RECV\n", strlen("RECV\n")); // 모든 메시지 전송 후 RECV 응답
                    queueStart = 0; queueEnd = 0; // 큐 초기화
                }
            }

            close(clnt_sock);
            exit(EXIT_SUCCESS);
        } else {
            close(clnt_sock);
        }
    }

    close(serv_sock);
    return 0;
}
