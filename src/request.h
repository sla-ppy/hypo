#ifndef REQUEST_H
struct RequestLine {
    char* method;
    char* url;
    char* protocol_version;
};

struct RequestLine getRequestLine(char buffer[1024], char request_line[128]);
#define REQUEST_H

#endif // REQUEST_H
