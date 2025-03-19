#pragma once

#include <stdbool.h>

typedef struct RequestLine {
    char* method;
    char* url;
    char* protocol_version;
} RequestLine;

typedef struct Request {
    bool error;
    RequestLine request_line;
} Request;

RequestLine getRequestLine(char buffer[1024], char request_line[128]);
Request receiveRequest(int client_sfd);
int handleClient(int client_sfd);
