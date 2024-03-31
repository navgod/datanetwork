#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define PORT 8081

void error_handling(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUF_SIZE];
    int str_len;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버의 IP 주소 설정
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("connect error!");
    } else {
        puts("Connected to the server");
    }

    while(1) {
        fputs("Input message(Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        if (!strcmp(message, "Q\n") || !strcmp(message, "q\n")) {
            break;
        }

        write(sock, message, strlen(message)); // 서버로 메시지 전송

        if (!strcmp(message, "ECHO_CLOSE\n")) {
            read(sock, message, BUF_SIZE); // 서버로부터의 응답 수신
            printf("Message from server: %s\n", message);
            break; // 서버로부터 "ECHO_CLOSE" 응답을 받으면 종료
        }
    }

    close(sock);
    return 0;
}
