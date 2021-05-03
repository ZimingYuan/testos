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
isize sys_exec(char *name, char **argv) {
    struct vector vname; vector_new(&vname, 1);
    PhysAddr pgtbl = current_user_pagetable(); usize l = 0;
    for (;;) {
        char c; copy_area(pgtbl, (VirtAddr)name + l, &c, 1, 0); 
        vector_push(&vname, &c); if (c == '\0') break; else l++;
    }
    struct vector vargv; vector_new(&vargv, 1); usize argc = 0;
    for (;;) {
        char *addr;
        copy_area(pgtbl, (VirtAddr)argv + argc * sizeof(char *),
                &addr, sizeof(char *), 0); 
        if (!addr) break; else argc++; l = 0;
        for (;;) {
            char c; copy_area(pgtbl, (VirtAddr)addr + l, &c, 1, 0);
            vector_push(&vargv, &c); if (c == '\0') break; else l++;
        }
    }
    isize r = exec(vname.buffer, vargv.buffer, vargv.size);
    vector_free(&vname); vector_free(&vargv); return r;
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
isize sys_open(char *path, usize flags) {
    struct vector vpath; vector_new(&vpath, 1);
    PhysAddr pgtbl = current_user_pagetable(); usize l = 0;
    for (;;) {
        char c; copy_area(pgtbl, (VirtAddr)path + l, &c, 1, 0); 
        vector_push(&vpath, &c); if (c == '\0') break; else l++;
    }
    isize r = make_fnode(vpath.buffer, flags);
    vector_free(&vpath); return r;
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
