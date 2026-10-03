#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void memory_initialize(uint32_t multiboot_magic, uint32_t multiboot_info_address);
uint32_t physical_frame_allocate();
uint32_t physical_frames_allocate(uint32_t count);
void physical_frame_free(uint32_t address);
uint32_t memory_total_kilobytes();
uint32_t memory_free_kilobytes();
void* kmalloc(size_t size);
void kfree(void* pointer);
size_t heap_bytes_in_use();

#endif
