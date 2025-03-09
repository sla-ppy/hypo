#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <complex.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "request.h"
#include "util.h"

#define BACKLOG 20

int initServer(const char* server_addr, const char* server_port, struct addrinfo* servinfo, int** server_sfd) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints)); // zero it
    hints.ai_family = AF_INET; // AF_INET - IPv4 || AF_INET6 - IPv6
    hints.ai_socktype = SOCK_STREAM;

    // 0. fill servinfo
    int rc = getaddrinfo(server_addr, server_port, &hints, &servinfo);
    if (rc != 0) {
        printTime(stderr);
        fprintf(stderr, "[ERR] getaddrinfo() error: %s\n", strerror(errno));
        return -1;
    } else {
        printTime(stdout);
        printf("[SERVER] Fetching address information successful\n");
    }

    // 1. create server socket
    int socket_sfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    *server_sfd = &socket_sfd;
    if (**server_sfd == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] socket() error: %s\n", strerror(errno));
        return -1;
    } else {
        printTime(stdout);
        printf("[SERVER] Listening socket created\n");
    }

    // 2. bind server socket to server port
    rc = bind(**server_sfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] bind() error: %s\n", strerror(errno));
        return -1;
    } else {
        printTime(stdout);
        printf("[SERVER] Succesfully bound to port: %s\n", server_port);
    }

    // 3. start listening on port
    rc = listen(**server_sfd, BACKLOG); // 20 - backlog specifies how many connections are allowed on the incomming queue
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] listen() error: %s\n", strerror(errno));
        return -1;
    } else {
        printTime(stdout);
        printf("[SERVER] Listening on: %s:%s\n", server_addr, server_port);
    }

    return 0;
}

int handleClient(int client_sfd) {
    // server data
    int bytes_received;
    int bytes_sent;
    char buffer[1024];

    // process browser request first so we know what we have to respond with
    bytes_received = recv(client_sfd, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] recv() error during transcieving: %s\n", strerror(errno));
    } else if (bytes_received == 0) {
        printTime(stdout);
        printf("[SERVER] Remote side has closed the connection\n");
        close(client_sfd);
        return 1;
    } else {
        printTime(stdout);
        printf("[CLIENT]\n%s", buffer);
    }

    char request_line[128];
    struct RequestLine request;
    request = getRequestLine(buffer, request_line);
    printf("GET Request Line: %s %s %s\r\n", request.method, request.url, request.protocol_version);

    const char* file_path = "content/index.html";
    FILE* file = fopen(file_path, "r");
    struct stat file_info;

    // get information about the file, namely file size
    int rc = stat(file_path, &file_info);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] stat() error: %s\n", strerror(errno));
        exit(1);
    }

    // allocate memory depending on file size, cast to char* which is already in bytes
    char* file_bytes = (char*)malloc(file_info.st_size);
    if (file_bytes == NULL) {
        printTime(stderr);
        fprintf(stderr, "[ERR] malloc() error: %s\n", strerror(errno));
        exit(1);
    }
    fread(file_bytes, 1, file_info.st_size, file); // read file into allocated memory

    // file_bytes[file_info.st_size - 1] = '\r';
    // file_bytes[file_info.st_size] = '\0';

    // TODO: continue from here! add headers
    // FIXME: content is being sent but its not rendered, missing headers?
    // Full response:
    // Status-Line
    // (General Header
    // Response Header
    // Entity Header)
    // CRLF
    // Entity-Body

    // General Header:
    // Date S10.6
    // Pragma S10.12
    //
    // Response Header:
    // Location S10.11
    // Server S10.14
    // WWW-Authenticate S10.16
    //
    // Entity Header
    // Allow S10.1
    // Content-Encoding S10.3
    // Content-Length S10.4
    // Content-Type S10.5
    // Expires S10.7
    // Last-Modified S10.10
    // extension-header => HTTP-header

    // Status line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    // Status line: HTTP/1.0 200

    // send back content
    bytes_sent = send(client_sfd, file_bytes, file_info.st_size, 0);
    if (bytes_sent == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] send() error during transcieving: %s\n", strerror(errno));
    }

    //  clear buffer, get ready to recv() next msg
    memset(file_bytes, 0, file_info.st_size);

    free(file_bytes); // do at some point

    return 0;
}

int main(int argc, char** argv) {
    struct addrinfo* servinfo = NULL;
    int* server_sfd_ptr = NULL;
    char* server_port = argv[1];

    checkArgument(argc, argv);

    int rc = initServer("127.0.0.1", server_port, servinfo, &server_sfd_ptr);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] initServer() error: %s\n", strerror(errno));
        return -1;
    }
    int server_sfd = *server_sfd_ptr;

    // client data
    socklen_t addr_size;
    struct sockaddr_storage client_addr;
    addr_size = sizeof client_addr;
    char ip_addr[INET_ADDRSTRLEN];
    int client_port;
    int client_sfd;

    while (true) {
        // 4. We need two loops here so the server doesn't close after losing its
        // client, this way client can reconnect, or in the future multiple
        // clients can connect
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

        while (true) {
            int rc = handleClient(client_sfd);
            if (rc == 1) {
                close(client_sfd);
                break;
            }
        }
    }

    close(server_sfd);
    freeaddrinfo(servinfo);
    return 0;
}
