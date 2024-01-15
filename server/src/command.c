#include "command.h"


void handleSYST(struct curr_user_state* usr, int connfd){
    usr->RNFR_valid = 0;
    send(connfd, "215 UNIX Type: L8\r\n", strlen("215 UNIX Type: L8\r\n"), 0);
}

void handleTYPE(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    char I[MAX_SIZE];
    memset(I, 0, MAX_SIZE);
    sscanf(sentence, "TYPE %s", I);
    if(strcmp(I, "I") == 0)
        send(connfd, "200 Type set to I.\r\n", strlen("200 Type set to I.\r\n"), 0);
    else
        send(connfd, "200 Unkown type.\r\n",strlen("200 Unkown type.\r\n"),0);
}

void handleQUIT(struct curr_user_state* usr, int connfd){
    usr->RNFR_valid = 0;
    send(connfd,"221 Goodbye.\r\n",strlen("221 Goodbye.\r\n"),0);
    close(connfd);
    if(usr->filefd != -1)
        close(usr->filefd);
    exit(0);
}

void handleMKD(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    char dirpath[MAX_SIZE];
    memset(dirpath, 0, MAX_SIZE);
    sscanf(sentence, "MKD %s", dirpath);
    
    //not allowed to visit parent directory
    if(strstr(dirpath, "../") != NULL || strstr(dirpath, "..") != NULL){
        send(connfd, "550 Permission denied.\r\n",
                strlen("550 Permission denied.\r\n"), 0);
        return;
    }
    
    if(strlen(dirpath) > 0)
        if(dirpath[0] == '/')
            concatenate(usr->path_to_true_root, dirpath);

    if(mkdir(dirpath, 0777) == 0)
        send(connfd, "250 Directory created successfully.\r\n", strlen("250 Directory created successfully.\r\n"), 0);
    else
        send(connfd, "550 Error make directory.\r\n", strlen("550 Error make directory.\r\n"), 0);
}

void handleCWD(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    char dirpath[MAX_SIZE];
    memset(dirpath, 0, MAX_SIZE);
    sscanf(sentence, "CWD %s", dirpath);
    
    //not allowed to visit parent directory
    if(strstr(dirpath, "../") != NULL || strstr(dirpath, "..") != NULL){
        send(connfd, "550 Permission denied: no authority to visit parent directory.\r\n",
                strlen("550 Permission denied: no authority to visit parent directory.\r\n"), 0);
        return;
    }

    if(strlen(dirpath) > 0)
        if(dirpath[0] == '/')
            concatenate(usr->path_to_true_root, dirpath);


    if(chdir(dirpath) == 0){
        send(connfd, "250 Directory changed successfully.\r\n", strlen("250 Directory changed successfully.\r\n"), 0);
    }
    else
        send(connfd, "Error change directory.\r\n", strlen("Error change directory.\r\n"), 0);
}

void handlePWD(struct curr_user_state* usr, int connfd){
    usr->RNFR_valid = 0;
    char response[MAX_SIZE + 20];
    memset(response, 0, MAX_SIZE + 20);
    memset(usr->curr_dir, 0, MAX_SIZE);
    getcwd(usr->curr_dir, MAX_SIZE);
    memmove(usr->curr_dir, usr->curr_dir + strlen(usr->path_to_true_root), MAX_SIZE - strlen(usr->path_to_true_root));
    if(usr->curr_dir[0] == 0)
        usr->curr_dir[0] = '/';
    sprintf(response, "257 %s\r\n", usr->curr_dir);
    send(connfd, response, strlen(response), 0);
}

void handleRMD(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    char dirpath[MAX_SIZE];
    memset(dirpath, 0, MAX_SIZE);
    sscanf(sentence, "RMD %s", dirpath);
    
    //not allowed to visit parent directory
    if(strstr(dirpath, "../") != NULL || strstr(dirpath, "..") != NULL){
        send(connfd, "550 Permission denied.\r\n",
                strlen("550 Permission denied.\r\n"), 0);
        return;
    }

    if(strlen(dirpath) > 0)
        if(dirpath[0] == '/')
            concatenate(usr->path_to_true_root, dirpath);
    
    char command[MAX_SIZE + 30];
    memset(command, 0, MAX_SIZE + 30);
    sprintf(command, "rm -r %s", dirpath);
        
    if(system(command) == 0)
        send(connfd, "250 Directory removed successfully.\r\n", strlen("250 Directory removed successfully.\r\n"), 0);
    else
        send(connfd, "550 Error remove directory.\r\n", strlen("550 Error remove directory.\r\n"), 0);
}

