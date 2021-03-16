#include "common.h"
#include "syscall.h"
#include "filec.h"

isize syscall(usize id, usize a0, usize a1, usize a2) {
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
isize write(usize fd, char *buf) {
    usize len = 0; while (buf[len] != '\0') len++;
    return syscall(SYSCALL_WRITE, fd, (usize)buf, len);
}
isize exit(int exit_code) {
    return syscall(SYSCALL_EXIT, (usize)exit_code, 0, 0);
}
isize yield() {
    return syscall(SYSCALL_YIELD, 0, 0, 0);
}
isize get_time() {
    return syscall(SYSCALL_GET_TIME, 0, 0, 0);
}
void consputc(char x) {
    char s[2]; s[0] = x; s[1] = '\0'; write(FD_STDOUT, s);
}
int main();
__attribute__((section(".text.entry")))
void _start() {
    exit(main());
}
