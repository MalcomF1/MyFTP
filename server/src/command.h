#ifndef COMMAND
#define COMMAND

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <pthread.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "func.h"

#define MAX_SIZE 8192
#define PORT 0
#define PASV 1
#define NOMODE 2

struct curr_user_state{
    int mode;
    int filefd;
    int ip[4];
    int port;
    struct sockaddr_in data_address;
    char curr_dir[MAX_SIZE];
    char root[MAX_SIZE];
    char path_to_true_root[MAX_SIZE];
    int RNFR_valid;
    char RNFR_file[MAX_SIZE];
};

void handleSYST(struct curr_user_state* usr, int connfd);
void handleTYPE(struct curr_user_state* usr, char* sentence, int connfd);
void handleQUIT(struct curr_user_state* usr, int connfd);
void handlePORT(struct curr_user_state* usr, char* sentence, int connfd);
void handlePASV(struct curr_user_state* usr, int connfd);
void handleRETR(struct curr_user_state* usr, char* sentence, int connfd);
void handleSTOR(struct curr_user_state* usr, char* sentence, int connfd);
void handleMKD(struct curr_user_state* usr, char* sentence, int connfd);
void handleCWD(struct curr_user_state* usr, char* sentence, int connfd);
void handlePWD(struct curr_user_state* usr, int connfd);
void handleRMD(struct curr_user_state* usr, char* sentence, int connfd);
void handleLIST(struct curr_user_state* usr, char* sentence, int connfd);
void handleRNFR(struct curr_user_state* usr, char* sentence, int connfd);
void handleRNTO(struct curr_user_state* usr, char* sentence, int connfd);
void connection_handler(struct curr_user_state *usr, int connfd);
void command_handler(struct curr_user_state* usr, char* sentence, int connfd);
#endif
