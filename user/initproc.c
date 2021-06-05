#include "user.h"

int main() {
    int f = 0; //fork();
    if (f == 0) exec("hello_world0");
    else {
        for (;;) {
            int exit_code = 0;
            int pid = wait(&exit_code);
            if (pid == -1) {
                yield(); continue;
            }
            printf("[initproc] Released a zombie process, pid=%d, exit_code=%d\n", pid, exit_code);
            if (pid == f) break;
        }
    }
    return 0;
}
