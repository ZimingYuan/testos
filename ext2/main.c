#include "kernel.h"

char help[] =
"Usage: ext2 -I|-O <outside path> <inside path> <image path>\n"
"-I: Copy outside file to image\n"
"-O: Copy file in image to outside";
char *image_path;
int main(int argc, char **argv) {
    if (argc != 5) {
        puts(help); return -1;
    }
    image_path = argv[4]; init_ext2();
    if (strcmp(argv[1], "-I") == 0) {
        FILE *f = fopen(argv[2], "rb");
        if (!f) {
            puts("ext2: File outside not found"); return -1;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f); fseek(f, 0, SEEK_SET);
        char *s = malloc(size); fread(s, 1, size, f);
        FNode *fn = inode_get(argv[3], 1); fn->offset = 0;
        inode_clear(fn); inode_write(fn, s, size);
        free(s); fclose(f); bd_free(fn); bcache_save();
    } else if (strcmp(argv[1], "-O") == 0) {
        FNode *fn = inode_get(argv[3], 0);
        if (!fn) {
            puts("ext2: File in image not found"); return -1;
        }
        fn->offset = 0; long size = *(int *)(fn->dinode + 4);
        char *s = malloc(size); inode_read(fn, s, size);
        FILE *f = fopen(argv[2], "wb"); fwrite(s, 1, size, f);
        free(s); fclose(f); bd_free(fn);
    } else {
        puts(help); return -1;
    }
    return 0;
}
