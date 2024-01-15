#include "command.h"
int isPrefix(char* str, char* prefix) {
    int strLength = strlen(str);
    int prefixLength = strlen(prefix);
    
    if (strLength < prefixLength) {
        return 0;
    }

    for (int i = 0; i < prefixLength; i++) {
        if (str[i] != prefix[i]) {
            return 0;
        }
    }

    return 1;
}

void handleMOST(struct curr_user_state* usr, int connfd){
    char buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
}

void handleLIST(struct curr_user_state* usr, int connfd){
    char buffer[MAX_SIZE];
    int transfd = -1;
    
    if(usr->mode == NOMODE){

        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        printf("%s", buffer);
        return;
    }
    
    //PORT mode
    if(usr->mode == PORT){
        //server connects to the client spontaneously
        if((transfd = accept(usr->filefd, NULL, NULL)) < 0){
            perror("Error accept");
            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        close(usr->filefd);
    }
    
    //PASV mode
    else if(usr->mode == PASV){
        if(connect(usr->filefd, (struct sockaddr *)&usr->data_address, sizeof(usr->data_address)) < 0){
            perror("Error connect");
            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        transfd = usr->filefd;
    }

    usr->mode = NOMODE;
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
    
    if(strstr(buffer, "150") == NULL){
        if(transfd != -1)
            close(transfd);
        return;
    }
    
    memset(buffer, 0, MAX_SIZE);
    recv(transfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
    
    if(transfd != -1)
        close(transfd);
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
}

void handlePORT(struct curr_user_state* usr, char* request, int connfd){
    if(usr->filefd != -1)
        close(usr->filefd);
    
    usr->mode = NOMODE;
    
    char buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    
    if(!isPrefix(buffer, "200")){
        printf("%s", buffer);
        return;
    }
    
    int num1, num2, num3, num4, num5, num6;
    int count = sscanf(request, "PORT %d,%d,%d,%d,%d,%d", &num1, &num2, &num3, &num4, &num5, &num6);

    if (count == 6) {
        usr->ip[0] = num1;
        usr->ip[1] = num2;
        usr->ip[2] = num3;
        usr->ip[3] = num4;
        usr->port = 256 * num5 + num6;

        int filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (filefd < 0) {
            perror("Error socket");
            return;
        }
        usr->data_address.sin_family = AF_INET;
        usr->data_address.sin_port = htons(usr->port);
        usr->data_address.sin_addr.s_addr = htonl(INADDR_ANY);


        char serverIP[100];
        memset(serverIP, 0, 100);
        sprintf(serverIP, "%d.%d.%d.%d", usr->ip[0], usr->ip[1], usr->ip[2], usr->ip[3]);

        //check whether ip address is legal
        if (inet_pton(AF_INET, serverIP, &usr->data_address.sin_addr) <= 0){
            perror("Error ip address");
            close(filefd);
            return;
        }
        
        if (bind(filefd, (struct sockaddr*)&usr->data_address, sizeof(usr->data_address)) == -1) {
            perror("Error bind");
            //printf("%d", usr->port);
            close(filefd);
            return;
        }
        
        if (listen(filefd, 10) < 0) {
            perror("Error listen");
            close(filefd);
            return;
        }
        
        usr->mode = PORT;
        usr->filefd = filefd;
    }
    printf("%s", buffer);
}

void handlePASV(struct curr_user_state* usr, int connfd) {
    //close former port
    if(usr->filefd != -1)
        close(usr->filefd);
    
    usr->mode = NOMODE;
    
    char buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    
    if(strstr(buffer, "227") == NULL){
        printf("%s", buffer);
        return;
    }
    
    char tmp[MAX_SIZE];
    memset(tmp, 0, MAX_SIZE);
    int num1, num2, num3, num4, num5, num6;
    int j = 0, start = 0;
    for (int i = 0; i<=strlen(buffer) - 1; i++){
        if(start)
            tmp[j++] = buffer[i];
        if(buffer[i] == '('){
            tmp[j++] = buffer[i];
            start = 1;
        }
        if(buffer[i] == ')')
            break;
    }
        
    if(sscanf(tmp, "(%d,%d,%d,%d,%d,%d)", &num1, &num2, &num3, &num4, &num5, &num6) == 6){
        char serverIP[100];
        memset(serverIP, 0, 100);
        sprintf(serverIP, "%d.%d.%d.%d", num1, num2, num3, num4);
        
        usr->port = 256 * num5 + num6;
        
        
        int filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (filefd < 0) {
            perror("Error socket");
            return;
        }
        usr->data_address.sin_family = AF_INET;
        usr->data_address.sin_port = htons(usr->port);

        //check whether ip address is legal
        if (inet_pton(AF_INET, serverIP, &usr->data_address.sin_addr) <= 0){
            perror("Error ip address");
            close(filefd);
            return;
        }
        usr->mode = PASV;
        usr->filefd = filefd;
    }
    printf("%s", buffer);
}


//function to handle RETR command
void handleRETR(struct curr_user_state* usr, char* request, int connfd) {
    char filepath[MAX_SIZE];
    char buffer[MAX_SIZE];
    int transfd = -1;
    memset(filepath, 0, MAX_SIZE);
    sscanf(request, "RETR %s", filepath);
    

    if(usr->mode == NOMODE){
        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        printf("%s", buffer);
        return;
    }
    
    //PORT mode
    if(usr->mode == PORT){
        //server connects to the client spontaneously
        if((transfd = accept(usr->filefd, NULL, NULL)) < 0){
            perror("Error accept");
            usr->mode = NOMODE;
            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            close(usr->filefd);
            return;
        }
        close(usr->filefd);
    }
    
    //PASV mode
    else if(usr->mode == PASV){
        if(connect(usr->filefd, (struct sockaddr *)&usr->data_address, sizeof(usr->data_address)) < 0){
            perror("Error connect");
            usr->mode = NOMODE;
            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            close(usr->filefd);
            return;
        }
        transfd = usr->filefd;
    }

    usr->mode = NOMODE;
    
    FILE* file = fopen(filepath, "wb");
    if (file == NULL){
        
        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        
        char read_buf[MAX_SIZE];
        memset(read_buf, 0, MAX_SIZE);
        
        int num_bytes = 0;
        while ((num_bytes = read(transfd, read_buf, MAX_SIZE)) > 0) {
            continue;
        }
        
        if(transfd != -1)
            close(transfd);
        
        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        
        perror("File not found");
        
        return;
    }
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
    
    if(strstr(buffer, "150") == NULL){
        usr->mode = NOMODE;
        if(transfd != -1)
            close(transfd);
        return;
    }
    
    char read_buf[MAX_SIZE];
    memset(read_buf, 0, MAX_SIZE);
    
    int num_bytes = 0;
    while ((num_bytes = read(transfd, read_buf, MAX_SIZE)) > 0) {
        fwrite(read_buf, 1, num_bytes, file);
        memset(read_buf, 0, MAX_SIZE);
    }
    fclose(file);
    
    if(transfd != -1)
        close(transfd);
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
}


void handleSTOR(struct curr_user_state* usr, char* request, int connfd){
    char filepath[MAX_SIZE];
    char buffer[MAX_SIZE];
    int transfd = -1;
    memset(filepath, 0, MAX_SIZE);
    sscanf(request, "STOR %s", filepath);
    
    if(usr->mode == NOMODE){
        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        printf("%s", buffer);
        return;
    }

    //PORT mode
    if(usr->mode == PORT){
        //server connects to the client spontaneously
        if((transfd = accept(usr->filefd, NULL, NULL)) < 0){
            perror("Error accept");
            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        close(usr->filefd);
    }
    
    //PASV mode
    else if(usr->mode == PASV){
        if(connect(usr->filefd, (struct sockaddr *)&usr->data_address, sizeof(usr->data_address)) < 0){
            perror("Error connect");
            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        transfd = usr->filefd;
    }

    usr->mode = NOMODE;
    
    FILE* file = fopen(filepath, "rb");
    if (file == NULL){
        perror("File not found");
        
        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        
        if(transfd != -1)
            close(transfd);
        
        memset(buffer, 0, MAX_SIZE);
        recv(connfd, buffer, MAX_SIZE, 0);
        return;
    }
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);

    if(strstr(buffer, "150") == NULL){
        usr->mode = NOMODE;
        if(transfd != -1)
            close(transfd);
        return;
    }
    
    char write_buf[MAX_SIZE];
    memset(write_buf, 0, MAX_SIZE);
    
    int num_bytes = 0;
    while ((num_bytes = fread(write_buf, 1, MAX_SIZE, file)) > 0) {
        write(transfd, write_buf, num_bytes);
        memset(write_buf, 0, MAX_SIZE);
    }
    fclose(file);
    
    if(transfd != -1)
        close(transfd);
    
    memset(buffer, 0, MAX_SIZE);
    recv(connfd, buffer, MAX_SIZE, 0);
    printf("%s", buffer);
}


// handle all commands after logging in
void command_handler(struct curr_user_state* usr, char* request, int connfd){

    if(isPrefix(request, "PORT ") && strcmp(request, "PORT ") != 0)
        handlePORT(usr, request, connfd);

    else if(strcmp(request, "PASV") == 0)
        handlePASV(usr, connfd);

    else if(isPrefix(request, "RETR ") && strcmp(request, "RETR ") != 0)
        handleRETR(usr, request, connfd);
    
    else if(isPrefix(request, "STOR ") && strcmp(request, "STOR ") != 0)
        handleSTOR(usr, request, connfd);
    
    else if(isPrefix(request, "LIST"))
        handleLIST(usr, connfd);

    else
        handleMOST(usr, connfd);
}
