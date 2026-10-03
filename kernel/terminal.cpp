#include "terminal.h"


// ============================================================
// VGA configuration
// ============================================================

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;


// VGA text memory
static volatile unsigned short* const VGA_MEMORY =
    (unsigned short*)0xB8000;


// Current cursor position
static size_t row = 0;
static size_t column = 0;


// Current text color
static unsigned char color = 0x07;

static void scroll_if_needed()
{
    if (row < VGA_HEIGHT)
    {
        return;
    }

    for (size_t y = 1; y < VGA_HEIGHT; ++y)
    {
        for (size_t x = 0; x < VGA_WIDTH; ++x)
        {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] =
                VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; ++x)
    {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            ((unsigned short)color << 8) | ' ';
    }
    row = VGA_HEIGHT - 1;
}


// ============================================================
// Port I/O
// ============================================================

static inline void outb(
    unsigned short port,
    unsigned char value)
{
    asm volatile(
        "outb %0, %1"
        :
        : "a"(value),
          "Nd"(port)
    );
}


// ============================================================
// Create VGA color
// ============================================================

static unsigned char make_color(
    VGAColor foreground,
    VGAColor background)
{
    return foreground | (background << 4);
}


// ============================================================
// Update hardware cursor
// ============================================================

static void update_cursor()
{
    unsigned short position =
        row * VGA_WIDTH + column;


    // Tell VGA that we are writing the
    // cursor low byte
    outb(0x3D4, 0x0F);

    outb(
        0x3D5,
        position & 0xFF
    );


    // Tell VGA that we are writing the
    // cursor high byte
    outb(0x3D4, 0x0E);

    outb(
        0x3D5,
        (position >> 8) & 0xFF
    );
}


// ============================================================
// Clear screen
// ============================================================

void terminal_clear()
{
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            VGA_MEMORY[
                y * VGA_WIDTH + x
            ] =
                ((unsigned short)color << 8) | ' ';
        }
    }


    row = 0;
    column = 0;

    update_cursor();
}


// ============================================================
// Initialize terminal
// ============================================================

void terminal_initialize()
{
    row = 0;
    column = 0;

    color = make_color(
        COLOR_LIGHT_GREY,
        COLOR_BLACK
    );

    terminal_clear();
}


// ============================================================
// Change text color
// ============================================================

void terminal_set_color(
    VGAColor foreground,
    VGAColor background)
{
    color = make_color(
        foreground,
        background
    );
}


// ============================================================
// Print one character
// ============================================================

void terminal_putchar(char c)
{
    // --------------------------------------------------------
    // Newline
    // --------------------------------------------------------

    if (c == '\n')
    {
        column = 0;
        row++;
        scroll_if_needed();

        update_cursor();

        return;
    }


    // --------------------------------------------------------
    // Backspace
    // --------------------------------------------------------

    if (c == '\b')
    {
        if (column == 0 && row == 0)
        {
            update_cursor();
            return;
        }

        if (column > 0)
        {
            column--;
        }
        else if (row > 0)
        {
            row--;
            column = VGA_WIDTH - 1;
        }

        VGA_MEMORY[
            row * VGA_WIDTH + column
        ] =
            ((unsigned short)color << 8) | ' ';

        update_cursor();

        return;
    }


    // --------------------------------------------------------
    // Print character
    // --------------------------------------------------------

    VGA_MEMORY[
        row * VGA_WIDTH + column
    ] =
        ((unsigned short)color << 8) | c;


    column++;


    // --------------------------------------------------------
    // End of line
    // --------------------------------------------------------

    if (column >= VGA_WIDTH)
    {
        column = 0;
        row++;
        scroll_if_needed();
    }


    // Update hardware cursor
    update_cursor();
}


// ============================================================
// Print string
// ============================================================

void terminal_write(const char* str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
    {
        terminal_putchar(str[i]);
    }
}


// ============================================================
// Print string + newline
// ============================================================

void terminal_writeline(const char* str)
{
    terminal_write(str);

    terminal_putchar('\n');
}


// ============================================================
// Print unsigned integer
// ============================================================

void terminal_write_number(unsigned int number)
{
    if (number == 0)
    {
        terminal_putchar('0');
        return;
    }


    char buffer[10];

    int i = 0;


    while (number > 0)
    {
        buffer[i++] =
            '0' + (number % 10);

        number /= 10;
    }


    while (i > 0)
    {
        terminal_putchar(
            buffer[--i]
        );
    }
}

void terminal_write_hex(unsigned int number)
{
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4)
    {
        terminal_putchar(digits[(number >> shift) & 0xF]);
    }
}