#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/un.h>
#include <math.h>
#include <signal.h>

#define M_PI 3.14159265358979323846

struct provider{
    char name[20];          //Provider name
    int performance;        
    int price;
    int duration;
    pthread_t thread;       //Provider's own thread id
    int degree[2];          //provider's queue for degrees
    int fd[2];              //provider's queue for connection file des.'es.
    int done;               //Jobs count done by provider
    struct provider* next;
};

struct provider *head = NULL;

pthread_mutex_t mutex;
pthread_mutexattr_t mutexattr;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int totaldone = 0;

void signalhandler(int sig){
    fprintf( stderr, "Termination signal received.\n");
    struct provider *temp;
    
    temp=head;
    fprintf( stderr, "Terminating all clients\n");
    fprintf( stderr, "Terminating all providers.\n");
    
    temp=head;
    
    while(temp != NULL){
        if(temp->fd[0] != -1){
            close(temp->fd[0]);
        }
        if(temp->fd[1] != -1){
            close(temp->fd[1]);
        }
        temp = temp->next;
    }
    
    fprintf( stderr, "Statistics\n");
    fprintf( stderr, "Name       Number of clients served\n");
    temp = head;
    
    while(temp != NULL){
        fprintf( stderr, "%s           %d\n",temp->name,temp->done);
        temp = temp->next;
    }
    
    fprintf( stderr, "Goodbye.\n");
    temp=head;
    while(temp != NULL){
        pthread_kill(temp->thread,SIGKILL);
        temp = temp->next;
    }
}

double fac(int n){
    int c;
    double result = 1;
 
    for (c = 1; c <= n; c++){
        result = result * c;
    }
    return result;
}

double taylor(double degree){
    double sum=0;
    degree *= M_PI / 180.0;
    for(int i=0; i<=15; i++ ) {
        sum += (double)pow( -1,i )*(double)pow( degree,2*i )/(double)fac( 2*i );
    }
    return(sum);
}

void delete(struct provider **head,int n){
    if (*head == NULL){
        return;
    }
    
    struct provider *temp = *head; 
    if (n == 0){
        *head = temp->next; 
        free(temp);       
        return;
    }
    
    for (int i=0; temp!=NULL && i<n-1; i++){
         temp = temp->next;
    }
    
    struct provider *next = temp->next->next;
 
    free(temp->next);
 
    temp->next = next;
}

void* threadfunction(void* empty){
    struct provider *threadnode = head;
    double degree;
    int fd;
    int control =1 ;
    int whichnode=0;
    double random[2];
    int que;
    char buf[26];
    char thestring[50]="";
    
    sigset_t intmask;
    sigemptyset(&intmask);
    sigaddset(&intmask, SIGINT);
    sigprocmask(SIG_BLOCK, &intmask, NULL);
    

    while(threadnode!=NULL){
        if(pthread_equal(pthread_self(),threadnode->thread)!=0){
            break;
        }
        threadnode = threadnode->next;
        ++whichnode;
    }
    
    while(1){
        thestring[0] = '\0';
        buf[0] = '\0';
        pthread_mutex_lock(&mutex);
        while(threadnode->degree[0] == -1){
            if(control){
                fprintf( stderr, "Provider %s is waiting for tasks.\n",threadnode->name);
                control =0 ;
            }
            pthread_cond_wait(&cond,&mutex);
        }
        control =1 ;
        if(threadnode->degree[1]!=-1){
            que=2;
        }
        else{
            que=1;
        }
        
        for(int i=0; i<que; ++i){
            degree = threadnode->degree[0];
            fd = threadnode->fd[0];
            threadnode->degree[0] = threadnode->degree[1];
            threadnode->fd[0] = threadnode->fd[1];
            threadnode->fd[1] = -1;
            threadnode->degree[1] = -1;
            threadnode->done += 1;
            
            fprintf( stderr, "Provider %s is processing task number %d: %f\n",threadnode->name,totaldone,degree);
            
            random[0] = 5.0 + (double)rand() / (double)(((double) RAND_MAX)/10.0);
            random[1] = random[0];
            
            threadnode->duration -= random[0];
                        
            pthread_mutex_unlock(&mutex);
    
            while((random[1] = sleep(random[1])) != 0){}
            
            strcat(thestring,threadnode->name);
            strcat(thestring," ");
            
            buf[0] = '\0';
            sprintf(buf,"%f",random[0]);
            strcat(thestring,buf);
            strcat(thestring," ");
            
            degree=taylor(degree);
            buf[0] = '\0';
            sprintf(buf,"%f",degree);
            strcat(thestring,buf);
            strcat(thestring," ");
            
            buf[0] = '\0';
            sprintf(buf,"%d",threadnode->price);
            strcat(thestring,buf);
            strcat(thestring," ");
        
            write(fd,thestring,sizeof(thestring));

            close(fd);
            
        }
        if(threadnode->duration <= 0){
            fprintf( stderr, "Provider %s is logging off...\n",threadnode->name);
            free(threadnode);
            delete(&head,whichnode);
            break;
        }
    }
    
}

