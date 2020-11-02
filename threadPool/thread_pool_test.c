#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include <unistd.h>
#include "thread_pool.h"


pthread_t g_producer_1 = -1;
pthread_t g_producer_2 = -1;
pthread_t g_producer_3 = -1;
//pthread_t g_consumer = -1;
thread_pool_t *g_pool = NULL;
pthread_mutex_t data_lock;
int g_start_num = 0;


static void clean_handler(void * arg) {
    if (arg) {
        pthread_mutex_unlock((pthread_mutex_t *)arg);
    }
}


static void sig_usr(int signum)
{
   if (signum == SIGUSR1) {
       printf("[%ld]recv SIGUSR1.\n", pthread_self());
       pthread_exit(0);
   }

}

void *produce_int_num(void *param)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);//SIG_BLOCK SIG_SETMASK 会屏蔽掉SIGINT，但SIG_UNBLOCK不会屏蔽SIGINT

    struct sigaction action;
    action.sa_flags = 0;
    action.sa_handler = sig_usr;
    sigaction(SIGUSR1, &action, NULL);
    thread_pool_t *pool = (thread_pool_t *)param;
    while(1) {
        if (pool && pool->queue) {
            pthread_cleanup_push(clean_handler, &data_lock);
            pthread_mutex_lock(&data_lock);
            int *num = (int *)malloc(sizeof(int));
            *num = g_start_num;
            queue_push(pool->queue, num);
            g_start_num++;
            pthread_mutex_unlock(&data_lock);
            pthread_cleanup_pop(0);
            //sleep(2);
        }
    }
    return NULL;
}

#if 0
void *consume_int_num(void *param)
{
    thread_pool_t *pool = (thread_pool_t *)param;
    Queue_t *queue_data  = NULL;
    int *num = NULL;
    while(1) {
         if (pool && pool->queue) {
            queue_data  = queue_get(pool->queue);
            if (queue_data) {
                num = (int *)queue_data->data;
                if (num) {
                     printf("get: %d\n", *num);
                     free(num);
                     num = NULL;
                     free(queue_data);
                     queue_data = NULL;
                     sleep(2);
                }

            }

        }
    }
    return NULL;
}
#endif

void consume_int_num(void *data)
{
    int *num = (int *)data;
    if (num) {
        printf("get: %d\n", *num);
        //free(num);
        //num = NULL;
    }
}

void my_pthread_kill(pthread_t id)
{
    if (id != -1) {
        pthread_kill(id, SIGUSR1);
    }
}
void stop(int sig)
{
    printf("%s start \n", __FUNCTION__);
    //pthread_cancel(g_producer_1);
   // pthread_cancel(g_producer_2);
   // pthread_cancel(g_producer_3);

    my_pthread_kill(g_producer_1);
    my_pthread_kill(g_producer_2);
    my_pthread_kill(g_producer_3);
    printf("%s progrss. \n", __FUNCTION__);
    //pthread_cancel(g_consumer);
    //g_consumer = -1;
    pthread_join(g_producer_1, NULL);
    pthread_join(g_producer_2, NULL);
    pthread_join(g_producer_3, NULL);

    g_producer_1 = -1;
    g_producer_2 = -1;
    g_producer_3 = -1;

    if (g_pool) {
        destroy_thread_pool(g_pool);
        g_pool = NULL;
    }
 
    //pthread_mutex_lock(&data_lock);
    pthread_mutex_destroy(&data_lock);
    exit(0);
    printf("%s end\n", __FUNCTION__);
}

int main()
{
    int ret = -1;
    signal(SIGINT, stop);
    pthread_mutex_init(&data_lock, NULL);
    ret = create_thread_pool(&g_pool, 10, consume_int_num);
    if (ret != 0) {
        printf("[%s]failed.\n", __FUNCTION__);
        return -1;
    }
    pthread_create(&g_producer_1, NULL, produce_int_num, g_pool);
    pthread_create(&g_producer_2, NULL, produce_int_num, g_pool);
    pthread_create(&g_producer_3, NULL, produce_int_num, g_pool);
    for(;;);
    return ret;
}
