#include <pthread.h>
#include "lab.h"

typedef struct queue {
    void **data;        // Array to hold the queue elements
    int capacity;      // Maximum capacity of the queue
    int size;          // Current size of the queue
    int head;         // Index of the first element
    int tail;          // Index of the next free position
    bool shutdown;     // Flag to indicate if the queue is shutting down
    pthread_mutex_t lock; // lock for thread safety
    pthread_cond_t not_empty; // Condition variable for not empty
    pthread_cond_t not_full;  // Condition variable for not full
} *queue_t;

queue_t queue_init(int capacity){
    // Allocate memory for the queue
    queue_t q = (queue_t)malloc(sizeof(queue_t));
    if (q == NULL) {
        return NULL;
    }

    // Initialize the queue
    q->capacity = capacity;
    q->size = 0;
    q->head = 0;
    q->tail = 1;
    q->shutdown = false;

    // Allocate memory for the data
    q->data = (void **)malloc(capacity * sizeof(void *));
    if (q->data == NULL) {
        free(q);
        return NULL;
    }

    // Initialize the mutex and condition variables
    // If any intialization fails, free the allocated memory and return NULL
    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        free(q->data);
        free(q);
        return NULL;
    }
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&q->lock);
        free(q->data);
        free(q);
        return NULL;
    }
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        pthread_cond_destroy(&q->not_empty);
        pthread_mutex_destroy(&q->lock);
        free(q->data);
        free(q);
        return NULL;
    }

    return q;
}

void queue_destroy(queue_t q){
    if (q == NULL) {
        return;
    }
    // Lock the queue to ensure thread safety
    pthread_mutex_lock(&q->lock);
    // Free the data array
    free(q->data);
    // Destroy the mutex and condition variables
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    // Free the queue structure itself
    free(q);
}

void enqueue(queue_t q, void *data){
    if (q == NULL || data == NULL) {
        return;
    }
    // Lock the queue to ensure thread safety
    pthread_mutex_lock(&q->lock);
    // Wait until the queue is not full and shutdown is not called
    while (q->size == q->capacity && !q->shutdown) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    if (q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return;
    }
    // Add the data to the queue
    q->data[q->tail] = data;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    // Signal that the queue is not empty if it was previously empty
    if (q->size == 1) {
        pthread_cond_signal(&q->not_empty);
    }
    // Unlock the queue now that modifications are done
    pthread_mutex_unlock(&q->lock);

}

void *dequeue(queue_t q){
    if (q == NULL) {
        return NULL;
    }
    // Lock the queue to ensure thread safety
    pthread_mutex_lock(&q->lock);
    // Wait until the queue is not empty and shutdown is not called
    while (q->size == 0 && !q->shutdown) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    if (q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    // Retrieve the data from the queue
    void *data = q->data[q->head];
    // Update the head index and size
    q->data[q->head] = NULL; // Clear the data at the head (Leave no witnesses)
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    // Signal that the queue is not full if it was previously full
    if (q->size == q->capacity - 1) {
        pthread_cond_signal(&q->not_full);
    }
    // Unlock the queue now that modifications are done
    pthread_mutex_unlock(&q->lock);
    return data;
}

void queue_shutdown(queue_t q){
    if (q == NULL) {
        return;
    }
    // Lock the queue to ensure thread safety
    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

bool is_empty(queue_t q){
    if (q == NULL) {
        return true;
    }
    // Lock the queue to ensure thread safety
    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

bool is_shutdown(queue_t q){
    if (q == NULL) {
        return true;
    }
    // Lock the queue to ensure thread safety
    pthread_mutex_lock(&q->lock);
    bool shutdown = q->shutdown;
    pthread_mutex_unlock(&q->lock);
    return shutdown;
}