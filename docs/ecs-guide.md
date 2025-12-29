# ECS Development Guide

A practical guide for building ECS systems with Polaris and Godot.

> **Reference implementations:** All patterns described here have working examples in the `source--control` project.

## Table of Contents

1. [Naming Conventions](#naming-conventions)
2. [Folder Structure](#folder-structure)
3. [Creating Components](#creating-components)
4. [Creating Systems](#creating-systems)
5. [Creating Configs](#creating-configs)
6. [Creating Custom Nodes](#creating-custom-nodes)
7. [Accessing Nodes in Systems](#accessing-nodes-in-systems)
8. [Quick Reference](#quick-reference)

---

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Namespace | PascalCase, matches folder | `Locomotion`, `Combat` |
| Component struct | PascalCase, no suffix | `PawnInput`, `WeaponState` |
| System function | `register_[name]_system` | `register_input_system` |
| Flecs system name | `"[Namespace]_[Name]"` | `"Locomotion_Input"` |
| Config class | PascalCase + `Config` | `MovementConfig` |
| Custom node class | PascalCase, describes purpose | `PlayerPawn`, `InventoryNode` |
| Data resource class | PascalCase + `Data` | `WeaponData` |

**Folder indicates type** - no suffix needed in filenames:
- `component/PawnInput.h` (not `PawnInputComponent.h`)
- `config/MovementConfig.h`
- `system/Movement.cpp`

---

## Folder Structure

```
src/
├── system/
│   └── register.h/.cpp              # Central entry point
├── [module]/
│   ├── [CustomNode].h/.cpp          # Main custom node
│   ├── component/                   # Pure data structs
│   ├── config/                      # Godot Resources
│   ├── system/
│   │   ├── register.h/.cpp          # Module registration
│   │   ├── Systems.h                # Declarations
│   │   └── [Domain].cpp             # Grouped systems
│   └── data/                        # Static data resources
```

> 📁 **Reference:** `source--control/src/locomotion/` for complete module structure

---

## Creating Components

Components are **pure data structs** for Flecs with no Godot bindings.

### Guidelines

1. **Pure data only** - No methods, no Godot bindings
2. **Use Godot types** - `Vector2`, `Vector3`, `String`
3. **Default initialize** - All fields have sensible defaults

### Component Types

| Type | Purpose | Example |
|------|---------|---------|
| State | Runtime data | `PawnMovement`, `WeaponState` |
| Input | Per-frame input | `PawnInput`, `WeaponInput` |
| Config Pointers | References to Resources | `PawnConfig` |
| Node References | Child node pointers | `PawnRefs`, `CombatNodes` |

> 📁 **Reference:** `source--control/src/locomotion/component/` for examples

---

## Creating Systems

Systems are registered via a **callback pattern** when Polaris initializes.

> **See [architecture.md](architecture.md#system-registration-callback)** for why this pattern is used.

### Registration Flow

```
register_types.cpp
    └── polaris_set_system_callback(Pawn::register_systems)
            └── src/system/register.cpp
                    ├── Locomotion::register_systems(world)
                    └── Combat::register_systems(world)
```

### Key Pattern

```cpp
// In [module]/system/register.cpp
void register_systems(flecs::world& world) {
    register_input_system(world);
    register_movement_system(world);
    // ...
}
```

### System Template

```cpp
flecs::entity register_example_system(flecs::world& world) {
    auto physics_phase = Polaris::System::get_physics_phase(world);

    return world.system<ComponentA, const ComponentB>("Namespace_Name")
        .kind(physics_phase)
        .each([](ComponentA& a, const ComponentB& b) {
            // Process entities...
        });
}
```

> 📁 **Reference:**
> - `source--control/src/system/register.cpp` - Central entry point
> - `source--control/src/locomotion/system/register.cpp` - Module registration
> - `source--control/src/locomotion/system/Movement.cpp` - System implementations

---

## Creating Configs

Configs are **Godot Resources** exposed in the inspector.

### Key Pattern

```cpp
class MyConfig : public godot::Resource {
    GDCLASS(MyConfig, godot::Resource)

    float m_value = 1.0f;

protected:
    static void _bind_methods();  // ADD_PROPERTY bindings

public:
    float get_value() const { return m_value; }
    void set_value(float v) { m_value = v; }
};
```

### _bind_methods Pattern

```cpp
void MyConfig::_bind_methods() {
    ADD_GROUP("Group Name", "");

    ClassDB::bind_method(D_METHOD("get_value"), &MyConfig::get_value);
    ClassDB::bind_method(D_METHOD("set_value", "value"), &MyConfig::set_value);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value", PROPERTY_HINT_RANGE, "0,10,0.1"),
                 "set_value", "get_value");
}
```

> 📁 **Reference:** `source--control/src/locomotion/config/MovementConfig.h`

---

## Creating Custom Nodes

Custom nodes inherit from Godot nodes and add ECS components to existing entities.

### Key Principles

1. **Use existing entity** - `engine->get_entity_for_node(this)`, don't create new
2. **Add components in `_on_ecs_ready()`** - Called by PolarisEngine when entity exists
3. **No system registration** - Systems registered via callback, not in nodes
4. **Automatic cleanup** - PolarisEngine destroys entity when node removed

### Lifecycle

```
_ready()           → Resolve child node references
_on_ecs_ready()    → Get entity, add components
[game runs]        → Systems process entity
_on_ecs_exit()     → Clear entity reference (cleanup automatic)
```

### Key Pattern

```cpp
void MyNode::_on_ecs_ready() {
    auto* engine = Polaris::PolarisEngine::get_singleton();

    // Get entity PolarisEngine already created
    m_entity = engine->get_entity_for_node(this);

    // Add components to existing entity
    m_entity.set<MyComponent>({});
    m_entity.set<MyConfig>({ .ptr = m_config.ptr() });
}
```

### Required Bindings

```cpp
ClassDB::bind_method(D_METHOD("_on_ecs_ready"), &MyNode::_on_ecs_ready);
ClassDB::bind_method(D_METHOD("_on_ecs_exit"), &MyNode::_on_ecs_exit);
```

> 📁 **Reference:**
> - `source--control/src/locomotion/PlayerPawn.cpp` - CharacterBody3D custom node
> - `source--control/src/inventory/InventoryNode.cpp` - Node custom node

---

## Accessing Nodes in Systems

### Entity's Own Node (GodotNode)

PolarisEngine adds `GodotNode` to every entity:

```cpp
world.system<Polaris::Component::GodotNode, MyComponent>("MySystem")
    .each([](Polaris::Component::GodotNode& gn, MyComponent& comp) {
        auto* body = gn.get_as<CharacterBody3D>();
        if (!body) return;
        // Use body...
    });
```

### Child Nodes (Refs Component)

Store child node pointers in a refs component:

```cpp
world.system<MyRefs, const MyState>("MySystem")
    .each([](MyRefs& refs, const MyState& state) {
        if (!refs.camera) return;
        refs.camera->set_rotation(...);
    });
```

### Class Hierarchy Tags

Query by Godot class type:

```cpp
world.query<Polaris::Component::GodotNode>()
    .with(world.entity("CharacterBody3D"))
    .each([](flecs::entity e, Polaris::Component::GodotNode& gn) {
        // All CharacterBody3D entities
    });
```

---

## Quick Reference

### File Creation Checklist

| Type | Files | Register in |
|------|-------|-------------|
| Component | `[module]/component/[Name].h` | - |
| System | `[module]/system/[Domain].cpp` | `[module]/system/register.cpp` |
| Config | `[module]/config/[Name]Config.h/.cpp` | `register_types.cpp` |
| Custom Node | `[module]/[Name].h/.cpp` | `register_types.cpp` |
| Data Resource | `[module]/data/[Name]Data.h/.cpp` | `register_types.cpp` |

### Common Patterns

```cpp
// Get delta time
float delta = Util::get_physics_delta();  // or get_process_delta()

// Get entity's node as specific type
auto* body = gn.get_as<CharacterBody3D>();

// Get config values
float speed = config.movement->get_max_speed();

// Register on physics phase
auto phase = Polaris::System::get_physics_phase(world);
world.system("Name").kind(phase).each(...);

// Register on process phase
auto phase = Polaris::System::get_process_phase(world);
world.system("Name").kind(phase).each(...);
```

### Reference Files

| Pattern | Example File |
|---------|--------------|
| Component | `locomotion/component/PawnInput.h` |
| System | `locomotion/system/Movement.cpp` |
| Config | `locomotion/config/MovementConfig.h` |
| Custom Node | `locomotion/PlayerPawn.cpp` |
| Data Resource | `combat/weapon/data/WeaponData.h` |
| System Registration | `system/register.cpp` |
