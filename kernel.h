typedef unsigned long long usize;
typedef long long isize;
#define SYSCALL_WRITE 0
#define SYSCALL_EXIT 1
#define FD_STDOUT 1
struct TrapContext {
    usize x[32], sstatus, sepc;
};
typedef struct TrapContext TrapContext;

void exit_all();
void consputc(char);

void panic(char *);
void printf(char *, ...);

void run_next_app();

isize sys_write(usize, char *, usize);
isize sys_exit(int);
