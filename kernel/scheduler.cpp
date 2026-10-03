#include "scheduler.h"
#include "memory.h"
#include "terminal.h"

static const uint32_t MAX_TASKS = 8;
static const size_t TASK_STACK_SIZE = 2048;

enum TaskState
{
    TASK_UNUSED,
    TASK_RUNNABLE,
    TASK_RUNNING,
    TASK_TERMINATED
};

struct Task
{
    uint32_t id;
    const char* name;
    void (*entry)();
    uint32_t* stack_pointer;
    void* stack;
    TaskState state;
};

static Task tasks[MAX_TASKS];
static uint32_t current_task;
static uint32_t next_task_id;

extern "C" void context_switch(uint32_t** old_stack, uint32_t* new_stack);
extern "C" void task_bootstrap();

static void task_a()
{
    for (;;)
    {
        terminal_write("A ");
        scheduler_yield();
    }
}

static void task_b()
{
    for (;;)
    {
        terminal_write("B ");
        scheduler_yield();
    }
}

static void task_c()
{
    for (;;)
    {
        terminal_write("C ");
        scheduler_yield();
    }
}

static void reap_terminated_tasks()
{
    for (uint32_t i = 0; i < MAX_TASKS; ++i)
    {
        if (i != current_task && tasks[i].state == TASK_TERMINATED && tasks[i].stack != 0)
        {
            kfree(tasks[i].stack);
            tasks[i].stack = 0;
            tasks[i].stack_pointer = 0;
        }
    }
}

void scheduler_initialize()
{
    for (uint32_t i = 0; i < MAX_TASKS; ++i)
    {
        tasks[i].id = 0;
        tasks[i].name = 0;
        tasks[i].entry = 0;
        tasks[i].stack_pointer = 0;
        tasks[i].stack = 0;
        tasks[i].state = TASK_UNUSED;
    }

    current_task = 0;
    next_task_id = 1;
    tasks[0].id = 0;
    tasks[0].name = "shell";
    tasks[0].state = TASK_RUNNING;
}

static bool create_task(const char* name, void (*entry)())
{
    uint32_t slot = 1;
    while (slot < MAX_TASKS && tasks[slot].state != TASK_UNUSED)
    {
        ++slot;
    }
    if (slot == MAX_TASKS)
    {
        return false;
    }

    void* stack = kmalloc(TASK_STACK_SIZE);
    if (stack == 0)
    {
        return false;
    }

    uintptr_t top = (reinterpret_cast<uintptr_t>(stack) + TASK_STACK_SIZE) & ~static_cast<uintptr_t>(0xF);
    uint32_t* frame = reinterpret_cast<uint32_t*>(top) - 5;
    frame[0] = 0;
    frame[1] = 0;
    frame[2] = 0;
    frame[3] = 0;
    frame[4] = reinterpret_cast<uintptr_t>(task_bootstrap);

    tasks[slot].id = next_task_id++;
    tasks[slot].name = name;
    tasks[slot].entry = entry;
    tasks[slot].stack_pointer = frame;
    tasks[slot].stack = stack;
    tasks[slot].state = TASK_RUNNABLE;
    return true;
}

bool scheduler_create_demo_tasks()
{
    if (!create_task("A", task_a))
    {
        return false;
    }
    if (!create_task("B", task_b))
    {
        return false;
    }
    if (!create_task("C", task_c))
    {
        return false;
    }
    return true;
}

void scheduler_yield()
{
    uint32_t previous = current_task;
    if (tasks[previous].state == TASK_RUNNING)
    {
        tasks[previous].state = TASK_RUNNABLE;
    }

    uint32_t next = previous;
    for (uint32_t offset = 1; offset <= MAX_TASKS; ++offset)
    {
        uint32_t candidate = (previous + offset) % MAX_TASKS;
        if (tasks[candidate].state == TASK_RUNNABLE)
        {
            next = candidate;
            break;
        }
    }

    if (next == previous)
    {
        tasks[previous].state = TASK_RUNNING;
        return;
    }

    tasks[next].state = TASK_RUNNING;
    current_task = next;
    context_switch(&tasks[previous].stack_pointer, tasks[next].stack_pointer);
    reap_terminated_tasks();
}

void scheduler_run_demo_rounds(uint32_t rounds)
{
    while (rounds-- != 0)
    {
        scheduler_yield();
    }
}

uint32_t scheduler_task_count()
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_TASKS; ++i)
    {
        if (tasks[i].state != TASK_UNUSED)
        {
            ++count;
        }
    }
    return count;
}

const char* scheduler_task_name(uint32_t index)
{
    if (index >= MAX_TASKS || tasks[index].state == TASK_UNUSED)
    {
        return 0;
    }
    return tasks[index].name;
}

const char* scheduler_task_state(uint32_t index)
{
    if (index >= MAX_TASKS)
    {
        return "invalid";
    }

    switch (tasks[index].state)
    {
        case TASK_RUNNABLE: return "runnable";
        case TASK_RUNNING: return "running";
        case TASK_TERMINATED: return "terminated";
        default: return "unused";
    }
}

extern "C" void task_bootstrap()
{
    tasks[current_task].entry();
    tasks[current_task].state = TASK_TERMINATED;
    scheduler_yield();
    for (;;)
    {
        asm volatile("hlt");
    }
}
