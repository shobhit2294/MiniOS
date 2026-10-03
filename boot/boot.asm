bits 32

section .multiboot
align 4

    dd 0x1BADB002
    dd 3
    dd -(0x1BADB002 + 3)

section .text

global _start
extern kernel_main

_start:
    cli
    mov esi, eax
    mov edi, ebx
    lgdt [gdt_descriptor]
    jmp 0x08:reload_segments

reload_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, stack_top
    xor ebp, ebp

    push edi
    push esi
    call kernel_main

hang:
    hlt
    jmp hang

section .rodata
align 8
gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

section .bss
align 16
global stack_bottom
stack_bottom:
    resb 16384
stack_top: