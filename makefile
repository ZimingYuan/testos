CC = riscv64-unknown-elf-gcc -ffreestanding -nostdlib -g -mcmodel=medany -Icommon
OC = riscv64-unknown-elf-objcopy --strip-all -O binary
user_obj = $(foreach i, $(filter-out user/lib.c, $(wildcard user/*.c)), build/$(basename $(notdir $i)))
file_img = ext2/fs.img
rust_sbi = common/rustsbi-qemu.bin

run: compile
	qemu-system-riscv64 -machine virt -nographic -bios $(rust_sbi) \
		-device loader,file=build/os.bin,addr=0x80200000 \
		-drive file=$(file_img),if=none,format=raw,id=x0 \
		-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
$(user_obj): $(wildcard user/*.c) common/common.c
	$(if $(wildcard build),,mkdir build)
	$(CC) user/$(@F).c user/lib.c common/common.c -T user/linker.ld -o $@
	ext2/ext2 -I $@ /$(@F) $(file_img)
compile: $(user_obj)
	$(CC) kernel/*.c kernel/*.S common/common.c -T kernel/linker.ld -o build/os
	riscv64-unknown-elf-objcopy --strip-all -O binary build/os build/os.bin
debug: compile
	qemu-system-riscv64 -machine virt -nographic -bios $(rust_sbi) \
		-device loader,file=build/os.bin,addr=0x80200000 -s -S \
		-drive file=$(file_img),if=none,format=raw,id=x0 \
		-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
clean:
	rm build/*
.PHONY: compile run debug clean
