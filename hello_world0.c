#include "user.h"

int main() {
    char *s = "Program[0] print0\n";
    isize w;
    for (int i = 0; i < 10; i++) {
        s[16] = (i % 10) + '0';
        write(FD_STDOUT, s);
        w = get_time() + 10;
        while (get_time() < w); // yield();
    }
    return 0;
}
