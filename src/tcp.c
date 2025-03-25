#include <stddef.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>

#include "tcp.h"
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

    // ensure the port can be reused
    int optval = 1;
    setsockopt(**server_sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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
