bits 32

section .text

global idt_load
global context_switch
global interrupt_stub_table
extern interrupt_dispatch

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

context_switch:
    push ebp
    push ebx
    push esi
    push edi
    mov eax, [esp + 20]
    mov [eax], esp
    mov esp, [esp + 24]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

interrupt_common:
    pusha

    xor eax, eax
    mov ax, ds
    push eax
    xor eax, eax
    mov ax, es
    push eax
    xor eax, eax
    mov ax, fs
    push eax
    xor eax, eax
    mov ax, gs
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    mov ebp, esp
    and esp, 0xFFFFFFF0
    sub esp, 12
    push ebp
    call interrupt_dispatch
    mov esp, ebp

    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax
    popa
    add esp, 8
    iretd

%macro ISR_NO_ERROR 1
isr_stub_%1:
    push dword 0
    push dword %1
    jmp interrupt_common
%endmacro

%macro ISR_ERROR 1
isr_stub_%1:
    push dword %1
    jmp interrupt_common
%endmacro

%assign vector 0
%rep 256
    %if vector = 8 || vector = 10 || vector = 11 || vector = 12 || vector = 13 || vector = 14 || vector = 17 || vector = 21 || vector = 29 || vector = 30
        ISR_ERROR vector
    %else
        ISR_NO_ERROR vector
    %endif
    %assign vector vector + 1
%endrep

section .rodata
align 4
interrupt_stub_table:
%assign vector 0
%rep 256
    dd isr_stub_%+vector
    %assign vector vector + 1
%endrep
