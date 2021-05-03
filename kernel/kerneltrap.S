.altmacro
.macro SAVE_GP n
    sd x\n, \n*8(sp)
.endm
.macro LOAD_GP n
    ld x\n, \n*8(sp)
.endm
.global __kerneltrap
.align 2
__kerneltrap:
    addi sp, sp, -256
    .set n, 1
    .rept 31
        SAVE_GP %n
        .set n, n+1
    .endr
    call trap_from_kernel
    .set n, 1
    .rept 31
        LOAD_GP %n
        .set n, n+1
    .endr
    addi sp, sp, 256
    sret
