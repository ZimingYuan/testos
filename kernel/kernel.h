#include "common.h"

#define MAX_APP_NUM 10
#define MEMORY_END 0x80800000
#define PAGE_SIZE 4096
#define TRAMPOLINE (~(PAGE_SIZE - 1))
#define TRAP_CONTEXT (TRAMPOLINE - PAGE_SIZE)
#define USER_STACK_SIZE (4096 * 2)
#define KERNEL_STACK_SIZE (4096 * 2)

#define V (1 << 0)
#define R (1 << 1)
#define W (1 << 2)
#define X (1 << 3)
#define U (1 << 4)
#define FLOOR(pa) ((pa) / PAGE_SIZE)
#define CEIL(pa) (((pa) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PPN2PA(ppn) ((ppn) * PAGE_SIZE)
#define PPN2PTE(ppn, flags) (((ppn) << 10) | (flags))
#define PTE2PPN(pte) (((pte) >> 10) & ((1L << 44) - 1))
#define PGTB2SATP(pgtb) ((8L << 60) | (pgtb))

struct TrapContext {
    usize x[32], sstatus, sepc;
    usize kernel_satp, kernel_sp, trap_handler;
};
typedef struct TrapContext TrapContext;
struct TaskContext {
    usize ra, s[12];
};
typedef struct TaskContext TaskContext;
struct list {
  struct list *next;
  struct list *prev;
};
typedef usize PhysAddr;
typedef usize VirtAddr;
typedef usize PhysPageNum;
typedef usize VirtPageNum;
typedef usize PageTableEntry;
typedef unsigned char PTEFlags;

// sbicall.c
void exit_all();
void set_timer(usize);

// task.c
void task_init();
PhysPageNum current_user_pagetable();
TrapContext *current_user_trap_cx();
void suspend_current_and_run_next();
void exit_current_and_run_next();
void run_first_task();

// loader.c
void from_elf(char *, PhysPageNum *, usize *, usize *);

// syscall.c
isize sys_write(usize, char *, usize);
isize sys_exit(int);
isize sys_yield();
isize sys_get_time();

// timer.c
void set_next_trigger();
usize get_time_ms();

// list.c
void lst_init(struct list*);
void lst_remove(struct list*);
void lst_push(struct list*, void *);
void *lst_pop(struct list*);
void lst_print(struct list*);
int lst_empty(struct list*);

// buddy.c
void bd_init(void*, void*);
void bd_free(void*);
void *bd_malloc(usize);

// frame.c
void frame_init();
PhysPageNum frame_alloc();
void frame_dealloc(PhysPageNum);
void print_frame_num();

// pagetable.c
void kvm_init();
PageTableEntry *find_pte(PhysPageNum, VirtPageNum, int);
void map_area(PhysPageNum, VirtAddr, VirtAddr, PTEFlags, int);
void unmap_area(PhysPageNum, VirtAddr, VirtAddr, int);
void copy_area(PhysPageNum, VirtAddr, void *, int, int);
void free_pagetable(PhysPageNum);
void map_trampoline(PhysPageNum);

//trap.c
void trap_handler();
void trap_return();
