typedef unsigned long long usize;
typedef long long isize;
#define SYSCALL_WRITE 0
#define SYSCALL_EXIT 1
#define FD_STDOUT 1
isize write(usize, char *);
isize exit(int);
