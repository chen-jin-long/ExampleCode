#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#define QUEUE_INIT_NUM 0

void clean_handler(void * arg) {
    if (arg) {
        pthread_mutex_unlock((pthread_mutex_t *)arg);
    }
}

int queue_init(QueueEntry_t **head, int max_num) {
    if (max_num <= 0 ) return -QUEUE_OVER_MAX_NUM;
    if (head == NULL) return -QUEUE_POINIT_NULL;
    if (*head != NULL)  return -QUEUE_ERR_INITED_ENTRY;
    int ret = QUEUE_ERR_UNKNOWN;
    QueueEntry_t *entry = (QueueEntry_t *)malloc(sizeof(QueueEntry_t));
    if (entry) {
        entry->tail = NULL;
        entry->header = NULL;
        entry->max_num = max_num;
        entry->cur_num = QUEUE_INIT_NUM;
        *head = entry;
        pthread_mutex_init(&entry->lock, NULL);
        pthread_cond_init(&entry->queue_empty, NULL);
        pthread_cond_init(&entry->queue_full, NULL);
        ret = QUEUE_OPER_SUCCESS;
    } else {
        ret = -QUEUE_ERR_MALLOC;
    }
    return ret;
}

int queue_push(QueueEntry_t *entry, void *data) {
    int ret = -QUEUE_ERR_UNKNOWN;

    if (entry == NULL || data == NULL) {
        return -QUEUE_ENTRY_NULL;
    }

    pthread_cleanup_push(clean_handler, &entry->lock);
    pthread_mutex_lock(&entry->lock);
    /* 解决linux 多线程虚假唤醒需要对条件使用while, 不能用if */
    while (entry->cur_num >= entry->max_num) {
        pthread_cond_wait(&entry->queue_full, &entry->lock);
    }

    Queue_t *queue = (Queue_t *)malloc(sizeof(Queue_t));
    if (queue) {
        queue->next = NULL;
        queue->data = data;
        printf("push: %d\n", (int)(*(int *)data));
        if (entry->cur_num == QUEUE_INIT_NUM) {
            entry->header = queue;
            entry->tail = queue;
        } else {
            entry->tail->next = queue;
        }
        entry->tail = queue;
        entry->cur_num++;
        ret = QUEUE_OPER_SUCCESS;
        //printf("[%s] cur_num = %d\n", __FUNCTION__, entry->cur_num);
        pthread_mutex_unlock(&entry->lock);
        pthread_cond_signal(&entry->queue_empty);
        //return ret;
    } else {
        printf("[%s]malloc failed.\n", __FUNCTION__);
        ret = -QUEUE_ERR_MALLOC;
        pthread_mutex_unlock(&entry->lock);
    }
    //pthread_mutex_unlock(&entry->lock);
    pthread_cleanup_pop(0);
    return ret;
}

Queue_t * queue_get(QueueEntry_t *entry) {
    if (entry == NULL) return NULL;
    Queue_t *cur = NULL;
 
    pthread_cleanup_push(clean_handler, &entry->lock);
    pthread_mutex_lock(&entry->lock);

    /* 解决linux 多线程虚假唤醒需要对条件使用while, 不能用if */
    while (entry->cur_num <= 0) {
    //if (entry->header == NULL) {
       // printf("[1][%s] cur_num = %d\n", __FUNCTION__, entry->cur_num);
        pthread_cond_wait(&entry->queue_empty, &entry->lock);
    }

    cur = entry->header;
    entry->cur_num--;
    /*
    printf("[2][%s] cur_num = %d\n", __FUNCTION__, entry->cur_num);
    if (cur == NULL) {
        printf("[1]cur is NULL.\n");
    }
    */
    if (entry->header) {
        entry->header = entry->header->next;
    }
    /*
    if (cur == NULL) {
        printf("[2]cur is NULL.\n");
    }
    */
    //pthread_cond_signal(&entry->queue_full);

    pthread_mutex_unlock(&entry->lock);
    pthread_cleanup_pop(0);
    pthread_cond_signal(&entry->queue_full);
    /*
    if (cur == NULL) {
        printf("[3]cur is NULL.\n");
    }
    */
    return cur;
}

int queue_destroy(QueueEntry_t *entry) {

    int ret = -QUEUE_ERR_UNKNOWN;
    if (entry == NULL) {
        return -QUEUE_ENTRY_NULL;
    }
    printf("%s....\n", __FUNCTION__);
    pthread_mutex_lock(&entry->lock);
    printf("%s get lock start..\n", __FUNCTION__);
    Queue_t *cur = entry->header;
    while(cur) {
        Queue_t *temp = cur;
        cur = cur->next;
        free(temp);
        temp = NULL;
    }

    entry->cur_num = QUEUE_INIT_NUM;
    pthread_mutex_unlock(&entry->lock);
    printf("%s get lock end..\n", __FUNCTION__);
    pthread_mutex_destroy(&entry->lock);
    pthread_cond_destroy(&entry->queue_empty);
    pthread_cond_destroy(&entry->queue_full);
    if (entry) {
        free(entry);
        entry = NULL;
    }

    return QUEUE_OPER_SUCCESS;
}