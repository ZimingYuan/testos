#include "kernel.h"

enum TaskStatus {
    UnInit, Ready, Running, Exited, Zombie
};
typedef struct TaskControlBlock TaskControlBlock;
typedef struct TaskControlBlock {
    usize pid; PhysPageNum pagetable;
    enum TaskStatus task_status; usize base_size; int exit_code;
    PhysAddr task_cx_ptr; PhysPageNum trap_cx_ppn;
    struct TaskControlBlock *parent; struct list children;
    struct vector fd_table;
} TaskControlBlock;
typedef struct tlist {
    struct list lst; TaskControlBlock *tcb;
} tlist;

TaskControlBlock *alloc_proc() {
    PhysPageNum user_pagetable = frame_alloc();
    TaskControlBlock *tcb = bd_malloc(sizeof(TaskControlBlock));
    usize pid = pid_alloc();
    // map some page
    map_trampoline(user_pagetable);
    VirtAddr top = TRAMPOLINE - pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    VirtAddr bottom = top - KERNEL_STACK_SIZE;
    extern PhysPageNum kernel_pagetable;
    map_area(kernel_pagetable, bottom, top, PTE_R | PTE_W, 1);
    map_area(user_pagetable, TRAP_CONTEXT, TRAMPOLINE, PTE_R | PTE_W, 1);
    map_area(user_pagetable, TRAP_CONTEXT - USER_STACK_SIZE, TRAP_CONTEXT, PTE_R | PTE_W | PTE_U, 1);
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
    lst_init(&tcb->children);
    // std io file desc
    vector_new(&tcb->fd_table, sizeof(File));
    File t; t.occupied = 1;
    t.read = std_read; t.write = illegal_rw;
    t.copy = illegal_c; t.close = std_close;
    vector_push(&tcb->fd_table, &t);
    t.read = illegal_rw; t.write = std_write;
    t.copy = illegal_c; t.close = std_close;
    vector_push(&tcb->fd_table, &t);
    vector_push(&tcb->fd_table, &t);

    return tcb;
}
void user_init(TaskControlBlock *tcb, char *name, char *argv, usize argv_len) {
    usize entry_point;
    from_elf(get_app_data_by_name(name), tcb->pagetable, &tcb->base_size, &entry_point);
    // fill argv
    VirtAddr user_sp = TRAP_CONTEXT - argv_len;
    copy_area(tcb->pagetable, user_sp, argv, argv_len, 1);
    struct vector vargv; vector_new(&vargv, sizeof(char *));
    for (usize i = 0; i < argv_len; i += strlen(argv + i) + 1) {
        char *t = (char *)user_sp + i; vector_push(&vargv, &t);
    }
    user_sp -= vargv.size * sizeof(char *);
    copy_area(tcb->pagetable, user_sp, vargv.buffer,
            vargv.size * sizeof(char *), 1);
    // fill trap context
    TrapContext *trap_cx = (TrapContext *)PPN2PA(tcb->trap_cx_ppn);
    memset(trap_cx->x, 0, sizeof(usize) * 32);
    trap_cx->x[2] = trap_cx->x[11] = user_sp; trap_cx->x[10] = vargv.size;
    usize sstatus; asm volatile("csrr %0, sstatus":"=r"(sstatus));
    sstatus &= ~(1L << 8); trap_cx->sstatus = sstatus;
    trap_cx->sepc = entry_point;
    extern PhysPageNum kernel_pagetable;
    trap_cx->kernel_satp = PGTB2SATP(kernel_pagetable);
    trap_cx->kernel_sp = TRAMPOLINE - tcb->pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    trap_cx->trap_handler = (usize)trap_handler;

    vector_free(&vargv);
}

struct queue tqueue;
TaskControlBlock *initproc;
void add_task(TaskControlBlock *task) {
    queue_push(&tqueue, &task);
}
TaskControlBlock *fetch_task() {
    if (queue_empty(&tqueue)) return 0;
    TaskControlBlock *x = *(TaskControlBlock **)queue_front(&tqueue);
    queue_pop(&tqueue); return x;
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
    queue_new(&tqueue, sizeof(TaskControlBlock *));
    pid_init(); initproc = alloc_proc();
    user_init(initproc, "initproc", "initproc", 9);
    // fill task context
    TaskContext *task_cx_ptr = (TaskContext *)initproc->task_cx_ptr;
    task_cx_ptr->ra = (usize)trap_return; 
    memset(task_cx_ptr->s, 0, sizeof(usize) * 12);
    add_task(initproc); run(&PROCESSOR);
}
void free_uvm(TaskControlBlock *tcb) {
    unmap_area(tcb->pagetable, 0, tcb->base_size, 1);
    // free user stack and trap context
    unmap_area(tcb->pagetable, TRAP_CONTEXT - USER_STACK_SIZE, TRAMPOLINE, 1);
    // free user pagetable
    free_pagetable(tcb->pagetable);
}

