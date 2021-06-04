    .section .data
    .global _num_app
    .align 3
_num_app:
    .quad 3
    .quad app_0_start
    .quad app_1_start
    .quad app_2_start
    .quad app_2_end

    .align 3
    .section .data
    .global app_0_start
    .global app_0_end
app_0_start:
    .incbin "build/hello_world0"
app_0_end:

    .align 3
    .section .data
    .global app_1_start
    .global app_1_end
app_1_start:
    .incbin "build/hello_world1"
app_1_end:

    .align 3
    .section .data
    .global app_2_start
    .global app_2_end
app_2_start:
    .incbin "build/hello_world2"
app_2_end:
