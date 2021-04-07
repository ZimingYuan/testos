#include "user.h"

int main() {
    usize parent_fd[2], child_fd[2];
    char buf[16];
    pipe(parent_fd); pipe(child_fd);
    if (fork() == 0) {
        read(parent_fd[0], buf, 4);
        buf[4] = '\0'; printf("%d: received %s\n", getpid(), buf);
        write(child_fd[1], "pong", 4);
    } else {
        write(parent_fd[1], "ping", 4);
        read(child_fd[0], buf, 4);
        buf[4] = '\0'; printf("%d: received %s\n", getpid(), buf);
    }
    return 0;
}
