#include "user.h"

#define LINE 1024

char line[LINE + 1]; int linen;

int main() {
    printf("C user shell\n");
    printf(">> ");
    for (;;) {
        gets(line, LINE);
        if (! strcmp(line, "exit")) return 0;
        isize pid = fork();
        if (pid == 0) {
            // child process
            if (exec(line) == -1) {
                printf("Error when executing!\n");
                return -4;
            }
        } else {
            int exit_code = 0;
            isize exit_pid = waitpid(pid, &exit_code);
            printf("Shell: Process %d exited with code %d\n", pid, exit_code);
        }
        printf(">> ");
    }
    return 0;
}
