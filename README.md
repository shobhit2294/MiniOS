# MiniOS

MiniOS is a small 32-bit operating system built in C++ and assembly for learning OS fundamentals such as booting, interrupts, keyboard handling, terminal output, memory management, and a simple shell.

## Features

- 32-bit multiboot kernel
- GRUB bootable ISO
- VGA text-mode terminal
- Interrupt Descriptor Table (IDT)
- Programmable Interrupt Controller (PIC)
- PS/2 keyboard input handling
- Physical memory management and heap allocation
- Cooperative task scheduler
- Minimal shell with built-in commands

## Project Structure

- `boot/` - bootloader and interrupt assembly stubs
- `kernel/` - kernel source code
- `iso/` - ISO staging directory
- `linker.ld` - ELF linker script
- `Makefile` - build rules
- `MiniOS.iso` - generated bootable image

## Build

From the MiniOS project directory:

```bash
make clean
make -j2
```

Build requirements: GNU Make, NASM, a 32-bit-capable GNU C++ toolchain and linker, and GRUB's `grub2-mkrescue` utility.

This produces the bootable image:

- `MiniOS.iso`

## Run

```bash
qemu-system-i386 -m 64 -cdrom MiniOS.iso
```

QEMU is required to run the image. Use a graphical display to interact with the VGA terminal and keyboard.

## Shell Commands

Available commands in the built-in shell:

- `help`
- `clear`
- `info`
- `memory`
- `echo <text>`
- `tasks`

## Validation Checklist

To verify the OS is working correctly:

1. Build succeeds with no compiler or linker errors.
2. The ISO image is generated successfully.
3. QEMU boots the kernel without a panic.
4. Terminal displays startup messages correctly.
5. Keyboard input is accepted and echoed.
6. Shell commands respond correctly.
7. Memory statistics print valid values.
8. No hangs or crashes occur during a short stability test.

## Notes

This project is intended for educational and experimentation purposes. It is a simple kernel designed to practice low-level OS concepts rather than provide a full production operating system.
