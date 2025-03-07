/* FULL REQUEST
Request Line    => Methods: GET, HEAD, POST, extension-method = token
General Header
Request Header
Entity Header
CRLF
Entity-Body
*/

#include <stdio.h>

struct FullRequest {
    char *method;
    char *path;
    char *protocol_version;
};

struct FullResponse {
    char *protocol_version;
    char *status_code;
    char *reason_phrase;
};

int main(void) {
    struct FullRequest request;
    request.method = "GET";
    request.path = "/";
    request.protocol_version = "HTTP/0.9";

    struct FullResponse response;
    response.protocol_version = "HTTP/0.9";
    response.status_code = "";
    response.reason_phrase = "";

    // FULL REQUEST
    printf("%s %s\r\n", request.method, request.path, request.protocol_version); // Request Line

    // FULL RESPONSE
    printf("%s %s %s\r\n", response.protocol_version, response.status_code, response.reason_phrase); // Status Line
}
