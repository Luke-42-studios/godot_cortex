# Polaris Node Component System Architecture

## Overview

Polaris is an Entity Component System (ECS) framework built on [Flecs](https://github.com/SanderMertens/flecs) that bridges Godot's scene tree with high-performance ECS patterns. It automatically tracks all nodes in the scene tree as ECS entities, enabling data-oriented queries and systems while maintaining full compatibility with Godot's node-based workflow.

```
┌─────────────────────────────────────────────────────────────────┐
│                         GDScript                                │
│                  Polaris / ECSWorld / NodeWatcher               │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Polaris::Engine                            │
│                    (Main Coordinator)                           │
│  ┌─────────────────┐              ┌──────────────────────────┐  │
│  │  Node* → Entity │              │  Callback Wiring         │  │
│  │     HashMap     │              │  Scene Tree ↔ ECS        │  │
│  └─────────────────┘              └──────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
          │                                       │
          ▼                                       ▼
┌──────────────────────┐              ┌──────────────────────────┐
│ Context::ECSWorld    │              │ System::NodeWatcher      │
│                      │              │                          │
│ • Flecs world owner  │              │ • Scene tree signals     │
│ • Component registry │              │ • Computes depth/tree_id │
│ • Pre-built queries  │              │ • Fires add/remove CBs   │
│ • Safe node access   │              │ • Thin event emitter     │
└──────────────────────┘              └──────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Flecs World                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │   Entity    │  │   Entity    │  │   Entity    │  ...         │
│  │ "Player"    │  │ "Enemy"     │  │ "Camera3D"  │              │
│  │             │  │             │  │             │              │
│  │ • GodotNode │  │ • GodotNode │  │ • GodotNode │              │
│  │ • NodeDepth │  │ • NodeDepth │  │ • NodeDepth │              │
│  │ • TreeId    │  │ • TreeId    │  │ • TreeId    │              │
│  │ • Active    │  │ • Active    │  │ • Active    │              │
│  │ • Node3D    │  │ • Node3D    │  │ • Node3D    │              │
│  │ • Player    │  │ • Enemy     │  │ • Camera3D  │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

## Namespace Convention

```
Polaris::Component::*   // Pure data attached to entities
Polaris::Tag::*         // Zero-size markers for filtering
Polaris::System::*      // Logic that processes entities
Polaris::Context::*     // State containers / world wrappers
Polaris::Engine         // Main coordinator
Polaris::Log            // Debug utilities
```

## Directory Structure

```
src/
├── Engine.h / .cpp           # Main coordinator singleton
├── Log.h                     # Debug logging utilities
├── polaris_init.h            # Module registration
├── component/
│   └── GodotNode.h           # Core components (GodotNode, NodeDepth, TreeId)
├── tag/
│   └── Tags.h                # Tag markers (Active)
├── context/
│   └── ECSWorld.h / .cpp     # Flecs world manager
├── system/
│   └── NodeWatcher.h / .cpp  # Scene tree event emitter
└── node/
    └── CNode.h / .cpp        # Custom node types
```

## Core Components

### Component::GodotNode

Hybrid safe reference to a Godot Node using ObjectID + cached pointer.

```cpp
struct GodotNode {
    ObjectID id;              // 8 bytes - Godot's safe object identifier
    mutable Node* cached_ptr; // 8 bytes - Cached pointer for fast access

    Node* get();              // Safe path (validates via ObjectDB)
    Node* get_unchecked();    // Fast path (uses cache, may dangle)
    bool is_valid();          // Check if node still exists

    template<typename T>
    T* get_as();              // Type-safe cast (e.g., get_as<Camera3D>())
};
```

**Why hybrid?**
- `ObjectID` provides safety: Godot invalidates IDs when nodes are freed
- Cached pointer provides speed: Avoids ObjectDB lookup on hot paths
- Cache is validated lazily and updated when stale

### Component::NodeDepth

```cpp
struct NodeDepth {
    uint16_t value = 0;  // Distance from scene root
};
```

### Component::TreeId

```cpp
struct TreeId {
    uint32_t value = 0;  // Identifier for multi-tree support
};
```

### Tag::Active

Zero-size marker indicating the entity's node is in the scene tree.

```cpp
struct Active {};
```

## Class Hierarchy Tags

When a node is registered, its entire class inheritance chain is added as ECS tags:

```
MeshInstance3D node gets tags:
  • MeshInstance3D
  • GeometryInstance3D
  • VisualInstance3D
  • Node3D
  • Node
```

This enables powerful queries like:
```cpp
// Find all 3D nodes
world.query<Component::GodotNode>()
    .with(world.entity("Node3D"))
    .each([](flecs::entity e, Component::GodotNode& gn) { ... });
```

## Data Flow

### Node Added to Scene Tree

```
1. SceneTree emits "node_added" signal
          │
          ▼
2. NodeWatcher::_on_node_added(node)
   • Computes depth from root
   • Computes tree_id
          │
          ▼
3. Callback fires → Engine::_on_node_registered(node, depth, tree_id)
          │
          ▼
4. Engine::_create_entity_for_node()
   • Creates Flecs entity with node name
   • Adds GodotNode component (stores ObjectID + pointer)
   • Adds NodeDepth component
   • Adds TreeId component
   • Adds Active tag
   • Adds class hierarchy tags (Node3D, Camera3D, etc.)
          │
          ▼
5. Entity stored in Node* → Entity map for O(1) lookup
```

### Node Removed from Scene Tree

```
1. SceneTree emits "node_removed" signal
          │
          ▼
2. NodeWatcher::_on_node_removed(node)
          │
          ▼
3. Callback fires → Engine::_on_node_unregistered(node)
          │
          ▼
4. Engine::_destroy_entity_for_node()
   • Looks up entity from Node* map
   • Calls entity.destruct()
   • Removes from map
```

## Singleton Access

### From GDScript

```gdscript
# Main coordinator
Polaris.debug_enabled = true
Polaris.get_entity_count()

# ECS World
ECSWorld.debug_enabled = true
ECSWorld.print_state()
ECSWorld.get_entity_count()

# Node Watcher
NodeWatcher.debug_enabled = true
```

### From C++

```cpp
// Static singleton access (fast, no string lookup)
auto* engine = Polaris::Engine::get_singleton();
auto* ecs = Polaris::Context::ECSWorld::get_singleton();
auto* watcher = Polaris::System::NodeWatcher::get_singleton();

// Direct world access
flecs::world& world = engine->get_world();
```

## Query Patterns

### Iterate All Valid Nodes

```cpp
ecs->each_valid_node([](flecs::entity e, Node* node) {
    // Only called for nodes that still exist
});
```

### Query by Depth

```cpp
ecs->each_at_depth(0, [](flecs::entity e, Node* node) {
    // Root-level nodes only
});
```

### Query by Class

```cpp
ecs->each_with_class("Camera3D", [](flecs::entity e, Node* node) {
    auto* camera = Object::cast_to<Camera3D>(node);
});
```

### Custom Flecs Queries

```cpp
auto& world = engine->get_world();

// All active Node3D entities
world.query_builder<Component::GodotNode>()
    .with<Tag::Active>()
    .with(world.entity("Node3D"))
    .build()
    .each([](flecs::entity e, Component::GodotNode& gn) {
        if (Node3D* n = gn.get_as<Node3D>()) {
            // Process...
        }
    });
```

## Debug Logging

Per-class debug control using `Polaris::Log`:

```cpp
// In your class
bool m_debug_enabled = false;

// Conditional logging
Log::print(m_debug_enabled, "[MySystem] Processing ", count, " entities");
Log::warn(m_debug_enabled, "[MySystem] Cache miss");

// Always print (lifecycle messages)
Log::info("[MySystem] Initialized");

// Always print (errors)
Log::error("[MySystem] Fatal error: ", message);
```

## Thread Safety

Singleton creation uses `std::call_once` for thread-safe initialization:

```cpp
Engine* Engine::create_global_instance() {
    static std::once_flag init_once;
    std::call_once(init_once, []() {
        singleton_instance = memnew(Engine);
        // ... registration
    });
    return singleton_instance;
}
```

## Initialization Order

```
1. polaris_register_classes()
   │
   ├─► GDREGISTER_CLASS(Polaris::Context::ECSWorld)
   ├─► GDREGISTER_CLASS(Polaris::System::NodeWatcher)
   ├─► GDREGISTER_CLASS(Polaris::Engine)
   │
   └─► Polaris::Engine::create_global_instance()
       │
       ├─► Creates ECSWorld (registers as "ECSWorld" singleton)
       ├─► Creates NodeWatcher (registers as "NodeWatcher" singleton)
       ├─► Wires callbacks between NodeWatcher → Engine
       └─► Triggers _try_auto_bind() → collects existing scene nodes
```

## Shutdown Order

```
1. polaris_unregister_classes()
   │
   └─► Polaris::Engine::destroy_global_instance()
       │
       ├─► Clears Node* → Entity map
       ├─► Unregisters & deletes NodeWatcher
       └─► Unregisters & deletes ECSWorld
```

## Performance Considerations

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Node → Entity lookup | O(1) | HashMap in Engine |
| Entity → Node lookup | O(1) | Component access |
| Node validity check | O(1) | ObjectDB lookup |
| Find by class | O(n) | Scans all entities with class tag |
| Add/remove node | O(1) amortized | HashMap insert/remove |

## Design Decisions

### Why ObjectID + Cached Pointer?

Pure pointer storage is unsafe—Godot can free nodes at any time. Pure ObjectID is safe but slow (ObjectDB lookup every access). The hybrid approach gives:
- **Safety**: ObjectID validates node existence
- **Speed**: Cached pointer for hot paths
- **Simplicity**: Lazy validation updates cache automatically

### Why Thin NodeWatcher?

NodeWatcher only emits events and computes metadata. It doesn't store node data. This separation:
- Keeps responsibilities clear
- Allows ECS to be the single source of truth
- Makes testing easier
- Reduces memory duplication

### Why Class Hierarchy as Tags?

Adding the full inheritance chain as tags enables:
- Query all Node3D (includes MeshInstance3D, Camera3D, etc.)
- Query specific types exactly
- Combine with other component queries
- Zero runtime cost (tags are zero-size)

## Frame System

Polaris integrates with Godot's frame loop through `PolarisEngine` (a Node) and `FrameTicker`. This enables ECS systems to run every physics and process frame.

### Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Godot Frame Loop                             │
│                                                                 │
│   _physics_process(delta)          _process(delta)             │
└──────────────┬─────────────────────────────┬───────────────────┘
               │                             │
               ▼                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                      PolarisEngine (Node)                       │
│                                                                 │
│   _physics_process(delta)          _process(delta)             │
│         │                                │                      │
│         ▼                                ▼                      │
│   m_ticker->on_physics_frame()    m_ticker->on_process_frame() │
└──────────────┬─────────────────────────────┬───────────────────┘
               │                             │
               ▼                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                      FrameTicker                                │
│                                                                 │
│   1. Update PhysicsFrame/ProcessFrame singleton                 │
│   2. Set CurrentPhase singleton                                 │
│   3. Call world.progress(delta) ──► Runs all ECS systems       │
│   4. Clear CurrentPhase                                         │
└─────────────────────────────────────────────────────────────────┘
```

### Scene Setup

**PolarisEngine must be the root node of your scene** to receive frame callbacks:

```
MyScene (PolarisEngine)          <- Root: receives _physics_process/_process
├── World (Node3D)               <- Game world with Context
│   ├── Player
│   ├── Enemies
│   └── Environment
└── UI (CanvasLayer)
```

Example scene file:
```
[node name="MyScene" type="PolarisEngine"]

[node name="World" type="Node3D" parent="."]
context = SubResource("MyContext")

[node name="Player" type="CharacterBody3D" parent="World"]
```

### Frame Singletons

FrameTicker maintains three singleton components accessible to all ECS systems:

```cpp
/// Physics frame data - updated every physics tick
struct PhysicsFrame {
    double delta = 0.0;         // Time since last physics frame
    uint64_t frame = 0;         // Physics frame counter
    double time = 0.0;          // Total elapsed physics time
};

/// Process frame data - updated every render frame
struct ProcessFrame {
    double delta = 0.0;         // Time since last process frame
    uint64_t frame = 0;         // Process frame counter
    double time = 0.0;          // Total elapsed process time
};

/// Current phase indicator
enum class FramePhase : uint8_t {
    None = 0,
    Physics = 1,
    Process = 2
};

struct CurrentPhase {
    FramePhase phase = FramePhase::None;
};
```

### Accessing Frame Data in Systems

```cpp
// In your ECS system callback
auto* engine = Polaris::PolarisEngine::get_singleton();
auto& world = engine->get_world();

// Check current phase
const auto* phase = world.get<Polaris::System::CurrentPhase>();
if (phase->phase == Polaris::System::FramePhase::Physics) {
    const auto* pf = world.get<Polaris::System::PhysicsFrame>();
    // Use pf->delta, pf->frame, pf->time
}
```

## Context Lifecycle

Godot's Context system allows attaching behavior resources to nodes. Polaris extends this with ECS lifecycle callbacks.

### Lifecycle Flow

```
1. Node added to scene tree
          │
          ▼
2. NodeWatcher detects node, fires callback
          │
          ▼
3. PolarisEngine::_on_node_registered()
   • Creates ECS entity for node
   • Checks if node has a Context
          │
          ▼
4. If context has "_on_ecs_ready" method:
   • context.call("_on_ecs_ready", node)
   • Context can now register ECS systems
          │
          ▼
5. [Game runs, ECS systems execute each frame]
          │
          ▼
6. Node removed from scene tree
          │
          ▼
7. PolarisEngine::_on_node_unregistered()
   • If context has "_on_ecs_exit" method:
     • context.call("_on_ecs_exit", node)
     • Context should clean up ECS systems
   • Destroys ECS entity
```

### Implementing a Context (C++)

```cpp
// MyContext.h
class MyContext : public Context {
    GDCLASS(MyContext, Context)

private:
    flecs::entity m_my_system;

protected:
    static void _bind_methods();

public:
    // Standard Godot Context callbacks (no ECS available yet)
    virtual void _on_context_ready(Node* owner) override;
    virtual void _on_context_exit(Node* owner) override;

    // ECS lifecycle callbacks (ECS is ready)
    void _on_ecs_ready(Node* owner);
    void _on_ecs_exit(Node* owner);
};

// MyContext.cpp
void MyContext::_bind_methods() {
    // Must bind ECS methods for Polaris to call them
    ClassDB::bind_method(D_METHOD("_on_ecs_ready", "owner"), &MyContext::_on_ecs_ready);
    ClassDB::bind_method(D_METHOD("_on_ecs_exit", "owner"), &MyContext::_on_ecs_exit);
}

void MyContext::_on_ecs_ready(Node* owner) {
    auto* engine = Polaris::PolarisEngine::get_singleton();
    auto& world = engine->get_world();

    // Register an ECS system
    m_my_system = world.system("MySystem")
        .kind(flecs::OnUpdate)
        .run([](flecs::iter& it) {
            // This runs every frame during world.progress()
        });
}

void MyContext::_on_ecs_exit(Node* owner) {
    // Clean up ECS system
    if (m_my_system.is_valid()) {
        m_my_system.destruct();
        m_my_system = flecs::entity::null();
    }
}
```

### Implementing a Context (GDScript)

```gdscript
extends Context
class_name MyContext

var my_system_id: int = 0

func _on_context_ready(owner: Node) -> void:
    # Called when context is attached, but ECS may not be ready
    print("Context ready on: ", owner.name)

func _on_ecs_ready(owner: Node) -> void:
    # Called when ECS entity exists for this node
    # Register systems here
    print("ECS ready for: ", owner.name)

func _on_ecs_exit(owner: Node) -> void:
    # Called before ECS entity is destroyed
    # Clean up systems here
    print("ECS exit for: ", owner.name)
```

## Registering ECS Systems

### Task System (No Entity Matching)

For systems that run once per frame without entity queries:

```cpp
world.system("MyFrameTask")
    .kind(flecs::OnUpdate)
    .run([](flecs::iter& it) {
        // Runs once per world.progress() call
        auto* engine = Polaris::PolarisEngine::get_singleton();
        const auto* phase = engine->get_world().get<Polaris::System::CurrentPhase>();

        if (phase->phase == Polaris::System::FramePhase::Physics) {
            // Handle physics frame
        }
    });
```

### Entity Query System

For systems that iterate over entities with specific components:

```cpp
world.system<Component::GodotNode>("MoveSystem")
    .with(world.entity("CharacterBody3D"))  // Filter by class tag
    .kind(flecs::OnUpdate)
    .each([](flecs::entity e, Component::GodotNode& gn) {
        if (auto* body = gn.get_as<CharacterBody3D>()) {
            // Process each CharacterBody3D
        }
    });
```

### Physics vs Process Systems

Both physics and process frames call `world.progress()`. To differentiate, check `CurrentPhase`:

```cpp
world.system("PhysicsOnlySystem")
    .kind(flecs::OnUpdate)
    .run([](flecs::iter& it) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* phase = world.get<Polaris::System::CurrentPhase>();

        // Only run during physics frames
        if (phase->phase != Polaris::System::FramePhase::Physics) {
            return;
        }

        const auto* pf = world.get<Polaris::System::PhysicsFrame>();
        // Use pf->delta for physics calculations
    });
```

## Complete Example

### Scene Structure

```
GameLevel (PolarisEngine)
├── World (Node3D)                 [context = PlayerContext]
│   ├── Player (CharacterBody3D)
│   └── Ground (StaticBody3D)
└── HUD (CanvasLayer)
```

### PlayerContext.cpp

```cpp
#include <godot_cpp/classes/context.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include "Engine.h"
#include "system/FrameTicker.h"

class PlayerContext : public Context {
    GDCLASS(PlayerContext, Context)

private:
    flecs::entity m_movement_system;

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("_on_ecs_ready", "owner"),
                            &PlayerContext::_on_ecs_ready);
        ClassDB::bind_method(D_METHOD("_on_ecs_exit", "owner"),
                            &PlayerContext::_on_ecs_exit);
    }

public:
    void _on_ecs_ready(Node* owner) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();

        m_movement_system = world.system<Polaris::Component::GodotNode>("PlayerMovement")
            .with(world.entity("CharacterBody3D"))
            .kind(flecs::OnUpdate)
            .each([](flecs::entity e, Polaris::Component::GodotNode& gn) {
                auto& w = Polaris::PolarisEngine::get_singleton()->get_world();
                const auto* phase = w.get<Polaris::System::CurrentPhase>();

                // Only process during physics frames
                if (phase->phase != Polaris::System::FramePhase::Physics) {
                    return;
                }

                if (auto* body = gn.get_as<CharacterBody3D>()) {
                    const auto* pf = w.get<Polaris::System::PhysicsFrame>();

                    // Simple movement example
                    Vector3 velocity = body->get_velocity();
                    velocity.y -= 9.8 * pf->delta;  // Gravity
                    body->set_velocity(velocity);
                    body->move_and_slide();
                }
            });
    }

    void _on_ecs_exit(Node* owner) {
        if (m_movement_system.is_valid()) {
            m_movement_system.destruct();
            m_movement_system = flecs::entity::null();
        }
    }
};
```

## Debug Control

Enable frame system debugging via GDScript:

```gdscript
# polaris_debug.gd
extends Node

@export var polaris_debug: bool = false:
    set(value):
        polaris_debug = value
        if Engine.has_singleton("Polaris"):
            Engine.get_singleton().set_debug_enabled(value)

@export var frame_ticker_debug: bool = false:
    set(value):
        frame_ticker_debug = value
        if Engine.has_singleton("FrameTicker"):
            Engine.get_singleton("FrameTicker").set_debug_enabled(value)
```

When enabled, you'll see frame logging:
```
[PolarisEngine] Physics frame 60
[Polaris::System::FrameTicker] Physics frame 60 delta=0.016667 time=1.0
```
