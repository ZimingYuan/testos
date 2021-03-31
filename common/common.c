// Most of this file is copy from xv6-riscv(https://github.com/mit-pdos/xv6-riscv)

#include <stdarg.h>
#include "common.h"

typedef unsigned int uint;
typedef unsigned long long uint64;
static char digits[] = "0123456789abcdef";
void consputc(char x);
void panic(char *);

static void
printint(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

static void
printptr(uint64 x)
{
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }
}

void
panic(char *s)
{
  printf("panic: ");
  printf(s);
  printf("\n");
  for(;;)
    ;
}

void *memset(void *dst, int c, unsigned int n) {
    char *cdst = (char *)dst;
    for (int i = 0; i < n; i++)
        cdst[i] = c;
    return dst;
}
void *memcpy(void *dst, void *src, unsigned int n) {
    char *cdst = (char *)dst, *csrc = (char *)src;
    for (int i = 0; i < n; i++) cdst[i] = csrc[i];
    return dst;
}
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2; else s1++, s2++;
    }
    return *s1 - *s2;
}
usize strlen(const char *s) {
    usize len = 0; while (s[len]) len++; return len; 
}
