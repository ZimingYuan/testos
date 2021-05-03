#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BSIZE 1024

typedef unsigned long usize;
// bio.c
struct buf {
    int occupied, disk, modify, blockno, time;
    char data[BSIZE];
};

// inode.c
typedef struct FNode {
    int refcnt, dnum; usize offset;
    char dinode[128];
} FNode;
void init_ext2();
FNode *inode_get(char *, int);
usize inode_read(FNode *, char *, usize);
usize inode_write(FNode *, char *, usize);
void inode_clear(FNode *);
char *inode_list(FNode *, int *);

// bcache.c
void bcache_init();
void bcache_rw(int, int, int, void *, int);
void bcache_save();

// others.c
void virtio_disk_rw(struct buf *, int);
void bd_free(void*);
void *bd_malloc(usize);
void panic(char *);
