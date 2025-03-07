#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void createSimpleRequest() {
    struct SimpleRequest {
        char* method;
        char* path;
    };

    struct SimpleRequest request;
    request.method = "GET";
    request.path = "/";

    // SIMPLE REQUEST
    // printf("%s %s\r\n", request.method, request.path); // Request Line
    // "GET" SP Request-URI CRLF

    // SIMPLE RESPONSE
    // printf("Entity-Body"); // Response
    // [ Entity-Body ]
}

int main(void) {
    const char* filepath = "resources/content.html";
    struct stat fileinfo; // return information about the file

    int rc = stat(filepath, &fileinfo);
    if (rc == -1) {
        fprintf(stderr, "[ERR] stat() error: %s\n", strerror(errno));
        return 1;
    }
    // malloc returns a void pointer, which probably points to the allocated memory
    void* filemem_ptr = malloc(fileinfo.st_size); // allocate enough memory for the file size, make a ptr point to it

    // ptr: stores memory addr of another variable as its value
    printf("Pointer: %p\n", filemem_ptr); // variable's value
    printf("Pointer : %p\n", &filemem_ptr); // variable's address in memory
    // printf("Pointer : %p\n", *filemem_ptr); // dereferencing, value at address
}
