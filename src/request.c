#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// network libs
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "request.h"
#include "util.h"

RequestLine getRequestLine(char* request_buffer, char* request_line_buffer) {
    // read data straight from buffer
    char* pch = strtok(request_buffer, "\r");
    request_line_buffer = pch;

    // cut string into substrings and get data from first line
    // FIXME: there is probably a better way to do this
    RequestLine request;
    pch = strtok(request_line_buffer, " ");
    for (int i = 0; pch != NULL; i++) {
        if (i == 0) {
            request.method = pch;
        } else if (i == 1) {
            request.url = pch;
        } else if (i == 2) {
            request.protocol_version = pch;
        }
        pch = strtok(NULL, " ");
    }

    return request;
}

Request receiveRequest(int client_sfd) {
    Request request;

    // server data
    int request_bytes;
    char request_buffer[2048];
    memset(request_buffer, 0, sizeof(request_buffer));
    request_bytes = recv(client_sfd, request_buffer, sizeof(request_buffer), 0);
    if (request_bytes == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] recv() error during transcieving: %s\n", strerror(errno));
    } else if (request_bytes == 0) {
        printTime(stdout);
        printf("[SERVER] Remote side has closed the connection\n");
        close(client_sfd);
        request.error = true;
        return request;
    } else {
        printTime(stdout);
        printf("[CLIENT]\n%s", request_buffer);
    }

    // get first line of the request
    char request_line_buffer[128];
    memset(request_line_buffer, 0, sizeof(request_line_buffer));
    request.request_line = getRequestLine(request_buffer, request_line_buffer);

    request.error = false;
    return request;
}

int handleClient(int client_sfd) {
    Request request;
    request = receiveRequest(client_sfd);
    if (request.error) {
        printTime(stderr);
        fprintf(stderr, "[ERR] Request method was not GET\n");
        return 1;
    }

    // TODO: handle depending on method
    // close if request method is not correct
    /*
    if (strcmp(request.request_line.method, "GET")) {
        printTime(stderr);
        fprintf(stderr, "[ERR] Request method was not GET\n");
        return 1;
    }
    */

    // process file requests accordingly, use concatenation
    char* root_path = "content";
    char* file_path = request.request_line.url;
    char file_path_buffer[PATH_MAX];
    memset(file_path_buffer, 0, sizeof(file_path_buffer));

    strcpy(file_path_buffer, root_path);
    if (strcmp(request.request_line.url, "/") == 0) {
        strcat(file_path_buffer, "/index.html");
    } else {
        strcat(file_path_buffer, file_path);
    }

    // check if file exists and is readable
    int rc = access(file_path_buffer, R_OK);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] access(): %s\n", strerror(errno));
        // TODO: return "HTTP-1.0 404 Not Found\r\n"
        return 1;
    }

    // get information about the file, namely file size
    FILE* file = fopen(file_path_buffer, "r");
    struct stat file_info;
    rc = stat(file_path_buffer, &file_info);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] stat() error: %s\n", strerror(errno));
        return 1;
    }

    char* status_line = "HTTP/1.0 200 OK\r\n";
    char* header_content = "Content-Type: text/html\r\nContent-Length: ";
    char* headers = (char*)malloc(strlen(header_content) + sizeof(file_info.st_size) + 2);
    sprintf(headers, "%s%lu%s", header_content, file_info.st_size, "\r\n");

    // TODO: continue by restructuring header, fetch Content-Type: from URL then rewrite the header system so it can be variable like Content-Length

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
        return 1;
    }
    memset(entity_body, 0, file_info.st_size);

    // read file into allocated memory
    fread(entity_body, 1, file_info.st_size, file);

    // file_bytes[file_info.st_size - 1] = '\r';
    // file_bytes[file_info.st_size] = '\0';

    char* empty_line = "\r\n";

    size_t status_headers_size = strlen(status_line) + strlen(headers) + 2;
    size_t full_response_size = status_headers_size + file_info.st_size;
    char* full_response = (char*)malloc(full_response_size);
    memset(full_response, 0, full_response_size);
    strcpy(full_response, status_line);
    strcat(full_response, headers);
    strcat(full_response, empty_line);
    memcpy(full_response + status_headers_size, entity_body, file_info.st_size);
    free(entity_body);
    full_response[strlen(full_response)] = '\0';

    fclose(file);

    // send back content
    int response_bytes = send(client_sfd, full_response, strlen(full_response), 0);
    if (response_bytes == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] send() error during transcieving: %s\n", strerror(errno));
        return 1;
    }
    free(full_response);

    return 0;
}
