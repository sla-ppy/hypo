#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <arpa/inet.h>
#include <complex.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 20

void printTime(FILE* restrict stream) {
    time_t t = time(NULL); // get local time in calendar format

    char time_buffer[70]; // format local time in calendar to custom format

    if (strftime(time_buffer, sizeof time_buffer, "[%Y.%m.%d - %H:%M:%S] ", localtime(&t))) {
        fprintf(stream, "%s", time_buffer);
    } else {
        fprintf(stderr, "[ERR] getTime() fails to retrietve formatted time\n");
    }
}

void handleClient(int rc, int client_sfd) {
    // SIMPLE RESPONSE
    // printf("Entity-Body"); // Response
    // [ Entity-Body ] => Body = *OCTET, where * is a Kleene Star => 0 or more OCTETs, 0 or more bytes of data

    const char* s_filepath = "/content.html";
    struct stat s_fileinfo; // return information about the file

    // get content info
    rc = stat(s_filepath, &s_fileinfo);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] stat() error: %s\n", strerror(errno));
        exit(1);
    }

    // prepare memory for content
    char* file_ptr = (char*)malloc(s_fileinfo.st_size); // allocate enough memory for the file size, make a ptr point to it, then typcast it to usable data type
    if (file_ptr == NULL) {
        printTime(stderr);
        fprintf(stderr, "[ERR] malloc() error: %s\n", strerror(errno));
        exit(1);
    }

    printf("%s\n", file_ptr);
    // CTRL+R - terminal reverse search

    // 1. read request from client
    // 2. respond with file accordingly

    // GET /example/stuff_1 HTTP/1.1//
    // POSt /resources/more_stuff/stuff_2 HTTP/1.1//
    //  / [] H

    // server data
    int bytes_received;
    int bytes_sent;

    while (true) {
        // 5. check if we need to recv anything first
        bytes_received = recv(client_sfd, file_ptr, s_fileinfo.st_size, 0); // TODO: what is the data size we are trying to send?
        if (bytes_received == -1) {
            printTime(stderr);
            fprintf(stderr, "[ERR] recv() error during transcieving: %s\n", strerror(errno));
        } else if (bytes_received == 0) {
            printTime(stdout);
            printf("[SERVER] Remote side has closed the connection\n");
            close(client_sfd);
            break;
        } else {
            printTime(stdout);
            printf("[CLIENT] %s | %d", file_ptr, bytes_received); // TODO: what data are we sending?
        }

        // 6. send content
        bytes_sent = send(client_sfd, file_ptr, strlen(file_ptr) + 1, 0);
        if (bytes_sent == -1) {
            printTime(stderr);
            fprintf(stderr, "[ERR] send() error during transcieving: %s\n", strerror(errno));
        }

        // 7. clear buffer, get ready to recv() next msg
        memset(file_ptr, 0, s_fileinfo.st_size);
    }

    //  free(file_ptr); // do at some point
}

int main(void) {
    const char* server_addr = "127.0.0.1";
    const char* server_port = "10250"; // http - 80

    struct addrinfo hints;
    struct addrinfo* servinfo;
    memset(&hints, 0, sizeof(hints)); // zero it
    hints.ai_family = AF_INET; // AF_INET - IPv4 || AF_INET6 - IPv6
    hints.ai_socktype = SOCK_STREAM;

    // 0. fill servinfo
    int rc = getaddrinfo(server_addr, server_port, &hints, &servinfo);
    if (rc != 0) { // rc => return code
        printTime(stderr);
        fprintf(stderr, "[ERR] getaddrinfo() error: %s\n", strerror(errno));
        return 1;
    }

    // 1. create server socket
    int server_sfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (server_sfd == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] socket() error: %s\n", strerror(errno));
        return 1;
    }

    // 2. bind server socket to server port
    rc = bind(server_sfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] bind() error: %s\n", strerror(errno));
        return 1;
    }

    // 3. start listening on port
    rc = listen(server_sfd, BACKLOG); // 20 - backlog specifies how many connections are allowed on the incomming queue
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] listen() error: %s\n", strerror(errno));
        return 1;
    } else {
        printTime(stdout);
        printf("[SERVER] Listening on: %s:%s\n", server_addr, server_port);
    }

    // client data
    socklen_t addr_size;
    struct sockaddr_storage client_addr;
    addr_size = sizeof client_addr;
    char ip_addr[INET_ADDRSTRLEN];
    int client_port;

    int client_sfd;

    while (true) {
        // 4. We need two loops here so the server doesn't close after losing its client, this way client can reconnect, or in the future multiple clients can connect
        client_sfd = accept(server_sfd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sfd == -1) {
            printTime(stderr);
            fprintf(stderr, "[ERR] accept() error: %s\n", strerror(errno));
            continue;
        }

        // get client data
        getpeername(client_sfd, (struct sockaddr*)&client_addr, &addr_size);
        struct sockaddr_in* s = (struct sockaddr_in*)&client_addr;
        client_port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ip_addr, sizeof(ip_addr));
        printTime(stdout);
        printf("[SERVER] Accepted connection: %s:%d\n", ip_addr, client_port);

        handleClient(rc, client_sfd);
    }

    close(server_sfd);
    freeaddrinfo(servinfo);
    return 0;
}
