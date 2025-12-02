#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define DEFAULT_LISTEN 8000
#define DEFAULT_BROKER_HOST "127.0.0.1"
#define DEFAULT_BROKER_PORT 9000
#define BUF_SIZE 1024

char broker_host[128];
int broker_port;

int connect_to_broker() {
    int sock;
    struct sockaddr_in serv;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return -1; }
    serv.sin_family = AF_INET;
    serv.sin_port = htons(broker_port);
    inet_pton(AF_INET, broker_host, &serv.sin_addr);
    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect to broker");
        close(sock);
        return -1;
    }
    return sock;
}

void* pub_handler(void* arg) {
    int client = *(int*)arg;
    free(arg);
    char buf[BUF_SIZE];
    ssize_t n;
    int broker_sock = connect_to_broker();
    if (broker_sock < 0) {
        close(client);
        return NULL;
    }
    // Keep the broker connection open per publisher handler.
    while ((n = recv(client, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = 0;
        // expect PUBLISH <topic> <payload>\n
        // forward raw to broker
        send(broker_sock, buf, strlen(buf), 0);
    }
    close(client);
    close(broker_sock);
    return NULL;
}

int main(int argc, char** argv) {
    int listen_port = (argc > 1) ? atoi(argv[1]) : DEFAULT_LISTEN;
    strncpy(broker_host, (argc > 2) ? argv[2] : DEFAULT_BROKER_HOST, sizeof(broker_host)-1);
    broker_port = (argc > 3) ? atoi(argv[3]) : DEFAULT_BROKER_PORT;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(1); }
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(listen_port);
    serv.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("bind"); exit(1); }
    if (listen(listenfd, 10) < 0) { perror("listen"); exit(1); }
    printf("Gateway listening for publishers on port %d and forwarding to broker %s:%d\n", listen_port, broker_host, broker_port);

    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int client = accept(listenfd, (struct sockaddr*)&cli, &clilen);
        if (client < 0) { perror("accept"); continue; }
        printf("Publisher connected: %s:%d (fd=%d)\n", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port), client);
        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = client;
        pthread_create(&tid, NULL, pub_handler, pclient);
        pthread_detach(tid);
    }
    close(listenfd);
    return 0;
}