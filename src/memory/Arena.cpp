#include "Arena.h"
#include "../util/Log.h"

#include <cstdlib>
#include <cstring>

namespace Polaris {

// =============================================================================
// Arena Implementation
// =============================================================================

Arena arena_create(size_t capacity) {
    Arena arena;
    arena.base = (uint8_t*)malloc(capacity);
    arena.offset = 0;
    arena.capacity = capacity;

    if (!arena.base) {
        Log::error("[Arena] Failed to allocate ", capacity, " bytes");
        arena.capacity = 0;
    } else {
        // Zero out memory for safety
        memset(arena.base, 0, capacity);
        Log::info("[Arena] Created arena: ", capacity, " bytes");
    }

    return arena;
}

void arena_destroy(Arena* arena) {
    if (arena && arena->base) {
        Log::info("[Arena] Destroying arena: ", arena->capacity, " bytes, peak usage: ", arena->offset, " bytes");
        free(arena->base);
        arena->base = nullptr;
        arena->offset = 0;
        arena->capacity = 0;
    }
}

void* arena_alloc(Arena* arena, size_t size, size_t align) {
    if (!arena || !arena->base) {
        return nullptr;
    }

    // Align the current offset
    size_t aligned_offset = (arena->offset + align - 1) & ~(align - 1);

    // Check if we have enough space
    if (aligned_offset + size > arena->capacity) {
        Log::error("[Arena] Out of memory! Requested: ", size, " bytes, Available: ", 
                   arena->capacity - aligned_offset, " bytes");
        return nullptr;
    }

    // Allocate by bumping the pointer
    void* ptr = arena->base + aligned_offset;
    arena->offset = aligned_offset + size;

    return ptr;
}

void arena_reset(Arena* arena) {
    if (arena) {
        arena->offset = 0;
        // Optional: Zero out for safety (can be disabled for performance)
        // memset(arena->base, 0, arena->capacity);
    }
}

size_t arena_used(const Arena* arena) {
    return arena ? arena->offset : 0;
}

size_t arena_remaining(const Arena* arena) {
    return arena ? (arena->capacity - arena->offset) : 0;
}

float arena_usage_percent(const Arena* arena) {
    if (!arena || arena->capacity == 0) {
        return 0.0f;
    }
    return (float)arena->offset / (float)arena->capacity;
}

} // namespace Polaris
