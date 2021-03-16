#include "kernel.h"

PhysPageNum current, end;
struct rnode {
    struct list lnode;
    usize ppn;
};
struct list *recycled;
usize frame_num;
void frame_init() {
    extern char ekernel;
    current = CEIL((usize)&ekernel);
    end = FLOOR(MEMORY_END);
    recycled = bd_malloc(sizeof(struct list));
    lst_init(recycled);
    frame_num = end - current;
}
PhysPageNum frame_alloc() {
    int ret; PhysPageNum ppn;
    if (! lst_empty(recycled)) {
        struct rnode *x = lst_pop(recycled);
        ppn = x->ppn; bd_free(x);
    } else {
        if (current == end) panic("frame_alloc: no free physical page");
        else ppn = current++;
    }
    memset((void *)PPN2PA(ppn), 0, PAGE_SIZE);
    frame_num--;
    return ppn;
}
void frame_dealloc(PhysPageNum ppn) {
    if (ppn >= current) goto fail;
    for (struct list *p = recycled->next; p != recycled; p = p->next) {
        if (((struct rnode *)p)->ppn == ppn) goto fail;
    }
    struct rnode *x = bd_malloc(sizeof(struct rnode));
    x->ppn = ppn; lst_push(recycled, x);
    frame_num++;
    return;
fail: panic("frame_dealloc failed!");
}
void print_frame_num() {
    printf("frame_num:%d\n", frame_num);
}
