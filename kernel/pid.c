#include "kernel.h"
#include "queue.h"

usize pcurrent;
typedef struct plist {
    usize pid; LIST_ENTRY(plist) entries;
} plist;
LIST_HEAD(plist_head, plist);
struct plist_head precycled;
void pid_init() {
    pcurrent = 0;
    LIST_INIT(&precycled);
}
usize pid_alloc() {
    usize pid;
    if (! LIST_EMPTY(&precycled)) {
        plist *x = LIST_FIRST(&precycled);
        LIST_REMOVE(x, entries);
        pid = x->pid; bd_free(x);
    } else pid = pcurrent++;
    return pid;
}
void pid_dealloc(usize pid) {
    if (pid >= pcurrent) goto fail;
    plist *x;
    LIST_FOREACH(x, &precycled, entries) {
        if (x->pid == pid) goto fail;
    }
    x = bd_malloc(sizeof(plist));
    x->pid = pid; LIST_INSERT_HEAD(&precycled, x, entries);
    return;
fail: panic("pid_dealloc failed!");
}
