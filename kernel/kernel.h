#include "common.h"

#define MEMORY_END 0x80800000
#define PAGE_SIZE 4096
#define TRAMPOLINE (~(PAGE_SIZE - 1))
#define TRAP_CONTEXT (TRAMPOLINE - PAGE_SIZE)
#define USER_STACK_SIZE (4096 * 2)
#define KERNEL_STACK_SIZE (4096 * 2)

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)
#define FLOOR(pa) ((pa) / PAGE_SIZE)
#define CEIL(pa) (((pa) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PPN2PA(ppn) ((ppn) * PAGE_SIZE)
#define PPN2PTE(ppn, flags) (((ppn) << 10) | (flags))
#define PTE2PPN(pte) (((pte) >> 10) & ((1L << 44) - 1))
#define PTE2FLAG(pte) ((unsigned char)((pte) & 255))
#define PGTB2SATP(pgtb) ((8L << 60) | (pgtb))

#define BSIZE 1024
#define VIRTIO0 0x10001000
#define PLIC 0x0c000000L
#define VIRTIO0_IRQ 1

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
isize exec(char *, char *, usize);
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
isize sys_exec(char *, char **);
isize sys_waitpid(isize, int *);
isize sys_gets(char *, usize);
isize sys_pipe(usize *);
isize sys_open(char *, usize);
isize sys_dup(usize);
isize sys_getpid();
isize sys_getsize(usize fd);

// timer.c
void set_next_trigger();
usize get_time_ms();
void time_intr_switch(int);

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
void copy_area(PhysPageNum, VirtAddr, void *, usize, int);
void copy_virt_area(PhysPageNum, PhysPageNum, VirtAddr, VirtAddr, VirtAddr);
void free_pagetable(PhysPageNum);
void map_trampoline(PhysPageNum);

// trap.c
void trap_handler();
void trap_return();
void trap_from_kernel();
void kernel_intr_switch(int);

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
void std_close(File *);
usize illegal_rw(File *, char *, usize);
void illegal_c(File *);
void make_pipe(usize *);
isize make_fnode(char *, usize);

// spinlock.h
struct spinlock {};
void initlock(struct spinlock *, char *);
void acquire(struct spinlock *);
void release(struct spinlock *);

// bcache.c
struct buf {
    int occupied, disk, modify, blockno, time;
    char data[BSIZE];
};
void bcache_init();
void bcache_rw(int, int, int, void *, int);
void bcache_save();

// virtio_disk.c
void virtio_disk_init();
void virtio_disk_rw(struct buf *, int);
void virtio_disk_intr();

// inode.c
typedef struct FNode {
    int refcnt, dnum; usize offset;
    char dinode[128];
} FNode;
void init_ext2();
FNode *inode_get(char *, int);
usize inode_read(FNode *, char *, usize);
usize inode_write(FNode *, char *, usize);
void inode_clear(FNode *);
char *inode_list(FNode *, int *);

// plic.c
void plicinit();
void plicinithart();
int plic_claim();
void plic_complete(int);
