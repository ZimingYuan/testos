#include "user.h"

#define LINE 1024
#define LF 0x0a
#define CR 0x0d
#define DL 0x7f
#define BS 0x08

char line[LINE + 1]; int linen;

int main() {
    printf("C user shell\n");
    linen = 0; printf(">> ");
    for (;;) {
        char c = getchar();
        switch (c) {
            case LF:
            case CR: {
                printf("\n");
                if (linen) {
                    line[linen] = '\0';
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
                    linen = 0;
                }
                printf(">> ");
                break;
            }
            case BS:
            case DL: {
                if (linen) {
                    consputc(BS);
                    consputc(' ');
                    consputc(BS);
                    linen--;
                }
                break;
            }
            default: {
                if (linen < LINE) {
                    consputc(c);
                    line[linen++] = c;
                }
                break;
            }
        }
    }
    return 0;
}
