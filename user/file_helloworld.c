#include "user.h"

char s[] = "Hello world!";
int main() {
    isize f = open("/helloworld.txt", O_CREAT);
    write(f, s, strlen(s)); close(f);
    return 0;
}
