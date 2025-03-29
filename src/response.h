#pragma once
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

#include "util.h"

typedef enum Method {
    OK = 200,
    NOT_FOUND = 404,
    NOT_IMPLEMENTED = 501
} Method;

int constructFullResponse(const int client_sfd, char* content_type, FILE* file, struct stat file_info);
int sendFullResponse(const int client_sfd, char* buffer, const size_t buffer_size);
