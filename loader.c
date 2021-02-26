#include "kernel.h"

#define APP_BASE_ADDRESS 0x80400000
#define STEP 0x20000
#define USER_STACK_SIZE (4096 * 2)
#define KERNEL_STACK_SIZE (4096 * 2)

void __alltraps();
void load_all() {
    extern usize _num_app;
    usize *num_app_ptr = &_num_app + 1;
    asm volatile("fence.i");
    for (int i = 0; i < _num_app; i++) {
        usize base = num_app_ptr[i];
        for (usize j = base; j < num_app_ptr[i + 1]; j++)
            *(char *)(APP_BASE_ADDRESS + i * STEP + j - base) = *(char *)j;
    }
    task_init();
    asm volatile("csrw stvec, %0"::"r"(__alltraps));
    usize sie; asm volatile("csrr %0, sie":"=r"(sie));
    sie |= (1 << 5); asm volatile("csrw sie, %0"::"r"(sie));
    set_next_trigger(); run_first_task();
}
char KERNEL_STACK[MAX_APP_NUM][KERNEL_STACK_SIZE];
char USER_STACK[MAX_APP_NUM][USER_STACK_SIZE];
void __restore(usize);
usize init_app_cx(int app_id) {
    char *top = KERNEL_STACK[app_id] + KERNEL_STACK_SIZE;

    TrapContext *trap_cx_ptr = (TrapContext *)(top - sizeof(TrapContext));
    for (int i = 0; i < 32; i++) trap_cx_ptr->x[i] = 0;
    trap_cx_ptr->sepc = APP_BASE_ADDRESS + STEP * app_id;
    usize sstatus; asm volatile("csrr %0, sstatus":"=r"(sstatus));
    sstatus &= ~(1L << 8); trap_cx_ptr->sstatus = sstatus;
    trap_cx_ptr->x[2] = (usize)USER_STACK[app_id] + USER_STACK_SIZE;

    TaskContext *task_cx_ptr = (TaskContext *)((usize)trap_cx_ptr - sizeof(TaskContext));
    task_cx_ptr->ra = (usize)__restore;
    for (int i = 0; i < 12; i++) task_cx_ptr->s[i] = 0;
    return (usize)task_cx_ptr;
}
