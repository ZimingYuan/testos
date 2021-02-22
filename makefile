default: os.bin
	riscv64-unknown-elf-gcc os.c printf.c entry.S -T linker.ld -ffreestanding -nostdlib -g -o os -mcmodel=medany
	riscv64-unknown-elf-objcopy os --strip-all -O binary os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin -device loader,file=os.bin,addr=0x80200000
