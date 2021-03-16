#include "kernel.h"
#include "elf.h"

#define KERNEL_HEAP_SIZE 0x300000

char HEAP_SPACE[KERNEL_HEAP_SIZE];
void from_elf(char *elf_data, PhysPageNum *user_pagetable, usize *user_sp, usize *entry_point) {
    PhysPageNum user_pgtb = frame_alloc(); map_trampoline(user_pgtb);
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
    VirtAddr user_stack_bottom = PPN2PA(max_end_vpn) + PAGE_SIZE;
    VirtAddr user_stack_top = user_stack_bottom + USER_STACK_SIZE;
    map_area(user_pgtb, user_stack_bottom, user_stack_top, R | W | U, 1);
    // map trap context
    map_area(user_pgtb, TRAP_CONTEXT, TRAMPOLINE, R | W, 1);
    *user_pagetable = user_pgtb; *user_sp = user_stack_top; *entry_point = elf->entry;
}
void load_all() {
    // load heap buffer
    bd_init(HEAP_SPACE, HEAP_SPACE + KERNEL_HEAP_SIZE);
    // load kernel virtual memory
    kvm_init();
    // load tasks
    task_init();
    // load timer interrupt
    usize sie; asm volatile("csrr %0, sie":"=r"(sie));
    sie |= (1 << 5); asm volatile("csrw sie, %0"::"r"(sie));
    set_next_trigger();

    run_first_task();
}
