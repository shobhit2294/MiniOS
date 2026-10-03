AS=nasm
CC=g++
LD=ld

CFLAGS=-m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib
LDFLAGS=-m elf_i386 -T linker.ld

.PHONY: all iso run clean

all: iso

build:
	mkdir -p build

build/boot.o: boot/boot.asm | build
	$(AS) -f elf32 boot/boot.asm -o build/boot.o

build/interrupt.o: boot/interrupt.asm | build
	$(AS) -f elf32 boot/interrupt.asm -o build/interrupt.o

build/kernel.o: kernel/kernel.cpp kernel/interrupts.h kernel/keyboard.h kernel/memory.h kernel/pic.h kernel/scheduler.h kernel/shell.h kernel/terminal.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/kernel.cpp -o build/kernel.o

build/terminal.o: kernel/terminal.cpp kernel/terminal.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/terminal.cpp -o build/terminal.o

build/idt.o: kernel/idt.cpp kernel/idt.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/idt.cpp -o build/idt.o

build/pic.o: kernel/pic.cpp kernel/pic.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/pic.cpp -o build/pic.o

build/keyboard.o: kernel/keyboard.cpp kernel/keyboard.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/keyboard.cpp -o build/keyboard.o

build/interrupts.o: kernel/interrupts.cpp kernel/interrupts.h kernel/idt.h kernel/keyboard.h kernel/pic.h kernel/terminal.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/interrupts.cpp -o build/interrupts.o

build/memory.o: kernel/memory.cpp kernel/memory.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/memory.cpp -o build/memory.o

build/scheduler.o: kernel/scheduler.cpp kernel/scheduler.h kernel/memory.h kernel/terminal.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/scheduler.cpp -o build/scheduler.o

build/shell.o: kernel/shell.cpp kernel/shell.h kernel/interrupts.h kernel/keyboard.h kernel/memory.h kernel/scheduler.h kernel/terminal.h | build
	$(CC) $(CFLAGS) -Ikernel -c kernel/shell.cpp -o build/shell.o

build/kernel.bin: build/boot.o build/interrupt.o build/kernel.o build/terminal.o build/idt.o build/pic.o build/keyboard.o build/interrupts.o build/memory.o build/scheduler.o build/shell.o
	$(LD) $(LDFLAGS) -o build/kernel.bin $^

iso: build/kernel.bin
	cp build/kernel.bin iso/boot/kernel.bin
	grub2-mkrescue -o MiniOS.iso iso

run: iso
	qemu-system-i386 -cdrom MiniOS.iso

clean:
	rm -rf build/*.o build/kernel.bin
	rm -f iso/boot/kernel.bin
	rm -f MiniOS.iso
