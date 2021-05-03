#include "common.h"
#include "filec.h"

isize write(usize, char *, usize);
isize read(usize, char *, usize);
isize close(usize);
char getchar();
void consputc(char);
isize exit(int);
isize yield();
isize get_time();
isize fork();
isize wait(int *);
isize waitpid(usize, int *);
isize exec(char *, char **);
isize gets(char *, usize);
isize pipe(usize *);
isize open(char *, usize);
isize getpid();
