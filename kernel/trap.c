#include "kernel.h"
#include "syscall.h"

void kernel_intr_switch(int on) {
    usize sstatus; asm volatile("csrr %0, sstatus":"=r"(sstatus));
    if (on) sstatus |= (1 << 1); else sstatus &= ~(1 << 1);
    asm volatile("csrw sstatus, %0"::"r"(sstatus));
}
isize syscall(usize syscall_id, usize args0, usize args1, usize args2) {
    switch (syscall_id) {
        case SYSCALL_WRITE:
            return sys_write(args0, (char *)args1, args2);
        case SYSCALL_READ:
            return sys_read(args0, (char *)args1, args2);
        case SYSCALL_CLOSE:
            return sys_close(args0);
        case SYSCALL_EXIT:
            return sys_exit((int)args0);
        case SYSCALL_YIELD:
            return sys_yield();
        case SYSCALL_GET_TIME:
            return sys_get_time();
        case SYSCALL_FORK:
            return sys_fork();
        case SYSCALL_EXEC:
            return sys_exec((char *)args0, (char **)args1);
        case SYSCALL_WAITPID:
            return sys_waitpid(args0, (int *)args1);
        case SYSCALL_GETS:
            return sys_gets((char *)args0, args1);
        case SYSCALL_PIPE:
            return sys_pipe((usize *)args0);
        case SYSCALL_OPEN:
            return sys_open((char *)args0, args1);
        case SYSCALL_DUP:
            return sys_dup(args0);
        case SYSCALL_GETSIZE:
            return sys_getsize(args0);
        case SYSCALL_GETPID:
            return sys_getpid();
        default:
            printf("%p\n", syscall_id);
            panic("Other syscall");
    }
}
void device_interrupt_handle() {
    int irq = plic_claim();
    if (irq == VIRTIO0_IRQ) {
        virtio_disk_intr();
    } else if (irq) {
        printf("Other IRQ: %d\n", irq);
    }
    if (irq) plic_complete(irq);
}
void trap_from_kernel() {
    usize scause, sepc;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, sepc\n"
            :"=r"(scause), "=r"(sepc) 
            );
    if (scause == (1L << 63) + 9) {
        device_interrupt_handle();
    } else {
        printf("scause:%p\n", scause); printf("sepc:%p\n", sepc);
        panic("Other trap from kernel");
    }
}
void trap_handler() {
    void __kerneltrap();
    asm volatile("csrw stvec, %0"::"r"(__kerneltrap));
    time_intr_switch(0); kernel_intr_switch(1);
    usize scause, stval;
    asm volatile (
            "csrr %0, scause\n"
            "csrr %1, stval\n"
            :"=r"(scause), "=r"(stval)
            );
    TrapContext *cx = current_user_trap_cx();
    if (scause == 8) {
        cx->sepc += 4; int is_exec = cx->x[17] == SYSCALL_EXEC;
        isize result = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
        cx = current_user_trap_cx();
        if (!is_exec || result == -1) cx->x[10] = result;
    } else if (scause == (1L << 63) + 5) {
        set_next_trigger();
        suspend_current_and_run_next();
    } else if (scause == (1L << 63) + 9) {
        device_interrupt_handle();
    } else {
        printf("scause:%p\n", scause); printf("sepc:%p\n", cx->sepc);
        panic("Other trap from user");
    }
    trap_return();
}
void trap_return() {
    kernel_intr_switch(0); time_intr_switch(1);
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
