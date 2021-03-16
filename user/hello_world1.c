#include "user.h"

int main() {
    isize w;
    for (int i = 0; i < 10; i++) {
        printf("Program[1] print%d\n", i);
        w = get_time() + 20;
        while (get_time() < w); // yield();
    }
    return 0;
}
