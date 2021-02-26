CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
default: os.bin
	$(CC) hello_world0.c lib.c -T linkeru0.ld -o hello_world
	$(OC) hello_world hello_world0.bin
	$(CC) hello_world1.c lib.c -T linkeru1.ld -o hello_world
	$(OC) hello_world hello_world1.bin
	$(CC) hello_world2.c lib.c -T linkeru1.ld -o hello_world
	$(OC) hello_world hello_world2.bin
	$(CC) task.c loader.c mod.c timer.c printf.c syscall.c sbicall.c \
		entry.S link_app.S trap.S switch.S \
		-T linkers.ld -o os
	$(OC) os os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000
debug: os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000 -s -S
