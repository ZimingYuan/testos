#include "kernel.h"
#include "filec.h"

isize sys_write(usize fd, char *buffer, usize len) {
    switch (fd) {
        case FD_STDOUT: {
            char *pbuffer = bd_malloc(len + 1); pbuffer[len] = '\0';
            copy_area(current_user_pagetable(), (VirtAddr)buffer, pbuffer, len, 0);
            printf(pbuffer); bd_free(pbuffer); return (isize)len;
        }
    }
}
isize sys_read(usize fd, char *buffer, usize len) {
    switch (fd) {
        case FD_STDIN: {
            char *pbuffer = bd_malloc(len);
            for (int i = 0; i < len; i++) {
                char c;
                while (!(c = console_getchar())) suspend_current_and_run_next();
                pbuffer[i] = c;
            }
            copy_area(current_user_pagetable(), (VirtAddr)buffer, pbuffer, len, 1);
            bd_free(pbuffer); return (isize)len;
        }
    }
}
isize sys_yield() {
    suspend_current_and_run_next();
    return 0;
}
isize sys_exit(int xstate) {
    printf("Application exited with code %d\n", xstate);
    exit_current_and_run_next(xstate);
}
isize sys_get_time() {
    return (isize)get_time_ms();
}
isize sys_fork() {
    return fork();
}
isize sys_exec(char *name, usize len) {
    char *pbuffer = bd_malloc(len + 1); pbuffer[len] = '\0';
    copy_area(current_user_pagetable(), (VirtAddr)name, pbuffer, len, 0);
    isize r = exec(pbuffer); bd_free(pbuffer); return r;
}
isize sys_waitpid(isize pid, int *exit_code) {
    int _exit_code; isize r = waitpid(pid, &_exit_code);
    copy_area(current_user_pagetable(), (VirtAddr)exit_code, &_exit_code, sizeof(int), 1);
    return r;
}
