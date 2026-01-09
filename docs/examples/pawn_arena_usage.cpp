// examples/pawn_arena_usage.cpp
// Shows arena usage in the context of Pawn systems

#include "polaris/core/Runtime.h"
#include "polaris/memory/Arena.h"
#include "pawn_composition.cpp"

namespace Game::Systems {

// ============================================================================
// AI Pathfinding with Arena
// ============================================================================

struct PathNode {
    godot::Vector3 position;
    float g_cost;  // Cost from start
    float h_cost;  // Heuristic to goal
    int parent_index;
};

void ai_pathfinding(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    // Get phase arena - automatically resets after physics phase
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
    
    const int max_nodes = 256;
    
    // Allocate scratch space for A* algorithm
    // This memory is freed automatically when phase ends
    PathNode* open_set = (PathNode*)arena_alloc(
        arena, max_nodes * sizeof(PathNode), alignof(PathNode)
    );
    
    PathNode* closed_set = (PathNode*)arena_alloc(
        arena, max_nodes * sizeof(PathNode), alignof(PathNode)
    );
    
    if (!open_set || !closed_set) {
        // Arena exhausted - shouldn't happen with proper sizing
        // If this occurs, increase Phase_Physics arena size
        return;
    }
    
    for (auto i : it) {
        if (!node[i].is_valid()) continue;
        if (ai[i].state != AIState::Chase) continue;
        
        // Run A* pathfinding using scratch space
        int open_count = 0;
        int closed_count = 0;
        
        godot::Vector3 start = node[i].get_position();
        godot::Vector3 goal = godot::Vector3();  // Get from target
        
        if (ai[i].target.is_alive()) {
            auto* target_node = ai[i].target.get<Gd::Node3D>();
            if (target_node && target_node->is_valid()) {
                goal = target_node->get_position();
            }
        }
        
        // Initialize start node
        open_set[open_count++] = {
            .position = start,
            .g_cost = 0.0f,
            .h_cost = start.distance_to(goal),
            .parent_index = -1
        };
        
        // A* main loop (simplified)
        while (open_count > 0) {
            // Find node with lowest f_cost
            int current_idx = 0;
            float lowest_f = open_set[0].g_cost + open_set[0].h_cost;
            
            for (int j = 1; j < open_count; j++) {
                float f = open_set[j].g_cost + open_set[j].h_cost;
                if (f < lowest_f) {
                    lowest_f = f;
                    current_idx = j;
                }
            }
            
            PathNode current = open_set[current_idx];
            
            // Remove from open set
            open_set[current_idx] = open_set[--open_count];
            
            // Add to closed set
            if (closed_count < max_nodes) {
                closed_set[closed_count++] = current;
            }
            
            // Check if reached goal
            if (current.position.distance_to(goal) < 1.0f) {
                // Path found - store final waypoint in AI component
                // (AI component persists, arena memory does not)
                break;
            }
            
            // Expand neighbors (simplified - would check navmesh)
            // ...
            
            // Prevent infinite loop
            if (closed_count >= max_nodes - 10) break;
        }
        
        // Path stored in AI component, scratch data freed automatically
    }
}

// ============================================================================
// Spatial Query with Arena
// ============================================================================

struct NearbyEntity {
    flecs::entity entity;
    float distance;
    godot::Vector3 position;
};

void ai_find_nearby_enemies(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
    
    const int max_nearby = 32;
    
    // Allocate temp array for spatial query results
    NearbyEntity* nearby = (NearbyEntity*)arena_alloc(
        arena,
        max_nearby * sizeof(NearbyEntity),
        alignof(NearbyEntity)
    );
    
    if (!nearby) return;
    
    for (auto i : it) {
        if (!node[i].is_valid()) continue;
        
        godot::Vector3 pos = node[i].get_position();
        int count = 0;
        
        // Find all enemies within range
        runtime()->world.each<Tag::Enemy, Gd::Node3D>(
            [&](flecs::entity e, Tag::Enemy, Gd::Node3D& enemy_node) {
                if (count >= max_nearby) return;
                if (!enemy_node.is_valid()) return;
                if (e == it.entity(i)) return;  // Skip self
                
                float dist = pos.distance_to(enemy_node.get_position());
                if (dist < ai[i].aggro_range) {
                    nearby[count++] = {
                        .entity = e,
                        .distance = dist,
                        .position = enemy_node.get_position()
                    };
                }
            }
        );
        
        // Sort by distance (bubble sort - simple for small arrays)
        for (int a = 0; a < count - 1; a++) {
            for (int b = 0; b < count - a - 1; b++) {
                if (nearby[b].distance > nearby[b + 1].distance) {
                    NearbyEntity temp = nearby[b];
                    nearby[b] = nearby[b + 1];
                    nearby[b + 1] = temp;
                }
            }
        }
        
        // Process nearest enemies first
        for (int j = 0; j < count; j++) {
            // AI logic using sorted nearby array
            // ...
        }
        
        // nearby array freed automatically at phase end
    }
}

// ============================================================================
// Combat Damage Calculation with Arena
// ============================================================================

struct DamageEvent {
    flecs::entity attacker;
    flecs::entity target;
    float amount;
    godot::Vector3 impact_point;
};

void process_combat(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
    
    const int max_damage_events = 64;
    
    // Allocate temp buffer for damage events this frame
    DamageEvent* events = (DamageEvent*)arena_alloc(
        arena,
        max_damage_events * sizeof(DamageEvent),
        alignof(DamageEvent)
    );
    
    if (!events) return;
    
    int event_count = 0;
    
    for (auto i : it) {
        if (!node[i].is_valid()) continue;
        if (ai[i].state != AIState::Attack) continue;
        if (!ai[i].target.is_alive()) continue;
        
        // Create damage event
        if (event_count < max_damage_events) {
            events[event_count++] = {
                .attacker = it.entity(i),
                .target = ai[i].target,
                .amount = 10.0f,
                .impact_point = node[i].get_position()
            };
        }
    }
    
    // Apply all damage events
    for (int j = 0; j < event_count; j++) {
        auto* hp = events[j].target.get_mut<Health>();
        if (hp) {
            hp->current -= events[j].amount;
            hp->current = godot::Math::max(hp->current, 0.0f);
        }
    }
    
    // Events freed automatically
}

// ============================================================================
// Debug String Building with Arena (Process Phase)
// ============================================================================

void debug_display_pawn_info(flecs::iter& it,
                             Health* hp,
                             Velocity* vel,
                             Gd::Node3D* node)
{
    // Use process arena for UI/debug strings
    Arena* arena = &runtime()->phases[Phase_Process].arena;
    
    for (auto i : it) {
        if (!node[i].is_valid()) continue;
        
        // Allocate temp string buffer
        char* buf = (char*)arena_alloc(arena, 256, 1);
        if (!buf) continue;
        
        flecs::entity e = it.entity(i);
        const char* type = "Unknown";
        if (e.has<Tag::Player>()) type = "Player";
        else if (e.has<Tag::Enemy>()) type = "Enemy";
        
        snprintf(buf, 256,
                 "%s\nHP: %.0f/%.0f (%.0f%%)\nSpeed: %.1f m/s\nPos: (%.1f, %.1f, %.1f)",
                 type,
                 hp[i].current, hp[i].max,
                 (hp[i].current / hp[i].max) * 100.0f,
                 vel[i].linear.length(),
                 node[i].get_position().x,
                 node[i].get_position().y,
                 node[i].get_position().z);
        
        // Use buf for debug overlay rendering
        // String copied to Godot, temp buffer freed at phase end
    }
}

// ============================================================================
// WHY ARENAS ARE CRITICAL FOR PAWNS
// ============================================================================

/*
 * Without arenas, every frame you'd need to:
 * 
 * 1. AI Pathfinding:
 *    - malloc 256 PathNodes (open set)
 *    - malloc 256 PathNodes (closed set)
 *    - free both at end
 *    = 2 mallocs, 2 frees PER AI PAWN PER FRAME
 *    = With 10 AI pawns: 20 mallocs/frees per frame at 60fps = 1200/second
 * 
 * 2. Spatial Queries:
 *    - malloc NearbyEntity array
 *    - free at end
 *    = 1 malloc, 1 free PER AI PAWN PER FRAME
 * 
 * 3. Combat Events:
 *    - malloc DamageEvent array
 *    - free at end
 *    = 1 malloc, 1 free PER FRAME
 * 
 * 4. Debug Strings:
 *    - malloc string buffer PER PAWN
 *    - free at end
 *    = With 20 pawns: 20 mallocs/frees per frame
 * 
 * TOTAL: ~50+ malloc/free calls per frame
 * 
 * With arenas:
 * - All allocations: bump pointer (5-10 cycles each)
 * - All frees: single arena reset (1 cycle)
 * - No fragmentation
 * - Cache-friendly (contiguous memory)
 * 
 * Performance difference:
 * - malloc/free: ~50-200 cycles each = 2500-10000 cycles per frame
 * - Arena: ~250-500 cycles total per frame
 * - 10-40x faster!
 */

}
