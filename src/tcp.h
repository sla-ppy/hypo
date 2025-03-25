#pragma once
#include <stddef.h>
#include <netdb.h>

int initServer(const char* server_addr, const char* server_port, struct addrinfo* servinfo, int** server_sfd);
