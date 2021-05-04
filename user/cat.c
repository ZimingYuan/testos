#include "user.h"

char content[4097];
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("cat: invalid argument\n"); return -1;
    }
    isize f = open(argv[1], O_RDONLY);
    if (f == -1) {
        printf("cat: file not found\n"); return -1;
    }
    usize len = getsize(f);
    if (len > 4096) {
        printf("cat: file too big\n"); return -1;
    }
    len = read(f, content, 4096); content[len] = '\0';
    printf("%s\n", content); return 0;
}
