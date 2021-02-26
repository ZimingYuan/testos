#include "kernel.h"

isize syscall(usize syscall_id, usize args0, usize args1, usize args2) {
    switch (syscall_id) {
        case SYSCALL_WRITE:
            sys_write(args0, (char *)args1, args2);
            break;
        case SYSCALL_EXIT:
            sys_exit((int)args0);
            break;
        case SYSCALL_YIELD:
            sys_yield();
            break;
        case SYSCALL_GET_TIME:
            sys_get_time();
            break;
        default:
            printf("%p\n", syscall_id);
            panic("Other syscall");
    }
}
TrapContext *trap_handler(TrapContext *cx) {
    usize scause, stval;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, stval\n"
            :"=r"(scause), "=r"(stval)
            );
    if (scause == 8) {
        cx->sepc += 4;
        cx->x[10] = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
        return cx;
    }
    if (scause == (1L << 63) + 5) {
        set_next_trigger();
        suspend_current_and_run_next();
        return cx;
    }
    printf("%p\n", scause); printf("%p\n", cx->sepc);
    panic("Other trap"); return cx;
}
