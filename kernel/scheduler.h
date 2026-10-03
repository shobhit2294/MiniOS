#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

void scheduler_initialize();
bool scheduler_create_demo_tasks();
void scheduler_yield();
void scheduler_run_demo_rounds(uint32_t rounds);
uint32_t scheduler_task_count();
const char* scheduler_task_name(uint32_t index);
const char* scheduler_task_state(uint32_t index);

#endif
