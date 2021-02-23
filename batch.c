#include "kernel.h"
#define MAX_APP_NUM 10
#define APP_BASE_ADDRESS 0x80400000
#define USER_STACK_SIZE (4096 * 2)
#define KERNEL_STACK_SIZE (4096 * 2)

usize num_app;
usize current_app = 0;
usize app_start[MAX_APP_NUM + 1];

void __alltraps();
void load_all() {
    extern usize _num_app;
    usize *num_app_ptr = &_num_app;
    num_app = _num_app;
    for (usize i = 1; i <= num_app + 1; i++)
        app_start[i - 1] = num_app_ptr[i];
    asm volatile ("csrw stvec, %0"::"r"(__alltraps));
    run_next_app();
}
void load_app(usize app_id) {
    if (app_id >= num_app) exit_all();
    asm volatile("fence.i");
    usize base = app_start[app_id];
    for (usize i = app_start[app_id]; i < app_start[app_id + 1]; i++) {
        *(char *)(APP_BASE_ADDRESS + i - base) = *(char *)i;
    }
}
char KERNEL_STACK[KERNEL_STACK_SIZE], USER_STACK[USER_STACK_SIZE];
const char *KERNEL_TOP = KERNEL_STACK + KERNEL_STACK_SIZE;
const char *USER_TOP = USER_STACK + USER_STACK_SIZE;
void __restore(usize);
TrapContext *app_init_context(usize entry, usize sp, TrapContext *cx) {
    usize sstatus; asm volatile("csrr %0, sstatus":"=r"(sstatus));
    sstatus &= ~(1L << 8);
    for (int i = 0; i < 32; i++) cx->x[i] = 0;
    cx->sepc = entry; cx->sstatus = sstatus;
    cx->x[2] = sp; return cx;
}
void run_next_app() {
    load_app(current_app); current_app++;
    __restore((usize)app_init_context(
                APP_BASE_ADDRESS,
                (usize)USER_TOP,
                (TrapContext *)(KERNEL_TOP - sizeof(TrapContext))
                ));
}
