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
