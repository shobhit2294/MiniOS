#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_initialize();
void keyboard_irq_handler();
int keyboard_getchar();
char keyboard_waitchar();

#endif
