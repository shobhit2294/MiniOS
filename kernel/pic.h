#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void pic_initialize();
void pic_send_eoi(uint8_t irq);
void pit_initialize(uint32_t frequency);

#endif
