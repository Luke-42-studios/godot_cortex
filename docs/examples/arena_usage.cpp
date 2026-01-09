// examples/arena_usage.cpp
// Demonstrates why phase arenas are important and how to use them

#include "polaris/core/Runtime.h"
#include "polaris/memory/Arena.h"

namespace Examples {

// ============================================================================
// WHY PHASE ARENAS?
// ============================================================================

/*
 * Phase arenas solve 3 critical problems:
 *
 * 1. PERFORMANCE - Allocation is O(1), no fragmentation, cache-friendly
 * 2. SAFETY - No memory leaks, automatic cleanup, no use-after-free
 * 3. SIMPLICITY - No manual free(), no tracking lifetimes
 *
 * Without arenas, you'd need to:
 * - malloc/free every frame (slow, fragments memory)
 * - Track ownership of every allocation (complex, error-prone)
 * - Risk memory leaks if you forget to free
 * - Deal with use-after-free bugs
 */

// ============================================================================
// BAD: Without Arenas
// ============================================================================

void spatial_query_bad(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    for (auto i : it) {
        // malloc every frame - SLOW!
        std::vector<flecs::entity> nearby;
        
        // Find nearby entities
        // ... query logic ...
        
        // Vector destructor frees - but this happens EVERY FRAME
        // Causes fragmentation and cache misses
    }
    // If you forget to clear or exception occurs = memory leak
}

// ============================================================================
// GOOD: With Phase Arena
// ============================================================================

void spatial_query_good(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
    
    for (auto i : it) {
        // Allocate from arena - FAST (just bump pointer)
        const int max_nearby = 32;
        flecs::entity* nearby = (flecs::entity*)arena_alloc(
            arena,
            max_nearby * sizeof(flecs::entity),
            alignof(flecs::entity)
        );
        
        if (!nearby) {
            // Arena full - shouldn't happen with proper sizing
            continue;
        }
        
        int count = 0;
        
        // Find nearby entities
        // ... query logic fills nearby array ...
        
        // Use nearby array
        // ... AI logic ...
        
        // NO FREE NEEDED - arena resets at end of phase
    }
    // All allocations freed in one O(1) operation when phase ends
}

// ============================================================================
// Example: Pathfinding with Arena
// ============================================================================

struct PathNode {
    Vector3 position;
    float g_cost;
    float h_cost;
    int parent_index;
};

void pathfinding_system(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
    
    const int max_nodes = 256;
    
    // Allocate scratch space for A* algorithm
    PathNode* open_set = (PathNode*)arena_alloc(
        arena, max_nodes * sizeof(PathNode), alignof(PathNode)
    );
    
    PathNode* closed_set = (PathNode*)arena_alloc(
        arena, max_nodes * sizeof(PathNode), alignof(PathNode)
    );
    
    if (!open_set || !closed_set) {
        // Arena exhausted - increase Phase_Physics arena size
        return;
    }
    
    for (auto i : it) {
        if (!node[i].is_valid()) continue;
        
        // Run A* pathfinding using scratch space
        int open_count = 0;
        int closed_count = 0;
        
        // ... pathfinding algorithm ...
        
        // Store final path in AI component (persistent)
        // Scratch data automatically freed at phase end
    }
}

// ============================================================================
// Example: String Building for Debug UI
// ============================================================================

void debug_ui_system(flecs::iter& it, Health* hp, Gd::Label* label) {
    Arena* arena = &runtime()->phases[Phase_Process].arena;
    
    for (auto i : it) {
        if (!label[i].is_valid()) continue;
        
        // Allocate temp string buffer
        char* buf = (char*)arena_alloc(arena, 128, 1);
        if (!buf) continue;
        
        snprintf(buf, 128, "HP: %.0f/%.0f (%.0f%%)",
                 hp[i].current, hp[i].max,
                 (hp[i].current / hp[i].max) * 100.0f);
        
        label[i].ptr->set_text(String(buf));
        
        // String copied to Godot, temp buffer freed at phase end
    }
}

// ============================================================================
// Example: Collision List Building
// ============================================================================

struct CollisionInfo {
    flecs::entity entity;
    Vector3 point;
    Vector3 normal;
    float penetration;
};

void collision_detection(flecs::iter& it, Gd::CharacterBody3D* body) {
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
    
    const int max_collisions = 16;
    CollisionInfo* collisions = (CollisionInfo*)arena_alloc(
        arena,
        max_collisions * sizeof(CollisionInfo),
        alignof(CollisionInfo)
    );
    
    if (!collisions) return;
    
    for (auto i : it) {
        if (!body[i].is_valid()) continue;
        
        int collision_count = 0;
        
        // Gather collision info
        for (int c = 0; c < body[i].ptr->get_slide_collision_count(); c++) {
            if (collision_count >= max_collisions) break;
            
            auto col = body[i].ptr->get_slide_collision(c);
            collisions[collision_count++] = {
                .entity = {}, // lookup entity from collider
                .point = col->get_position(),
                .normal = col->get_normal(),
                .penetration = col->get_depth()
            };
        }
        
        // Process collisions
        for (int c = 0; c < collision_count; c++) {
            // Handle collision response
        }
        
        // Collision list freed automatically
    }
}

// ============================================================================
// Permanent Arena Usage
// ============================================================================

void initialize_lookup_table(Runtime* rt) {
    Arena* perm = &rt->permanent;
    
    // Allocate data that lives for the entire application
    struct WeaponDef {
        const char* name;
        float damage;
        float cooldown;
    };
    
    const int weapon_count = 10;
    WeaponDef* weapons = (WeaponDef*)arena_alloc(
        perm,
        weapon_count * sizeof(WeaponDef),
        alignof(WeaponDef)
    );
    
    // Initialize weapon definitions
    weapons[0] = { "Sword", 25.0f, 0.5f };
    weapons[1] = { "Axe", 40.0f, 1.0f };
    // ... etc
    
    // This memory persists until application shutdown
    // No need to free - permanent arena never resets
}

// ============================================================================
// Arena Size Monitoring
// ============================================================================

void check_arena_usage(Runtime* rt) {
    const char* phase_names[] = { "Input", "Physics", "Process" };
    
    for (int i = 0; i < Phase_Count; i++) {
        Arena* a = &rt->phases[i].arena;
        float usage = (float)a->offset / (float)a->capacity * 100.0f;
        
        printf("[%s] Arena: %.1f%% used (%zu / %zu bytes)\n",
               phase_names[i], usage, a->offset, a->capacity);
        
        if (usage > 80.0f) {
            printf("  WARNING: Arena nearly full! Consider increasing size.\n");
        }
    }
}

}
