#ifndef __CTSGW_THREAD_POOL__
#define __CTSGW_THREAD_POOL__
#include <stdio.h>
#include "queue.h"

typedef enum {
    THREAD_POOL_INIT = 0,
    THREAD_POOL_CREATE,
    THREAD_POOL_RUN,
    THREAD_POOL_JOIN,
    THREAD_POOL_EXIST,
}THREAD_POOL_STATE;

typedef void (* thread_pool_callback)(void *pool);

typedef struct {
    pthread_t *thread;
    int thread_size;
    int *thread_status;
    QueueEntry_t *queue;
    thread_pool_callback func_callback;
}thread_pool_t;



int create_thread_pool(thread_pool_t **pool, int thread_size, thread_pool_callback callback);
int destroy_thread_pool(thread_pool_t *pool_entry);
#endif