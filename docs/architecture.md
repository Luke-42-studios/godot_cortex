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
