rustsbi-k210_size = 131072
k210_serialport = /dev/ttyUSB0
CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
default: user
	$(CC) task.c loader.c mod.c timer.c printf.c syscall.c sbicall.c \
		entry.S link_app.S trap.S switch.S \
		-T linkers.ld -o os
	$(OC) os os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000
k210: user
	$(CC) task.c loader.c mod.c timer.c printf.c syscall.c sbicall.c \
		entry.S link_app.S trap.S switch.S \
		-D K210 -T linkers_k210.ld -o os_k210
	$(OC) os_k210 os_k210.bin
	@cp rustsbi-k210.bin rustsbi-k210.copy
	@dd if=os_k210.bin of=rustsbi-k210.copy bs=$(rustsbi-k210_size) seek=1
	@mv rustsbi-k210.copy os_k210.bin
	@sudo chmod 777 $(k210_serialport)
	python3 kflash.py -p $(k210_serialport) -b 1500000 os_k210.bin
	python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct $(k210_serialport) 115200
debug: default
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000 -s -S
user:
	$(CC) hello_world0.c lib.c -T linkeru0.ld -o hello_world
	$(OC) hello_world hello_world0.bin
	$(CC) hello_world1.c lib.c -T linkeru1.ld -o hello_world
	$(OC) hello_world hello_world1.bin
	$(CC) hello_world2.c lib.c -T linkeru1.ld -o hello_world
	$(OC) hello_world hello_world2.bin
clean:
	$(if $(wildcard hello_world0), rm hello_world0)
	$(if $(wildcard hello_world0.bin), rm hello_world0.bin)
	$(if $(wildcard hello_world1), rm hello_world1)
	$(if $(wildcard hello_world1.bin), rm hello_world1.bin)
	$(if $(wildcard hello_world2), rm hello_world2)
	$(if $(wildcard hello_world2.bin), rm hello_world2.bin)
	$(if $(wildcard os), rm os)
	$(if $(wildcard os.bin), rm os.bin)
	$(if $(wildcard os_k210), rm os_k210)
	$(if $(wildcard os_k210.bin), rm os_k210.bin)  
