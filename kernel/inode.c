#include "kernel.h"

#define TO16(x) *((short *)(x))
#define TO32(x) *((int *)(x))
#define min(a, b) ((a) < (b) ? (a) : (b))
typedef unsigned short u16;
typedef unsigned char u8;

int superblock[2], inode_cnt, block_cnt;
int block_bitmap_addr, inode_bitmap_addr, inode_table;
u16 gdt_num[2]; char *block_bitmap, *inode_bitmap;
void init_ext2() {
    bcache_init();
    bcache_rw(1, 12, 8, superblock, 0);
    bcache_rw(1, 0, 4, &inode_cnt, 0);
    bcache_rw(1, 4, 4, &block_cnt, 0);
    bcache_rw(2, 0, 4, &block_bitmap_addr, 0); 
    bcache_rw(2, 4, 4, &inode_bitmap_addr, 0);
    bcache_rw(2, 8, 4, &inode_table, 0);
    bcache_rw(2, 12, 4, gdt_num, 0);
    block_bitmap = bd_malloc(block_cnt / 8);
    inode_bitmap = bd_malloc(inode_cnt / 8);
    bcache_rw(block_bitmap_addr, 0, block_cnt / 8, block_bitmap, 0);
    bcache_rw(inode_bitmap_addr, 0, inode_cnt / 8, inode_bitmap, 0);
}
int alloc_block() {
    for (int i = 0; i < block_cnt; i++)
        if (!(block_bitmap[i / 8] & (1 << (i % 8)))) {
            block_bitmap[i / 8] |= (1 << (i % 8));
            bcache_rw(block_bitmap_addr, 0, block_cnt / 8, block_bitmap, 1);
            superblock[0]--; gdt_num[0]--;
            bcache_rw(1, 12, 8, superblock, 1);
            bcache_rw(2, 12, 4, gdt_num, 1);
            char* zero = bd_malloc(BSIZE); memset(zero, 0, BSIZE);
            bcache_rw(i, 0, BSIZE, zero, 1); bd_free(zero); return i;
        }
    panic("alloc_block: no free block");
}
void free_block(int block_no) {
    if (!(block_bitmap[block_no / 8] & (1 << (block_no % 8))))
        panic("free_block: block has been freed");
    else {
        block_bitmap[block_no / 8] &= (~(1 << (block_no % 8)));
        bcache_rw(block_bitmap_addr, 0, block_cnt / 8, block_bitmap, 1);
        superblock[0]++; gdt_num[0]++;
        bcache_rw(1, 12, 8, superblock, 1);
        bcache_rw(2, 12, 4, gdt_num, 1);
    }
}
void find_block(char *dinode, usize st, int *l0, int *l1, int *l2) {
    *l0 = 0, *l1 = 0, *l2 = 0; int bn = BSIZE / 4;
    if (st < BSIZE * 12) *l0 = st / BSIZE;
    else if (st < BSIZE * 12 + bn * BSIZE) {
        *l0 = 12; *l1 = (st - BSIZE * 12) % BSIZE;
    } else if (st < BSIZE * 12 + bn * BSIZE + bn * bn * BSIZE) {
        *l0 = 13; *l1 = (st - BSIZE * 12) / (bn * BSIZE);
        *l2 = ((st - BSIZE * 12) % (bn * BSIZE)) / BSIZE;
    } else panic("find_block: offset too large");
}
int get_alloc_block(int now_block, int index) {
    int x; bcache_rw(now_block, index * 4, 4, &x, 0);
    if (!x) {
        x = alloc_block(); bcache_rw(now_block, index * 4, 4, &x, 1);
    }
    return x;
}

