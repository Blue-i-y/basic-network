#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

#define HOSTNAME "localhost"
#define PORT     5000
#define SIZE     256

void *listen_server(void *arg) {
    int sock = *(int *)arg;

    char buffer_recv[SIZE];
    while (1) {
        recv(sock, buffer_recv, sizeof(buffer_recv), 0);
        printf("message: %s\n", buffer_recv);
    }
}

void *listen_writer(void *arg) {
    int sock = *(int *)arg;
    int fd_read = *((int *)arg + 1);

    char buffer_recv[SIZE];
    while (1) {
        read(fd_read, buffer_recv, SIZE);

        send(sock, buffer_recv, SIZE, 0);

        printf("%s\n", buffer_recv);
    }
    
    return NULL;
}

int connect_server(char *hostname, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return -1;

    struct sockaddr_in sv_adr;
    sv_adr.sin_family = AF_INET;
    sv_adr.sin_port = PORT;
    if (inet_pton(sock, hostname, &sv_adr.sin_addr) <= 0)
        return -1;

    if (connect(sock, (struct sockaddr *)&sv_adr, sizeof(sv_adr)) == -1)
        return -1;

    return sock;
}

void choose_login(int s, char login[10]) {
    char resp;
    do {
        printf("login: ");
        scanf("%s", login);
        send(s, login, 10, 0);
        recv(s, &resp, 1, 0);
    } while (resp == 0);

}

int main() {
    // connection
    int sock = connect_server(HOSTNAME, PORT);
    if (sock == -1) {
        printf("erreur de connection\n");
        return 0;
    }

    // choisir un login
    char login[10];
    choose_login(sock, login);

    // creation d'un tube nomme
    int FIFO_login[2];
    pipe(FIFO_login);

    // creation de la semaphore
    int clef = rand();
    sem_t sem;
    sem_init(&sem, 1, clef);

    pid_t child = fork();
    if (child == 0) {
        char buffer[SIZE];
        int n = 5;
        sprintf(buffer, "gnome-terminal --bash -c \"writer %d\"", n);
        system(buffer);
    } else {
        pthread_t thread1, thread2;
        pthread_create(&thread1, NULL, listen_server, (void *)&sock);
        int args[2] = {sock, clef};
        pthread_create(&thread2, NULL, listen_writer, args);

        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
    }

    return 0;
}