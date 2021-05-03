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
        PTEFlags flags = PTE_U;
        if (ph->flags & ELF_PROG_FLAG_EXEC) flags |= PTE_X;
        if (ph->flags & ELF_PROG_FLAG_WRITE) flags |= PTE_W;
        if (ph->flags & ELF_PROG_FLAG_READ) flags |= PTE_R;
        // map and copy program data and code
        map_area(user_pgtb, ph->vaddr, ph->vaddr + ph->memsz, flags, 1);
        copy_area(user_pgtb, ph->vaddr, elf_data + ph->off, ph->filesz, 1);
        PhysAddr pa = PPN2PA(PTE2PPN(*find_pte(user_pgtb, FLOOR(ph->vaddr) + 1, 0)));
        if (CEIL(ph->vaddr + ph->memsz) > max_end_vpn)
            max_end_vpn = CEIL(ph->vaddr + ph->memsz);
        offset += sizeof(struct proghdr);
    }
    *entry_point = elf->entry; *user_size = PPN2PA(max_end_vpn);
    bd_free(elf_data);
}
char *get_app_data_by_name(char *name) {
    usize len = strlen(name); char *s = bd_malloc(len + 2);
    s[0] = '/'; memcpy(s + 1, name, len); s[len + 1] = '\0';
    FNode *fn = inode_get(s, 0); bd_free(s); if (!fn) return 0;
    len = *(int *)(fn->dinode + 4); s = bd_malloc(len);
    fn->offset = 0; inode_read(fn, s, len); bd_free(fn);
    if (((struct elfhdr *)s)->magic != ELF_MAGIC) return 0;
    return s;
}
void list_root() {
    FNode *fn = inode_get("/", 0); int len;
    char *s = inode_list(fn, &len);
    printf("**** ROOT ****\n");
    for (int i = 0; i < len; i += strlen(s + i) + 1)
        printf("%s\n", s + i);
    printf("**************\n");
    bd_free(s); bd_free(fn);
}
void load_all() {
    // set kernel trap
    void __kerneltrap();
    asm volatile("csrw stvec, %0"::"r"(__kerneltrap));
    // init heap buffer
    bd_init(HEAP_SPACE, HEAP_SPACE + KERNEL_HEAP_SIZE);
    // init kernel virtual memory
    kvm_init();
    // init external interrupt
    plicinit(); plicinithart(); kernel_intr_switch(1);
    usize sie; asm volatile("csrr %0, sie":"=r"(sie));
    sie |= (1 << 9); asm volatile("csrw sie, %0"::"r"(sie));
    // init file system
    virtio_disk_init(); init_ext2();
    // list app names
    list_root();
    // init timer interrupt
    set_next_trigger();
    // init tasks
    task_init_and_run();
}
