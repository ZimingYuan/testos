#include "kernel.h"

const usize SBI_PUTCHAR = 1;
const usize SBI_SHUTDOWN = 8;
const usize SBI_SETTIMER = 0;

isize sbi_call(usize id, usize a0, usize a1, usize a2) {
    isize ret;
    asm volatile (
            "mv x10, %1\n"
            "mv x11, %2\n"
            "mv x12, %3\n"
            "mv x17, %4\n"
            "ecall\n"
            "mv %0, x10\n"
            :"=r"(ret)
            :"r"(a0), "r"(a1), "r"(a2), "r"(id)
            :"x10", "x11", "x12", "x17"
            );
    return ret;
}
void consputc(char x) {
    sbi_call(SBI_PUTCHAR, x, 0, 0);
}
void exit_all() {
    sbi_call(SBI_SHUTDOWN,  0, 0, 0);
}
void set_timer(usize timer) {
    sbi_call(SBI_SETTIMER, timer, 0, 0);
}
