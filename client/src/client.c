
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void init_user(struct curr_user_state* usr){
    usr->mode = NOMODE;
    usr->filefd = -1;
    usr->ip[0] = 127;
    usr->ip[1] = 0;
    usr->ip[2] = 0;
    usr->ip[3] = 1;
    usr->port = 21;
};

int isInteger(const char *str) {
    char *endptr;
    errno = 0;
    strtol(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || endptr == str) {
        return 0;
    } else {
        return 1;
    }
}


int main(int argc, char **argv) {
    int port = 21;
    int connfd;
    struct sockaddr_in addr;
    char buffer[MAX_SIZE];
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if ((connfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("Error socket");
        return 1;
    }
    
    
    if(argc == 3){
        if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
            printf("Error inet_pton\n");
            return 1;
        }
        if (!isInteger(argv[2])) {
            printf("Invalid port\n");
            return 1;
        }
        port = atoi(argv[2]);
    }
    else if(argc != 1){
        printf("Invalid arguments\n");
        return 1;
    }
    
    addr.sin_port = htons(port);
    
    if (connect(connfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error connect");
        return 1;
    }
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
    
    struct curr_user_state *usr = (struct curr_user_state *)malloc(sizeof(struct curr_user_state));
    init_user(usr);
    
    while(1){
        printf(">>> ");
        memset(buffer, 0, MAX_SIZE);
        fgets(buffer, MAX_SIZE, stdin);
        send(connfd, buffer, strlen(buffer), 0);
        buffer[strlen(buffer) - 1] = 0;
        command_handler(usr, buffer, connfd);
        if(strcmp(buffer, "QUIT") == 0 || strcmp(buffer, "ABOR") == 0){
            close(connfd);
            if(usr->filefd != -1)
                close(usr->filefd);
            return 0;
        }
    }
    close(connfd);
    if(usr->filefd != -1)
        close(usr->filefd);
    return 0;
}
