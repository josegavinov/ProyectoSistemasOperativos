#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <topic> <broker_host> <broker_port>\n", argv[0]);
        return 1;
    }
    char* topic = argv[1];
    char* broker = argv[2];
    int bport = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(bport);
    inet_pton(AF_INET, broker, &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "SUBSCRIBE %s\n", topic);
    send(sock, cmd, strlen(cmd), 0);
    printf("Subscribed to topic '%s' at broker %s:%d\n", topic, broker, bport);

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = recv(sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = 0;
        // Expect "MSG <topic> <payload>\n"
        printf("%s", buf);
        fflush(stdout);
    }
    close(sock);
    return 0;
}