void handleLIST(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    char dirpath[MAX_SIZE], dirpath1[MAX_SIZE];
    char response[MAX_SIZE + 50];
    int transfd;
    memset(dirpath, 0, MAX_SIZE);
    memset(dirpath1, 0, MAX_SIZE);
    if(strcmp(sentence, "LIST") == 0 || strcmp(sentence, "LIST ") == 0){
        memset(usr->curr_dir, 0, MAX_SIZE);
        getcwd(usr->curr_dir, MAX_SIZE);
        memmove(usr->curr_dir, usr->curr_dir + strlen(usr->path_to_true_root), MAX_SIZE - strlen(usr->path_to_true_root));
        if(usr->curr_dir[0] == 0)
            usr->curr_dir[0] = '/';
        strcpy(dirpath, usr->curr_dir);
    }
    else
        sscanf(sentence, "LIST %s", dirpath);

    if(strlen(dirpath) == 0){
        memset(response, 0, MAX_SIZE + 40);
        sprintf(response,"550 Invalid command: %s.\r\n", sentence);
        usr->mode = NOMODE;
        send(connfd, response, strlen(response), 0);
        return;
    }

    //not allowed to visit parent directory
    if(strstr(dirpath, "../") != NULL || strstr(dirpath, "..") != NULL){
        send(connfd, "550 Permission denied: no authority to visit parent directory.\r\n",
                strlen("550 Permission denied: no authority to visit parent directory.\r\n"), 0);
        usr->mode = NOMODE;
        close(usr->filefd);
        return;
    }   
    
    //file transfer works only when PORT or PASV is set.
    if(usr->mode == NOMODE){
        send(connfd, "425 PORT or PASV mode should be set first\r\n", strlen("425 PORT or PASV mode should be set first\r\n"), 0);
        close(usr->filefd);
        return;
    }
    
    //PORT mode
    else if(usr->mode == PORT){
        //server connects to the client spontaneously
        if(connect(usr->filefd, (struct sockaddr *)&usr->data_address, sizeof(usr->data_address)) < 0){
            send(connfd, "425 No TCP connection was established.\r\n", strlen("425 No TCP connection was established.\r\n"), 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        transfd = usr->filefd;
    }
    
    //PASV mode
    else if(usr->mode == PASV){
        if((transfd = accept(usr->filefd, NULL, NULL)) < 0){
            send(connfd, "425 No TCP connection was established.\r\n", strlen("425 No TCP connection was established.\r\n"), 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        close(usr->filefd);
    }

    usr->mode = NOMODE;
    
    strcpy(dirpath1, dirpath);
    
    if(strlen(dirpath) > 0)
        if(dirpath[0] == '/')
            concatenate(usr->path_to_true_root, dirpath);

    char command[MAX_SIZE + 30];
    memset(command, 0, MAX_SIZE + 30);
    sprintf(command, "ls -l %s", dirpath);

    FILE *file = popen(command, "r");
    if (file == NULL){
        memset(response, 0, MAX_SIZE + 50);
        sprintf(response,"451 Running ls command failed.\r\n");
        send(connfd, response, strlen(response), 0);
        close(transfd);
        return;
    }
    
    memset(response, 0, MAX_SIZE + 50);
    sprintf(response,"150 Opening BINARY mode data connection for %s.\r\n", dirpath1);
    send(connfd, response, strlen(response), 0);
    
    char buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    size_t num_bytes;
    //read file content to buffer
    while ((num_bytes = fread(buffer, 1, MAX_SIZE, file)) > 0) {
        if (send(transfd, buffer, num_bytes, 0) < 0) {
            //if the connection is broken, reject the request with code 426
            send(connfd, "426 Connection broken.\r\n", strlen("426 Connection broken.\r\n"), 0);
            pclose(file);
            close(transfd);
            return;
        }
        memset(buffer, 0, MAX_SIZE);
    }
    
    // close the file and data connection
    if(pclose(file) != 0){
        memset(response, 0, MAX_SIZE + 50);
        sprintf(response,"451 No such file or directory: %s.\r\n", dirpath1);
        send(connfd, response, strlen(response), 0);
        close(transfd);
        return;
    }
    send(connfd, "226 LIST command successful.\r\n", strlen("226 LIST command successful.\r\n"), 0);
    close(transfd);

}

void handlePORT(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    //close former port
    if(usr->filefd != -1)
        close(usr->filefd);
    
    usr->mode = NOMODE;

    int num1, num2, num3, num4, num5, num6;
    int count = sscanf(sentence, "PORT %d,%d,%d,%d,%d,%d", &num1, &num2, &num3, &num4, &num5, &num6);

    if (count == 6) {
        usr->ip[0] = num1;
        usr->ip[1] = num2;
        usr->ip[2] = num3;
        usr->ip[3] = num4;
        usr->port = 256 * num5 + num6;

        int filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (filefd < 0) {
            send(connfd, "450 Error socket.\r\n", strlen("450 Error socket.\r\n"), 0);
            return;
        }
        
        if(!(usr->port >= 20000 && usr->port <= 65535)){
            send(connfd, "450 Error port, should be between 20000 and 65535.\r\n",
                 strlen("450 Error port, should be between 20000 and 65535.\r\n"), 0);
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
            send(connfd, "450 Error ip address.\r\n", strlen("450 Error ip address.\r\n"), 0);
            close(filefd);
            return;
        }
        
        
        usr->mode = PORT;
        usr->filefd = filefd;

        send(connfd, "200 Port command successful.\r\n", strlen("200 Port command successful.\r\n"), 0);
        
    }
    else{
        char response[MAX_SIZE + 30];
        memset(response, 0, MAX_SIZE + 30);
        sprintf(response, "550 Invalid Command: %s\r\n", sentence);
        send(connfd, response, strlen(response), 0);
    }
}

void handlePASV(struct curr_user_state* usr, int connfd) {
    usr->RNFR_valid = 0;
    //close former port
    if(usr->filefd != -1)
        close(usr->filefd);
    
    usr->mode = NOMODE;
    
    //generate a random port number between 20000 and 65535
    int port = 20000 + rand() % (65535 - 20000 + 1);

    //create a new socket for data connection
    int filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (filefd < 0) {
        send(connfd, "450 Error socket.\r\n", strlen("450 Error socket.\r\n"), 0);
        return;
    }
    
    //get the server's IP address
    char *serverIP = get_ip();
    if(serverIP == NULL){
        send(connfd, "450 Error get_ip().\r\n", strlen("450 Error get_ip().\r\n"), 0);
        close(filefd);
        return;
    }
    
    usr->data_address.sin_family = AF_INET;
    usr->data_address.sin_addr.s_addr = htonl(INADDR_ANY);
    usr->data_address.sin_port = htons(port);
    

    //find available port
    while (bind(filefd, (struct sockaddr *)&usr->data_address, sizeof(usr->data_address)) < 0){
        port = 20000 + rand() % (65535 - 20000 + 1);
        usr->data_address.sin_port = htons(port);
    }

    if (listen(filefd, 10) < 0) {
        send(connfd, "450 Error listen.\r\n", strlen("450 Error listen.\r\n"), 0);
        close(filefd);
        return;
    }

    usr->port = port;
    usr->mode = PASV;
    usr->filefd = filefd;
    
    int length = strlen(serverIP);
    char *modifiedIP = (char *)malloc((length + 1) * sizeof(char));

    if (modifiedIP == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    for (int i = 0; i < length; i++) {
        if (serverIP[i] == '.') {
            modifiedIP[i] = ',';
        } else {
            modifiedIP[i] = serverIP[i];
        }
    }
    modifiedIP[length] = '\0'; // 添加字符串结尾
    
    if (inet_pton(AF_INET, serverIP, &usr->data_address.sin_addr) <= 0){
        perror("Error ip address");
        close(filefd);
        return;
    }
    
    //send the response to the client
    char response[MAX_SIZE];
    memset(response, 0, MAX_SIZE);
    sprintf(response,"227 Entering Passive Mode (%s,%d,%d)\r\n", modifiedIP, (usr->port)/256, (usr->port)%256);
    send(connfd, response, strlen(response), 0);
}


//function to handle RETR command
void handleRETR(struct curr_user_state* usr, char* sentence, int connfd) {
    usr->RNFR_valid = 0;
    char filepath[MAX_SIZE], filepath1[MAX_SIZE];
    int transfd;
    memset(filepath, 0, MAX_SIZE);
    memset(filepath1, 0, MAX_SIZE);
    
    sscanf(sentence, "RETR %s", filepath);
    strcpy(filepath1, filepath);
    
    if(strstr(filepath, "../") != NULL){
        send(connfd, "550 Permission denied: no authority to visit parent directory.\r\n",
                strlen("550 Permission denied: no authority to visit parent directory.\r\n"), 0);
        usr->mode = NOMODE;
        close(usr->filefd);
        return;
    }   
    
    //file transfer works only when PORT or PASV is set.
    if(usr->mode == NOMODE){
        send(connfd, "425 PORT or PASV mode should be set first\r\n", strlen("425 PORT or PASV mode should be set first\r\n"), 0);
        close(usr->filefd);
        return;
    }
    
    //PORT mode
    else if(usr->mode == PORT){
        //server connects to the client spontaneously
        if(connect(usr->filefd, (struct sockaddr *)&usr->data_address, sizeof(usr->data_address)) < 0){
            send(connfd, "425 No TCP connection was established.\r\n", strlen("425 No TCP connection was established.\r\n"), 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        transfd = usr->filefd;
    }
    
    //PASV mode
    else if(usr->mode == PASV){
        if((transfd = accept(usr->filefd, NULL, NULL)) < 0){
            send(connfd, "425 No TCP connection was established.\r\n", strlen("425 No TCP connection was established.\r\n"), 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        close(usr->filefd);
    }

    usr->mode = NOMODE;

    if(strlen(filepath) > 0)
        if(filepath[0] == '/')
            concatenate(usr->path_to_true_root, filepath);
    
    FILE *file = fopen(filepath, "rb");
    
    if (file == NULL){
        send(connfd, "551 Can't read file from disk.\r\n", strlen("551 Can't read file from disk.\r\n"), 0);
        close(transfd);
        return;
    }
    
    char response[MAX_SIZE + 50];
    memset(response, 0, MAX_SIZE + 50);
    sprintf(response,"150 Opening BINARY mode data connection for %s.\r\n", filepath1);
    send(connfd, response, strlen(response), 0);
    
    char buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    size_t num_bytes;
    //read file content to buffer
    usleep(500);
    while ((num_bytes = fread(buffer, 1, MAX_SIZE, file)) > 0) {
        if (write(transfd, buffer, num_bytes) < 0) {
            //if the connection is broken, reject the request with code 426
            send(connfd, "426 Connection broken.\r\n", strlen("426 Connection broken.\r\n"), 0);
            fclose(file);
            close(transfd);
            return;
        }
        memset(buffer, 0, MAX_SIZE);
    }

    
    send(connfd, "226 File transfer successful.\r\n", strlen("226 File transfer successful.\r\n"), 0);

    // close the file and data connection
    fclose(file);
    close(transfd);
}


void handleSTOR(struct curr_user_state* usr, char* sentence, int connfd){
    usr->RNFR_valid = 0;
    char filepath[MAX_SIZE], filepath1[MAX_SIZE];
    int transfd;
    memset(filepath, 0, MAX_SIZE);
    memset(filepath1, 0, MAX_SIZE);
    sscanf(sentence, "STOR %s", filepath);
    
    if(strstr(filepath, "../") != NULL){
        send(connfd, "550 Permission denied: no authority to visit parent directory.\r\n",
                strlen("550 Permission denied: no authority to visit parent directory.\r\n"), 0);
        usr->mode = NOMODE;
        close(usr->filefd);
        return;
    }   
    
    //file transfer works only when PORT or PASV is set.
    if(usr->mode == NOMODE){
        send(connfd, "425 PORT or PASV mode should be set first\r\n", strlen("425 PORT or PASV mode should be set first\r\n"), 0);
        close(usr->filefd);
        return;
    }
    
    //PORT mode
    else if(usr->mode == PORT){
        //server connects to the client spontaneously
        if(connect(usr->filefd, (struct sockaddr*)&(usr->data_address), sizeof(usr->data_address)) < 0){
            send(connfd, "425 No TCP connection was established.\r\n", strlen("425 No TCP connection was established.\r\n"), 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        transfd = usr->filefd;
    }
    
    //PASV mode
    else if(usr->mode == PASV){
        if((transfd = accept(usr->filefd, NULL, NULL)) < 0){
            send(connfd, "425 No TCP connection was established.\r\n", strlen("425 No TCP connection was established.\r\n"), 0);
            usr->mode = NOMODE;
            close(usr->filefd);
            return;
        }
        close(usr->filefd);
    }

    usr->mode = NOMODE;
    
    strcpy(filepath1, filepath);

    if(strlen(filepath) > 0)
        if(filepath[0] == '/')
            concatenate(usr->path_to_true_root, filepath);
    
    FILE *file = fopen(filepath, "wb");
    
    if (file == NULL){
        send(connfd, "551 Can't write to the file in disk.\r\n", strlen("551 Can't write to the file in disk.\r\n"), 0);
        close(transfd);
        return;
    }
    
    char response[MAX_SIZE + 50];
    memset(response, 0, MAX_SIZE + 50);
    sprintf(response,"150 Opening BINARY mode data connection for %s.\r\n", filepath);
    send(connfd, response, strlen(response), 0);


    char buffer[MAX_SIZE];
    memset(buffer, 0, MAX_SIZE);
    size_t num_bytes;
    //read transfd to buffer
    while ((num_bytes = read(transfd, buffer, MAX_SIZE)) > 0) {
        if (fwrite(buffer, 1, num_bytes, file) < 0) {
            //if the connection is broken, reject the request with code 426
            send(connfd, "426 Connection broken.\r\n", strlen("426 Connection broken.\r\n"), 0);
            fclose(file);
            close(transfd);
            return;
        }
        memset(buffer, 0, MAX_SIZE);
    }

    send(connfd, "226 File transfer successful.\r\n", strlen("226 File transfer successful.\r\n"), 0);
    
    // close the file and data connection
    fclose(file);
    close(transfd);
}

void handleRNFR(struct curr_user_state* usr, char* sentence, int connfd){
    char filepath[MAX_SIZE];
    memset(filepath, 0, MAX_SIZE);
    sscanf(sentence, "RNFR %s", filepath);
    
    //not allowed to visit parent directory
    if(strstr(filepath, "../") != NULL || strstr(filepath, "..") != NULL){
        send(connfd, "550 Permission denied: no authority to visit parent directory.\r\n",
                strlen("550 Permission denied: no authority to visit parent directory.\r\n"), 0);
        return;
    }

    if(strlen(filepath) > 0)
        if(filepath[0] == '/')
            concatenate(usr->path_to_true_root, filepath);

    if(access(filepath, F_OK) == 0){
        usr->RNFR_valid = 1;
        strcpy(usr->RNFR_file, filepath);
        send(connfd, "350 RNFR accepted.\r\n", strlen("350 RNFR accepted.\r\n"), 0);
    }
    else{
        send(connfd, "550 Error access file path.\r\n", strlen("550 Error access file path.\r\n"), 0);
    }
}

void handleRNTO(struct curr_user_state* usr, char* sentence, int connfd){
    if(usr->RNFR_valid == 0){
        send(connfd, "503 RNTO must come immediately after RNFR.\r\n", strlen("503 RNTO must come immediately after RNFR.\r\n"), 0);
    }
    else{
        usr->RNFR_valid = 0;
        char filepath[MAX_SIZE];
        memset(filepath, 0, MAX_SIZE);
        sscanf(sentence, "RNTO %s", filepath);
        
        //not allowed to visit parent directory
        if(strstr(filepath, "../") != NULL || strstr(filepath, "..") != NULL){
            send(connfd, "550 Permission denied: no authority to visit parent directory.\r\n",
                    strlen("550 Permission denied: no authority to visit parent directory.\r\n"), 0);
            return;
        }

        if(strlen(filepath) > 0)
            if(filepath[0] == '/')
                concatenate(usr->path_to_true_root, filepath);

        if(rename(usr->RNFR_file, filepath) == 0)
            send(connfd, "250 Renamed successfully.\r\n", strlen("250 Renamed successfully.\r\n"), 0);
        else
            send(connfd, "550 Error access file path.\r\n", strlen("550 Error access file path.\r\n"), 0);
    }
}

// handle all commands after logging in
void command_handler(struct curr_user_state* usr, char* sentence, int connfd){
    
    if(strcmp(sentence, "SYST") == 0)
        handleSYST(usr, connfd);
    
    else if(isPrefix(sentence, "TYPE ") && strcmp(sentence, "TYPE ") != 0)
        handleTYPE(usr, sentence, connfd);

    else if(strcmp(sentence, "QUIT") == 0 || strcmp(sentence, "ABOR") == 0)
        handleQUIT(usr, connfd);

    else if(isPrefix(sentence, "PORT ") && strcmp(sentence, "PORT ") != 0)
        handlePORT(usr, sentence, connfd);

    else if(strcmp(sentence, "PASV") == 0)
        handlePASV(usr, connfd);

    else if(isPrefix(sentence, "RETR ") && strcmp(sentence, "RETR ") != 0)
        handleRETR(usr, sentence, connfd);
    
    else if(isPrefix(sentence, "STOR ") && strcmp(sentence, "STOR ") != 0)
        handleSTOR(usr, sentence, connfd);

    else if(isPrefix(sentence, "MKD ") && strcmp(sentence, "MKD ") != 0)
        handleMKD(usr, sentence, connfd);

    else if(isPrefix(sentence, "CWD ") && strcmp(sentence, "CWD ") != 0)
        handleCWD(usr, sentence, connfd);

    else if(strcmp(sentence, "PWD") == 0)
        handlePWD(usr, connfd);

    else if(isPrefix(sentence, "RMD ") && strcmp(sentence, "RMD ") != 0)
        handleRMD(usr, sentence, connfd);

    else if(isPrefix(sentence, "LIST"))
        handleLIST(usr, sentence, connfd);

    else if(isPrefix(sentence, "RNFR ") && strcmp(sentence, "RNFR ") != 0)
        handleRNFR(usr, sentence, connfd);

    else if(isPrefix(sentence, "RNTO ") && strcmp(sentence, "RNTO ") != 0)
        handleRNTO(usr, sentence, connfd);

    else{
        usr->RNFR_valid = 0;
        char response[MAX_SIZE + 30];
        memset(response, 0, MAX_SIZE + 30);
        sprintf(response, "550 Invalid Command: %s\r\n", sentence);
        send(connfd, response, strlen(response), 0);
    }
}

//handle connection and logging in
void connection_handler(struct curr_user_state *usr, int connfd) {
    char *initial_message = "220 Anonymous FTP server ready.\r\n";
	char *confirmation_message = "331 Guest login ok, send your complete e-mail address as password.\r\n";
    char *user_response = "USER anonymous";
    char *success_response = "230 Guest login ok, access restrictions apply.\r\n";
	char sentence[MAX_SIZE];
    
    send(connfd, initial_message, strlen(initial_message), 0);

    while(1){
        //receiving USER command from client
        memset(sentence, 0, MAX_SIZE);
        recv(connfd, sentence, MAX_SIZE, 0);
    
        while(sentence[strlen(sentence) - 1] == '\r' || sentence[strlen(sentence) - 1] == '\n')
            sentence[strlen(sentence) - 1] = 0;
        
        if(strcmp(sentence, "QUIT") == 0 || strcmp(sentence, "ABOR") == 0)
            handleQUIT(usr, connfd);
        
        if (strcmp(sentence, user_response) != 0) {
            char response[MAX_SIZE + 30];
            memset(response, 0, MAX_SIZE + 30);
            sprintf(response, "550 Invalid Command: %s\r\n", sentence);
            send(connfd, response, strlen(response), 0);
            continue;
        }

        //sending confirmation to the client
        send(connfd, confirmation_message, strlen(confirmation_message), 0);
            
        memset(sentence, 0, MAX_SIZE);
        recv(connfd, sentence, MAX_SIZE, 0);
        
        while(sentence[strlen(sentence) - 1] == '\r' || sentence[strlen(sentence) - 1] == '\n')
            sentence[strlen(sentence) - 1] = 0;
        
        if(strcmp(sentence, "QUIT") == 0 || strcmp(sentence, "ABOR") == 0)
            handleQUIT(usr, connfd);
        
        if (!isPrefix(sentence,"PASS")) {
            char response[MAX_SIZE + 30];
            memset(response, 0, MAX_SIZE+ 30);
            sprintf(response, "550 Invalid Command: %s\r\n", sentence);
            send(connfd, response, strlen(response), 0);
            continue;
        }

        //sending success message to the client
        send(connfd, success_response, strlen(success_response), 0);
        break;
    }

    getcwd(usr->path_to_true_root, MAX_SIZE);
    
    memset(sentence, 0, MAX_SIZE);
    while(recv(connfd, sentence, MAX_SIZE, 0) > 0){
        while(sentence[strlen(sentence) - 1] == '\r' || sentence[strlen(sentence) - 1] == '\n')
            sentence[strlen(sentence) - 1] = 0;
        command_handler(usr, sentence, connfd);
        memset(sentence, 0, MAX_SIZE);
    }

    close(connfd);
}