void insert(struct provider **head,FILE* datafp){
    struct provider* p = (struct provider*) malloc(sizeof(struct provider));
    struct provider *last = *head; 

    fscanf(datafp,"%s",p->name);
    fscanf(datafp,"%d",&p->performance);
    fscanf(datafp,"%d",&p->price);
    fscanf(datafp,"%d",&p->duration);
    p->degree[0]=-1;
    p->degree[1]=-1;
    p->fd[0]=-1;
    p->fd[1]=-1;
    p->done=0;
    p->next=NULL;
 
    if (*head == NULL){
       *head = p;
       return;
    }
 
    while (last->next != NULL){
        last = last->next;
    }
    last->next = p;
    return;
}

int main(int argc,char* argv[]){
    int sockfd;
    int acceptfd;
    FILE* datafp;
    char tempfordata[20];
    int threadcount = 0;
    struct provider *temp;
    
    if(argc<4){
        fprintf( stderr, "usage error!\n");
        exit(1);
    }
    
    
    datafp = fopen(argv[2],"r");
    logfile = fopen(argv[3],"w");
    fprintf( stderr, "Logs kept at %s\n",argv[2]);
    
    for(int i=0;i<4;++i){
        fscanf(datafp,"%s,",tempfordata);
    }
    
    while(!feof(datafp)){
        insert(&head,datafp);
        ++threadcount;
    }
    fprintf( stderr, "%d provider threads created.\n",threadcount);
    
    fprintf( stderr, "   Name    Performance Price Duration\n");
    fprintf( stderr, "-------------------------------------\n");
    
    temp = head;
    while(temp != NULL){
        fprintf( stderr, "%8s        %d     %6d %6d  \n",temp->name,temp->performance,temp->price,temp->duration);
        temp = temp->next;
    }
    
    
    
    if(pthread_mutexattr_init(&mutexattr)!=0){
        fprintf( stderr, "phtread_mutexattr_init() error!\n");
        exit(1);
    }
    if(pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_ERRORCHECK)!=0){
        fprintf( stderr, "phtread_mutexattr_settype() error!\n");
        exit(1);
    }
    if(pthread_mutex_init(&mutex,&mutexattr)!=0){
        fprintf( stderr, "phtread_mutex_init() error!\n");
        exit(1);
    }
    
    srand((unsigned) time(0));
    
    temp = head;
    while(temp != NULL){
        if(pthread_create(&temp->thread,NULL,threadfunction,NULL) != 0){
            fprintf( stderr, "phtread_create() error!\n");
            fprintf( logfile, "phtread_create() error!\n");
            exit(1);
        }
        temp = temp->next;
    }
    
    signal(SIGINT, signalhandler);
    
    //Open socket
    if((sockfd = socket(AF_LOCAL,SOCK_STREAM,0))==-1){
        fprintf( stderr, "socket() error!\n");
        fprintf( logfile, "socket() error!\n");
        exit(1);
    }
    
    //Nonblock socket
    //fcntl(sockfd, F_SETFL, O_NONBLOCK);
    
    //Bind port to socket
    struct sockaddr_un port;
    memset(&port, 0, sizeof(struct sockaddr_un));
    port.sun_family = AF_LOCAL;
    strncpy(port.sun_path, argv[1],sizeof(port.sun_path) - 1);
    
    if (bind (sockfd, (struct sockaddr *) &port, sizeof(struct sockaddr_un)) == -1){
        unlink(argv[1]);
        if (bind (sockfd, (struct sockaddr *) &port, sizeof(struct sockaddr_un)) == -1){
            fprintf( stderr, "bind() error!\n");
            exit(1);
        }
    }
    
    //ready to "listen()"
    if(listen(sockfd,9999) == -1){
        fprintf( stderr, "listen() error!\n");
        unlink(argv[1]);
        exit(1);
    }

    //accept() incoming connections
    int size = sizeof(struct sockaddr_un);
    struct sockaddr_un sun;
    char thestring[50]="";
    char* tempname;
    char* temppriority;
    double tempdegree;
    char* tempdeg;
    int maxquality=-1;
    int mincost=head->price;
    char chosenone[20]="";
    
    fprintf( stderr, "Server is waiting for client connections at port %s\n",argv[1]);
    while(1){
        acceptfd = accept(sockfd,(struct sockaddr *) &sun,&size);
        read(acceptfd,&thestring,sizeof(thestring));
        
        temp=head;
        tempname = strtok(thestring," ");
        temppriority = strtok(NULL," ");
        tempdeg=strtok(NULL," ");
        tempdegree = strtod(tempdeg,NULL);
        
        pthread_mutex_lock(&mutex);
        
        fprintf( stderr, "Client %s (%s %f) connected,",tempname,temppriority,tempdegree);
        
        if(temppriority[0]=='Q'){
            maxquality = -1;
            temp = head;
            chosenone[0] = '\0';
            
            while(temp!=NULL){
                if(temp->performance > maxquality && temp->degree[1] == -1 ){
                    chosenone[0] = '\0';
                    strcpy(chosenone,temp->name);
                    maxquality = temp->performance;
                }
                temp=temp->next;
            }
            
        }
        
        else if(temppriority[0]=='C'){
            temp=head;
            mincost = head->price;
            chosenone[0] = '\0';
            
            while(temp!=NULL){
                if(temp->price < mincost && temp->degree[1] == -1 ){
                    chosenone[0] = '\0';
                    strcpy(chosenone,temp->name);
                    mincost = temp->price;
                }
                temp=temp->next;
            }

        }
            
        else if(temppriority[0]=='T'){
        
            chosenone[0] = '\0';
            temp = head;
            
            while(temp!=NULL){
            
                if(temp->degree[0] == -1 ){
                    chosenone[0] = '\0';
                    strcpy(chosenone,temp->name);
                    break;
                }
                temp=temp->next;
            }
            
        }
        
        temp=head;

        while(strcmp(temp->name,chosenone) != 0){
            temp=temp->next;
        }
        
        if(temp->degree[0] != -1){
            temp->degree[1] = tempdegree;
            temp->fd[1] = acceptfd;
        }
        else{
            temp->degree[0] = tempdegree;
            temp->fd[0] = acceptfd;
        }
        
        fprintf( stderr, "forwarded to provider %s\n",temp->name);
        
        //Wake all threads up
        if(pthread_cond_broadcast(&cond)!=0){
            fprintf( stderr, "broadcast() error!\n");
            exit(1);
        }
        pthread_mutex_unlock(&mutex);
        
    }
    
    close(acceptfd);
    close(sockfd);
    unlink(argv[1]);
    
}
