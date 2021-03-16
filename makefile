CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany -Icommon
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
normal: compile
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000
compile:
	$(CC) user/hello_world0.c user/lib.c common/common.c -T user/linker.ld -o build/hello_world0
	$(CC) user/hello_world1.c user/lib.c common/common.c -T user/linker.ld -o build/hello_world1
	$(CC) user/hello_world2.c user/lib.c common/common.c -T user/linker.ld -o build/hello_world2
	$(CC) kernel/*.c kernel/*.S common/common.c -T kernel/linker.ld -o build/os
	$(OC) build/os build/os.bin
debug: compile
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000 -s -S
clean:
	rm build/*
