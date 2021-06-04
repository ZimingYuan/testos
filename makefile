rustsbi-k210_size := 131072
k210_serialport := /dev/ttyUSB0
CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany -Icommon
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
.PHONY: default debug k210 user clean
default: user
	$(CC) kernel/*.c kernel/*.S common/common.c -T kernel/linker.ld -o build/os
	$(OC) build/os build/os.bin
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000
debug: user
	$(CC) kernel/*.c kernel/*.S common/common.c -T kernel/linker.ld -o build/os
	$(OC) build/os build/os.bin
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000 -s -S
k210: user
	$(CC) kernel/*.c kernel/*.S common/common.c -D K210 -T kernel/linker_k210.ld -o build/os_k210
	$(OC) build/os_k210 build/os_k210.bin
	@cp common/rustsbi-k210.bin build/rustsbi-k210.copy
	@dd if=build/os_k210.bin of=build/rustsbi-k210.copy bs=$(rustsbi-k210_size) seek=1
	@mv build/rustsbi-k210.copy build/os_k210.bin
	@sudo chmod 777 $(k210_serialport)
	python3 common/kflash.py -p $(k210_serialport) -b 1500000 build/os_k210.bin
	python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct $(k210_serialport) 115200
user:
ifneq (build, $(wildcard build))
	mkdir build
endif
	$(CC) user/hello_world0.c user/lib.c common/common.c -T user/linker.ld -o build/hello_world0
	$(CC) user/hello_world1.c user/lib.c common/common.c -T user/linker.ld -o build/hello_world1
	$(CC) user/hello_world2.c user/lib.c common/common.c -T user/linker.ld -o build/hello_world2
clean:
	rm build/*
