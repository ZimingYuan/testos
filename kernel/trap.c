#include "kernel.h"
#include "syscall.h"

isize syscall(usize syscall_id, usize args0, usize args1, usize args2) {
    switch (syscall_id) {
        case SYSCALL_WRITE:
            return sys_write(args0, (char *)args1, args2);
        case SYSCALL_READ:
            return sys_read(args0, (char *)args1, args2);
        case SYSCALL_EXIT:
            return sys_exit((int)args0);
        case SYSCALL_YIELD:
            return sys_yield();
        case SYSCALL_GET_TIME:
            return sys_get_time();
        case SYSCALL_FORK:
            return sys_fork();
        case SYSCALL_EXEC:
            return sys_exec((char *)args0, args1);
        case SYSCALL_WAITPID:
            return sys_waitpid(args0, (int *)args1);
        case SYSCALL_GETS:
            return sys_gets((char *)args0, args1);
        default:
            printf("%p\n", syscall_id);
            panic("Other syscall");
    }
}
__attribute__ ((aligned (4))) void trap_from_kernel() {
    usize scause, sepc, stvec;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, sepc\n"
            "csrr %2, stvec\n"
            :"=r"(scause), "=r"(sepc), "=r"(stvec)
            );
    printf("scause:%p\n", scause); printf("sepc:%p\n", sepc);
    printf("stvec:%p\n", stvec);
    panic("trap_from_kernel");
}
void trap_handler() {
    asm volatile("csrw stvec, %0"::"r"(trap_from_kernel));
    usize scause, stval, sepc;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, stval\n"
            "csrr %2, sepc\n"
            :"=r"(scause), "=r"(stval), "=r"(sepc)
            );
    TrapContext *cx = current_user_trap_cx();
    if (scause == 8) {
        cx->sepc += 4;
        isize result = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
        cx = current_user_trap_cx();
        cx->x[10] = result;
    } else if (scause == (1L << 63) + 5) {
        set_next_trigger();
        suspend_current_and_run_next();
    } else {
        printf("scause:%p\n", scause);
        printf("stval:%p\n", stval);
        printf("sepc:%p %p\n", cx->sepc, sepc);
        printf("sp:%p\n", cx->x[2]);
        printf("s0:%p\n", cx->x[8]);
        panic("Other trap");
    }
    trap_return();
}
void trap_return() {
    asm volatile("csrw stvec, %0"::"r"(TRAMPOLINE));
    usize user_satp = PGTB2SATP(current_user_pagetable());
    void __alltraps(); void __restore();
    asm volatile(
            "mv a0, %1\n"
            "mv a1, %2\n"
            "jr %0"
            ::"r"((usize)__restore - (usize)__alltraps + TRAMPOLINE),
            "r"(TRAP_CONTEXT), "r"(user_satp)
            :"a0", "a1"
            );
    panic("trap_return");
}
