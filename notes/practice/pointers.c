#include <stdio.h>

void function(int** ptr) {
    int socket = 5;
    *ptr = &socket;
}

int main(void) {
    int* ptr = NULL;
    function(&ptr);

    printf("ptr after function(): %d", *ptr);

    return 0;
}
