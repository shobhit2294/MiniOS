#include "idt.h"

static IDTEntry idt[256];
static IDTPointer idt_pointer;
extern "C" uint32_t interrupt_stub_table[];
extern "C" void idt_load(IDTPointer* pointer);

void idt_initialize()
{
    for (uint32_t vector = 0; vector < 256; ++vector)
    {
        uint32_t handler = interrupt_stub_table[vector];
        idt[vector].offset_low = handler & 0xFFFF;
        idt[vector].selector = 0x08;
        idt[vector].zero = 0;
        idt[vector].type_attr = 0x8E;
        idt[vector].offset_high = (handler >> 16) & 0xFFFF;
    }

    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = reinterpret_cast<uint32_t>(idt);
    idt_load(&idt_pointer);
}
