#include "interrupts.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "terminal.h"

static volatile uint32_t ticks = 0;

static const char* const exception_names[] =
{
    "Divide error", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound range", "Invalid opcode",
    "Device not available", "Double fault",
    "Coprocessor segment overrun", "Invalid TSS",
    "Segment not present", "Stack-segment fault",
    "General protection fault", "Page fault", "Reserved",
    "x87 floating-point", "Alignment check", "Machine check",
    "SIMD floating-point", "Virtualization", "Control protection",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Hypervisor injection",
    "VMM communication", "Security exception", "Reserved"
};

static __attribute__((noreturn)) void panic(const InterruptFrame* frame)
{
    asm volatile("cli");
    terminal_writeline("");
    terminal_writeline("KERNEL EXCEPTION");
    terminal_write("Vector: ");
    terminal_write_number(frame->vector);
    terminal_write("  ");
    if (frame->vector < 32)
    {
        terminal_writeline(exception_names[frame->vector]);
    }
    else
    {
        terminal_writeline("Unexpected interrupt");
    }
    terminal_write("Error code: 0x");
    terminal_write_hex(frame->error_code);
    terminal_write("  EIP: 0x");
    terminal_write_hex(frame->eip);
    terminal_writeline("");
    for (;;)
    {
        asm volatile("hlt");
    }
}

void interrupts_initialize()
{
    idt_initialize();
}

uint32_t timer_ticks()
{
    return ticks;
}

extern "C" void interrupt_dispatch(InterruptFrame* frame)
{
    if (frame->vector < 32)
    {
        panic(frame);
    }

    if (frame->vector >= 32 && frame->vector < 48)
    {
        uint8_t irq = frame->vector - 32;
        if (irq == 0)
        {
            ticks = ticks + 1;
        }
        else if (irq == 1)
        {
            keyboard_irq_handler();
        }
        pic_send_eoi(irq);
        return;
    }

    panic(frame);
}
