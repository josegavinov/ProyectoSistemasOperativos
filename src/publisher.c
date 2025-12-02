#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define BUF_SIZE 512

double rand_range(double a, double b) {
    return a + (rand()/(double)RAND_MAX) * (b - a);
}

int main(int argc, char** argv) {
    if (argc < 5) {
        printf("Usage: %s <node_id> <gateway_host> <gateway_port> <interval_seconds>\n", argv[0]);
        return 1;
    }
    char* node_id = argv[1];
    char* gw = argv[2];
    int gwport = atoi(argv[3]);
    int interval = atoi(argv[4]);

    srand(time(NULL) ^ getpid());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(gwport);
    inet_pton(AF_INET, gw, &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Publisher %s connected to gateway %s:%d\n", node_id, gw, gwport);

    while (1) {
        double temp = rand_range(20.0, 30.0);
        double hum = rand_range(30.0, 80.0);
        char payload[256];

        // publish temperature
        snprintf(payload, sizeof(payload), "%s temperature %.2f", node_id, temp);
        char msg[512];
        snprintf(msg, sizeof(msg), "PUBLISH temperature %s\n", payload);
        send(sock, msg, strlen(msg), 0);

        // publish humidity
        snprintf(payload, sizeof(payload), "%s humidity %.2f", node_id, hum);
        snprintf(msg, sizeof(msg), "PUBLISH humidity %s\n", payload);
        send(sock, msg, strlen(msg), 0);

        sleep(interval);
    }
    close(sock);
    return 0;
}