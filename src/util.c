#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void printTime(FILE* stream) {
    time_t t = time(NULL); // get local time in calendar format

    char time_buffer[70]; // format local time in calendar to custom format

    if (strftime(time_buffer, sizeof time_buffer, "[%Y.%m.%d - %H:%M:%S] ", localtime(&t))) {
        fprintf(stream, "%s", time_buffer);
    } else {
        fprintf(stderr, "[ERR] getTime() fails to retrietve formatted time\n");
    }
}

void checkArgument(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stdout, "Usage: ./server <port>\n");
        exit(1);
    }
    if (argv[1] == NULL) {
        fprintf(stdout, "Usage: ./server <port>\n");
        exit(1);
    }
    for (int i = 0; isdigit(argv[1][i]) == 0; i++) {
        fprintf(stdout, "Argument for port may only be a number\n");
        exit(1);
    }
    if (atoi(argv[1]) < 1024 || atoi(argv[1]) > 65535) {
        fprintf(stdout, "Argument for port out of range. Range: [1024 - 65535]\n");
        exit(1);
    }
}
