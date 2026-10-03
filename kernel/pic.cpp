#include "pic.h"

static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void io_wait()
{
    outb(0x80, 0);
}

void pic_initialize()
{
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

void pit_initialize(uint32_t frequency)
{
    if (frequency == 0)
    {
        return;
    }

    uint32_t divisor = 1193182 / frequency;
    if (divisor == 0)
    {
        divisor = 1;
    }
    if (divisor > 0xFFFF)
    {
        divisor = 0xFFFF;
    }

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}
