#include "user.h"

int main() {
    isize w;
    for (int i = 0; i < 10; i++) {
        printf("Program[0] print%d\n", i);
        w = get_time() + 10;
        while (get_time() < w); // yield();
    }
    return 0;
}
