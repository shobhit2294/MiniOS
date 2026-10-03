#include "interrupts.h"
#include "keyboard.h"
#include "memory.h"
#include "pic.h"
#include "scheduler.h"
#include "shell.h"
#include "terminal.h"

extern "C" void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info)
{
    terminal_initialize();
    terminal_writeline("==============================");
    terminal_writeline("        MiniOS Stage 4-7");
    terminal_writeline("==============================");

    terminal_writeline("Initializing physical memory and heap...");
    memory_initialize(multiboot_magic, multiboot_info);
    if (memory_free_kilobytes() == 0)
    {
        terminal_writeline("FATAL: no usable memory map was provided.");
        for (;;)
        {
            asm volatile("cli; hlt");
        }
    }
    terminal_writeline("Memory manager ready");

    terminal_writeline("Initializing IDT and interrupt handlers...");
    interrupts_initialize();
    terminal_writeline("IDT and ISR ready");

    terminal_writeline("Initializing PIC and PIT...");
    pic_initialize();
    pit_initialize(100);
    terminal_writeline("PIC and 100 Hz timer ready");

    keyboard_initialize();
    scheduler_initialize();
    if (!scheduler_create_demo_tasks())
    {
        terminal_writeline("FATAL: could not allocate demo task stacks.");
        for (;;)
        {
            asm volatile("cli; hlt");
        }
    }

    asm volatile("sti");
    shell_run();
}