void get_inode(int inode_num, char *buf) {
    bcache_rw(inode_table + (inode_num - 1) * 128 / BSIZE,
            ((inode_num - 1) * 128) % BSIZE, 128, buf, 0);
}
void upd_inode(int inode_num, char *buf) {
    bcache_rw(inode_table + (inode_num - 1) * 128 / BSIZE,
            ((inode_num - 1) * 128) % BSIZE, 128, buf, 1);
}
int alloc_inode() {
    for (int i = 0; i < inode_cnt; i++)
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) {
            inode_bitmap[i / 8] |= (1 << (i % 8));
            bcache_rw(inode_bitmap_addr, 0, inode_cnt / 8, inode_bitmap, 1);
            superblock[1]--; gdt_num[1]--;
            bcache_rw(1, 12, 8, superblock, 1);
            bcache_rw(2, 12, 4, gdt_num, 1);
            return i + 1;
        }
    panic("alloc_inode: no free inode");
}
void init_inode(int inode_num, char *dinode) {
    memset(dinode, 0, 128);
    TO16(dinode) = 0x81ff; TO16(dinode + 26) = 1;
    upd_inode(inode_num, dinode);
}
void read_inode(char *dinode, usize st, usize len, char *buf) {
    int l0, l1, l2;
    find_block(dinode, st, &l0, &l1, &l2);
    usize cpoff = 0; st %= BSIZE;
    while (len) {
        usize cplen = min(BSIZE - st, len);
        if (l0 < 12) {
            bcache_rw(TO32(dinode + 40 + 4 * l0), st, cplen, buf + cpoff, 0);
            l0++;
        } else if (l0 == 12) {
            int block_no; bcache_rw(TO32(dinode + 88), l1 * 4, 4, &block_no, 0);
            bcache_rw(block_no, st, cplen, buf + cpoff, 0); l1++;
            if (l1 == BSIZE / 4) { l1 = 0; l0++; }
        } else if (l0 == 13) {
            int block_no; bcache_rw(TO32(dinode + 92), l1 * 4, 4, &block_no, 0);
            bcache_rw(block_no, l2 * 4, 4, &block_no, 0);
            bcache_rw(block_no, st, cplen, buf + cpoff, 0); l2++;
            if (l2 == BSIZE / 4) { l2 = 0; l1++; }
            if (l1 == BSIZE / 4) panic("read_inode: len too large");
        } else panic("read_inode: len too large");
        st = 0; len -= cplen; cpoff += cplen;
    }
}
void write_inode(int inode_num, char *dinode, usize st, usize len, char *buf) {
    if (st + len > TO32(dinode + 4)) {
        TO32(dinode + 4) = st + len;
        upd_inode(inode_num, dinode);
    }
    int l0, l1, l2;
    find_block(dinode, st, &l0, &l1, &l2);
    usize cpoff = 0; st %= BSIZE;
    while (len) {
        usize cplen = min(BSIZE - st, len);
        if (TO32(dinode + 40 + 4 * l0) == 0) {
            TO32(dinode + 40 + 4 * l0) = alloc_block();
            upd_inode(inode_num, dinode);
        }
        if (l0 < 12) {
            bcache_rw(TO32(dinode + 40 + 4 * l0), st, cplen, buf + cpoff, 1); l0++;
        } else if (l0 == 12) {
            int block_no = get_alloc_block(TO32(dinode + 88), l1);
            bcache_rw(block_no, st, cplen, buf + cpoff, 1); l1++;
            if (l1 == BSIZE / 4) { l1 = 0; l0++; }
        } else if (l0 == 13) {
            int block_no = get_alloc_block(TO32(dinode + 92), l1);
            block_no = get_alloc_block(block_no, l1);
            bcache_rw(block_no, st, cplen, buf + cpoff, 1); l2++;
            if (l2 == BSIZE / 4) { l2 = 0; l1++; }
            if (l1 == BSIZE / 4) panic("write_inode: len too large");
        } else panic("write_inode: len too large");
        st = 0; len -= cplen; cpoff += cplen;
    }
}

