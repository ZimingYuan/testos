#include "kernel.h"
#define INIT_CAPACITY 8

void queue_new(struct queue *q, usize dsize) {
    q->size = q->front = q->tail = 0;
    q->capacity = INIT_CAPACITY; q->dsize = dsize;
    q->buffer = bd_malloc(q->capacity * q->dsize);
}
void queue_push(struct queue *q, void *d) {
    if (q->size == q->capacity) {
        q->capacity <<= 1;
        char *t = bd_malloc(q->capacity * q->dsize);
        memcpy(t, q->buffer, q->tail * q->dsize);
        memcpy(t + (q->capacity - (q->size - q->front)) * q->dsize,
                q->buffer + q->front * q->dsize,
                (q->size - q->front) * q->dsize);
        q->front = q->capacity - (q->size - q->front);
        bd_free(q->buffer); q->buffer = t;
    }
    memcpy(q->buffer + q->tail * q->dsize, d, q->dsize);
    q->tail = (q->tail + 1) % q->capacity; q->size++;
}
void queue_pop(struct queue *q) {
    if (q->size == 0) panic("empty queue pop");
    q->front = (q->front + 1) % q->capacity; q->size--;
}
void *queue_front(struct queue *q) {
    if (! q->size) panic("empty queue front");
    return q->buffer + q->front * q->dsize;
}
int queue_empty(struct queue *q) {
    return ! q->size;
}
void queue_free(struct queue *q) {
    bd_free(q->buffer);
}
