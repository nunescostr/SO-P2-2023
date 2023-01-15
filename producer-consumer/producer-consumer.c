#include "producer-consumer.h"
#include "logging.h"
#include <pthread.h>

pthread_mutex_t mutex;
pthread_cond_t podeProd;
pthread_cond_t podeCons;


int count = 0;

pthread_cond_t not_empty;
pthread_cond_t not_full;

// pcq_create: create a queue, with a given (fixed) capacity
//
// Memory: the queue pointer must be previously allocated
// (either on the stack or the heap)

int pcq_create(pc_queue_t *queue, size_t capacity) {
    queue->pcq_buffer = (void**) malloc(sizeof(void*) * capacity);
    if (queue->pcq_buffer == NULL) {
        return -1;
    }
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;

    if (pthread_mutex_init(&queue->pcq_current_size_lock, NULL) != 0){
        return -1;
    }
    
    if(pthread_mutex_init(&queue->pcq_head_lock, NULL) != 0){
        return -1;
    }
    
    if(pthread_mutex_init(&queue->pcq_tail_lock, NULL) != 0){
        return -1;
    }
    
    if(pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL) != 0){
        return -1;
    }
    
    if(pthread_cond_init(&queue->pcq_pusher_condvar, NULL) != 0){
        return -1;
    }
    
    if(pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL) != 0)
        return -1;
    
    if(pthread_cond_init(&queue->pcq_popper_condvar, NULL) != 0){
        return -1;
    }

    return 0;
}


// pcq_enqueue: insert a new element at the front of the queue
//
// If the queue is full, sleep until the queue has space
int pcq_destroy(pc_queue_t *queue) {
    free(queue->pcq_buffer);
    if(pthread_mutex_destroy(&queue->pcq_current_size_lock) != 0){
        return -1;
    }
    if(pthread_mutex_destroy(&queue->pcq_head_lock) != 0){
        return -1;
    }
    if(pthread_mutex_destroy(&queue->pcq_tail_lock) != 0){
        return -1;
    }
    if(pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock) != 0){
        return -1;
    }
    if(pthread_cond_destroy(&queue->pcq_pusher_condvar) != 0){
        return -1;
    }
    if(pthread_mutex_destroy(&queue->pcq_popper_condvar_lock) != 0){
        return -1;
    }
    if(pthread_cond_destroy(&queue->pcq_popper_condvar) != 0){
        return -1;
    }
    return 0;
}

// pcq_enqueue: insert a new element at the front of the queue
//
// If the queue is full, sleep until the queue has space

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    while (queue->pcq_current_size == queue->pcq_capacity) {
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_current_size_lock);
    }
    queue->pcq_current_size++;
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    pthread_mutex_lock(&queue->pcq_head_lock);
    int head = (int)queue->pcq_head;
    pthread_mutex_unlock(&queue->pcq_head_lock);

    queue->pcq_buffer[head] = elem;

    pthread_mutex_lock(&queue->pcq_head_lock);
    queue->pcq_head = (unsigned long)(head + 1) % queue->pcq_capacity;
    pthread_mutex_unlock(&queue->pcq_head_lock);

    pthread_cond_signal(&queue->pcq_popper_condvar);
    return 0;
}

  

// pcq_dequeue: remove an element from the back of the queue
//
// If the queue is empty, sleep until the queue has an element

void *pcq_dequeue(pc_queue_t *queue) {
    void *elem;
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    while (queue->pcq_current_size == 0) {
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_current_size_lock);
    }
    queue->pcq_current_size--;
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    pthread_mutex_lock(&queue->pcq_tail_lock);
    int tail = (int)queue->pcq_tail;
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    elem = queue->pcq_buffer[tail];

    pthread_mutex_lock(&queue->pcq_tail_lock);
    queue->pcq_tail = (unsigned long)(tail + 1) % queue->pcq_capacity;
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    pthread_cond_signal(&queue->pcq_pusher_condvar);
    return elem;
}


