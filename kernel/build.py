from pathlib import Path

program = list(filter(lambda x: x != Path('user/lib.c'), Path('user/').glob('*.c')))
f = open('build/link_app.S', 'w')
f.write('''.section .data
    .global _num_app
    .align 3
_num_app:
''')
f.write(f'    .quad {len(program)}\n')
for i in range(len(program)):
    f.write(f'    .quad app_{i}_start\n')
f.write(f'    .quad app_{len(program) - 1}_end\n')
f.write('''
    .global _app_names
_app_names:
''')
for i in program:
    f.write(f'    .string "{i.stem}"\n')
for i, j in enumerate(program):
    f.write(f'''
    .section .data
    .global app_{i}_start
    .global app_{i}_end
    .align 3
app_{i}_start:
    .incbin "build/{j.stem}"
app_{i}_end:
''')
f.close()
