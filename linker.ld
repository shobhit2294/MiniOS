ENTRY(_start)

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    .text :
    {
        *(.multiboot)
        *(.text)
    }

    . = ALIGN(4K);
    .rodata :
    {
        *(.rodata)
    }

    . = ALIGN(4K);
    .data :
    {
        *(.data)
    }

    . = ALIGN(4K);
    .bss :
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    __kernel_end = .;
}