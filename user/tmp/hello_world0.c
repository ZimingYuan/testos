#include <stdio.h>
#include <unistd.h>

int main() {
    int p = fork();
    for (int i = 0; i < 10; i++) {
        if (p == 0) {
            printf("Program[0] print%d\n", i);
        } else {
            printf("Program[1] print%d\n", i);
        }
    }
    if (p == 0) execl("hello_world2", NULL); else execl("hello_world1", NULL);
    return 0;
}
