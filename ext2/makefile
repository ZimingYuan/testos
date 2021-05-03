all:
ifeq ($(wildcard fs.img), )
	dd if=/dev/zero of=./fs.img bs=4096k count=1
endif
	gcc bcache.c inode.c others.c main.c -o ext2
	mke2fs fs.img
