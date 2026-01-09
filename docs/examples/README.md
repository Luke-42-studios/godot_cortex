# Examples Index

This directory contains complete, runnable code examples referenced by the documentation.

## Purpose

Instead of cluttering documentation with large code blocks, we provide:
- **Snippets in docs** - Small, focused examples inline
- **Complete examples here** - Full, working implementations you can reference

## Examples

### [pawn_composition.cpp](pawn_composition.cpp) ⭐ **START HERE**
Complete Pawn hierarchy showing Composition pattern:
- **Pawn** - Base class with Health, Velocity, MoveSpeed
- **PlayerPawn** - Adds InputState, Tag::Player
- **AIPawn** - Adds AI component, Tag::Enemy
- Godot integration (inspector properties, metadata)
- compose() and decompose() lifecycle

**Referenced by:** All docs (0-5)

**This is the foundation** - All other examples build on these Pawns.

---

### [pawn_systems.cpp](pawn_systems.cpp) ⭐ **CORE SYSTEMS**
Complete system implementations for Pawns:
- **Input Phase** - Player input capture
- **Physics Phase** - Player movement, AI movement, gravity, velocity application, ground detection, AI state machine, target acquisition, combat
- **Process Phase** - Animation updates
- Full registration example

**Referenced by:** `3. Lifecycle.md`, `4. Coding Standards.md`

**Shows the complete picture** - How Pawns come alive through systems.

---

### [pawn_arena_usage.cpp](pawn_arena_usage.cpp) ⭐ **MEMORY MANAGEMENT**
Arena usage in Pawn systems:
- AI pathfinding with scratch space
- Spatial queries (find nearby enemies)
- Combat damage event batching
- Debug string building
- Performance analysis (why arenas matter)

**Referenced by:** `5. Memory Management.md`, `3. Lifecycle.md`

**Critical for performance** - Shows why arenas are 10-40x faster than malloc.

---

### [movement_system.cpp](movement_system.cpp)
Complete movement system showing:
- Input capture (Phase_Input)
- Player movement calculation (Phase_Physics)
- Gravity application (Phase_Physics)
- Velocity application with `move_and_slide()` (Phase_Physics)
- Ground detection (Phase_Physics)
- Visual position updates (Phase_Process)

**Referenced by:** `3. Lifecycle.md`, `2.0 Entity.md`

---

### [custom_phases.cpp](custom_phases.cpp)
Advanced phase control showing:
- Creating custom sub-phases for fine-grained ordering
- Using `flecs::DependsOn` for explicit dependencies
- Physics sub-phases: Input → Movement → Apply → Collision
- Process sub-phases: Interpolate → Animate → Effects
- Alternative: Registration order vs explicit dependencies

**Referenced by:** `3. Lifecycle.md`

**Why use custom phases?**
- Precise control over system execution order
- Multiple systems can target the same sub-phase
- Clear semantic grouping (e.g., "all collision response runs here")
- Easier to reason about complex system interactions

---

### [arena_usage.cpp](arena_usage.cpp)
Memory management patterns showing:
- Why phase arenas matter (performance, safety, simplicity)
- Bad vs good allocation patterns
- Pathfinding with scratch space
- String building for UI
- Collision list building
- Permanent arena for lookup tables
- Arena usage monitoring

**Referenced by:** `5. Memory Management.md`

**Why arenas?**
1. **Performance** - O(1) allocation, no fragmentation, cache-friendly
2. **Safety** - No leaks, automatic cleanup, no use-after-free
3. **Simplicity** - No manual `free()`, no lifetime tracking

---

## How to Use These Examples

### Reading the Docs
When you see a reference like:
```
See: examples/pawn_composition.cpp for complete implementation
```

Open that file to see the full, working code with comments.

### Copying Code
These examples are designed to be:
- **Copy-paste ready** - Minimal dependencies, clear structure
- **Well-commented** - Explains the "why" not just the "what"
- **Production-quality** - Follows Polaris coding standards

### Running Examples
To integrate an example into your game:
1. Copy the relevant functions
2. Adjust component types to match your game
3. Register systems during initialization
4. Test and iterate

---

## Example Template

When adding new examples, follow this structure:

```cpp
// examples/your_example.cpp
// Brief description of what this demonstrates

#include "polaris/core/Runtime.h"
// ... other includes

namespace YourNamespace {

// ============================================================================
// Section Header
// ============================================================================

/* 
 * Explanation of the concept or pattern
 */

void example_system(flecs::iter& it, Component* comp) {
    // Implementation with inline comments
}

}
```

---

## Contributing Examples

Good examples should:
- Demonstrate one concept clearly
- Include explanatory comments
- Show both good and bad patterns (when relevant)
- Be self-contained (minimal external dependencies)
- Follow Polaris coding standards (see `4. Coding Standards.md`)
