#include "user.h"

int main() {
    int f = fork();
    if (f == 0) {
        char *t = 0;  exec("user_shell", &t);
    } else {
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