int find_file(char *s, int len, char *dinode) {
    int size = TO32(dinode + 4); char *entry = bd_malloc(size);
    read_inode(dinode, 0, size, entry);
    for (int i = 0; i < size; i += TO16(entry + i + 4)) {
        u8 l = entry[i + 6];
        if (l == len) {
            int f = 0;
            for (int j = 0; j < l; j++)
                if (entry[i + 8 + j] != s[j]) {
                    f = 1; break;
                }
            if (!f) {
                int r = TO32(entry + i); bd_free(entry);
                return r;
            }
        }
    }
    bd_free(entry); return 0;
}
void add_file(char *name, int len, int inode_num, int inodep_num, char *dinode) {
    int size = TO32(dinode + 4); char *entry = bd_malloc(size);
    read_inode(dinode, 0, size, entry); int i = 0;
    while (i + TO16(entry + i + 4) < size) i += TO16(entry + i + 4);
    u8 l = entry[i + 6]; int real_size = (8 + l + 3) / 4 * 4;
    if (TO16(entry + i + 4) - real_size >= (8 + len + 3) / 4 * 4) {
        TO16(entry + i + 4) = real_size;
        TO32(entry + i + real_size) = inode_num;
        TO16(entry + i + real_size + 4) = size - i - real_size;
        entry[i + real_size + 6] = len; entry[i + real_size + 7] = 1;
        memcpy(entry + i + real_size + 8, name, len);
        write_inode(inodep_num, dinode, 0, size, entry);
    } else {
        real_size = (8 + len + 3) / 4 * 4; memset(entry, 0, real_size);
        TO32(entry) = inode_num; TO16(entry + 4) = BSIZE;
        entry[6] = len; entry[7] = 1; memcpy(entry + 8, name, len);
        write_inode(inodep_num, dinode, size, size + real_size, entry);
    }
    bd_free(entry);
}
char dinode[128];
FNode *inode_get(char *path, int create) {
    if (path[0] != '/') return 0;
    get_inode(2, dinode); int len = strlen(path);
    int j = 1, inodep_num = 2;
    for (int i = 1; i < len; i++) {
        if (path[i] == '/') {
            if (i != j) {
                inodep_num = find_file(path + j, i - j, dinode);
                if (! inodep_num) panic("inode_get:path not found");
                get_inode(inodep_num, dinode);
            }
            j = i + 1;
        }
    }
    if (j == len) {
        FNode *fn = bd_malloc(sizeof(FNode)); fn->dnum = inodep_num;
        memcpy(fn->dinode, dinode, 128); return fn;
    }
    int inode_num = find_file(path + j, len - j, dinode);
    if (inode_num) get_inode(inode_num, dinode);
    else {
        if (! create) return 0;
        inode_num = alloc_inode();
        add_file(path + j, len - j, inode_num, inodep_num, dinode);
        init_inode(inode_num, dinode);
    }
    FNode *fn = bd_malloc(sizeof(FNode)); fn->dnum = inode_num;
    memcpy(fn->dinode, dinode, 128); return fn;
}
usize inode_read(FNode *fn, char *buf, usize len) {
    if (fn->offset + len > TO32(fn->dinode + 4)) {
        usize r = TO32(fn->dinode + 4) - fn->offset;
        read_inode(fn->dinode, fn->offset, r, buf); 
        fn->offset += r; return r;
    } else {
        read_inode(fn->dinode, fn->offset, len, buf);
        fn->offset += len; return len;
    }
}
usize inode_write(FNode *fn, char *buf, usize len) {
    write_inode(fn->dnum, fn->dinode, fn->offset, len, buf); 
    fn->offset += len; return len;
}
int dfs_clear(int block_num, int dep) {
    if (block_num == 0) return 1;
    if (dep == 0) {
        free_block(block_num); return 0;
    }
    int *table = bd_malloc(BSIZE);
    bcache_rw(block_num, 0, BSIZE, table, 0);
    int f = 0;
    for (int i = 0; i < BSIZE / 4; i++)
        if (dfs_clear(table[i], dep - 1)) { f = 1; break; }
    bd_free(table); free_block(block_num); return f;
}
void inode_clear(FNode *fn) {
    TO32(fn->dinode + 4) = 0;
    for (int i = 0; i < 12; i++)
        if (dfs_clear(TO32(fn->dinode + 40 + i * 4), 0)) goto write;
    if (dfs_clear(TO32(fn->dinode + 88), 1)) goto write;
    if (dfs_clear(TO32(fn->dinode + 92), 2)) goto write;
    dfs_clear(TO32(fn->dinode + 96), 3);
write:
    memset(fn->dinode + 40, 0, 60); upd_inode(fn->dnum, fn->dinode);
}
char* inode_list(FNode *fn, int *slen) {
    int size = TO32(dinode + 4); char *entry = bd_malloc(size);
    read_inode(dinode, 0, size, entry); int len = 0;
    for (int i = 0; i < size; i += TO16(entry + i + 4))
        len += (u8)entry[i + 6] + 1;
    char *s = bd_malloc(len); len = 0;
    for (int i = 0; i < size; i += TO16(entry + i + 4)) {
        u8 l = entry[i + 6]; memcpy(s + len, entry + i + 8, l);
        s[len + l] = '\0'; len += l + 1;
    }
    bd_free(entry); *slen = len; return s;
}
