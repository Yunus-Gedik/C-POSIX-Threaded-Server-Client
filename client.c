#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/un.h>
#include <time.h>
#include <signal.h>

int main(int argc,char* argv[]){
    int sockfd;
    int connectfd;
    
    clock_t start, end;
    double difference;
    start = clock();
    
    char* serverrandom;
    char* price;
    char* taylor;
    char* provider;
    
    if(argc<6){
        fprintf( stderr, "usage error!\n");
        exit(1);
    }
    
    
    char thestring[50] = "";
    strcat(thestring,argv[1]);
    strcat(thestring," ");
    strcat(thestring,argv[2]);
    strcat(thestring," ");
    strcat(thestring,argv[3]);
    
    if((sockfd = socket(AF_LOCAL,SOCK_STREAM,0))==-1){
        fprintf( stderr, "socket() error!\n");
        exit(1);
    }
    
    struct sockaddr_un port;
    memset(&port, 0, sizeof(struct sockaddr_un));
    port.sun_family = AF_LOCAL;
    strncpy(port.sun_path,argv[5],sizeof(port.sun_path) - 1);
    
    if( (connectfd = connect(sockfd, (struct sockaddr *)&port, sizeof(port))) == -1){
        fprintf( stderr, "connect() error!\n");
        exit(1);
    } 
    
    printf("Client %s is requesting %s %s from server 127.0.0.1:%s\n",argv[1],argv[2],argv[3],argv[5]);
    write(sockfd,thestring,sizeof(thestring));
    
    read(sockfd,&thestring,sizeof(thestring));
    printf("%s\n",thestring);
    provider = strtok(thestring," ");
    serverrandom = strtok(NULL," ");
    taylor = strtok(NULL," ");
    price = strtok(NULL," ");
    
    end = clock();
    difference = ((double) (end - start)) / CLOCKS_PER_SEC;
    
printf("%s’s task completed by %s in %s seconds, cos(%s)=%s, cost is %sTL,total time spent %f seconds.\n",argv[1],provider,serverrandom,argv[3],taylor,price,strtod(serverrandom,NULL)+difference);
    
    return(0);
}