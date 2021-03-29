#include "kernel.h"

PhysPageNum kernel_pagetable;
PageTableEntry *find_pte(PhysPageNum root, VirtPageNum vpn, int create) {
    usize idx[3];
    for (int i = 2; i >= 0; i--) {
        idx[i] = vpn & 511; vpn >>= 9;
    }
    for (int i = 0; i < 3; i++) {
        PageTableEntry *pte_p = (PageTableEntry *)PPN2PA(root) + idx[i];
        if (i == 2) return pte_p;
        if (!(*pte_p & V)) {
            if (! create) panic("find_pte failed");
            PhysPageNum frame = frame_alloc();
            *pte_p = PPN2PTE(frame, V);
        }
        root = PTE2PPN(*pte_p);
    }
    return 0;
}
void map(PhysPageNum root, VirtPageNum vpn, PhysPageNum ppn, PTEFlags flags) {
    PageTableEntry *pte_p = find_pte(root, vpn, 1);
    if (*pte_p & V)
        panic("map: vpn is mapped before mapping!");
    *pte_p = PPN2PTE(ppn, flags | V);
}
PhysPageNum unmap(PhysPageNum root, VirtPageNum vpn) {
    PageTableEntry *pte_p = find_pte(root, vpn, 0);
    if (!(*pte_p & V))
        panic("unmap: vpn is invalid before unmapping");
    PhysPageNum ppn = PTE2PPN(*pte_p);
    *pte_p = 0; return ppn;
}
void map_area(PhysPageNum root, VirtAddr start_va, VirtAddr end_va, PTEFlags flags, int alloc) {
    VirtPageNum start_vpn = FLOOR(start_va), end_vpn = CEIL(end_va);
    for (VirtPageNum i = start_vpn; i < end_vpn; i++) {
        PhysPageNum ppn = alloc ? frame_alloc() : i;
        map(root, i, ppn, flags);
    }
}
void unmap_area(PhysPageNum root, VirtAddr start_va, VirtAddr end_va, int dealloc) {
    VirtPageNum start_vpn = FLOOR(start_va), end_vpn = CEIL(end_va);
    for (VirtPageNum i = start_vpn; i < end_vpn; i++) {
        PhysPageNum ppn = unmap(root, i); if (dealloc) frame_dealloc(ppn);
    }
}
void copy_area(PhysPageNum root, VirtAddr start_va, void *data, int len, int to_va) {
    char *cdata = (char *)data; VirtPageNum vpn = FLOOR(start_va);
    while (len) {
        usize frame_off = start_va > PPN2PA(vpn) ? start_va - PPN2PA(vpn) : 0;
        usize copy_len = PAGE_SIZE - frame_off < len ? PAGE_SIZE - frame_off : len;
        PageTableEntry *pte_p = find_pte(root, vpn, 0);
        if (to_va) memcpy((void *)PPN2PA(PTE2PPN(*pte_p)) + frame_off, cdata, copy_len);
        else memcpy(cdata, (void *)PPN2PA(PTE2PPN(*pte_p)) + frame_off, copy_len);
        len -= copy_len; cdata += copy_len; vpn++;
    }
}
void copy_virt_area(PhysPageNum dstp, PhysPageNum srcp, VirtAddr dst_st, VirtAddr src_st, VirtAddr src_en) {
    VirtPageNum src_st_vpn = FLOOR(src_st), src_en_vpn = CEIL(src_en);
    VirtPageNum dst_st_vpn = FLOOR(dst_st);
    for (VirtPageNum i = src_st_vpn; i < src_en_vpn; i++) {
        PageTableEntry *src_pte_p = find_pte(srcp, i, 0);
        PhysAddr src_pa = PPN2PA(PTE2PPN(*src_pte_p));
        PageTableEntry *dst_pte_p = find_pte(dstp, i - src_st_vpn + dst_st_vpn, 1);
        if (!(*dst_pte_p & V)) {
            PhysPageNum ppn = frame_alloc();
            *dst_pte_p = PPN2PTE(ppn, PTE2FLAG(*src_pte_p));
        }
        PhysAddr dst_pa = PPN2PA(PTE2PPN(*dst_pte_p));
        memcpy((char *)dst_pa, (char *)src_pa, PAGE_SIZE);
    }
}
void map_trampoline(PhysPageNum root) {
    extern char strampoline;
    map(root, FLOOR(TRAMPOLINE), FLOOR((PhysAddr)&strampoline), R | X);
}
void free_pagetable(PhysPageNum root) {
    PageTableEntry *pte_p = (PageTableEntry *)PPN2PA(root);
    for (int i = 0; i < 512; i++) {
        if (pte_p[i] & V) {
            if (!(pte_p[i] & R) && !(pte_p[i] & W) && !(pte_p[i] & X))
                free_pagetable(PTE2PPN(pte_p[i]));
        }
    }
    frame_dealloc(root);
}
void kvm_init() {
    frame_init();
    kernel_pagetable = frame_alloc();
    extern char stext, etext, srodata, erodata, sdata, edata,
           sbss_with_stack, ebss, ekernel;
    map_trampoline(kernel_pagetable);
    map_area(kernel_pagetable, (VirtAddr)&stext, (VirtAddr)&etext, R | X, 0);
    map_area(kernel_pagetable, (VirtAddr)&srodata, (VirtAddr)&erodata, R, 0);
    map_area(kernel_pagetable, (VirtAddr)&sdata, (VirtAddr)&edata, R | W, 0);
    map_area(kernel_pagetable, (VirtAddr)&sbss_with_stack, (VirtAddr)&ebss, R | W, 0);
    map_area(kernel_pagetable, (VirtAddr)&ekernel, MEMORY_END, R | W, 0);
    usize satp = PGTB2SATP(kernel_pagetable);
    asm volatile ("csrw satp, %0\nsfence.vma"::"r"(satp));
}
