#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// network libs
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

// user libs
#include "request.h"
#include "tcp.h"
#include "util.h"

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
                shutdown(client_sfd, 1);
                break;
            }
            // TODO: figure out how to send back and forth effectively
            // we signal the browser to stop loading with closing the socket
            shutdown(client_sfd, 1);
            break;
        }
    }

    close(server_sfd);
    freeaddrinfo(servinfo);
    return 0;
}
