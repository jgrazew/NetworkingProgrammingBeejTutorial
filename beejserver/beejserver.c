//server.c is a stream socket server demo from the beej tutorial

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <features.h>

#define PORT "3490" // the port that users will be connecting to

#define BACKLOG 10 //how many pending connections the queue will hold- goes into the listen function

void sigcld_handler(int s){
    //waitpid() might overwrite errno. so we save and restore it
    int saved_errno = errno;

    /*waitpid() is used to wait for a process to finish; 
    plugging in -1 makes you wait for any child process; WNOHANG returns immediately 
    if no child has exited*/
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

//get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd; //listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connectors address info
    socklen_t sin_size;
    struct sigaction sa; //i think this is responcible for reaping zombie processes
    int yes =1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    //loop though all the results and bind to the first one that we can
    //getaddrinfo() can return several results but we want to just take the 1st one
    //that works (p. 10 Beej), that is why we are breaking at the end of the loop
    //so we dont finish the results
    for(p = servinfo; p != NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("server: socket");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    } //end for loop

    freeaddrinfo(servinfo); //all done with this structure

    if(p == NULL){
        fprintf(stderr, "the server failed to bind");
        exit(1);
    }

    if(listen(sockfd, BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigcld_handler; //reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        perror("sigaction");
        exit(1);
    }

    printf("server is waiting for connections...\n");

    while(1){ //main accept loop
        sin_size = sizeof their_addr;
        /*if there are no pending connections when accept() is called, 
        the call blocks until a connection request arrives (p. 1157 linux programming interface)*/
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if(new_fd == -1){
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        /*basically we have accepted a connection request so we are going to spawn another 
        process to send the data we want from that connection and then we will continue
        our while loop with the parent to accept another connection, remeber listen() queue 
        will accept up to 10 in the backlog*/
        if(!fork()){ /*this is the child process; returns 0; child process will make its own
                    copy of everything- I THINK that is why we want to close sockfd below*/
            close(sockfd); //child doesnt need the listener
            if(send(new_fd, "Hello, world!", 13, 0) == -1){
                perror("send");
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd); //parent doesnt need this the child does, but parent still needs sockfd
    }

    return 0;
}