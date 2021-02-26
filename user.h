typedef unsigned long long usize;
typedef long long isize;
#define SYSCALL_WRITE 0
#define SYSCALL_EXIT 1
#define SYSCALL_YIELD 2
#define SYSCALL_GET_TIME 3
#define FD_STDOUT 1
isize write(usize, char *);
isize exit(int);
isize yield();
isize get_time();
