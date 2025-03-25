#include <stddef.h>
#include <stdio.h>
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
    size_t request_buffer_size = 2048;
    char request_buffer[request_buffer_size];
    memset(request_buffer, 0, request_buffer_size);
    request_bytes = recv(client_sfd, request_buffer, request_buffer_size, 0);
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
    const char* file_path = request.request_line.url;
    char file_path_buffer[PATH_MAX] = "content";
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

    // get file extension
    const char* extension = strrchr(file_path_buffer, '.') + 1; // +1 because otherwise we are pointing to the delimiter
    if (!extension) {
        printTime(stderr);
        fprintf(stderr, "[ERR] File extension not found for file!\n");
        return 1;
    }

    // check what file extension we have
    const char* text_types[] = { "css", "html", "javascript" };
    const char* image_types[] = { "apng", "avif", "gif", "jpeg", "png", "svg+xml", "webp", "x-icon" };
    const size_t text_types_size = sizeof(text_types) / sizeof(*text_types);
    const size_t image_types_size = sizeof(image_types) / sizeof(*image_types);
    char content_type[64];
    memset(content_type, 0, sizeof(content_type));
    bool content_type_found = false;
    FILE* file = NULL;
    if (!content_type_found) {
        for (size_t i = 0; i < text_types_size; i++) {
            if (strcmp(extension, text_types[i]) == 0) {
                strcpy(content_type, "text/");
                strcat(content_type, text_types[i]);
                content_type_found = true;
                file = fopen(file_path_buffer, "r");
                break;
            }
        }
    }
    if (!content_type_found) {
        for (size_t i = 0; i < image_types_size; i++) {
            // .ico the file extension name doesn't align with the MIME type x-icon
            char* extension_ico = "ico";
            // FIXME: this is a bad way to solve this
            if (strcmp(extension, extension_ico) == 0) {
                strcpy(content_type, "image/");
                strcat(content_type, image_types[i]);
                content_type_found = true;
                file = fopen(file_path_buffer, "rb");
                break;
            }

            if (strcmp(extension, image_types[i]) == 0) {
                strcpy(content_type, "image/");
                strcat(content_type, image_types[i]);
                content_type_found = true;
                file = fopen(file_path_buffer, "rb");
                break;
            }
        }
    }

    // get information about the file, namely file size
    struct stat file_info;
    rc = stat(file_path_buffer, &file_info);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] stat() error: %s\n", strerror(errno));
        return 1;
    }

    char* status_line = "HTTP/1.0 200 OK\r\n";

    char* header_content_beg = "Content-Type: ";
    char* header_content_end = "\r\nContent-Length: ";
    size_t headers_size = strlen(header_content_beg) + strlen(content_type) + strlen(header_content_end) + sizeof(file_info.st_size) + 2;
    char* const headers = calloc(headers_size + 1, 1);
    if (headers == NULL) {
        printTime(stderr);
        fprintf(stderr, "[ERR] calloc() error: %s\n", strerror(errno));
        return 1;
    }
    sprintf(headers, "%s%s%s%lu\r\n", header_content_beg, content_type, header_content_end, file_info.st_size);

    /* Recommended by Adler => suggest using snprintf() to check for size, this instead of using an arbitary size
    char const *headers = "... %s%lu%s ...;
    int required =  snprintf(NULL, 0, headers, ...);
    if(required < 0) { ... }
    char *actual_headers = malloc(required + ...);
    if(!actual_headers) { ... }
    (void)sprintf(actual_headers, ...);
    */

    char* empty_line = "\r\n";

    // allocate mem and read from file depending on size, char* is already in bytes
    char* entity_body = calloc(file_info.st_size, 1);
    if (entity_body == NULL) {
        printTime(stderr);
        fprintf(stderr, "[ERR] malloc() error: %s\n", strerror(errno));
        return 1;
    }
    memset(entity_body, 0, file_info.st_size);
    fread(entity_body, 1, file_info.st_size, file); // read file into allocated memory
    fclose(file);

    // construct full response
    size_t status_headers_line_size = strlen(status_line) + strlen(headers) + 2;
    size_t full_response_size = status_headers_line_size + file_info.st_size;
    char* full_response = calloc(full_response_size + 1, 1);
    if (full_response == NULL) {
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

    // send response
    int response_bytes = send(client_sfd, full_response, full_response_size, 0);
    if (response_bytes == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] send() error during transcieving: %s\n", strerror(errno));
        return 1;
    }
    free(full_response);

    return 0;
}
