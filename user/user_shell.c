#include "user.h"

#define LINE 1024
#define ARGMAX 64

char line[LINE + 1];
char *argv[ARGMAX]; int argc;

int main() {
    printf("C user shell\n");
    printf(">> ");
    for (;;) {
        gets(line, LINE);
        if (! strcmp(line, "exit")) return 0;
        int f = 0, l = strlen(line); argc = 0;
        for (int i = 0; i < l; i++) {
            if (line[i] == ' ') {
                if (f) {
                    line[i] = '\0'; f = 0;
                }
            } else if (!f) {
                argv[argc++] = line + i; f = 1;
            }
        }
        argv[argc] = 0;
        isize pid = fork();
        if (pid == 0) {
            // child process
            if (argc > 2) {
                if (argv[argc - 2][0] == '<') {
                    isize f = open(argv[argc - 1], O_RDONLY);
                    if (f < 0) {
                        printf("Redirect failed!\n");
                        return -4;
                    }
                    close(0); dup(f); close(f); argv[argc - 2] = 0;
                }
                if (argv[argc - 2][0] == '>') {
                    isize f = open(argv[argc - 1], O_CREAT | O_WRONLY);
                    if (f < 0) {
                        printf("Redirect failed!\n");
                        return -4;
                    }
                    close(1); dup(f); close(f); argv[argc - 2] = 0;
                }
            }
            if (exec(argv[0], argv + 1) == -1) {
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
