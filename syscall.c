#include "kernel.h"

isize sys_write(usize fd, char *buffer, usize len) {
    switch (fd) {
        case FD_STDOUT:
            printf(buffer);
            return (int)len;
    }
}
isize sys_exit(int xstate) {
    printf("Application exited with code %d\n", xstate);
    run_next_app();
}
