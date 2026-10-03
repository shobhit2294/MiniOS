#include "memory.h"

static const uint32_t PAGE_SIZE = 4096;
static const uint32_t MAX_FRAMES = 1u << 20;
static const uint32_t BITMAP_BYTES = MAX_FRAMES / 8;
static const uint32_t MULTIBOOT_MAGIC = 0x2BADB002;
static uint8_t frame_bitmap[BITMAP_BYTES];
static uint8_t allocated_bitmap[BITMAP_BYTES];
static uint32_t frame_count_free;
static uint64_t usable_bytes;
static uint32_t first_free_frame;
static uint32_t boot_info_address;
static uint32_t memory_map_address;
static uint32_t memory_map_length;
static uint32_t kernel_end_address;
static bool allocator_ready;

struct MultibootInfo
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t command_line;
    uint32_t modules_count;
    uint32_t modules_address;
    uint32_t symbols[4];
    uint32_t memory_map_length;
    uint32_t memory_map_address;
} __attribute__((packed));

struct MultibootMemoryMapEntry
{
    uint32_t size;
    uint64_t address;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

struct MultibootModule
{
    uint32_t start;
    uint32_t end;
    uint32_t command_line;
    uint32_t reserved;
} __attribute__((packed));

struct HeapArena;

struct __attribute__((aligned(16))) HeapBlock
{
    size_t size;
    bool free;
    HeapArena* arena;
    HeapBlock* previous;
    HeapBlock* next;
};

struct HeapArena
{
    HeapArena* next;
    HeapBlock* first;
};

struct __attribute__((aligned(16))) LargeAllocation
{
    LargeAllocation* next;
    uint32_t frame_count;
    size_t size;
    uint32_t reserved;
};

static HeapArena* heap_arenas;
static LargeAllocation* large_allocations;
static size_t heap_used;

extern "C" uint8_t __kernel_end;

static bool bit_is_set(const uint8_t* bitmap, uint32_t frame)
{
    return (bitmap[frame / 8] & (1u << (frame % 8))) != 0;
}

static void set_bit(uint8_t* bitmap, uint32_t frame)
{
    bitmap[frame / 8] |= 1u << (frame % 8);
}

static void clear_bit(uint8_t* bitmap, uint32_t frame)
{
    bitmap[frame / 8] &= ~(1u << (frame % 8));
}

static uint64_t saturating_end(uint64_t address, uint64_t length)
{
    uint64_t end = address + length;
    return end < address ? UINT64_MAX : end;
}

static bool overlaps(uint64_t begin, uint64_t end, uint64_t other_begin, uint64_t other_end)
{
    return begin < other_end && other_begin < end;
}

static bool frame_is_reserved(uint64_t begin, uint64_t end, const MultibootInfo* info)
{
    if (begin < kernel_end_address)
    {
        return true;
    }

    if (info != 0)
    {
        if (overlaps(begin, end, boot_info_address, boot_info_address + sizeof(MultibootInfo)) ||
            overlaps(begin, end, memory_map_address, (uint64_t)memory_map_address + memory_map_length))
        {
            return true;
        }

        if ((info->flags & (1u << 3)) != 0)
        {
            const MultibootModule* modules =
                reinterpret_cast<const MultibootModule*>(info->modules_address);
            for (uint32_t i = 0; i < info->modules_count; ++i)
            {
                if (overlaps(begin, end, modules[i].start, modules[i].end))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

static void release_available_range(uint64_t address, uint64_t length, const MultibootInfo* info)
{
    uint64_t end = saturating_end(address, length);
    if (address >= (uint64_t)MAX_FRAMES * PAGE_SIZE)
    {
        return;
    }
    if (end > (uint64_t)MAX_FRAMES * PAGE_SIZE)
    {
        end = (uint64_t)MAX_FRAMES * PAGE_SIZE;
    }

    usable_bytes += end - address;
    uint64_t first = (address + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t last = end / PAGE_SIZE;
    for (uint64_t frame = first; frame < last; ++frame)
    {
        uint64_t frame_begin = frame * PAGE_SIZE;
        if (frame_is_reserved(frame_begin, frame_begin + PAGE_SIZE, info))
        {
            continue;
        }
        clear_bit(frame_bitmap, frame);
        ++frame_count_free;
        if (frame < first_free_frame)
        {
            first_free_frame = frame;
        }
    }
}

static HeapArena* create_arena()
{
    uint32_t address = physical_frame_allocate();
    if (address == 0)
    {
        return 0;
    }

    HeapArena* arena = reinterpret_cast<HeapArena*>(address);
    arena->next = heap_arenas;
    arena->first = reinterpret_cast<HeapBlock*>(
        (reinterpret_cast<uintptr_t>(address + sizeof(HeapArena)) + 15u) & ~static_cast<uintptr_t>(15u));
    arena->first->size = PAGE_SIZE - (reinterpret_cast<uintptr_t>(arena->first) - address) - sizeof(HeapBlock);
    arena->first->free = true;
    arena->first->arena = arena;
    arena->first->previous = 0;
    arena->first->next = 0;
    heap_arenas = arena;
    return arena;
}

void memory_initialize(uint32_t multiboot_magic, uint32_t multiboot_info_addr)
{
    for (uint32_t i = 0; i < BITMAP_BYTES; ++i)
    {
        frame_bitmap[i] = 0xFF;
        allocated_bitmap[i] = 0;
    }

    frame_count_free = 0;
    usable_bytes = 0;
    first_free_frame = MAX_FRAMES;
    boot_info_address = multiboot_info_addr;
    kernel_end_address = (reinterpret_cast<uintptr_t>(&__kernel_end) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    heap_arenas = 0;
    large_allocations = 0;
    heap_used = 0;
    allocator_ready = false;

    const MultibootInfo* info = 0;
    if (multiboot_magic == MULTIBOOT_MAGIC && multiboot_info_addr != 0)
    {
        info = reinterpret_cast<const MultibootInfo*>(multiboot_info_addr);
    }

    memory_map_address = 0;
    memory_map_length = 0;
    if (info != 0 && (info->flags & (1u << 6)) != 0 && info->memory_map_address != 0)
    {
        memory_map_address = info->memory_map_address;
        memory_map_length = info->memory_map_length;
        uint32_t offset = 0;
        while (offset + sizeof(uint32_t) <= memory_map_length)
        {
            const MultibootMemoryMapEntry* entry =
                reinterpret_cast<const MultibootMemoryMapEntry*>(memory_map_address + offset);
            uint32_t entry_size = entry->size;
            if (entry_size < 20 || entry_size > memory_map_length - offset - sizeof(uint32_t))
            {
                break;
            }
            if (entry->type == 1)
            {
                release_available_range(entry->address, entry->length, info);
            }
            offset += entry_size + sizeof(uint32_t);
        }
    }
    else if (info != 0 && (info->flags & 1u) != 0)
    {
        memory_map_address = multiboot_info_addr;
        memory_map_length = sizeof(MultibootInfo);
        release_available_range(0x100000, (uint64_t)info->mem_upper * 1024, info);
    }

    allocator_ready = frame_count_free != 0;
}

uint32_t physical_frame_allocate()
{
    return physical_frames_allocate(1);
}

uint32_t physical_frames_allocate(uint32_t count)
{
    if (count == 0 || count > MAX_FRAMES ||
        (!allocator_ready && first_free_frame == MAX_FRAMES))
    {
        return 0;
    }

    for (uint32_t frame = first_free_frame; frame <= MAX_FRAMES - count;)
    {
        uint32_t available = 0;
        while (available < count && !bit_is_set(frame_bitmap, frame + available))
        {
            ++available;
        }
        if (available == count)
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                set_bit(frame_bitmap, frame + i);
                set_bit(allocated_bitmap, frame + i);
            }
            frame_count_free -= count;
            first_free_frame = frame + count;
            return frame * PAGE_SIZE;
        }

        frame += available + 1;
    }
    return 0;
}

void physical_frame_free(uint32_t address)
{
    if ((address & (PAGE_SIZE - 1)) != 0)
    {
        return;
    }

    uint32_t frame = address / PAGE_SIZE;
    if (frame >= MAX_FRAMES || !bit_is_set(allocated_bitmap, frame))
    {
        return;
    }

    clear_bit(allocated_bitmap, frame);
    clear_bit(frame_bitmap, frame);
    ++frame_count_free;
    if (frame < first_free_frame)
    {
        first_free_frame = frame;
    }
}

uint32_t memory_total_kilobytes()
{
    return usable_bytes / 1024;
}

uint32_t memory_free_kilobytes()
{
    return (uint64_t)frame_count_free * PAGE_SIZE / 1024;
}

void* kmalloc(size_t size)
{
    if (size == 0 || !allocator_ready || size > static_cast<size_t>(-1) - 15u)
    {
        return 0;
    }

    size = (size + 15u) & ~static_cast<size_t>(15u);
    const size_t arena_offset =
        (sizeof(HeapArena) + 15u) & ~static_cast<size_t>(15u);
    if (size > PAGE_SIZE - arena_offset - sizeof(HeapBlock))
    {
        if (size > static_cast<size_t>(-1) - sizeof(LargeAllocation) - (PAGE_SIZE - 1))
        {
            return 0;
        }
        uint32_t page_count =
            (size + sizeof(LargeAllocation) + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t address = physical_frames_allocate(page_count);
        if (address == 0)
        {
            return 0;
        }

        LargeAllocation* allocation = reinterpret_cast<LargeAllocation*>(address);
        allocation->next = large_allocations;
        allocation->frame_count = page_count;
        allocation->size = size;
        allocation->reserved = 0;
        large_allocations = allocation;
        heap_used += size;
        return allocation + 1;
    }

    for (;;)
    {
        for (HeapArena* arena = heap_arenas; arena != 0; arena = arena->next)
        {
            for (HeapBlock* block = arena->first; block != 0; block = block->next)
            {
                if (!block->free || block->size < size)
                {
                    continue;
                }

                if (block->size >= size + sizeof(HeapBlock) + 16)
                {
                    HeapBlock* split = reinterpret_cast<HeapBlock*>(
                        reinterpret_cast<uintptr_t>(block + 1) + size);
                    split->size = block->size - size - sizeof(HeapBlock);
                    split->free = true;
                    split->arena = arena;
                    split->previous = block;
                    split->next = block->next;
                    if (split->next != 0)
                    {
                        split->next->previous = split;
                    }
                    block->next = split;
                    block->size = size;
                }

                block->free = false;
                heap_used += block->size;
                return block + 1;
            }
        }

        if (create_arena() == 0)
        {
            return 0;
        }
    }
}

void kfree(void* pointer)
{
    if (pointer == 0)
    {
        return;
    }

    LargeAllocation** large_link = &large_allocations;
    while (*large_link != 0)
    {
        LargeAllocation* allocation = *large_link;
        if (allocation + 1 == pointer)
        {
            uint32_t base = reinterpret_cast<uintptr_t>(allocation);
            uint32_t frame_count = allocation->frame_count;
            *large_link = allocation->next;
            heap_used -= allocation->size;
            for (uint32_t i = 0; i < frame_count; ++i)
            {
                physical_frame_free(base + i * PAGE_SIZE);
            }
            return;
        }
        large_link = &allocation->next;
    }

    HeapBlock* block = 0;
    for (HeapArena* arena = heap_arenas; arena != 0 && block == 0; arena = arena->next)
    {
        for (HeapBlock* candidate = arena->first; candidate != 0; candidate = candidate->next)
        {
            if (candidate + 1 == pointer)
            {
                block = candidate;
                break;
            }
        }
    }
    if (block == 0 || block->free)
    {
        return;
    }

    block->free = true;
    heap_used -= block->size;
    if (block->next != 0 && block->next->free)
    {
        HeapBlock* next = block->next;
        block->size += sizeof(HeapBlock) + next->size;
        block->next = next->next;
        if (block->next != 0)
        {
            block->next->previous = block;
        }
    }
    if (block->previous != 0 && block->previous->free)
    {
        HeapBlock* previous = block->previous;
        previous->size += sizeof(HeapBlock) + block->size;
        previous->next = block->next;
        if (previous->next != 0)
        {
            previous->next->previous = previous;
        }
        block = previous;
    }

    HeapArena* arena = block->arena;
    if (block->previous == 0 && block->next == 0 && arena->next != 0)
    {
        HeapArena** link = &heap_arenas;
        while (*link != arena)
        {
            link = &(*link)->next;
        }
        *link = arena->next;
        physical_frame_free(reinterpret_cast<uintptr_t>(arena));
    }
}

size_t heap_bytes_in_use()
{
    return heap_used;
}
