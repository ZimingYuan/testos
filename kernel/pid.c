#include "kernel.h"

usize pcurrent;
struct pnode {
    struct list lnode;
    usize pid;
};
struct list *precycled;
void pid_init() {
    pcurrent = 0;
    precycled = bd_malloc(sizeof(struct list));
    lst_init(precycled);
}
usize pid_alloc() {
    usize pid;
    if (! lst_empty(precycled)) {
        struct pnode *x = lst_pop(precycled);
        pid = x->pid; bd_free(x);
    } else pid = pcurrent++;
    return pid;
}
void pid_dealloc(usize pid) {
    if (pid >= pcurrent) goto fail;
    for (struct list *p = precycled->next; p != precycled; p = p->next) {
        if (((struct pnode *)p)->pid == pid) goto fail;
    }
    struct pnode *x = bd_malloc(sizeof(struct pnode));
    x->pid = pid; lst_push(precycled, x);
    return;
fail: panic("pid_dealloc failed!");
}
