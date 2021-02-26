#include "kernel.h"

isize sys_write(usize fd, char *buffer, usize len) {
    switch (fd) {
        case FD_STDOUT:
            printf(buffer);
            return (isize)len;
    }
}
isize sys_yield() {
    suspend_current_and_run_next();
    return 0;
}
isize sys_exit(int xstate) {
    printf("Application exited with code %d\n", xstate);
    exit_current_and_run_next();
}
isize sys_get_time() {
    return (isize)get_time_ms();
}
