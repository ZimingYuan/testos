default: os.bin
	riscv64-unknown-elf-gcc hello_world.c lib.c \
		-T linkeru.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o hello_world
	riscv64-unknown-elf-objcopy hello_world --strip-all -O binary hello_world.bin
	riscv64-unknown-elf-gcc batch.c mod.c printf.c syscall.c sbicall.c entry.S link_app.S trap.S \
		-T linkers.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o os
	riscv64-unknown-elf-objcopy os --strip-all -O binary os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000
debug: os.bin
	riscv64-unknown-elf-gcc hello_world.c lib.c \
		-T linkeru.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o hello_world
	riscv64-unknown-elf-objcopy hello_world --strip-all -O binary hello_world.bin
	riscv64-unknown-elf-gcc batch.c mod.c printf.c syscall.c sbicall.c entry.S link_app.S trap.S \
		-T linkers.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o os
	riscv64-unknown-elf-objcopy os --strip-all -O binary os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000 -s -S
