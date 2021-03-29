#include "kernel.h"

PhysPageNum fcurrent, fend;
struct fnode {
    struct list lnode;
    usize ppn;
};
struct list *frecycled;
usize frame_num;
void frame_init() {
    extern char ekernel;
    fcurrent = CEIL((usize)&ekernel);
    fend = FLOOR(MEMORY_END);
    frecycled = bd_malloc(sizeof(struct list));
    lst_init(frecycled);
    frame_num = fend - fcurrent;
}
PhysPageNum frame_alloc() {
    PhysPageNum ppn;
    if (! lst_empty(frecycled)) {
        struct fnode *x = lst_pop(frecycled);
        ppn = x->ppn; bd_free(x);
    } else {
        if (fcurrent == fend) panic("frame_alloc: no free physical page");
        else ppn = fcurrent++;
    }
    memset((void *)PPN2PA(ppn), 0, PAGE_SIZE);
    printf("frame_alloc:remain%d\n", --frame_num);
    return ppn;
}
void frame_dealloc(PhysPageNum ppn) {
    if (ppn >= fcurrent) goto fail;
    for (struct list *p = frecycled->next; p != frecycled; p = p->next) {
        if (((struct fnode *)p)->ppn == ppn) goto fail;
    }
    struct fnode *x = bd_malloc(sizeof(struct fnode));
    x->ppn = ppn; lst_push(frecycled, x);
    printf("frame_dealloc:remain%d\n", ++frame_num);
    return;
fail: panic("frame_dealloc failed!");
}
void print_frame_num() {
    printf("frame_num:%d\n", frame_num);
}
