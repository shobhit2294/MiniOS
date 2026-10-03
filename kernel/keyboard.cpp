#include "keyboard.h"
#include <stdint.h>

static inline unsigned char inb(unsigned short port)
{
    unsigned char result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static const char keyboard_map[] =
{
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0
};

static char input_buffer[128];
static volatile uint16_t input_head;
static volatile uint16_t input_tail;
static bool left_shift;
static bool right_shift;
static bool caps_lock;
static bool extended_scancode;

static char shifted_character(char character)
{
    switch (character)
    {
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '[': return '{';
        case ']': return '}';
        case ';': return ':';
        case '\'': return '"';
        case '`': return '~';
        case '\\': return '|';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
        default: return character;
    }
}

void keyboard_initialize()
{
    input_head = 0;
    input_tail = 0;
    left_shift = false;
    right_shift = false;
    caps_lock = false;
    extended_scancode = false;
}

void keyboard_irq_handler()
{
    unsigned char scancode = inb(0x60);
    if (scancode == 0xE0)
    {
        extended_scancode = true;
        return;
    }

    if (extended_scancode)
    {
        extended_scancode = false;
        return;
    }

    bool released = (scancode & 0x80) != 0;
    unsigned char key = scancode & 0x7F;
    if (key == 0x2A)
    {
        left_shift = !released;
        return;
    }
    if (key == 0x36)
    {
        right_shift = !released;
        return;
    }
    if (key == 0x3A)
    {
        if (!released)
        {
            caps_lock = !caps_lock;
        }
        return;
    }
    if (released || key >= sizeof(keyboard_map))
    {
        return;
    }

    char character = keyboard_map[key];
    if (character == 0)
    {
        return;
    }

    bool shift = left_shift || right_shift;
    if (character >= 'a' && character <= 'z')
    {
        if (shift != caps_lock)
        {
            character = character - 'a' + 'A';
        }
    }
    else if (shift)
    {
        character = shifted_character(character);
    }

    uint16_t next = (input_head + 1) % sizeof(input_buffer);
    if (next != input_tail)
    {
        input_buffer[input_head] = character;
        input_head = next;
    }
}

int keyboard_getchar()
{
    uint32_t flags;
    asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");

    if (input_tail == input_head)
    {
        asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
        return -1;
    }

    char character = input_buffer[input_tail];
    input_tail = (input_tail + 1) % sizeof(input_buffer);
    asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
    return character;
}

char keyboard_waitchar()
{
    for (;;)
    {
        uint32_t flags;
        asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");

        if (input_tail != input_head)
        {
            char character = input_buffer[input_tail];
            input_tail = (input_tail + 1) % sizeof(input_buffer);
            asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
            return character;
        }

        asm volatile("sti; hlt" : : : "memory");
    }
}
