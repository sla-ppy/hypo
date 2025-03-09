#include <stdio.h>
#include <string.h>

#include "request.h"

struct RequestLine getRequestLine(char buffer[1024], char request_line[128]) {
    // read data straight from buffer
    char* pch = strtok(buffer, "\r");
    request_line = pch;

    // cut string into substrings and get data from first line
    // FIXME: there is probably a better way to do this
    struct RequestLine request;
    pch = strtok(request_line, " ");
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
