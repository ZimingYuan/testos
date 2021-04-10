#include "kernel.h"
#define INIT_CAPACITY 8

void vector_new(struct vector *v, usize dsize) {
    v->size = 0; v->capacity = INIT_CAPACITY; v->dsize = dsize;
    v->buffer = bd_malloc(v->capacity * v->dsize);
}
void vector_push(struct vector *v, void *d) {
    if (v->size == v->capacity) {
        v->capacity <<= 1;
        char *t = bd_malloc(v->capacity * v->dsize);
        memcpy(t, v->buffer, v->size * v->dsize);
        bd_free(v->buffer); v->buffer = t;
    }
    memcpy(v->buffer + (v->size++) * v->dsize, d, v->dsize);
}
void vector_pop(struct vector *v) {
    if (v->size == 0) panic("empty vector pop"); v->size--;
}
void *vector_back(struct vector *v) {
    if (! v->size) panic("empty vector back");
    return v->buffer + (v->size - 1) * v->dsize;
}
int vector_empty(struct vector *v) {
    return ! v->size;
}
void vector_free(struct vector *v) {
    bd_free(v->buffer);
}

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
