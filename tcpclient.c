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
        memset(message, 0, BUF_SIZE);
        fputs("Input message (Q to send, bye to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "bye\n")) {
            write(sock, "ECHO_CLOSE\n", strlen("ECHO_CLOSE\n"));
            if (read(sock, message, BUF_SIZE) != -1) {
                printf("Server: %s\n", message);
            }
            break; // 서버 응답 후 종료
        } else if (!strcmp(message, "Q\n")) {
            write(sock, "SEND\n", strlen("SEND\n"));
            while (!dequeue(message)) {
                write(sock, message, strlen(message));
            }
            write(sock, "RECV\n", strlen("RECV\n"));
        } else {
            enqueue(message);
        }
    }

    close(sock);
    return 0;
}
