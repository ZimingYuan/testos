#include "user.h"

int main() {
    isize p = fork(); isize w;
    for (int i = 0; i < 10; i++) {
        if (p == 0) {
            printf("Program[0] print%d\n", i);
            w = get_time() + 10;
        } else {
            printf("Program[1] print%d\n", i);
            w = get_time() + 20;
        }
        while (get_time() < w) yield();
    }
    char *t = 0;
    if (p == 0) exec("hello_world2", &t); else exec("hello_world1", &t);
    return 0;
}
