#include <stdlib.h>
#include <string.h>

// network libs
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "request.h"
#include "response.h"
#include "util.h"

RequestLine getRequestLine(char* buffer, char* line_buffer) {
    // read data straight from buffer
    char* pch = strtok(buffer, "\r");
    line_buffer = pch;

    // cut string into substrings and get data from first line
    // FIXME: there is probably a better way to do this
    RequestLine request;
    pch = strtok(line_buffer, " ");
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

Request receiveRequest(const int client_sfd) {
    Request request;

    // server data
    int request_bytes;
    const size_t buffer_size = 2048;
    char buffer[buffer_size];
    memset(buffer, 0, buffer_size);
    request_bytes = recv(client_sfd, buffer, buffer_size, 0);
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
        printf("[CLIENT]\n%s", buffer);
    }

    // get first line of the request
    const size_t line_buffer_size = 128;
    char line_buffer[line_buffer_size];
    memset(line_buffer, 0, sizeof(line_buffer));
    request.request_line = getRequestLine(buffer, line_buffer);

    request.error = false;
    return request;
}

RequestedContent processRequest(const int client_sfd) {
    Request request;
    RequestedContent content;
    request = receiveRequest(client_sfd);
    if (request.error) {
        printTime(stderr);
        fprintf(stderr, "[ERR] Request method was not GET\n");
        content.error = true;
        return content;
    }

    // TODO: handle depending on method
    // close if request method is not correct
    const char* methods[] = { "GET", "HEAD", "POST" };
    const size_t methods_size = sizeof(methods) / sizeof(*methods);
    for (size_t i = 0; i < methods_size; i++) {
    }
    if (strcmp(request.request_line.method, "GET") != 0) {
        printTime(stderr);
        fprintf(stderr, "[ERR] Request method was not GET\n");
        content.error = true;
        return content;
    }

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
        fprintf(stderr, "[ERR] access()\n");
        // TODO: return "HTTP-1.0 404 Not Found\r\n"
        content.error = true;
        return content;
    }

    // get file extension
    const char* extension = strrchr(file_path_buffer, '.') + 1; // +1 because otherwise we are pointing to the delimiter
    if (!extension) {
        printTime(stderr);
        fprintf(stderr, "[ERR] File extension not found for file!\n");
        content.error = true;
        return content;
    }

    // check what file extension we need to respond with to the GET request
    const char* text_types[] = { "css", "html", "javascript" };
    const char* image_types[] = { "apng", "avif", "gif", "jpeg", "png", "svg+xml", "webp", "x-icon" };
    const size_t text_types_size = sizeof(text_types) / sizeof(*text_types);
    const size_t image_types_size = sizeof(image_types) / sizeof(*image_types);
    const size_t content_type_size = 64;
    char content_type[content_type_size];
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
    content.type = content_type;
    content.file = file;

    // get information about the file, namely file size
    struct stat file_info;
    rc = stat(file_path_buffer, &file_info);
    if (rc == -1) {
        printTime(stderr);
        fprintf(stderr, "[ERR] stat() error\n");
        content.error = true;
        return content;
    }
    content.file_info = file_info;

    content.error = false;
    return content;
}

int handleClient(const int client_sfd) {
    RequestedContent content = processRequest(client_sfd);
    if (content.error) {
        printTime(stderr);
        fprintf(stderr, "[ERR] Requested content couldn't be created\n");
        return 1;
    }

    constructFullResponse(client_sfd, content.type, content.file, content.file_info);

    return 0;
}
