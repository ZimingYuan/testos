#include "kernel.h"
#include "queue.h"

enum TaskStatus {
    UnInit, Ready, Running, Exited, Zombie
};
typedef struct TaskControlBlock TaskControlBlock;
typedef struct tlist {
    TaskControlBlock *tcb;
    LIST_ENTRY(tlist) entries;
} tlist;
LIST_HEAD(tlist_head, tlist);
typedef struct TaskControlBlock {
    usize pid; PhysPageNum pagetable;
    enum TaskStatus task_status; usize base_size; int exit_code;
    PhysAddr task_cx_ptr; PhysPageNum trap_cx_ppn;
    struct TaskControlBlock *parent; struct tlist_head children;
} TaskControlBlock;
TaskControlBlock *alloc_proc() {
    PhysPageNum user_pagetable = frame_alloc();
    TaskControlBlock *tcb = bd_malloc(sizeof(TaskControlBlock));
    usize pid = pid_alloc();
    // map some page
    map_trampoline(user_pagetable);
    VirtAddr top = TRAMPOLINE - pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    VirtAddr bottom = top - KERNEL_STACK_SIZE;
    extern PhysPageNum kernel_pagetable;
    map_area(kernel_pagetable, bottom, top, R | W, 1);
    map_area(user_pagetable, TRAP_CONTEXT, TRAMPOLINE, R | W, 1);
    map_area(user_pagetable, TRAP_CONTEXT - USER_STACK_SIZE, TRAP_CONTEXT, R | W | U, 1);
    // fill task context
    tcb->pid = pid;
    tcb->pagetable = user_pagetable;
    tcb->task_status = Ready;
    tcb->base_size = 0;
    tcb->exit_code = 0;
    tcb->task_cx_ptr = top - sizeof(TaskContext);
    PageTableEntry *pte_p = find_pte(user_pagetable, FLOOR(TRAP_CONTEXT), 0);
    tcb->trap_cx_ppn = PTE2PPN(*pte_p);
    tcb->parent = 0;
    LIST_INIT(&tcb->children);
}
void user_init(TaskControlBlock *tcb, char *name) {
    usize entry_point;
    from_elf(get_app_data_by_name(name), tcb->pagetable, &tcb->base_size, &entry_point);
    // fill trap context
    TrapContext *trap_cx = (TrapContext *)PPN2PA(tcb->trap_cx_ppn);
    memset(trap_cx->x, 0, sizeof(usize) * 32);
    trap_cx->x[2] = TRAP_CONTEXT;
    usize sstatus; asm volatile("csrr %0, sstatus":"=r"(sstatus));
    sstatus &= ~(1L << 8); trap_cx->sstatus = sstatus;
    trap_cx->sepc = entry_point;
    extern PhysPageNum kernel_pagetable;
    trap_cx->kernel_satp = PGTB2SATP(kernel_pagetable);
    trap_cx->kernel_sp = TRAMPOLINE - tcb->pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    trap_cx->trap_handler = (usize)trap_handler;
}

typedef struct tqueue {
    TaskControlBlock *tcb;
    TAILQ_ENTRY(tqueue) entries;
} tqueue;
TAILQ_HEAD(tqueue_head, tqueue);
TaskControlBlock *initproc;
struct tqueue_head t_head;
void add_task(TaskControlBlock *task) {
    tqueue *x = bd_malloc(sizeof(tqueue)); x->tcb = task;
    TAILQ_INSERT_TAIL(&t_head, x, entries);
}
TaskControlBlock *fetch_task() {
    if (TAILQ_EMPTY(&t_head)) return 0;
    tqueue *x = TAILQ_FIRST(&t_head);
    TAILQ_REMOVE(&t_head, x, entries);
    TaskControlBlock *y = x->tcb;
    bd_free(x); return y;
}

typedef struct Processor {
    TaskControlBlock *current;
    usize idle_task_cx_ptr;
} Processor;
Processor PROCESSOR;
void __switch(usize *, usize *);
void run(Processor *p) {
    for (;;) {
        TaskControlBlock *tcb;
        if (tcb = fetch_task()) {
            usize *idle_task_cx_ptr2 = &p->idle_task_cx_ptr;
            usize *next_task_cx_ptr2 = &tcb->task_cx_ptr;
            tcb->task_status = Running; p->current = tcb;
            __switch(idle_task_cx_ptr2, next_task_cx_ptr2);
        } else shutdown();
    }
}
void schedule(usize *switched_task_cx_ptr2) {
    usize *idle_task_cx_ptr2 = &PROCESSOR.idle_task_cx_ptr;
    __switch(switched_task_cx_ptr2, idle_task_cx_ptr2);
}

