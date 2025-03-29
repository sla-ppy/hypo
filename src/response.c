#include "response.h"

int sendFullResponse(const int client_sfd, char* buffer, const size_t buffer_size) {
    int bytes = send(client_sfd, buffer, buffer_size, 0);
    if (bytes == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] send() error during transcieving: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int constructFullResponse(const int client_sfd, char* content_type, FILE* file, struct stat file_info) {
    // response status line
    const char* status_line = "HTTP/1.0 200 OK\r\n";
    const size_t status_line_size = snprintf(NULL, 0, "%s", status_line);
    if (status_line_size < 0) {
        printTime(stderr);
        fprintf(stderr, "[ERR] snprintf() error\n");
        return 1;
    }

    // response headers
    const char* headers_fmt = "Content-Type: %s\r\nContent-Length: %lu\r\n";
    const int headers_size = snprintf(NULL, 0, headers_fmt, content_type, file_info.st_size);
    if (headers_size < 0) {
        printTime(stderr);
        fprintf(stderr, "[ERR] snprintf() error\n");
        return 1;
    }
    char* headers = calloc(headers_size + 1, 1);
    if (!headers) {
        printTime(stderr);
        fprintf(stderr, "[ERR] calloc() error: %s\n", strerror(errno));
        return 1;
    }
    (void)sprintf(headers, headers_fmt, content_type, file_info.st_size);

    // response empty line
    const char* empty_line = "\r\n";

    // response body
    char* entity_body = calloc(file_info.st_size, 1); // FIXME: do i need +1 here for binary data?
    if (!entity_body) {
        printTime(stderr);
        fprintf(stderr, "[ERR] calloc() error: %s\n", strerror(errno));
        return 1;
    }
    memset(entity_body, 0, file_info.st_size);
    fread(entity_body, 1, file_info.st_size, file); // read file into allocated memory
    fclose(file);

    // construct full response from components
    const size_t status_headers_line_size = status_line_size + headers_size + 2;
    const size_t full_response_size = status_headers_line_size + file_info.st_size;
    char* full_response = calloc(full_response_size + 1, 1);
    if (!full_response) {
        printTime(stderr);
        fprintf(stderr, "[ERR] calloc() error: %s\n", strerror(errno));
        return 1;
    }
    strcpy(full_response, status_line);
    strcat(full_response, headers);
    strcat(full_response, empty_line);
    memcpy(full_response + status_headers_line_size, entity_body, file_info.st_size);
    free(headers);
    free(entity_body);

    sendFullResponse(client_sfd, full_response, full_response_size);
    free(full_response);

    return 0;
}