void suspend_current_and_run_next() {
    TaskControlBlock *tcb = PROCESSOR.current;
    if (! tcb) return; // this function may be called before task init
    tcb->task_status = Ready; add_task(tcb);
    schedule(&tcb->task_cx_ptr);
}
void exit_current_and_run_next(int exit_code) {
    TaskControlBlock *tcb = PROCESSOR.current;
    tcb->task_status = Zombie;
    tcb->exit_code = exit_code;
    while (! lst_empty(&tcb->children)) {
        tlist *child = (tlist *)tcb->children.next;
        child->tcb->parent = initproc; lst_pop(&tcb->children);
        lst_push(&initproc->children, (struct list *)child);
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
    lst_push(&p->children, (struct list *)x);
    // copy file desc
    File *t = (File *)p->fd_table.buffer;
    for (int i = 3; i < p->fd_table.size; i++) {
        t[i].copy(t + i); vector_push(&tcb->fd_table, t + i);
    }
    // fill task context
    TaskContext *task_cx_ptr = (TaskContext *)tcb->task_cx_ptr;
    task_cx_ptr->ra = (usize)trap_return; 
    // fill trap context
    TrapContext *trap_cx = (TrapContext *)PPN2PA(tcb->trap_cx_ppn);
    trap_cx->kernel_sp = TRAMPOLINE - tcb->pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    trap_cx->x[10] = 0;

    add_task(tcb); return tcb->pid;
}
isize exec(char *name, char *argv, usize argv_len) {
    if (! get_app_data_by_name(name)) return -1;
    TaskControlBlock *p = PROCESSOR.current;
    unmap_area(p->pagetable, 0, p->base_size, 1);
    user_init(p, name, argv, argv_len);
}
isize waitpid(isize pid, int *exit_code) {
    TaskControlBlock *p = PROCESSOR.current;
    for (struct list *lst = p->children.next;
            lst != &p->children; lst = lst->next) {
        tlist *child = (tlist *)lst;
        if (pid == -1 || (usize)pid == child->tcb->pid) {
            if (child->tcb->task_status == Zombie) {
                *exit_code = child->tcb->exit_code;
                pid = child->tcb->pid; // pid may be -1 before
                VirtAddr top = TRAMPOLINE - pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
                VirtAddr bottom = top - KERNEL_STACK_SIZE;
                extern PhysPageNum kernel_pagetable;
                unmap_area(kernel_pagetable, bottom, top, 1);
                File *t = (File *)child->tcb->fd_table.buffer;
                for (int i = 0; i < child->tcb->fd_table.size; i++) {
                    if (t[i].occupied) t[i].close(&t[i]);
                }
                vector_free(&child->tcb->fd_table);
                pid_dealloc(pid); bd_free(child->tcb);
                lst_remove(lst); bd_free(child);
                return pid;
            } else return -2;
        }
    }
    return -1;
}
usize getpid() {
    return PROCESSOR.current->pid;
}

PhysPageNum current_user_pagetable() {
    return PROCESSOR.current->pagetable;
}
TrapContext *current_user_trap_cx() {
    return (TrapContext *)PPN2PA(PROCESSOR.current->trap_cx_ppn);
}
File *current_user_file(usize fd) {
    struct vector *ftb = &PROCESSOR.current->fd_table;
    File *a = (File *)ftb->buffer;
    if (fd >= ftb->size || a[fd].occupied == 0) return 0;
    return (File *)ftb->buffer + fd;
}
File *alloc_fd(usize *fd) {
    struct vector *ftb = &PROCESSOR.current->fd_table;
    File *a = (File *)ftb->buffer;
    for (usize i = 0; i < ftb->size; i++) if (a[i].occupied == 0) {
        a[i].occupied = 1; *fd = i; return a + i;
    }
    File t; vector_push(ftb, &t);
    *fd = ftb->size - 1; return vector_back(ftb);
}
void shutdown() {
    VirtAddr top = TRAMPOLINE - initproc->pid * (KERNEL_STACK_SIZE + PAGE_SIZE);
    VirtAddr bottom = top - KERNEL_STACK_SIZE;
    extern PhysPageNum kernel_pagetable;
    unmap_area(kernel_pagetable, bottom, top, 1);
    pid_dealloc(initproc->pid); bd_free(initproc);
    exit_all();
}
