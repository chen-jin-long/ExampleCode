#ifndef __CTSGW_QUEUE__
#define __CTSGW_QUEUE__

#include <stdio.h>
#include <pthread.h>

typedef struct Queue
{
    struct Queue *next;
    void *data;
}Queue_t;

typedef struct QueueEntry 
{
    Queue_t *header;
    Queue_t *tail;
    //void *data;
    int max_num;
    int cur_num;
    pthread_mutex_t lock;
    pthread_cond_t queue_empty;
    pthread_cond_t queue_full;
}QueueEntry_t;

typedef enum {
    QUEUE_OPER_SUCCESS = 0,
    QUEUE_ENTRY_NULL = 1,
    QUEUE_DATA_NULL = 2,
    QUEUE_OVER_MAX_NUM = 3,
    QUEUE_ERR_MALLOC = 4,
    QUEUE_ERR_INITED_ENTRY = 5,
    QUEUE_POINIT_NULL = 6,
    QUEUE_ERR_UNKNOWN,
}QUEUE_ERROR;

int queue_init(QueueEntry_t **head, int max_num);
int queue_push(QueueEntry_t *entry, void *data);
Queue_t * queue_get(QueueEntry_t *entry);
int queue_destroy(QueueEntry_t *entry);
#endif