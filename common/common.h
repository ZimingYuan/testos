#ifndef COMMON_H
#define COMMON_H

typedef unsigned long long usize;
typedef long long isize;

void *memset(void *, int, unsigned int);
void *memcpy(void *, void *, unsigned int);
int strcmp(const char *, const char *);
usize strlen(const char *);
void printf(char *, ...);
void panic(char *);

#endif
