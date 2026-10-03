#include "shell.h"
#include "interrupts.h"
#include "keyboard.h"
#include "memory.h"
#include "scheduler.h"
#include "terminal.h"

static const size_t COMMAND_CAPACITY = 128;

static bool equals(const char* left, const char* right)
{
    size_t i = 0;
    while (left[i] != '\0' && right[i] != '\0' && left[i] == right[i])
    {
        ++i;
    }
    return left[i] == right[i];
}

static void print_help()
{
    terminal_writeline("Commands:");
    terminal_writeline("  help");
    terminal_writeline("  clear");
    terminal_writeline("  info");
    terminal_writeline("  memory");
    terminal_writeline("  echo <text>");
    terminal_writeline("  tasks");
}

static void print_memory()
{
    terminal_write("Usable memory: ");
    terminal_write_number(memory_total_kilobytes());
    terminal_writeline(" KiB");
    terminal_write("Free frames:   ");
    terminal_write_number(memory_free_kilobytes());
    terminal_writeline(" KiB");
    terminal_write("Heap in use:   ");
    terminal_write_number(heap_bytes_in_use());
    terminal_writeline(" bytes");
}

static void print_tasks()
{
    terminal_writeline("ID  NAME   STATE");
    for (uint32_t i = 0; i < 8; ++i)
    {
        const char* name = scheduler_task_name(i);
        if (name == 0)
        {
            continue;
        }
        terminal_write_number(i);
        terminal_write("   ");
        terminal_write(name);
        terminal_write("      ");
        terminal_writeline(scheduler_task_state(i));
    }

    terminal_writeline("Running one round-robin task cycle:");
    scheduler_run_demo_rounds(1);
    terminal_writeline("");
}

static void execute_command(const char* command)
{
    if (command[0] == '\0')
    {
        return;
    }
    if (equals(command, "help"))
    {
        print_help();
    }
    else if (equals(command, "clear"))
    {
        terminal_clear();
    }
    else if (equals(command, "info"))
    {
        terminal_writeline("MiniOS 32-bit kernel");
        terminal_writeline("Interrupt-driven keyboard and 100 Hz PIT timer");
        terminal_write("Timer ticks: ");
        terminal_write_number(timer_ticks());
        terminal_writeline("");
        terminal_writeline("Cooperative round-robin scheduler");
    }
    else if (equals(command, "memory"))
    {
        print_memory();
    }
    else if (command[0] == 'e' && command[1] == 'c' &&
             command[2] == 'h' && command[3] == 'o' &&
             (command[4] == '\0' || command[4] == ' '))
    {
        const char* text = command + 4;
        if (*text == ' ')
        {
            ++text;
        }
        terminal_writeline(text);
    }
    else if (equals(command, "tasks"))
    {
        print_tasks();
    }
    else
    {
        terminal_write("Unknown command: ");
        terminal_writeline(command);
        terminal_writeline("Type 'help' to see available commands.");
    }
}

void shell_run()
{
    char command[COMMAND_CAPACITY];
    size_t length = 0;

    terminal_writeline("MiniOS Shell");
    terminal_writeline("Type 'help' for commands.");

    for (;;)
    {
        terminal_write("MiniOS> ");
        length = 0;

        for (;;)
        {
            char character = keyboard_waitchar();
            if (character == '\n')
            {
                terminal_putchar('\n');
                command[length] = '\0';
                break;
            }
            if (character == '\b')
            {
                if (length != 0)
                {
                    --length;
                    terminal_putchar('\b');
                }
                continue;
            }
            if (character >= 32 && character <= 126 && length + 1 < COMMAND_CAPACITY)
            {
                command[length++] = character;
                terminal_putchar(character);
            }
        }

        execute_command(command);
    }
}
