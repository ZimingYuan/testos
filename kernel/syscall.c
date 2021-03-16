#include "kernel.h"
#include "filec.h"

isize sys_write(usize fd, char *buffer, usize len) {
    switch (fd) {
        case FD_STDOUT: {
            PageTableEntry *pte_p = find_pte(current_user_pagetable(), FLOOR((VirtAddr)buffer), 0);
            PhysAddr pa = PPN2PA(PTE2PPN(*pte_p));
            char *pbuffer = bd_malloc(len + 1);
            copy_area(current_user_pagetable(), (VirtAddr)buffer, pbuffer, len + 1, 0);
            printf(pbuffer); bd_free(pbuffer); return (isize)len;
        }
    }
}
isize sys_yield() {
    suspend_current_and_run_next();
    return 0;
}
isize sys_exit(int xstate) {
    printf("Application exited with code %d\n", xstate);
    exit_current_and_run_next();
}
isize sys_get_time() {
    return (isize)get_time_ms();
}
