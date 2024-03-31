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

char messageQueue[QUEUE_SIZE][MAXBUF]; // 메시지 큐
int queueStart = 0, queueEnd = 0; // 큐 시작점과 끝점

void enqueue(char *message) {
    if ((queueEnd + 1) % QUEUE_SIZE != queueStart) { // 큐가 가득 찼는지 확인
        strcpy(messageQueue[queueEnd], message);
        queueEnd = (queueEnd + 1) % QUEUE_SIZE;
    } else {
        printf("Queue is full. Cannot enqueue message.\n");
    }
}

int dequeue(char *message) {
    if (queueStart != queueEnd) { // 큐가 비어있지 않은지 확인
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

    printf("Server is running on port %d\n", PORT);

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            continue;

        int pid = fork();
        if (pid == -1) {
            close(clnt_sock);
            continue;
        } else if (pid == 0) { // 자식 프로세스
            close(serv_sock);

            char buf[MAXBUF];
            int state = 0; // 0: 일반, 1: 메시지 수신 모드

            while (1) {
                memset(buf, 0, MAXBUF); // 버퍼 초기화
                int str_len = read(clnt_sock, buf, MAXBUF-1); // 메시지 읽기
                if (str_len == -1) break; // 읽기 오류 발생
                buf[str_len] = '\0'; // NULL 문자로 문자열 종료

                // 클라이언트로부터 받은 메시지를 출력
                printf("Received from client: %s\n", buf);

                if (!state && strcmp(buf, "SEND\n") == 0) {
                    state = 1; // 메시지 수신 모드로 전환
                } else if (state && strcmp(buf, "RECV\n") == 0) {
                    while (dequeue(buf)) { // 큐에 있는 메시지를 모두 전송
                        write(clnt_sock, buf, strlen(buf));
                    }
                    state = 0; // 일반 모드로 전환
                } else if (strcmp(buf, "ECHO_CLOSE\n") == 0) {
                    write(clnt_sock, "ECHO_CLOSE\n", strlen("ECHO_CLOSE\n")); // 클라이언트에게 종료 응답
                    break;
                } else if (state) {
                    enqueue(buf); // 메시지 수신 모드에서는 메시지를 큐에 추가
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
