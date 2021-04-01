#include "kernel.h"
#include "queue.h"

PhysPageNum fcurrent, fend;
typedef struct flist {
    PhysPageNum ppn; LIST_ENTRY(flist) entries;
} flist;
LIST_HEAD(flist_head, flist);
struct flist_head frecycled;
usize frame_num;
void frame_init() {
    extern char ekernel;
    fcurrent = CEIL((usize)&ekernel);
    fend = FLOOR(MEMORY_END);
    LIST_INIT(&frecycled);
    frame_num = fend - fcurrent;
}
PhysPageNum frame_alloc() {
    PhysPageNum ppn;
    if (! LIST_EMPTY(&frecycled)) {
        flist *x = LIST_FIRST(&frecycled);
        LIST_REMOVE(x, entries);
        ppn = x->ppn; bd_free(x);
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
    flist *x;
    LIST_FOREACH(x, &frecycled, entries) {
        if (x->ppn == ppn) goto fail;
    }
    x = bd_malloc(sizeof(flist));
    x->ppn = ppn; LIST_INSERT_HEAD(&frecycled, x, entries);
    // printf("frame_dealloc:remain%d\n", ++frame_num);
    return;
fail: panic("frame_dealloc failed!");
}
