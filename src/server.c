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
    int request_bytes;
    int response_bytes;
    char request_buffer[1024];

    request_bytes = recv(client_sfd, request_buffer, sizeof(request_buffer), 0);
    if (request_bytes == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] recv() error during transcieving: %s\n", strerror(errno));
    } else if (request_bytes == 0) {
        printTime(stdout);
        printf("[SERVER] Remote side has closed the connection\n");
        close(client_sfd);
        return 1;
    } else {
        printTime(stdout);
        printf("[CLIENT]\n%s", request_buffer);
    }

    // get first line of the request
    char request_line[128];
    struct RequestLine request;
    request = getRequestLine(request_buffer, request_line);
    // use request.method, request.url, request.protocol_version

    // process file requests accordingly, use concatenation
    char* root_path = "content/";
    char* file_name;

    if (strcmp(request.url, "/") == 0) {
        file_name = "index.html";
    }
    /*
    else if () {
        int rc = access(root_path + file_name);
        if (rc == -1) {
            printTime(stderr);
            fprintf(stderr, "[ERR] access(): %s\n", strerror(errno));
            // return "HTTP-1.0 404 Not Found\r\n"
        }
    }
    */

    char* file_path = (char*)malloc(strlen(root_path) + strlen(file_name) + 1);
    strcpy(file_path, root_path);
    strcat(file_path, file_name);
    strcat(file_path, "\0");

    // get information about the file, namely file size
    FILE* file = fopen(file_path, "r");
    struct stat file_info;
    int rc = stat(file_path, &file_info);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] stat() error: %s\n", strerror(errno));
        exit(1);
    }

    char* status_line = "HTTP/1.1 200 OK\r\n";
    char* header_content = "Content-Type: text/html\r\nContent-Length: ";
    char* headers = (char*)malloc(strlen(header_content) + sizeof(file_info.st_size) + 2);
    sprintf(headers, "%s%lu%s", header_content, file_info.st_size, "\r\n");

    /* Recommended by Adler
    char const *headers = "... %s%lu%s ...;
    int required =  snprintf(NULL, 0, headers, ...);
    if(required < 0) { ... }
    char *actual_headers = malloc(required + ...);
    if(!actual_headers) { ... }
    (void)sprintf(actual_headers, ...);
    */

    // allocate memory depending on file size, cast to char* which is already in bytes
    char* entity_body = (char*)malloc(file_info.st_size);
    if (entity_body == NULL) {
        printTime(stderr);
        fprintf(stderr, "[ERR] malloc() error: %s\n", strerror(errno));
        exit(1);
    }

    // read file into allocated memory
    fread(entity_body, 1, file_info.st_size, file);

    // file_bytes[file_info.st_size - 1] = '\r';
    // file_bytes[file_info.st_size] = '\0';

    char* empty_line = "\r\n";

    char* full_response = (char*)malloc(strlen(status_line) + strlen(headers) + strlen(empty_line) + strlen(entity_body) + 1);
    strcpy(full_response, status_line);
    strcat(full_response, headers);
    strcat(full_response, empty_line);
    strcat(full_response, entity_body);
    full_response[strlen(full_response)] = '\0';

    // send back content
    response_bytes = send(client_sfd, full_response, strlen(full_response), 0);
    if (response_bytes == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] send() error during transcieving: %s\n", strerror(errno));
    }

    // clear buffer, get ready to recv() next msg
    memset(entity_body, 0, file_info.st_size);
    memset(full_response, 0, strlen(full_response));

    // FIXME: add all free-able variable names to an array and free by loop?
    free(full_response);
    free(entity_body);
    free(file_path);

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
