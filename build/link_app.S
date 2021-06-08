.section .data
    .global _num_app
    .align 3 
_num_app:
    .quad 7
    .quad app_0_start
    .quad app_1_start
    .quad app_2_start
    .quad app_3_start
    .quad app_4_start
    .quad app_5_start
    .quad app_6_start
    .quad app_6_end

    .global _app_names
_app_names:
    .string "prime"
    .string "initproc"
    .string "hello_world0"
    .string "hello_world2"
    .string "hello_world1"
    .string "pingpong"
    .string "user_shell"

    .align 3
    .section .data
    .global app_0_start
    .global app_0_end
app_0_start:
    .incbin "build/prime"
app_0_end:

    .align 3
    .section .data
    .global app_1_start
    .global app_1_end
app_1_start:
    .incbin "build/initproc"
app_1_end:

    .align 3
    .section .data
    .global app_2_start
    .global app_2_end
app_2_start:
    .incbin "build/hello_world0"
app_2_end:

    .align 3
    .section .data
    .global app_3_start
    .global app_3_end
app_3_start:
    .incbin "build/hello_world2"
app_3_end:

    .align 3
    .section .data
    .global app_4_start
    .global app_4_end
app_4_start:
    .incbin "build/hello_world1"
app_4_end:

    .align 3
    .section .data
    .global app_5_start
    .global app_5_end
app_5_start:
    .incbin "build/pingpong"
app_5_end:

    .align 3
    .section .data
    .global app_6_start
    .global app_6_end
app_6_start:
    .incbin "build/user_shell"
app_6_end:
