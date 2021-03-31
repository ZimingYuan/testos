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
isize write(usize fd, char *buf, usize len) {
    return syscall(SYSCALL_WRITE, fd, (usize)buf, len);
}
isize read(usize fd, char *buf, usize len) {
    return syscall(SYSCALL_READ, fd, (usize)buf, len);
}
void consputc(char x) {
    char s = x; write(FD_STDOUT, &s, 1);
}
char getchar() {
    char s; read(FD_STDIN, &s, 1); return s;
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
isize fork() {
    return syscall(SYSCALL_FORK, 0, 0, 0);
}
isize wait(int *exit_code) {
    for (;;) {
        isize exit_pid = syscall(SYSCALL_WAITPID, -1, (usize)exit_code, 0);
        if (exit_pid == -2) yield(); else return exit_pid;
    }
}
isize waitpid(usize pid, int *exit_code) {
    for (;;) {
        isize exit_pid = syscall(SYSCALL_WAITPID, pid, (usize)exit_code, 0);
        if (exit_pid == -2) yield(); else return exit_pid;
    }
}
isize exec(char *path) {
    return syscall(SYSCALL_EXEC, (usize)path, strlen(path), 0);
}
isize gets(char *buf, usize maxlen) {
    return syscall(SYSCALL_GETS, (usize)buf, maxlen, 0);
}
int main();
__attribute__((section(".text.entry")))
void _start() {
    exit(main());
}
