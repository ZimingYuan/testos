#include "user.h"

int main() {
    printf("initproc\n");
    int f = fork();
    if (f == 0) exec("user_shell");
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
