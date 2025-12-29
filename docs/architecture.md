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
       ├─► Creates TickerNode (registers as "PolarisTicker" singleton)
       ├─► Wires callbacks between NodeWatcher → Engine
       ├─► Triggers _try_auto_bind() → collects existing scene nodes
       │
       └─► Calls system registration callback (set via polaris_set_system_callback)
```

## System Registration Callback

Polaris uses a **callback pattern** for game system registration. You set a callback function **before** initializing Polaris, and it gets called automatically when the world is ready.

### Why Use a Callback?

ECS systems are **global queries** - one system processes ALL entities matching its component signature. Systems should be registered **once** when the world is ready, not per-entity.

```
❌ Anti-pattern: Registering systems in _on_ecs_ready
┌─────────────────────────────────────────────────────────────────┐
│ PlayerPawn A spawns → registers movement systems                │
│ PlayerPawn B spawns → registers SAME systems again (duplicate!) │
│                                                                 │
│ Result: Systems registered multiple times, undefined behavior   │
└─────────────────────────────────────────────────────────────────┘

✅ Correct: Registering systems via callback
┌─────────────────────────────────────────────────────────────────┐
│ polaris_set_system_callback(Pawn::register_systems)             │
│ polaris_register_classes()                                      │
│   └── PolarisEngine::initialize()                               │
│       └── Calls Pawn::register_systems(world) automatically     │
│                                                                 │
│ PlayerPawn A spawns → adds components to entity only            │
│ PlayerPawn B spawns → adds components to entity only            │
│                                                                 │
│ Result: Systems run once per frame, process ALL matching entities│
└─────────────────────────────────────────────────────────────────┘
```

### Setting Up the Callback

```cpp
// polaris_init.h provides:
void polaris_set_system_callback(void(*fn)(flecs::world&));
void polaris_register_classes();
```

**register_types.cpp:**
```cpp
#include "polaris_init.h"
#include "system/register.h"

void initialize_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    // Set callback BEFORE initializing Polaris
    polaris_set_system_callback(Pawn::register_systems);

    // This creates PolarisEngine and calls the callback when world is ready
    polaris_register_classes();

    // Register custom node classes
    GDREGISTER_CLASS(Locomotion::PlayerPawn);
    // ...
}

void uninitialize_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    polaris_unregister_classes();
}
```

### Module System Registration Pattern

Organize system registration by module with a centralized entry point:

```
src/
├── register_types.cpp           # Module entry point
├── system/
│   └── register.cpp/.h          # Pawn::register_systems - calls all modules
├── locomotion/
│   └── system/
│       ├── register.cpp/.h      # Locomotion::register_systems
│       └── ...
├── combat/
│   └── system/
│       ├── register.cpp/.h      # Combat::register_systems
│       └── ...
```

**src/system/register.h:**
```cpp
namespace Pawn {
    void register_systems(flecs::world& world);
}
```

**src/system/register.cpp:**
```cpp
#include "locomotion/system/register.h"
// #include "combat/system/register.h"

namespace Pawn {

void register_systems(flecs::world& world) {
    Locomotion::register_systems(world);
    // Combat::register_systems(world);
}

}
```

**locomotion/system/register.cpp:**
```cpp
namespace Locomotion {

void register_systems(flecs::world& world) {
    register_input_system(world);
    register_look_system(world);
    register_friction_system(world);
    register_accelerate_system(world);
    register_air_accelerate_system(world);
    register_gravity_system(world);
    register_jump_system(world);
    register_apply_system(world);
    register_crouch_system(world);
    register_view_bob_system(world);
    register_view_roll_system(world);
    register_torso_sync_system(world);
}

}
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

Polaris integrates with Godot's frame loop through `PolarisEngine` (a Node) and custom Flecs pipelines. Systems are separated into **physics** and **process** pipelines that run only during their designated Godot callback.

> **See [pipelines.md](pipelines.md) for detailed pipeline documentation.**

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
│   run_physics_pipeline()          run_process_pipeline()       │
└──────────────┬─────────────────────────────┬───────────────────┘
               │                             │
               ▼                             ▼
┌──────────────────────────┐    ┌──────────────────────────┐
│   Physics Pipeline       │    │   Process Pipeline       │
│                          │    │                          │
│   Only runs systems      │    │   Only runs systems      │
│   registered with        │    │   registered with        │
│   get_physics_phase()    │    │   get_process_phase()    │
└──────────────────────────┘    └──────────────────────────┘
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

