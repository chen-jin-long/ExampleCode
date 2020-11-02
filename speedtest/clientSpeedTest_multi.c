#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
  int sock;
  long allNum;
} ThreadParam;


#define MAX_LINE (1048576*20)
#define SERV_PORT 8030
#define SERV_IP "192.168.1.1"
#define DEFAULT_MAX_CLIENT_NUM 8
#define DEFAULT_TEST_TIME 30
#define SPEED_TEST_END_MSG  "close speed test\n"
#define SPEED_TEST_START_MSG "start speed test\n"
void stop(int signo);
void * clientFunc(void *param);
void handler(int sig);
void * clientStartSpeedTestFunc(void *arg);


int clientId = 0;
int g_controllFd = -1;
//pthread_t thread[MAX_CLIENT_NUM];
pthread_t *thread = NULL;
pthread_t controlThread;
char speedTestIp[128] = {0};
char speedTestPort[128] = {0};
ThreadParam *g_param = NULL;
char *g_buf = NULL;
int g_max_client_num = DEFAULT_MAX_CLIENT_NUM;
int g_max_wait_seconds = DEFAULT_TEST_TIME;
static void sig_usr(int signum)
{
   if (signum == SIGUSR1) {
       printf("[%ld]recv SIGUSR1.\n", pthread_self());
       pthread_exit(0);
   }

}

void handler(int sig)
{
    int i = 0, j = 0;
    long long sum = 0;
    printf("recv a sig=%d\n", sig);

      for(i = 0 ; i < 3; i++) {
          //for (j = 0; j < g_max_client_num; j++) {
            if (g_controllFd != -1) {
              int len = send(g_controllFd, SPEED_TEST_END_MSG, strlen(SPEED_TEST_END_MSG), 0);
              printf("[%s] len = %d\n", __FUNCTION__, len);
            }
          //}

      }
      g_controllFd = -1;


    for (i = 0; i < g_max_client_num; i++) {
       if (g_param) {
          close(g_param[i].sock);
          g_param[i].sock = -1;
       }
       pthread_kill(thread[i], SIGUSR1);
    }
    printf("=========end speed test=========\n");
    for(i = 0; i < g_max_client_num; i++) {
      if (g_param) {
        printf("allNum[%d] = %ld\n", i, g_param[i].allNum);
        sum += g_param[i].allNum;
      }
    }

    if (g_param) {
      free(g_param);
      g_param = NULL;
    }
    if (g_buf) {
      free(g_buf);
      g_buf = NULL;
    }
    printf("sum = %lld, average = %lld(M/s)\n", sum, sum/1024/1024/g_max_wait_seconds);
    exit(0);
}

int main(int argc,char **argv)
{
  if (argc < 3) {
    printf("please set thread num and seconds,if not use 8 theads and 30s. eg: ./multi 8 30\n");
  } else {
    g_max_client_num = atoi(argv[1]);
    if (g_max_client_num <= 0 && g_max_client_num > 20) {
        printf("g_max_client_num is err, please check:%d\n", g_max_client_num);
        return -1;
    }
    g_max_wait_seconds = atoi(argv[2]);
    if (g_max_wait_seconds < 10 && g_max_wait_seconds > 61) {
        printf("g_max_wait_seconds is err, please check:%d\n", g_max_wait_seconds);
        return -1;
    }
    printf("client_num = %d, seconds = %d\n", g_max_client_num, g_max_wait_seconds);
  }

  thread = (pthread_t *)malloc(sizeof(pthread_t)*g_max_client_num);
  if (thread == NULL) {
    printf("malloc error.\n");
    return -1;
  }
  g_param = (ThreadParam *)malloc(sizeof(ThreadParam) * g_max_client_num);
  if (g_param == NULL) {
    return -1;
  }
  memset(g_param, 0,  sizeof(ThreadParam) * g_max_client_num);

  g_buf = (char *)malloc(MAX_LINE);
  if (g_buf) {
     memset(g_buf, 0, MAX_LINE);
  } else {
    goto err_free_param;
  }


  if (signal(SIGALRM, handler) == SIG_ERR) {
      printf("error...\n");
  }
  
  clientStartSpeedTestFunc(NULL);
  /* 交互有bug, 向servr发了start消息后，server可能还未开始发送测速报文，这里需要sleep */
  sleep(5);
  struct timeval tv_interval = {0, 0}; // 后续不在调用
  struct timeval tv_value = {g_max_wait_seconds, 0}; //第一次调用是60秒。
  struct itimerval it;
  it.it_interval = tv_interval;
  it.it_value = tv_value;
  setitimer(ITIMER_REAL, &it, NULL);
  printf("=========start speed test=========\n");
  //pthread_create(&controlThread, NULL, clientStartSpeedTestFunc, NULL);
  int i = 0;
  for (i = 0; i < g_max_client_num; i++) {
    pthread_create(&thread[i], NULL, clientFunc, &g_param[i]);
  }


  for(;;);

  printf("finished ....\n");

  if (g_buf) {
    free(g_buf);
    g_buf = NULL;
  }
err_free_param:
  if (g_param) {
    free(g_param);
    g_param = NULL;
  }
  return 0;
}

void * clientFunc(void *arg)
{
  ThreadParam *param = (ThreadParam *)arg;
  struct sigaction action;
  action.sa_flags = 0;
  action.sa_handler = sig_usr;
  sigaction(SIGUSR1, &action, NULL);
  int sockfd = -1;
  sockfd = socket(AF_INET,SOCK_STREAM,0);
  if (sockfd < 0) {
    printf("[speedtestclient]usock error...\n");
    return NULL;
  }

  struct sockaddr_in server;
  memset(&server,0,sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr=inet_addr(SERV_IP);
  server.sin_port = htons(SERV_PORT);
  if(-1 == connect(sockfd,(struct sockaddr *)&server,sizeof(server)))
  {
    printf("error....\n");
     return NULL;
  }

  param->sock = sockfd;
  printf("[%ld]sockfd = %d\n", pthread_self(), sockfd);
  int num = 0;
  for(;;)
  {
    num = read(sockfd,g_buf,MAX_LINE);
    if(num  == -1 || num == 0)
    {
      printf("read error..\n");
    }
    else
    {
      param->allNum += num;
    }
  }
  return NULL;
}

void * clientStartSpeedTestFunc(void *arg)
{
  //ThreadParam *param = (ThreadParam *)arg;
 // struct sigaction action;
  //action.sa_flags = 0;
 // action.sa_handler = sig_usr;
 // sigaction(SIGUSR1, &action, NULL);
  int sockfd = -1;
  sockfd = socket(AF_INET,SOCK_STREAM,0);
  if (sockfd < 0) {
    printf("[speedtestclient]usock error...\n");
    return NULL;
  }
  g_controllFd = sockfd;
  struct sockaddr_in server;
  memset(&server,0,sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr=inet_addr(SERV_IP);
  server.sin_port = htons(SERV_PORT);
  if(-1 == connect(sockfd,(struct sockaddr *)&server,sizeof(server)))
  {
    printf("error....\n");
     return NULL;
  }

 // param->sock = sockfd;
  printf("[%ld]sockfd = %d\n", pthread_self(), sockfd);
  int i = 0;
  for (i = 0; i < 1; i++) {
    if (sockfd != -1) {
       send(sockfd,SPEED_TEST_START_MSG, strlen(SPEED_TEST_START_MSG), 0);
    }
  }
  //pthread_exit(0);
  return NULL;
}