#include "kernel.h"
#include "elf.h"

#define KERNEL_HEAP_SIZE 0x300000

char HEAP_SPACE[KERNEL_HEAP_SIZE];
void from_elf(char *elf_data, PhysPageNum user_pgtb, usize *user_size, usize *entry_point) {
    // get elf header
    struct elfhdr *elf = (struct elfhdr *)elf_data;
    if (elf->magic != ELF_MAGIC) panic("from_elf: invalid elf file");
    int offset = elf->phoff; VirtPageNum max_end_vpn = 0;
    for (int i = 0; i < elf->phnum; i++) { // per program section
        struct proghdr *ph = (struct proghdr *)(elf_data + offset);
        if(ph->type != ELF_PROG_LOAD) continue;
        PTEFlags flags = U;
        if (ph->flags & ELF_PROG_FLAG_EXEC) flags |= X;
        if (ph->flags & ELF_PROG_FLAG_WRITE) flags |= W;
        if (ph->flags & ELF_PROG_FLAG_READ) flags |= R;
        // map and copy program data and code
        map_area(user_pgtb, ph->vaddr, ph->vaddr + ph->memsz, flags, 1);
        copy_area(user_pgtb, ph->vaddr, elf_data + ph->off, ph->filesz, 1);
        PhysAddr pa = PPN2PA(PTE2PPN(*find_pte(user_pgtb, FLOOR(ph->vaddr) + 1, 0)));
        if (CEIL(ph->vaddr + ph->memsz) > max_end_vpn)
            max_end_vpn = CEIL(ph->vaddr + ph->memsz);
        offset += sizeof(struct proghdr);
    }
    // map user stack(in user space)
    // VirtAddr user_stack_bottom = PPN2PA(max_end_vpn) + PAGE_SIZE;
    // VirtAddr user_stack_top = user_stack_bottom + USER_STACK_SIZE;
    // map_area(user_pgtb, user_stack_bottom, user_stack_top, R | W | U, 1);
    // map trap context
    // map_area(user_pgtb, TRAP_CONTEXT, TRAMPOLINE, R | W, 1);
    *entry_point = elf->entry; *user_size = PPN2PA(max_end_vpn);
}
char **APP_NAMES; extern usize _num_app;
void load_app_names() {
    extern char _app_names; char *start = &_app_names;
    APP_NAMES = bd_malloc(_num_app * sizeof(char *));
    for (int i = 0; i < _num_app; i++) {
        APP_NAMES[i] = start; while (*start) start++; start++;
    }
}
char *get_app_data_by_name(char *name) {
    for (int i = 0; i < _num_app; i++)
        if (strcmp(name, APP_NAMES[i]) == 0) {
            return (char *)(&_num_app + 1)[i];
        }
    return 0;
}
void list_apps() {
    printf("/**** APPS ****\n");
    for (int i = 0; i < _num_app; i++)
        printf("%s\n", APP_NAMES[i]);
    printf("**************/\n");
}
void load_all() {
    // load heap buffer
    bd_init(HEAP_SPACE, HEAP_SPACE + KERNEL_HEAP_SIZE);
    // load kernel virtual memory
    kvm_init();
    // load app names
    load_app_names(); list_apps();
    // load timer interrupt
    usize sie; asm volatile("csrr %0, sie":"=r"(sie));
    sie |= (1 << 5); asm volatile("csrw sie, %0"::"r"(sie));
    // set_next_trigger();
    // load tasks
    task_init_and_run();
}
