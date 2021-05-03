#include "kernel.h"

#define NBUF 16

struct buf buf[NBUF];
int bcache_state;

void bcache_init() {
    for (int i = 0; i < NBUF; i++) buf[i].occupied = 0;
    bcache_state = 0;
}
void bcache_rw(int blockno, int st, int len, void *data, int write) {
    // while (bcache_state) suspend_current_and_run_next();
    bcache_state = 1;
    for (int i = 0; i < NBUF; i++)
        if (buf[i].occupied && buf[i].blockno == blockno) {
            if (write) {
                memcpy(buf[i].data + st, data, len); buf[i].modify = 1;
            } else memcpy(data, buf[i].data + st, len);
            buf[i].time = 0; goto updtime;
        }
    int mx = 0;
    for (int i = 0; i < NBUF; i++) 
        if (!buf[i].occupied) {
            buf[i].occupied = 1; buf[i].blockno = blockno;
            virtio_disk_rw(buf + i, 0);
            if (write) {
                memcpy(buf[i].data + st, data, len); buf[i].modify = 1;
            } else {
                memcpy(data, buf[i].data + st, len); buf[i].modify = 0;
            }
            buf[i].time = 0; goto updtime;
        } else if (buf[i].time > buf[mx].time) mx = i;
    buf[mx].time = 0; if (buf[mx].modify) virtio_disk_rw(buf + mx, 1);
    buf[mx].blockno = blockno; virtio_disk_rw(buf + mx, 0);
    if (write) {
        memcpy(buf[mx].data + st, data, len); buf[mx].modify = 1;
    } else {
        memcpy(data, buf[mx].data + st, len); buf[mx].modify = 0;
    }
updtime:
    for (int i = 0; i < NBUF; i++)
        if (buf[i].occupied) buf[i].time++;
    bcache_state = 0;
}
void bcache_save() {
    // while (bcache_state) suspend_current_and_run_next();
    bcache_state = 1;
    for (int i = 0; i < NBUF; i++) if (buf[i].modify) {
        virtio_disk_rw(buf + i, 1); buf[i].modify = 0;
    }
    bcache_state = 0;
}
