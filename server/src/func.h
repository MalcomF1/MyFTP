#ifndef FUNC
#define FUNC

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>

int isPrefix(char* str, char* prefix);
int concatenate(char* a, char* b);
char* get_ip();

#endif
