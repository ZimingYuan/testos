#include "kernel.h"

enum TaskStatus {
    UnInit, Ready, Running, Exited
};
struct TaskControlBlock {
    usize task_cx_ptr;
    enum TaskStatus task_status;
    PhysPageNum pagetable, trap_cx_ppn;
    usize base_size;
} tasks[MAX_APP_NUM];
typedef struct TaskControlBlock TaskControlBlock;
usize current_task;
void __switch(usize *, usize *);
extern usize _num_app;

void task_init() {
    for (int i = 0; i < MAX_APP_NUM; i++) {
        tasks[i].task_cx_ptr = 0; tasks[i].task_status = UnInit;
    }
    usize *num_app_ptr = &_num_app + 1;
    for (int i = 0; i < _num_app; i++) {
        PhysPageNum user_pagetable; usize user_sp, entry_point;
        from_elf((char *)num_app_ptr[i], &user_pagetable, &user_sp, &entry_point);
        // map kernel stack(per task)
        VirtAddr top = TRAMPOLINE - i * (KERNEL_STACK_SIZE + PAGE_SIZE);
        VirtAddr bottom = top - KERNEL_STACK_SIZE;
        extern PhysPageNum kernel_pagetable;
        map_area(kernel_pagetable, bottom, top, R | W, 1);
        // fill task context
        tasks[i].task_cx_ptr = top - sizeof(TaskContext);
        TaskContext *task_cx_ptr = (TaskContext *)tasks[i].task_cx_ptr;
        task_cx_ptr->ra = (usize)trap_return; 
        memset(task_cx_ptr->s, 0, sizeof(usize) * 12);
        // fill task block
        tasks[i].task_status = Ready;
        PageTableEntry *pte_p = find_pte(user_pagetable, FLOOR(TRAP_CONTEXT), 0);
        tasks[i].pagetable = user_pagetable;
        tasks[i].trap_cx_ppn = PTE2PPN(*pte_p);
        tasks[i].base_size = user_sp;
        // fill trap context
        TrapContext *trap_cx = (TrapContext *)PPN2PA(PTE2PPN(*pte_p));
        memset(trap_cx->x, 0, sizeof(usize) * 32);
        trap_cx->x[2] = user_sp;
        usize sstatus; asm volatile("csrr %0, sstatus":"=r"(sstatus));
        sstatus &= ~(1L << 8); trap_cx->sstatus = sstatus;
        trap_cx->sepc = entry_point;
        trap_cx->kernel_satp = PGTB2SATP(kernel_pagetable);
        trap_cx->kernel_sp = top;
        trap_cx->trap_handler = (usize)trap_handler;
    }
}
PhysPageNum current_user_pagetable() {
    return tasks[current_task].pagetable;
}
TrapContext *current_user_trap_cx() {
    return (TrapContext *)PPN2PA(tasks[current_task].trap_cx_ppn);
}
void run_next_task() {
    int next_task = -1;
    for (int i = current_task + 1; i < current_task + _num_app + 1; i++)
        if (tasks[i % _num_app].task_status == Ready) {
            next_task = i % _num_app; break;
        }
    if (next_task == -1) exit_all();
    // switch to next task
    tasks[next_task].task_status = Running;
    usize *current_task_cx_ptr2 = &tasks[current_task].task_cx_ptr;
    usize *next_task_cx_ptr2 = &tasks[next_task].task_cx_ptr;
    current_task = next_task;
    __switch(current_task_cx_ptr2, next_task_cx_ptr2);
}
void suspend_current_and_run_next() {
    tasks[current_task].task_status = Ready;
    run_next_task();
}
void exit_current_and_run_next() {
    tasks[current_task].task_status = Exited;
    // free user data and code
    VirtAddr seg_end = tasks[current_task].base_size - USER_STACK_SIZE - PAGE_SIZE;
    unmap_area(current_user_pagetable(), 0, seg_end, 1);
    // free user stack
    unmap_area(current_user_pagetable(), seg_end + PAGE_SIZE, tasks[current_task].base_size, 1);
    // free trap context
    unmap_area(current_user_pagetable(), TRAP_CONTEXT, TRAMPOLINE, 1);
    // free kernel stack
    VirtAddr top = TRAMPOLINE - current_task * (KERNEL_STACK_SIZE + PAGE_SIZE);
    VirtAddr bottom = top - KERNEL_STACK_SIZE;
    extern PhysPageNum kernel_pagetable;
    unmap_area(kernel_pagetable, bottom, top, 1);
    // free user pagetable
    free_pagetable(current_user_pagetable());

    run_next_task();
}
void run_first_task() {
    tasks[0].task_status = Running;
    usize t; __switch(&t, &tasks[0].task_cx_ptr);
}
