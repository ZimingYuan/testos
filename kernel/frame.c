#include "kernel.h"

PhysPageNum fcurrent, fend;
struct vector frecycled;
usize frame_num;
void frame_init() {
    extern char ekernel;
    fcurrent = CEIL((usize)&ekernel);
    fend = FLOOR(MEMORY_END);
    vector_new(&frecycled, sizeof(PhysPageNum));
    frame_num = fend - fcurrent;
}
PhysPageNum frame_alloc() {
    PhysPageNum ppn;
    if (! vector_empty(&frecycled)) {
        ppn = *(PhysPageNum *)vector_back(&frecycled);
        vector_pop(&frecycled);
    } else {
        if (fcurrent == fend) panic("frame_alloc: no free physical page");
        else ppn = fcurrent++;
    }
    memset((void *)PPN2PA(ppn), 0, PAGE_SIZE);
    // printf("frame_alloc:remain%d\n", --frame_num);
    return ppn;
}
void frame_dealloc(PhysPageNum ppn) {
    if (ppn >= fcurrent) goto fail;
    PhysPageNum *x = (PhysPageNum *)frecycled.buffer;
    for (int i = 0; i < frecycled.size; i++)
        if (x[i] == ppn) goto fail;
    vector_push(&frecycled, &ppn);
    // printf("frame_dealloc:remain%d\n", ++frame_num);
    return;
fail: panic("frame_dealloc failed!");
}
