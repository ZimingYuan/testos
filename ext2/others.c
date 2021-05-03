#include "kernel.h"

extern char *image_path;
void virtio_disk_rw(struct buf *b, int w) {
    FILE *f = fopen(image_path, "r+");
    if (!f) panic("ext2: Image file not found");
    fseek(f, b->blockno * BSIZE, SEEK_SET);
    if (w) fwrite(b->data, 1, BSIZE, f);
    else fread(b->data, 1, BSIZE, f);
    fclose(f);
}
void bd_free(void* d) {
    free(d);
}
void *bd_malloc(usize s) {
    return malloc(s);
}
void panic(char *s) {
    printf(s); for (;;);
}
