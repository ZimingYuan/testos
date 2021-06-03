rustsbi-k210_size := 131072
k210_serialport := /dev/ttyUSB0
default:
	riscv64-unknown-elf-gcc os.c printf.c entry.S -T linker.ld -ffreestanding -nostdlib -g -o os -mcmodel=medany
	riscv64-unknown-elf-objcopy os --strip-all -O binary os.bin
	qemu-system-riscv64 -machine virt -nographic -bios rustsbi-qemu.bin -device loader,file=os.bin,addr=0x80200000
k210:
	riscv64-unknown-elf-gcc os.c printf.c entry.S -T linker_k210.ld -ffreestanding -nostdlib -g -o os_k210 -mcmodel=medany
	riscv64-unknown-elf-objcopy os_k210 --strip-all -O binary os_k210.bin
	@cp rustsbi-k210.bin rustsbi-k210.copy
	@dd if=os_k210.bin of=rustsbi-k210.copy bs=$(rustsbi-k210_size) seek=1
	@mv rustsbi-k210.copy os_k210.bin
	@sudo chmod 777 $(k210_serialport)
	python3 kflash.py -p $(k210_serialport) -b 1500000 os_k210.bin
	python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct $(k210_serialport) 115200
clean:
	$(if $(wildcard os), rm os)
	$(if $(wildcard os.bin), rm os.bin)
	$(if $(wildcard os_k210), rm os_k210)
	$(if $(wildcard os_k210.bin), rm os_k210.bin)
