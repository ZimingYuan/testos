rustsbi-k210_size = 131072
k210_serialport = /dev/ttyUSB0
CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany -Icommon
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
user_obj = $(foreach i, $(filter-out user/lib.c, $(wildcard user/*.c)), build/$(basename $(notdir $i)))
run: compile
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000
$(user_obj): $(wildcard user/*.c) common/common.c
ifneq (build, $(wildcard build))
	mkdir build
endif
	$(CC) user/$(@F).c user/lib.c common/common.c -T user/linker.ld -o $@
compile: $(user_obj)
	python kernel/build.py
	$(CC) kernel/*.c kernel/*.S build/link_app.S common/common.c -T kernel/linker.ld -o build/os
	riscv64-unknown-elf-objcopy --strip-all -O binary build/os build/os.bin
debug: compile
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000 -s -S
k210: $(user_obj)
	python kernel/build.py
	$(CC) kernel/*.c kernel/*.S build/link_app.S common/common.c \
		-D K210 -T kernel/linker_k210.ld -o build/os_k210
	riscv64-unknown-elf-objcopy --strip-all -O binary build/os_k210 build/os_k210.bin
	@cp common/rustsbi-k210.bin build/rustsbi-k210.copy
	@dd if=build/os_k210.bin of=build/rustsbi-k210.copy bs=$(rustsbi-k210_size) seek=1
	@mv build/rustsbi-k210.copy build/os_k210.bin
	@sudo chmod 777 $(k210_serialport)
	python3 common/kflash.py -p $(k210_serialport) -b 1500000 build/os_k210.bin
	python3 -m serial.tools.miniterm --eol LF --dtr 0 --rts 0 --filter direct $(k210_serialport) 115200
clean:
	rm build/*
.PHONY: compile run debug k210 clean