When using custom pipelines, you don't need to check the phase - your system only runs during its registered phase:

```cpp
// Physics system - automatically only runs during _physics_process
auto physics_phase = Polaris::System::get_physics_phase(world);
world.system("MyPhysicsSystem")
    .kind(physics_phase)
    .run([](flecs::iter& it) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* pf = world.get<Polaris::System::PhysicsFrame>();
        // Use pf->delta, pf->frame, pf->time
    });

// Process system - automatically only runs during _process
auto process_phase = Polaris::System::get_process_phase(world);
world.system("MyProcessSystem")
    .kind(process_phase)
    .run([](flecs::iter& it) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* pf = world.get<Polaris::System::ProcessFrame>();
        // Use pf->delta, pf->frame, pf->time
    });
```

## Custom Node Pattern (Recommended)

The recommended way to use Polaris is with **custom nodes** that inherit from Godot node types and add ECS components to the entity that PolarisEngine already creates.

### Why Custom Nodes?

PolarisEngine automatically creates an ECS entity for every node in the scene tree. Custom nodes use this existing entity rather than creating a second one:

```
┌─────────────────────────────────────────────────────────────────┐
│ PolarisEngine creates Entity for PlayerPawn node                │
│   └── Components: GodotNode, NodeDepth, TreeId, class tags      │
│                                                                 │
│ PlayerPawn._on_ecs_ready() adds to SAME entity                  │
│   └── Components: + PlayerInput, + PlayerMovement, + configs    │
│                                                                 │
│ Result: ONE entity per node!                                    │
└─────────────────────────────────────────────────────────────────┘
```

### Custom Node Lifecycle

```
1. Node added to scene tree
          │
          ▼
2. Godot calls node._ready()
   • Resolve direct child references
          │
          ▼
3. NodeWatcher detects node, fires callback
          │
          ▼
4. PolarisEngine::_on_node_registered()
   • Creates ECS entity for node
   • Adds GodotNode, NodeDepth, TreeId, class tags
          │
          ▼
5. If node has "_on_ecs_ready" method:
   • node.call("_on_ecs_ready")
   • Node adds its components to existing entity
   • Node registers ECS systems (once globally)
          │
          ▼
6. [Game runs, ECS systems execute each frame]
          │
          ▼
7. Node removed from scene tree
          │
          ▼
8. PolarisEngine::_on_node_unregistered()
   • If node has "_on_ecs_exit" method:
     • node.call("_on_ecs_exit")
   • Destroys ECS entity (cleans up all components)
```

### Implementing a Custom Node (C++)

```cpp
// PlayerPawn.h
class PlayerPawn : public CharacterBody3D {
    GDCLASS(PlayerPawn, CharacterBody3D)

    // === Inspector-exposed configs ===
    Ref<MovementConfig> m_movement_config;

    // === Direct child references ===
    Camera3D* m_camera = nullptr;

    // === ECS entity (from PolarisEngine, not created by us) ===
    flecs::entity m_entity;

protected:
    static void _bind_methods();

public:
    void _ready() override {
        // Resolve children directly (no NodePath strings)
        m_camera = Object::cast_to<Camera3D>(get_node_or_null(NodePath("Camera3D")));
    }

    void _on_ecs_ready() {
        auto* engine = Polaris::PolarisEngine::get_singleton();

        // Get the entity PolarisEngine already created for this node
        m_entity = engine->get_entity_for_node(this);

        // Add game components to existing entity
        m_entity.set<PlayerInput>({});
        m_entity.set<PlayerMovement>({});
        m_entity.set<PlayerConfig>({
            .movement = m_movement_config.ptr()
        });
        m_entity.set<PlayerRefs>({
            .camera = m_camera
        });

        // NOTE: Systems are registered via world_ready signal, not here
        // See "world_ready Signal" section above
    }

    void _on_ecs_exit() {
        // Entity cleanup handled automatically by PolarisEngine
        // Just release any game-specific resources
        m_entity = flecs::entity::null();
    }
};

// PlayerPawn.cpp
void PlayerPawn::_bind_methods() {
    // Bind ECS lifecycle methods
    ClassDB::bind_method(D_METHOD("_on_ecs_ready"), &PlayerPawn::_on_ecs_ready);
    ClassDB::bind_method(D_METHOD("_on_ecs_exit"), &PlayerPawn::_on_ecs_exit);

    // Bind config properties
    ClassDB::bind_method(D_METHOD("get_movement_config"), &PlayerPawn::get_movement_config);
    ClassDB::bind_method(D_METHOD("set_movement_config", "config"), &PlayerPawn::set_movement_config);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "movement_config",
        PROPERTY_HINT_RESOURCE_TYPE, "MovementConfig"), "set_movement_config", "get_movement_config");
}
```

