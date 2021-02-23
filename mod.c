#include "kernel.h"

isize syscall(usize syscall_id, usize args0, usize args1, usize args2) {
    switch (syscall_id) {
        case SYSCALL_WRITE:
            sys_write(args0, (char *)args1, args2);
            break;
        case SYSCALL_EXIT:
            sys_exit((int)args0);
            break;
    }
}
TrapContext *trap_handler(TrapContext *cx) {
    usize scause, stval;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, stval\n"
            :"=r"(scause), "=r"(stval)
            );
    switch (scause & (~(1L << 63))) {
        case 8:
            cx->sepc += 4;
            cx->x[10] = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
            break;
        default:
            panic("Other trap");
    }
    return cx;
}
