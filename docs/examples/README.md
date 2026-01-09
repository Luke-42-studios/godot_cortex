# Examples Index

Complete, runnable code examples referenced by the documentation.

---

## Domain Modules ⭐ **START HERE**

The recommended way to organize systems. Each domain is self-contained.

| File | Pattern | Description |
|------|---------|-------------|
| [domains/game_init.cpp](domains/game_init.cpp) | Entry Point | Wire all domains together |
| [domains/movement.cpp](domains/movement.cpp) | Sub-Phases | Movement with Input → Apply → Resolve |
| [domains/combat.cpp](domains/combat.cpp) | Simple | No sub-phases, registration order |
| [domains/ai.cpp](domains/ai.cpp) | Sub-Phases | AI with Sense → Decide → Act |
| [domains/animation.cpp](domains/animation.cpp) | Simple | Visual updates in Phase_Process |
| [domains/input.cpp](domains/input.cpp) | Simple | Input capture in Phase_Input |

**Referenced by:** `4. Systems.md`

---

## Compositions

| File | Description |
|------|-------------|
| [pawn_composition.cpp](pawn_composition.cpp) | Complete Pawn hierarchy (Pawn, PlayerPawn, AIPawn) |

Shows Composition pattern:
- Base class with Health, Velocity, MoveSpeed
- Godot integration (inspector properties, metadata)
- compose() and decompose() lifecycle

**Referenced by:** `1.0 Composition.md`, `2.0 Entity.md`

---

## Memory Management

| File | Description |
|------|-------------|
| [arena_usage.cpp](arena_usage.cpp) | Arena patterns (pathfinding, strings, collision lists) |
| [pawn_arena_usage.cpp](pawn_arena_usage.cpp) | Arena usage in Pawn systems |

Shows why arenas are 10-40x faster than malloc:
- Phase arenas for temporary allocations
- Permanent arena for lookup tables
- Bad vs good allocation patterns

**Referenced by:** `6. Memory Management.md`, `3. Lifecycle.md`

---

## How to Use

### Reading the Docs
When you see:
```
See: examples/domains/movement.cpp
```
Open that file for complete, working code with comments.

### Copying Code
Examples are:
- **Copy-paste ready** — Minimal dependencies
- **Well-commented** — Explains the "why"
- **Production-quality** — Follows Polaris coding standards

---

## Adding Examples

Follow this structure:

```cpp
// examples/your_example.cpp
// Brief description

#include "polaris/core/Runtime.h"

namespace YourDomain {

// ============================================================================
// Section Header
// ============================================================================

static void example_system(flecs::iter& it, Component* comp) {
    // Implementation
}

void init(Runtime* rt) {
    // Registration
}

}
```
