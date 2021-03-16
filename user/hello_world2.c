#include "user.h"

int main() {
    isize w;
    for (int i = 0; i < 10; i++) {
        printf("Program[2] print%d\n", i);
        w = get_time() + 30;
        while (get_time() < w); // yield();
    }
    return 0;
}
