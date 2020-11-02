
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "thread_pool.h"


#define THREAD_POOL_MAX_SIZE 4
#define THREAD_POOL_QUEUE_SIZE 10

static void clean_handler(void * arg) {
    if (arg) {
        pthread_mutex_unlock((pthread_mutex_t *)arg);
    }
}

static void * handle_queue_data(void *param)
{
    thread_pool_t *pool = (thread_pool_t *)param;
    Queue_t *queue  = NULL;
    QueueEntry_t *entry = NULL;
    while(1) {
         if (pool && pool->queue) {
            entry = pool->queue;
            //pthread_cleanup_push(clean_handler, &entry->lock);
            //pthread_mutex_lock(&entry->lock);
            queue = queue_get(entry);
            if (queue && queue->data) {
                pool->func_callback(queue->data);
                if (queue->data) {
                    free(queue->data);
                    queue->data = NULL;
                }
                if (queue) {
                    free(queue);
                    queue = NULL;
                }
            } else {
                printf("CGD NULL....\n");
            }
           // pthread_mutex_unlock(&entry->lock);
           // pthread_cleanup_pop(0);
        }
    }
}

int create_thread_pool(thread_pool_t **pool, int thread_size, thread_pool_callback callback)
{
    int ret = -1, i = 0;
    if (callback == NULL) {
        return -1;
    }
    thread_pool_t *pool_entry = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (pool_entry) {
        memset(pool_entry, 0, sizeof(thread_pool_t));
        *pool = pool_entry;
    } else {
        return -1;
    }
    pool_entry->func_callback = callback;
    pool_entry->thread_size = thread_size;
    pool_entry->queue = NULL;
    ret = queue_init(&pool_entry->queue, THREAD_POOL_QUEUE_SIZE);
    if (ret != QUEUE_OPER_SUCCESS) {
        goto thread_malloc_catch;
    }

    pool_entry->thread_status = (int *)malloc(sizeof(int)*(pool_entry->thread_size));
    if (pool_entry->thread_status) {
        memset(pool_entry->thread_status, THREAD_POOL_INIT, pool_entry->thread_size*(sizeof(int)));
    } else {
        goto thread_malloc_catch;
    }
    pool_entry->thread = (pthread_t *)malloc(sizeof(pthread_t)*(pool_entry->thread_size));
    if (pool_entry->thread == NULL) {
        goto thread_malloc_catch;
    }
    for (i = 0; i < pool_entry->thread_size; i++) {
        ret = pthread_create(&pool_entry->thread[i], NULL, handle_queue_data, *pool);
        if (ret != 0) {
            goto thread_pthread_catch;
        } else {
            pool_entry->thread_status[i] = THREAD_POOL_CREATE;
            ret = 0;
        }
    }

    return ret;
    printf("[%s] error.\n", __FUNCTION__);
thread_pthread_catch:
    for (i = 0; i < pool_entry->thread_size; i++) {
        if (pool_entry->thread_status[i] != THREAD_POOL_INIT) {
            pthread_cancel(pool_entry->thread[i]);
            pool_entry->thread[i] = -1;
        }
    }
thread_malloc_catch:
    if (pool_entry) {
        free(pool_entry);
        pool_entry = NULL;
    }

    return -1;
}

int destroy_thread_pool(thread_pool_t *pool_entry)
{
    int i = 0;
    if (pool_entry == NULL) {
        return -1;
    }
    printf("%s start.\n", __FUNCTION__);
    for (i = 0; i < pool_entry->thread_size; i++) {
        if (pool_entry->thread_status[i] != THREAD_POOL_INIT) {
            pthread_cancel(pool_entry->thread[i]);
            pthread_join(pool_entry->thread[i], NULL);
            pool_entry->thread[i] = -1;
            pool_entry->thread_status[i] = THREAD_POOL_INIT;
        }

    }
    if (pool_entry && pool_entry->queue) {
        queue_destroy(pool_entry->queue);
    }

    return 0;
}