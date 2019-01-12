//program shows the IP adress for a given host on the command line
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]){
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if(argc != 2){
        fprintf(stderr, "showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AFNET or AFNET6 to force version
    hints.ai_socktype = SOCK_STREAM;

//hints gives info about what we are going to print in res such as if it is ipv4 or ipv6 IE they are hints
    if((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n\n", argv[1]);

    for( p = res; p != NULL; p = p->ai_next){
        void* addr;
        char *ipver;

        //get the pointer to the address itself, different fields in IPV$ and IPV^
        if(p->ai_family == AF_INET){ //IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else{ //IPV6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        //convert the IP to a string and print it!
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf(" %s: %s\n", ipver, ipstr);

        freeaddrinfo(res); //free the linked list

        return 0;
    }

    
}