#include "kernel.h"
#include "filec.h"

isize sys_write(usize fd, char *buffer, usize len) {
    File *f = current_user_file(fd); if (!f) return -1;
    char *pbuffer = bd_malloc(len + 1); pbuffer[len] = '\0';
    copy_area(current_user_pagetable(), (VirtAddr)buffer, pbuffer, len, 0);
    usize r = f->write(f, pbuffer, len);
    bd_free(pbuffer); return (isize)r;
}
isize sys_read(usize fd, char *buffer, usize len) {
    File *f = current_user_file(fd); if (!f) return -1;
    char *pbuffer = bd_malloc(len); usize r = f->read(f, pbuffer, len);
    copy_area(current_user_pagetable(), (VirtAddr)buffer, pbuffer, len, 1);
    bd_free(pbuffer); return (isize)r;
}
isize sys_close(usize fd) {
    File *f = current_user_file(fd); if (!f) return -1;
    f->close(f); return 0;
}
isize sys_yield() {
    suspend_current_and_run_next();
    return 0;
}
isize sys_exit(int xstate) {
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
isize sys_gets(char *buffer, usize maxlen) {
    char *pbuffer = bd_malloc(maxlen + 1), c; isize len = 0;
    for (;;) {
        while (!(c = console_getchar()))
            suspend_current_and_run_next();
        switch (c) {
            case LF:
            case CR:
                consputc('\n'); pbuffer[len] = '\0'; goto over;
            case BS:
            case DL: {
                if (len) {
                    consputc(BS);
                    consputc(' ');
                    consputc(BS);
                    len--;
                }
                break;
            }
            default: {
                if (len < maxlen) {
                    consputc(c);
                    pbuffer[len++] = c;
                }
                break;
            }
        }
    }
over:   copy_area(current_user_pagetable(), (VirtAddr)buffer, pbuffer, maxlen + 1, 1);
        bd_free(pbuffer); return len;
}
isize sys_pipe(usize *pipe) {
    usize t[2]; make_pipe(t);
    copy_area(current_user_pagetable(), (VirtAddr)pipe,
            t, 2 * sizeof(usize), 1);
    return 0;
}
isize sys_getpid() {
    return (isize)getpid();
}
