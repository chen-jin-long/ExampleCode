#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<pthread.h>

#define MAX_LINE 100
#define SERV_PORT 8030
#define POKER_MSG_LEN 14


#define handle_error(msg) \
  do { \
     perror(msg);\
     exit(EXIT_FAILURE);\
   }while(0)

void stop(int signo);
void *handleRecv(void *arg);
void *handleWrite(void *arg);
void handleMsg(const char *buf);

int sockfd;
int clientId = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc,char *argv[])
{
  struct sockaddr_in server;

  pthread_t pid_r = -1;
  pthread_t pid_w = -1;

  sockfd = socket(AF_INET,SOCK_STREAM,0);
  signal(SIGINT,stop);
  memset(&server,0,sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr=inet_addr("192.168.1.1");
  server.sin_port = htons(SERV_PORT);
  if(-1 == connect(sockfd,(struct sockaddr *)&server,sizeof(server)))
  {
    printf("error....\n");
    handle_error("connet");
  }


  if(pthread_create(&pid_r,NULL,handleRecv,NULL) < -1)
  {
     handle_error("pthread_create recv failed..\n");
  }
  #if 0
  if(pthread_create(&pid_w,NULL,handleWrite,NULL) < -1)
  {
     handle_error("pthread_create write failed..\n");
  }
  #endif
  for(;;);
  printf("finished ....\n");
  close(sockfd);
  return 0;
}

void stop(int signo)
{
  printf("stop...\n");
  close(sockfd);
  exit(0);
}

void *handleWrite(void *arg)
{
  char buf[MAX_LINE] = {0};
  while(1) {
    pthread_mutex_lock(&mutex);
    memset(buf,'b',MAX_LINE);
    buf[MAX_LINE-1] =  '\n';
    pthread_mutex_unlock(&mutex);
    write(sockfd,buf,strlen(buf));
    printf("send:%s",buf);
  }

}

void *handleRecv(void *arg)
{
  char buf[MAX_LINE] = {0};
  int num;
  for(;;)
  {
    num = read(sockfd,buf,MAX_LINE);
    if(num  == -1 || num == 0)
    {
      handle_error("read error..\n");
      exit(0); 
    }
    else
    {
      printf("[num = %d]%s\n", num, buf);
    }
    memset(buf,0,MAX_LINE); 
  }

}