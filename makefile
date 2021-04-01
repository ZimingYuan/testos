CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany -Icommon
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
user_obj = $(foreach i, $(filter-out user/lib.c, $(wildcard user/*.c)), build/$(basename $(notdir $i)))
run: build/os.bin
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000
$(user_obj): $(wildcard user/*.c)
ifneq (build, $(wildcard build))
	mkdir build
endif
	$(CC) user/$(@F).c user/lib.c common/common.c -T user/linker.ld -o $@
build/os.bin: $(user_obj)
	python kernel/build.py
	$(CC) kernel/*.c kernel/*.S build/link_app.S common/common.c -T kernel/linker.ld -o build/os
	riscv64-unknown-elf-objcopy --strip-all -O binary build/os build/os.bin
debug: build/os.bin
	qemu-system-riscv64 -machine virt -nographic -bios common/rustsbi-qemu.bin \
		-device loader,file=build/os.bin,addr=0x80200000 -s -S
clean:
	rm build/*
.PHONY: run debug clean
