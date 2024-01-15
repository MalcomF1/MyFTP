#include "command.h"

void init_user(struct curr_user_state* usr){
    usr->mode = NOMODE;
    usr->filefd = -1;
    usr->ip[0] = 127;
    usr->ip[1] = 0;
    usr->ip[2] = 0;
    usr->ip[3] = 1;
    usr->port = 21;
    memset(usr->curr_dir, 0, MAX_SIZE);
    strcpy(usr->curr_dir, "/tmp");
    memset(usr->root, 0, MAX_SIZE);
    strcpy(usr->root, "/tmp");
    memset(usr->path_to_true_root, 0, MAX_SIZE);
    strcpy(usr->path_to_true_root, "/tmp");
    usr->RNFR_valid = 0;
    memset(usr->RNFR_file, 0, MAX_SIZE);
};

int main(int argc, char **argv) {
	int port = 21;
    char root[MAX_SIZE] = "/tmp";

    if(argc == 3){
        if(strcmp(argv[1],"-port") == 0)
            sscanf(argv[2], "%d", &port);
		else if(strcmp(argv[1], "-root") == 0)
            sscanf(argv[2], "%s", root);
    }

    if(argc == 5){
        if(strcmp(argv[1],"-port") == 0){
			sscanf(argv[2], "%d", &port);
			if(strcmp(argv[3], "-root") == 0)
			    sscanf(argv[4], "%s", root);
		}	
        else if(strcmp(argv[1], "-root") == 0){
			sscanf(argv[2], "%s", root);
			if(strcmp(argv[3],"-port") == 0)
				sscanf(argv[4], "%d", &port);
		}
    }
    
    if(chdir(root) == -1){
        perror("Root directory not found");
        return 1;
    }

	int listenfd;
	struct sockaddr_in addr;

	//create socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("Error socket");
		return 1;
	}

	// Set socket options to allow multiple connections
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error socketopt");
        return 1;
    }


	//set host's ip and port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
    

	//bind host's ip and port with socket
	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Error bind");
		return 1;
	}

	//start listening socket
	if (listen(listenfd, 10) == -1) {
        perror("Error listen");
		return 1;
	}
        
	//keep listening requests
	while (1) {
    //wait for client to connect -- blocking function
        int connfd;
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            perror("Error accept");
            continue;
        }
        int child = fork();
        if(child == 0){
            struct curr_user_state *usr = (struct curr_user_state *)malloc(sizeof(struct curr_user_state));
            init_user(usr);
            usr->port = port;
            strcpy(usr->root, root);
            strcpy(usr->curr_dir, root);
            connection_handler(usr, connfd);
            free(usr);
        }
        
        else if(child < 0){
            perror("Error fork");
            close(connfd);
            close(listenfd);
            exit(EXIT_FAILURE);
        }
        
        else
            close(connfd);
    }

	close(listenfd);
}

