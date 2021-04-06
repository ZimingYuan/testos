#include "kernel.h"

usize pcurrent;
struct vector precycled;
void pid_init() {
    pcurrent = 0;
    vector_new(&precycled, sizeof(usize));
}
usize pid_alloc() {
    usize pid;
    if (! vector_empty(&precycled)) {
        pid = *(usize *)vector_back(&precycled);
        vector_pop(&precycled);
    } else pid = pcurrent++;
    return pid;
}
void pid_dealloc(usize pid) {
    if (pid >= pcurrent) goto fail;
    usize *x = (usize *)precycled.buffer;
    for (int i = 0; i < precycled.size; i++) {
        if (x[i] == pid) goto fail;
    }
    vector_push(&precycled, &pid);
    return;
fail: panic("pid_dealloc failed!");
}
