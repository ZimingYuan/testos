#include "kernel.h"
#include "syscall.h"

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
__attribute__((aligned (4))) void trap_from_kernel() {
    usize scause, sepc;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, sepc\n"
            :"=r"(scause), "=r"(sepc)
            );
    printf("%p %p\n", scause, sepc);
    panic("trap_from_kernel");
}
void trap_handler() {
    asm volatile("csrw stvec, %0"::"r"(trap_from_kernel));
    usize scause, stval;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, stval\n"
            :"=r"(scause), "=r"(stval)
            );
    TrapContext *cx = current_user_trap_cx();
    if (scause == 8) {
        cx->sepc += 4;
        cx->x[10] = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
    } else if (scause == (1L << 63) + 5) {
        set_next_trigger();
        suspend_current_and_run_next();
    } else {
        printf("scause:%p\n", scause); printf("sepc:%p\n", cx->sepc);
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
