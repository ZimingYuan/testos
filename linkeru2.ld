OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDRESS = 0x80440000;

SECTIONS
{
    . = BASE_ADDRESS;
    .text : {
        *(.text.entry)
        *(.text .text.*)
    }
    .rodata : {
        *(.rodata .rodata.*)
    }
    .data : {
        *(.data .data.*)
    }
    .bss : {
        sbss = .;
        *(.bss .bss.*)
        ebss = .;
    }
    /DISCARD/ : {
        *(.eh_frame)
        *(.debug*)
    }
}
