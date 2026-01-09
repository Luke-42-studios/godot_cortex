#ifndef POLARIS_ARENA_H
#define POLARIS_ARENA_H

#include <cstddef>
#include <cstdint>

namespace Polaris {

// =============================================================================
// Arena - Bump allocator for temporary memory
// =============================================================================
//
// An arena is a large pre-allocated block of memory. Allocations are made by
// bumping a pointer forward. All memory is freed at once by resetting the
// pointer.
//
// BENEFITS:
//   - Fast allocation (~5-10 cycles vs ~50-200 for malloc)
//   - No fragmentation
//   - No per-object overhead
//   - Automatic cleanup via reset
//
// USAGE:
//   Arena arena = arena_create(1024 * 1024);  // 1 MB
//   void* ptr = arena_alloc(&arena, 256, 8);  // Allocate 256 bytes, 8-byte aligned
//   // ... use ptr ...
//   arena_reset(&arena);  // Free everything
//   arena_destroy(&arena);  // Free the arena itself
//
// =============================================================================

struct Arena {
    uint8_t* base;      // Start of memory block
    size_t offset;      // Current allocation position (bytes used)
    size_t capacity;    // Total size (bytes)
};

// =============================================================================
// Arena API
// =============================================================================

/// Create an arena with the given capacity
Arena arena_create(size_t capacity);

/// Destroy an arena and free its memory
void arena_destroy(Arena* arena);

/// Allocate memory from the arena
/// Returns nullptr if out of memory
void* arena_alloc(Arena* arena, size_t size, size_t align);

/// Reset the arena (free all allocations)
void arena_reset(Arena* arena);

/// Get current usage
size_t arena_used(const Arena* arena);

/// Get remaining capacity
size_t arena_remaining(const Arena* arena);

/// Get usage percentage (0.0 to 1.0)
float arena_usage_percent(const Arena* arena);

} // namespace Polaris

#endif // POLARIS_ARENA_H
