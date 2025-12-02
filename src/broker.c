#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 9000
#define MAX_CLIENTS 100
#define BUF_SIZE 1024
#define MAX_TOPICS 100
#define MAX_SUBSCRIBERS_PER_TOPIC 100

typedef struct {
    char topic[128];
    int subs[MAX_SUBSCRIBERS_PER_TOPIC];
    int sub_count;
} topic_entry;

topic_entry topics[MAX_TOPICS];
int topic_count = 0;
pthread_mutex_t topics_lock = PTHREAD_MUTEX_INITIALIZER;

void add_subscriber(const char* topic, int sock) {
    pthread_mutex_lock(&topics_lock);
    int i;
    for (i = 0; i < topic_count; ++i) {
        if (strcmp(topics[i].topic, topic) == 0) {
            if (topics[i].sub_count < MAX_SUBSCRIBERS_PER_TOPIC) {
                topics[i].subs[topics[i].sub_count++] = sock;
            }
            pthread_mutex_unlock(&topics_lock);
            return;
        }
    }
    if (topic_count < MAX_TOPICS) {
        strncpy(topics[topic_count].topic, topic, sizeof(topics[topic_count].topic)-1);
        topics[topic_count].sub_count = 0;
        topics[topic_count].subs[topics[topic_count].sub_count++] = sock;
        topic_count++;
    }
    pthread_mutex_unlock(&topics_lock);
}

void remove_socket_from_all(int sock) {
    pthread_mutex_lock(&topics_lock);
    for (int i = 0; i < topic_count; ++i) {
        int j = 0;
        for (int k = 0; k < topics[i].sub_count; ++k) {
            if (topics[i].subs[k] != sock) {
                topics[i].subs[j++] = topics[i].subs[k];
            }
        }
        topics[i].sub_count = j;
    }
    pthread_mutex_unlock(&topics_lock);
}

void publish_to_topic(const char* topic, const char* payload) {
    pthread_mutex_lock(&topics_lock);
    for (int i = 0; i < topic_count; ++i) {
        if (strcmp(topics[i].topic, topic) == 0) {
            char msg[BUF_SIZE];
            snprintf(msg, sizeof(msg), "MSG %s %s\n", topic, payload);
            for (int j = 0; j < topics[i].sub_count; ++j) {
                int s = topics[i].subs[j];
                if (s <= 0) continue;
                send(s, msg, strlen(msg), 0);
            }
            break;
        }
    }
    pthread_mutex_unlock(&topics_lock);
}

void* client_handler(void* arg) {
    int sock = *(int*)arg;
    free(arg);
    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = recv(sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = 0;
        // process line by line (could be multiple commands)
        char *saveptr = NULL;
        char *line = strtok_r(buf, "\n", &saveptr);
        while (line) {
            // Trim leading spaces
            while (*line == ' ') line++;
            if (strncmp(line, "SUBSCRIBE ", 10) == 0) {
                char topic[128];
                if (sscanf(line + 10, "%127s", topic) == 1) {
                    add_subscriber(topic, sock);
                    char ack[256];
                    snprintf(ack, sizeof(ack), "SUBACK %s\n", topic);
                    send(sock, ack, strlen(ack), 0);
                }
            } else if (strncmp(line, "PUBLISH ", 8) == 0) {
                char topic[128], payload[800];
                if (sscanf(line + 8, "%127s %799[^\n]", topic, payload) >= 1) {
                    // payload may be empty or with spaces
                    publish_to_topic(topic, payload);
                }
            } else {
                // ignore unknown
            }
            line = strtok_r(NULL, "\n", &saveptr);
        }
    }

    // client disconnected
    remove_socket_from_all(sock);
    close(sock);
    return NULL;
}

int main(int argc, char** argv) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { perror("bind"); exit(1); }
    if (listen(listenfd, 10) < 0) { perror("listen"); exit(1); }

    printf("Broker listening on port %d\n", port);

    while (1) {
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        if (connfd < 0) { perror("accept"); continue; }
        printf("New connection from %s:%d (fd=%d)\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), connfd);
        pthread_t tid;
        int *pconn = malloc(sizeof(int));
        *pconn = connfd;
        pthread_create(&tid, NULL, client_handler, pconn);
        pthread_detach(tid);
    }
    close(listenfd);
    return 0;
}