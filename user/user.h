#include "common.h"
#include "filec.h"

isize write(usize, char *, usize);
isize read(usize, char *, usize);
char getchar();
void consputc(char);
isize exit(int);
isize yield();
isize get_time();
isize fork();
isize wait(int *);
isize waitpid(usize, int *);
isize exec(char *);
