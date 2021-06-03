rustsbi-k210_size := 131072
k210_serialport := /dev/ttyUSB0
default: hello_world.bin
	riscv64-unknown-elf-gcc batch.c mod.c printf.c syscall.c sbicall.c entry.S link_app.S trap.S \
		-T linkers.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o os
	riscv64-unknown-elf-objcopy os --strip-all -O binary os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000
k210: hello_world.bin
	riscv64-unknown-elf-gcc batch.c mod.c printf.c syscall.c sbicall.c entry.S link_app.S trap.S \
		-T linkers_k210.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o os_k210
	riscv64-unknown-elf-objcopy os_k210 --strip-all -O binary os_k210.bin
	@cp rustsbi-k210.bin rustsbi-k210.copy
	@dd if=os_k210.bin of=rustsbi-k210.copy bs=$(rustsbi-k210_size) seek=1
	@mv rustsbi-k210.copy os_k210.bin
	@sudo chmod 777 $(k210_serialport)
	python3 kflash.py -p $(k210_serialport) -b 1500000 os_k210.bin
	python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct $(k210_serialport) 115200
debug: hello_world.bin
	riscv64-unknown-elf-gcc batch.c mod.c printf.c syscall.c sbicall.c entry.S link_app.S trap.S \
		-T linkers.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o os
	riscv64-unknown-elf-objcopy os --strip-all -O binary os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin \
		-device loader,file=os.bin,addr=0x80200000 -s -S
hello_world.bin:
	riscv64-unknown-elf-gcc hello_world.c lib.c \
		-T linkeru.ld -ffreestanding -nostdlib -g -mcmodel=medany \
		-o hello_world
	riscv64-unknown-elf-objcopy hello_world --strip-all -O binary hello_world.bin
clean:
	$(if $(wildcard hello_world), rm hello_world)
	$(if $(wildcard hello_world.bin), rm hello_world.bin)
	$(if $(wildcard os), rm os)
	$(if $(wildcard os.bin), rm os.bin)
	$(if $(wildcard os_k210), rm os_k210)
	$(if $(wildcard os_k210.bin), rm os_k210.bin)
