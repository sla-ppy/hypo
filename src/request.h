#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

typedef struct RequestLine {
    char* method;
    char* url;
    char* protocol_version;
} RequestLine;

typedef struct Request {
    bool error;
    RequestLine request_line;
} Request;

typedef struct RequestedContent {
    bool error;
    char* type;
    FILE* file;
    struct stat file_info;
} RequestedContent;

int handleClient(const int client_sfd);
RequestedContent processRequest(const int client_sfd);
Request receiveRequest(const int client_sfd);
RequestLine getRequestLine(char* buffer, char* line_buffer);
