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
#define PTE2FLAG(pte) ((unsigned char)((pte) & 255))
#define PGTB2SATP(pgtb) ((8L << 60) | (pgtb))

typedef struct TrapContext {
    usize x[32], sstatus, sepc;
    usize kernel_satp, kernel_sp, trap_handler;
} TrapContext;
typedef struct TaskContext {
    usize ra, s[12];
} TaskContext;
typedef struct File {
    int occupied;
    usize (*read)(struct File *, char *, usize);
    usize (*write)(struct File *, char *, usize);
    void (*copy)(struct File *);
    void (*close)(struct File *);
    void *bind;
} File;
typedef usize PhysAddr;
typedef usize VirtAddr;
typedef usize PhysPageNum;
typedef usize VirtPageNum;
typedef usize PageTableEntry;
typedef unsigned char PTEFlags;

// sbicall.c
void exit_all();
void set_timer(usize);
usize console_getchar();
void consputc(char x);

// task.c
void task_init_and_run();
void suspend_current_and_run_next();
void exit_current_and_run_next(int);
usize fork();
isize exec(char *);
isize waitpid(isize, int *);
void shutdown();
PhysPageNum current_user_pagetable();
TrapContext *current_user_trap_cx();
File *current_user_file(usize);
File *alloc_fd(usize *);
usize getpid();

// loader.c
void from_elf(char *, PhysPageNum, usize *, usize *);
char *get_app_data_by_name(char *);

// syscall.c
isize sys_write(usize, char *, usize);
isize sys_read(usize, char *, usize);
isize sys_close(usize);
isize sys_exit(int);
isize sys_yield();
isize sys_get_time();
isize sys_fork();
isize sys_exec(char *, usize);
isize sys_waitpid(isize, int *);
isize sys_gets(char *, usize);
isize sys_pipe(usize *);
isize sys_getpid();

// timer.c
void set_next_trigger();
usize get_time_ms();

// list.c
struct list {
  struct list *next;
  struct list *prev;
};
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

// pid.c
void pid_init();
usize pid_alloc();
void pid_dealloc(usize);

// pagetable.c
void kvm_init();
PageTableEntry *find_pte(PhysPageNum, VirtPageNum, int);
void map_area(PhysPageNum, VirtAddr, VirtAddr, PTEFlags, int);
void unmap_area(PhysPageNum, VirtAddr, VirtAddr, int);
void copy_area(PhysPageNum, VirtAddr, void *, int, int);
void copy_virt_area(PhysPageNum, PhysPageNum, VirtAddr, VirtAddr, VirtAddr);
void free_pagetable(PhysPageNum);
void map_trampoline(PhysPageNum);

// trap.c
void trap_handler();
void trap_return();

// vector.c
struct vector {
    usize size, capacity, dsize;
    char *buffer;
};
void vector_new(struct vector *, usize);
void vector_push(struct vector *, void *);
void vector_pop(struct vector *);
void *vector_back(struct vector *);
int vector_empty(struct vector *);
void vector_free(struct vector *);

// queue.c
struct queue {
    usize size, front, tail, capacity, dsize;
    char *buffer;
};
void queue_new(struct queue *, usize);
void queue_push(struct queue *, void *);
void queue_pop(struct queue *);
void *queue_front(struct queue *);
int queue_empty(struct queue *);
void queue_free(struct queue *);

// file.c
usize std_read(File *, char *, usize);
usize std_write(File *, char *, usize);
usize illegal_rw(File *, char *, usize);
void illegal_c(File *);
void make_pipe(usize *);
