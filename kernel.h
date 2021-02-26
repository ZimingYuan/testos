typedef unsigned long long usize;
typedef long long isize;
#define SYSCALL_WRITE 0
#define SYSCALL_EXIT 1
#define SYSCALL_YIELD 2
#define SYSCALL_GET_TIME 3
#define FD_STDOUT 1
#define MAX_APP_NUM 10
struct TrapContext {
    usize x[32], sstatus, sepc;
};
typedef struct TrapContext TrapContext;
struct TaskContext {
    usize ra, s[12];
};
typedef struct TaskContext TaskContext;

void exit_all();
void consputc(char);
void set_timer(usize);

void panic(char *);
void printf(char *, ...);

void suspend_current_and_run_next();
void exit_current_and_run_next();
void task_init();
void run_first_task();

isize sys_write(usize, char *, usize);
isize sys_exit(int);
isize sys_yield();
isize sys_get_time();

usize init_app_cx(int);

void set_next_trigger();
usize get_time_ms();
