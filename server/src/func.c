#include "func.h"

//determine whether prefix is str's prefix
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

//concatenate a and b, and put the result in b
int concatenate(char* a, char* b) {
    char temp[8192];
    memset(temp, 0, 8192);
    if (strlen(a) + strlen(b) < 8192) {
        strcpy(temp, a);  
        strcat(temp, b);  
        strcpy(b, temp);
        return 1;
    } 
    else
        return 0;
}

char* get_ip(){
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    static char host[NI_MAXHOST];
    memset(host, 0, NI_MAXHOST);

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_netmask == NULL) continue;
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            char mask[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(netmask->sin_addr), mask, INET_ADDRSTRLEN);
            if(strcmp(mask, "255.0.0.0") == 0)
                continue;
            return host;
        }
    }
    return NULL;
}