void task_init_and_run() {
    TAILQ_INIT(&t_head); pid_init();
    initproc = alloc_proc();
    user_init(initproc, "initproc");
    // fill task context
    TaskContext *task_cx_ptr = (TaskContext *)initproc->task_cx_ptr;
    task_cx_ptr->ra = (usize)trap_return; 
    memset(task_cx_ptr->s, 0, sizeof(usize) * 12);
    add_task(initproc); run(&PROCESSOR);
}

PhysPageNum current_user_pagetable() {
    return PROCESSOR.current->pagetable;
}
TrapContext *current_user_trap_cx() {
    return (TrapContext *)PPN2PA(PROCESSOR.current->trap_cx_ppn);
}

void suspend_current_and_run_next() {
    TaskControlBlock *tcb = PROCESSOR.current;
    tcb->task_status = Ready; add_task(tcb);
    schedule(&tcb->task_cx_ptr);
}
void free_uvm(TaskControlBlock *tcb) {
    unmap_area(tcb->pagetable, 0, tcb->base_size, 1);
    // free user stack and trap context
    unmap_area(tcb->pagetable, TRAP_CONTEXT - USER_STACK_SIZE, TRAMPOLINE, 1);
    // free user pagetable
    free_pagetable(tcb->pagetable);
}
void exit_current_and_run_next(int exit_code) {
    TaskControlBlock *tcb = PROCESSOR.current;
    tcb->task_status = Zombie;
    tcb->exit_code = exit_code;
    while (! LIST_EMPTY(&tcb->children)) {
        tlist *child = LIST_FIRST(&tcb->children);
        child->tcb->parent = initproc;
        LIST_REMOVE(child, entries);
        LIST_INSERT_HEAD(&initproc->children, child, entries);
    }
    free_uvm(tcb); usize _unused = 0; schedule(&_unused);
}

usize fork() {
    TaskControlBlock *p = PROCESSOR.current;
    TaskControlBlock *tcb = alloc_proc();
    // copy user data and code
    copy_virt_area(tcb->pagetable, p->pagetable, 0, 0, p->base_size);
    // copy user stack and trap context
    copy_virt_area(tcb->pagetable, p->pagetable, TRAP_CONTEXT - USER_STACK_SIZE, TRAP_CONTEXT - USER_STACK_SIZE, TRAMPOLINE);
    // fill task block
    tcb->base_size = p->base_size; tcb->parent = p;
    // add children to parent
    tlist *x = bd_malloc(sizeof(tlist)); x->tcb = tcb;
    LIST_INSERT_HEAD(&p->children, x, entries);
    // fill task context
    TaskContext *task_cx_ptr = (TaskContext *)tcb->task_cx_ptr;
    task_cx_ptr->ra = (usize)trap_return; 
    memset(task_cx_ptr->s, 0, sizeof(usize) * 12);
    // fill trap context
    TrapContext *trap_cx = (TrapContext *)PPN2PA(tcb->trap_cx_ppn);
    trap_cx->kernel_sp = TRAMPOLINE - tcb->pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    trap_cx->x[10] = 0;

    add_task(tcb); return tcb->pid;
}
char a[20];
isize exec(char *name) {
    if (! get_app_data_by_name(name)) return -1;
    TaskControlBlock *p = PROCESSOR.current;
    unmap_area(p->pagetable, 0, p->base_size, 1);
    user_init(p, name); asm volatile ("fence.i");
}
isize waitpid(isize pid, int *exit_code) {
    TaskControlBlock *p = PROCESSOR.current;
    tlist *child;
    LIST_FOREACH(child, &p->children, entries) {
        if (pid == -1 || (usize)pid == child->tcb->pid) {
            if (child->tcb->task_status == Zombie) {
                *exit_code = child->tcb->exit_code;
                pid = child->tcb->pid; // pid may be -1 before
                VirtAddr top = TRAMPOLINE - pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
                VirtAddr bottom = top - KERNEL_STACK_SIZE;
                extern PhysPageNum kernel_pagetable;
                unmap_area(kernel_pagetable, bottom, top, 1);
                pid_dealloc(pid); bd_free(child->tcb);
                LIST_REMOVE(child, entries); bd_free(child);
                return pid;
            } else return -2;
        }
    }
    return -1;
}

void shutdown() {
    VirtAddr top = TRAMPOLINE - initproc->pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    VirtAddr bottom = top - KERNEL_STACK_SIZE;
    extern PhysPageNum kernel_pagetable;
    unmap_area(kernel_pagetable, bottom, top, 1);
    pid_dealloc(initproc->pid); bd_free(initproc);
    exit_all();
}
