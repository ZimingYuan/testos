#include "kernel.h"
#include "filec.h"

usize illegal_rw(File *self, char *buffer, usize len) {
    return -1;
}
void illegal_c(File *self) {
}

usize std_write(File *self, char *buffer, usize len) {
    printf(buffer); return len;
}
usize std_read(File *self, char *buffer, usize len) {
    for (int i = 0; i < len; i++) {
        char c;
        while (!(c = console_getchar()))
            suspend_current_and_run_next();
        buffer[i] = c;
    }
    return len;
}

typedef struct Pipe {
    usize refcnt, wrefcnt;
    struct queue q;
} Pipe;
usize pipe_read(File *self, char *buffer, usize len) {
    Pipe *p = (Pipe *)self->bind;
    for (int i = 0; i < len; i++) {
        while (queue_empty(&p->q)) {
            if (p->wrefcnt == 0) {
                buffer[i] = '\0'; return i;
            }
            suspend_current_and_run_next();
        }
        buffer[i] = *(char *)queue_front(&p->q); queue_pop(&p->q);
    }
    return len;
}
usize pipe_write(File *self, char *buffer, usize len) {
    Pipe *p = (Pipe *)self->bind;
    for (usize i = 0; i < len; i++) queue_push(&p->q, buffer + i);
    return len;
}
void pipe_copy(File *self) {
    Pipe *p = (Pipe *)self->bind;
    if (self->read == illegal_rw) p->wrefcnt++; p->refcnt++;
}
void pipe_close(File *self) {
    Pipe *p = (Pipe *)self->bind;
    if (self->read == illegal_rw) p->wrefcnt--;
    self->occupied = 0; p->refcnt--;
    if (p->refcnt == 0) {
        queue_free(&p->q); bd_free(p);
    }
}
void make_pipe(usize *p) {
    Pipe* t = bd_malloc(sizeof(Pipe));
    queue_new(&t->q, 1); t->refcnt = 2; t->wrefcnt = 1;
    File *f = alloc_fd(p); f->bind = (void *)t;
    f->read = pipe_read; f->write = illegal_rw;
    f->copy = pipe_copy; f->close = pipe_close;
    f = alloc_fd(p + 1); f->bind = (void *)t;
    f->read = illegal_rw; f->write = pipe_write;
    f->copy = pipe_copy; f->close = pipe_close;
}

usize fnode_write(File *self, char *buffer, usize len) {
    len = inode_write((FNode *)self->bind, buffer, len);
    return len;
}
usize fnode_read(File *self, char *buffer, usize len) {
    len = inode_read((FNode *)self->bind, buffer, len);
    return len;
}
void fnode_copy(File *self) {
    ((FNode *)self->bind)->refcnt++;
}
void fnode_close(File *self) {
    FNode *i = (FNode *)self->bind;
    self->occupied = 0; i->refcnt--; bcache_save();
    if (i->refcnt == 0) bd_free(i);
}
isize make_fnode(char *path, usize flags) {
    FNode *i;
    if (flags & O_CREAT) {
        i = inode_get(path, 1);
        inode_clear(i);
    } else {
        i = inode_get(path, 0); if (!i) return -1;
        if (flags & O_TRUNC) inode_clear(i);
    }
    i->refcnt = 1; i->offset = 0; usize fd;
    File *f = alloc_fd(&fd); f->bind = (void *)i;
    f->read = fnode_read; f->write = fnode_write;
    if (flags & O_RDONLY) f->write = illegal_rw;
    if (flags & O_WRONLY) f->read = illegal_rw;
    f->copy = fnode_copy; f->close = fnode_close;
    return fd;
}