### Accessing the Main Node in Systems

Since PolarisEngine adds `GodotNode` component to every entity, systems can access the node directly:

```cpp
// The entity already has GodotNode from PolarisEngine
world.system<Polaris::Component::GodotNode, PlayerMovement>("Player_Apply")
    .kind(physics_phase)
    .each([](Polaris::Component::GodotNode& gn, PlayerMovement& movement) {
        // Get the node directly from GodotNode
        CharacterBody3D* body = gn.get_as<CharacterBody3D>();
        if (!body) return;

        body->set_velocity(movement.velocity);
        body->move_and_slide();
    });
```

### Component Organization

Split node references from config pointers for clarity:

```cpp
// Config pointers (for systems to read settings)
struct PlayerConfig {
    MovementConfig* movement = nullptr;
    CrouchConfig* crouch = nullptr;
};

// Child node references (not the entity's own node - that's in GodotNode)
struct PlayerRefs {
    Camera3D* camera = nullptr;
    Node3D* weapon_mount = nullptr;
};
```

---

## Context Pattern (Legacy)

> **Note:** The Context pattern creates a **separate entity** from the one PolarisEngine creates. For new code, prefer the [Custom Node Pattern](#custom-node-pattern-recommended) above which uses the existing entity.

Godot's Context system allows attaching behavior resources to nodes. Polaris extends this with ECS lifecycle callbacks.

### Context Lifecycle Flow

```
1. Node added to scene tree
          │
          ▼
2. NodeWatcher detects node, fires callback
          │
          ▼
3. PolarisEngine::_on_node_registered()
   • Creates ECS entity for node (Entity A)
   • Checks if node has a Context
          │
          ▼
4. If context has "_on_ecs_ready" method:
   • context.call("_on_ecs_ready", node)
   • Context creates its own entity (Entity B) ← REDUNDANT
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
     • Context must destroy Entity B manually
   • Destroys Entity A
```

### Implementing a Context (C++)

```cpp
// MyContext.h
#include "system/FrameTicker.h"

class MyContext : public Context {
    GDCLASS(MyContext, Context)

private:
    flecs::entity m_my_entity;  // Separate entity (redundant with PolarisEngine's)
    flecs::entity m_my_system;

protected:
    static void _bind_methods();

public:
    void _on_ecs_ready(Node* owner);
    void _on_ecs_exit(Node* owner);
};

// MyContext.cpp
void MyContext::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_ecs_ready", "owner"), &MyContext::_on_ecs_ready);
    ClassDB::bind_method(D_METHOD("_on_ecs_exit", "owner"), &MyContext::_on_ecs_exit);
}

void MyContext::_on_ecs_ready(Node* owner) {
    auto* engine = Polaris::PolarisEngine::get_singleton();
    auto& world = engine->get_world();

    // Creates a SECOND entity (PolarisEngine already made one)
    m_my_entity = world.entity();
    m_my_entity.set<MyComponent>({});

    auto physics_phase = Polaris::System::get_physics_phase(world);
    m_my_system = world.system("MySystem")
        .kind(physics_phase)
        .run([](flecs::iter& it) { /* ... */ });
}

void MyContext::_on_ecs_exit(Node* owner) {
    // Must manually clean up the separate entity
    if (m_my_entity.is_valid()) {
        m_my_entity.destruct();
    }
    if (m_my_system.is_valid()) {
        m_my_system.destruct();
    }
}
```

### Implementing a Context (GDScript)

```gdscript
extends Context
class_name MyContext

func _on_ecs_ready(owner: Node) -> void:
    # Called when ECS entity exists for this node
    print("ECS ready for: ", owner.name)

func _on_ecs_exit(owner: Node) -> void:
    # Called before ECS entity is destroyed
    print("ECS exit for: ", owner.name)
```

## Registering ECS Systems

### Task System (No Entity Matching)

For systems that run once per frame without entity queries:

```cpp
auto physics_phase = Polaris::System::get_physics_phase(world);

world.system("MyPhysicsTask")
    .kind(physics_phase)
    .run([](flecs::iter& it) {
        // Runs once per _physics_process call
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* pf = world.get<Polaris::System::PhysicsFrame>();
        // Use pf->delta for physics calculations
    });
```

### Entity Query System

For systems that iterate over entities with specific components:

```cpp
auto physics_phase = Polaris::System::get_physics_phase(world);

world.system<Component::GodotNode>("MoveSystem")
    .with(world.entity("CharacterBody3D"))  // Filter by class tag
    .kind(physics_phase)
    .each([](flecs::entity e, Component::GodotNode& gn) {
        if (auto* body = gn.get_as<CharacterBody3D>()) {
            // Process each CharacterBody3D
        }
    });
```

### Physics vs Process Systems

Use custom pipelines to ensure systems run only during their intended phase:

```cpp
#include "system/FrameTicker.h"

// Physics system - runs during _physics_process only
auto physics_phase = Polaris::System::get_physics_phase(world);
world.system<Velocity, Position>("Movement")
    .kind(physics_phase)
    .each([](Velocity& v, Position& p) {
        auto& w = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* pf = w.get<Polaris::System::PhysicsFrame>();
        p.value += v.value * pf->delta;
    });

// Process system - runs during _process only
auto process_phase = Polaris::System::get_process_phase(world);
world.system<Transform>("Interpolate")
    .kind(process_phase)
    .each([](Transform& t) {
        auto& w = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* pf = w.get<Polaris::System::ProcessFrame>();
        // Visual interpolation at render framerate
    });
```

> **See [pipelines.md](pipelines.md) for more details on the pipeline system.**

## Complete Example

### Scene Structure

```
GameLevel (PolarisEngine)
├── Player (PlayerPawn)            <- Custom node type
│   ├── Camera3D
│   └── CollisionShape3D
├── Ground (StaticBody3D)
└── HUD (CanvasLayer)
```

### PlayerPawn.h/.cpp (Custom Node)

```cpp
// PlayerPawn.h
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include "Engine.h"
#include "system/FrameTicker.h"

// ECS Components
struct PlayerMovement {
    Vector3 velocity;
};

class PlayerPawn : public CharacterBody3D {
    GDCLASS(PlayerPawn, CharacterBody3D)

private:
    Camera3D* m_camera = nullptr;
    flecs::entity m_entity;

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("_on_ecs_ready"), &PlayerPawn::_on_ecs_ready);
        ClassDB::bind_method(D_METHOD("_on_ecs_exit"), &PlayerPawn::_on_ecs_exit);
    }

public:
    void _ready() override {
        m_camera = Object::cast_to<Camera3D>(get_node_or_null(NodePath("Camera3D")));
    }

    void _on_ecs_ready() {
        auto* engine = Polaris::PolarisEngine::get_singleton();

        // Use the entity PolarisEngine already created
        m_entity = engine->get_entity_for_node(this);

        // Add game components to existing entity
        m_entity.set<PlayerMovement>({});

        // Systems are registered via world_ready signal (see register_systems.cpp)
    }

    void _on_ecs_exit() {
        // Entity cleanup handled by PolarisEngine
        m_entity = flecs::entity::null();
    }
};

// register_systems.cpp - Systems registered via world_ready signal
void register_player_systems(flecs::world& world) {
    auto physics_phase = Polaris::System::get_physics_phase(world);

    world.system<Polaris::Component::GodotNode, PlayerMovement>("Player_Movement")
        .kind(physics_phase)
        .each([](Polaris::Component::GodotNode& gn, PlayerMovement& movement) {
            auto* body = gn.get_as<CharacterBody3D>();
            if (!body) return;

            auto& w = Polaris::PolarisEngine::get_singleton()->get_world();
            const auto* pf = w.get<Polaris::System::PhysicsFrame>();

            // Simple gravity
            movement.velocity.y -= 9.8 * pf->delta;
            body->set_velocity(movement.velocity);
            body->move_and_slide();
            movement.velocity = body->get_velocity();
        });
}
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

## Prefab Composition Pattern

The recommended approach for creating reusable game entities (pawns, vehicles, etc.) is **composition via child nodes**. Each feature is a separate node type that adds its own ECS components.

### Core Principle: Systems are Global, Components Control Behavior

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        ECS COMPOSITION MODEL                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   Systems are global queries registered ONCE at startup.                │
│   Entities "opt-in" to systems by having the required components.       │
│                                                                         │
│   ┌─────────────────┐          ┌─────────────────┐                     │
│   │   PlayerPawn    │          │     AIPawn      │                     │
│   ├─────────────────┤          ├─────────────────┤                     │
│   │ ✓ PawnMovement  │──┐    ┌──│ ✓ PawnMovement  │                     │
│   │ ✓ PawnRefs      │  │    │  │ ✓ PawnRefs      │                     │
│   │ ✓ PlayerInput   │  │    │  │ ✗ PlayerInput   │  ← No keyboard     │
│   │ ✗ AIInput       │  │    │  │ ✓ AIInput       │  ← NavAgent feeds  │
│   │ ✓ LocalViewRefs │  │    │  │ ✗ LocalViewRefs │                     │
│   │ ✓ WorldViewRefs │  │    │  │ ✓ WorldViewRefs │                     │
│   └─────────────────┘  │    │  └─────────────────┘                     │
│                        │    │                                           │
│                        ▼    ▼                                           │
│              ┌─────────────────────────┐                               │
│              │  movement_system        │  ← Processes BOTH             │
│              │  (queries PawnMovement) │                               │
│              └─────────────────────────┘                               │
│                                                                         │
│   ┌─────────────────────────┐    ┌─────────────────────────┐           │
│   │ player_input_system     │    │ ai_input_system         │           │
│   │ (queries PlayerInput)   │    │ (queries AIInput)       │           │
│   │ ← Only PlayerPawn       │    │ ← Only AIPawn           │           │
│   └─────────────────────────┘    └─────────────────────────┘           │
└─────────────────────────────────────────────────────────────────────────┘
```

### Prefab Structure Example

```
prefabs/
├── player_pawn.tscn
│   └── PlayerPawn (custom CharacterBody3D)  ← Handles _input for mouse look
│       ├── CollisionShape3D
│       ├── Camera3D (current=true)          ← Active camera
│       ├── TorsoMount
│       │   └── LocalViewNode                ← First-person viewmodel
│       ├── WorldViewNode                    ← Third-person model (for spectating/network)
│       │   └── player_model_path = "urban.mdl"
│       └── InventoryNode
│
└── ai_pawn.tscn
    └── CharacterBody3D                      ← No custom type needed (no _input)
        ├── CollisionShape3D
        ├── Camera3D (current=false)         ← For spectating AI POV
        ├── WorldViewNode                    ← Always visible (third-person)
        │   └── player_model_path = "urban.mdl"
        ├── InventoryNode
        └── NavigationAgent3D                ← AI pathfinding

```

### Feature Nodes

Each feature is a self-contained node type:

| Node Type | Components Added | Purpose |
|-----------|-----------------|---------|
| `PlayerPawn` | PawnInput, PawnMovement, PawnRefs | Player locomotion + input handling |
| `LocalViewNode` | LocalViewRefs, LocalViewState | First-person viewmodel rendering |
| `WorldViewNode` | WorldViewRefs, WorldViewState | Third-person player/weapon models |
| `InventoryNode` | WeaponInventory | Weapon slots and switching |

### ViewMode Component

The `ViewMode` component on the pawn entity controls which view is active:

```cpp
struct ViewMode {
    enum Mode {
        FirstPerson = 0,  // Show LocalView, hide WorldView player model
        ThirdPerson = 1   // Show WorldView, hide LocalView
    };
    Mode current_mode;
};
```

- `LocalViewNode` sets default to `FirstPerson` (for PlayerPawn)
- `WorldViewNode` sets default to `ThirdPerson` (for AIPawn)
- Systems check `ViewMode` to control visibility

### Adding New Pawn Types

To create a new pawn type (e.g., vehicle, turret):

1. **Decide which features it needs** - Look at existing node types
2. **Compose the prefab** - Add only the feature nodes required
3. **No system changes needed** - Systems already query by component

Example: Spectator pawn (camera only, no model)
```
spectator.tscn
└── CharacterBody3D
    └── Camera3D (current=true when spectating)
    # No LocalView, WorldView, or Inventory needed
```

### Different Configs for Same Systems

AI and players can share movement systems with different configs:

```
# player_movement.tres
forward_speed = 10.16
autohop = true
bunnyhop_cap_mode = 2

# ai_movement.tres
forward_speed = 5.0
autohop = false
bunnyhop_cap_mode = 0
```

For AI-specific input handling, add a separate component:

```cpp
// AIInput component - written to by NavigationAgent
struct AIInput {
    Vector3 target_position;
    bool should_attack = false;
};

// AI input system converts NavAgent output to PawnInput
// Movement systems still read PawnInput - works for both!
```
