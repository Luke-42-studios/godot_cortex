# ECS Development Guide

A practical guide for building ECS systems with Polaris and Godot.

> **Reference implementations:** All patterns described here have working examples in the `source--control` project.

## Table of Contents

1. [Core Pattern: Input → Component → System](#core-pattern-input--component--system)
2. [Naming Conventions](#naming-conventions)
3. [Folder Structure](#folder-structure)
4. [Creating Components](#creating-components)
5. [Creating Systems](#creating-systems)
6. [Creating Configs](#creating-configs)
7. [Creating Custom Nodes](#creating-custom-nodes)
8. [Entity Relationships](#entity-relationships)
9. [Input Handling](#input-handling)
10. [Accessing Nodes in Systems](#accessing-nodes-in-systems)
11. [Quick Reference](#quick-reference)

---

## Core Pattern: Input → Component → System

**The fundamental pattern:** Godot nodes write to ECS components, ECS systems read from them.

```
┌─────────────────────────────────────────────────────────────────┐
│                    GODOT NODES                                  │
│                 (Write to Components)                           │
│                                                                 │
│   PlayerPawn._input()      →  WeaponInput.switch_to_slot        │
│   PlayerPawn._process()    →  PawnInput.move_input              │
│   AIPawn.set_move_input()  →  PawnInput.move_input              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ECS COMPONENTS                               │
│                   (Pure Data Storage)                           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ECS SYSTEMS                                  │
│               (Read Components, Apply Logic)                    │
│                                                                 │
│   Movement system      ←  reads PawnInput.move_input            │
│   WeaponSwitch system  ←  reads WeaponInput.switch_to_slot      │
└─────────────────────────────────────────────────────────────────┘
```

### Why This Pattern?

| Aspect | Benefit |
|--------|---------|
| Separation | Input handling separate from game logic |
| Testability | Systems can be tested with mock component data |
| AI Support | AI writes same components as player input |
| Multiplayer | Network layer writes to components, systems unchanged |

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

---

## Creating Components

Components are **pure data structs** for Flecs with no Godot bindings.

### Component Types

| Type | Purpose | Example |
|------|---------|---------|
| **State** | Runtime data that changes | `PawnMovement`, `WeaponState` |
| **Input** | Per-frame input (cleared after read) | `PawnInput`, `WeaponInput` |
| **Config Pointers** | References to Godot Resources | `PawnConfig` |
| **Refs** | Child node or entity references | `PawnRefs`, `InventoryRefs` |

### Input Component Pattern

Input components need a `clear_frame()` method for discrete inputs:

```cpp
struct WeaponInput {
    bool attack_pressed = false;    // Discrete - clear after read
    bool attack_held = false;       // Continuous - don't clear
    int switch_to_slot = -1;        // Discrete - clear after read

    void clear_frame() {
        attack_pressed = false;
        switch_to_slot = -1;
        // Note: attack_held persists
    }
};
```

### Refs Component Pattern

Store references to related entities or nodes:

```cpp
struct InventoryRefs {
    flecs::entity pawn_entity;      // Parent pawn for syncing ActiveWeapon
};

struct PawnRefs {
    Camera3D* camera = nullptr;
    Inventory::InventoryNode* inventory_node = nullptr;  // For lazy entity lookup
};
```

---

## Creating Systems

Systems are registered via a **callback pattern** when Polaris initializes.

### Registration Flow

```
register_types.cpp
    └── polaris_set_system_callback(Systems::register_systems)
            └── src/system/register.cpp
                    ├── Locomotion::register_systems(world)
                    ├── Combat::register_systems(world)
                    └── LocalView::register_systems(world)
```

### System Template

```cpp
flecs::entity register_example_system(flecs::world& world) {
    auto physics_phase = Polaris::System::get_physics_phase(world);

    return world.system<ComponentA, const ComponentB>("Namespace_Name")
        .kind(physics_phase)
        .each([](flecs::entity e, ComponentA& a, const ComponentB& b) {
            // Process entities...
        });
}
```

### System That Clears Input

Systems consuming discrete input must clear it:

```cpp
world.system<WeaponInput, WeaponInventory>("Combat_WeaponSwitch")
    .each([](WeaponInput& input, WeaponInventory& inventory) {
        // Read input
        if (input.switch_to_slot >= 0) {
            inventory.active_slot = input.switch_to_slot;
        }

        // Clear discrete inputs after reading
        input.clear_frame();
    });
```

---

## Creating Configs

Configs are **Godot Resources** exposed in the inspector.

```cpp
class MyConfig : public godot::Resource {
    GDCLASS(MyConfig, godot::Resource)

    float m_value = 1.0f;

protected:
    static void _bind_methods();

public:
    float get_value() const { return m_value; }
    void set_value(float v) { m_value = v; }
};

void MyConfig::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_value"), &MyConfig::get_value);
    ClassDB::bind_method(D_METHOD("set_value", "value"), &MyConfig::set_value);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value"), "set_value", "get_value");
}
```

---

## Creating Custom Nodes

Custom nodes inherit from Godot nodes and add ECS components.

### Lifecycle

```
_ready()           → Resolve child node references
_on_ecs_ready()    → Get entity, add components (called by PolarisEngine)
[game runs]        → Systems process entity
_on_ecs_exit()     → Clear entity reference
```

### Basic Pattern

```cpp
void MyNode::_on_ecs_ready() {
    auto* engine = Polaris::PolarisEngine::get_singleton();

    // Get entity PolarisEngine already created
    m_entity = engine->get_entity_for_node(this);

    // Add components
    m_entity.set<MyState>({});
    m_entity.set<MyRefs>({ .camera = m_camera });
}
```

### Required Bindings

```cpp
ClassDB::bind_method(D_METHOD("_on_ecs_ready"), &MyNode::_on_ecs_ready);
ClassDB::bind_method(D_METHOD("_on_ecs_exit"), &MyNode::_on_ecs_exit);
```

---

## Entity Relationships

### Parent-Child Entity References

When child entities need to reference their parent:

```cpp
// In InventoryNode._on_ecs_ready()
// Walk up tree to find parent pawn
CharacterBody3D* pawn = nullptr;
Node* current = get_parent();
while (current && !pawn) {
    pawn = Object::cast_to<CharacterBody3D>(current);
    current = current->get_parent();
}

// Store reference to pawn entity
flecs::entity pawn_entity = engine->get_entity_for_node(pawn);
m_entity.set<InventoryRefs>({ .pawn_entity = pawn_entity });
```

### Lazy Entity Lookup

**Problem:** Child entity may not exist when parent initializes.

**Solution:** Store the node pointer, look up entity when needed.

```cpp
// In PawnNode._on_ecs_ready() - store NODE, not entity
Inventory::InventoryNode* inventory_node = get_node_or_null(...);
m_entity.set<PawnRefs>({ .inventory_node = inventory_node });

// In PlayerPawn._input() - look up entity lazily
const PawnRefs* refs = m_entity.try_get<PawnRefs>();
if (refs && refs->inventory_node) {
    flecs::entity inv_entity = engine->get_entity_for_node(refs->inventory_node);
    if (inv_entity.is_valid()) {
        // Now safe to access inventory entity
    }
}
```

### Bridge Components

When multiple systems need data from another entity, use a bridge component:

```cpp
// ActiveWeapon lives on PAWN, synced from INVENTORY
// Views query pawn (which they already reference) instead of finding inventory

// In WeaponSwitch system (runs on inventory entity):
refs.pawn_entity.ensure<ActiveWeapon>().set_weapon(data, slot);

// In LocalView system (runs on view entity):
const ActiveWeapon* active = refs.pawn_entity.try_get<ActiveWeapon>();
```

---

## Input Handling

### Discrete vs Continuous Input

| Type | Handler | Example | Cleared? |
|------|---------|---------|----------|
| Discrete | `_input()` | Key press, weapon switch | Yes |
| Continuous | `_process()` | Held keys, movement vector | No |

### Pattern: Discrete Events in _input()

```cpp
void PlayerPawn::_input(const Ref<InputEvent>& event) {
    // Weapon switching - discrete, must not miss
    if (event->is_action_pressed("weapon_slot_1")) {
        weapon_input.switch_to_slot = 0;
    }

    // Mouse motion - accumulated
    if (auto* motion = Object::cast_to<InputEventMouseMotion>(event.ptr())) {
        pawn_input.look_delta += motion->get_relative();
    }
}
```

### Pattern: Continuous State in _process()

```cpp
void PlayerPawn::_process(double delta) {
    Input* input = Input::get_singleton();

    // Movement vector - polled each frame
    pawn_input.move_input = input->get_vector("left", "right", "forward", "back");

    // Held state - polled each frame
    pawn_input.jump_held = input->is_action_pressed("jump");
    weapon_input.attack_held = input->is_action_pressed("attack");
}
```

### AI Input

AI pawns expose setters instead of reading from Input singleton:

```cpp
void AIPawn::set_move_input(Vector2 input) {
    PawnInput& pawn_input = m_entity.ensure<PawnInput>();
    pawn_input.move_input = input;
}
```

---

## Accessing Nodes in Systems

### Entity's Own Node (GodotNode)

```cpp
world.system<Polaris::Component::GodotNode, MyComponent>("MySystem")
    .each([](Polaris::Component::GodotNode& gn, MyComponent& comp) {
        auto* body = gn.get_as<CharacterBody3D>();
        body->set_velocity(...);
    });
```

### Child Nodes via Refs

```cpp
world.system<MyRefs, const MyState>("MySystem")
    .each([](MyRefs& refs, const MyState& state) {
        refs.camera->set_rotation(...);
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
float delta = Polaris::Util::get_physics_delta();

// Get entity's node
auto* body = gn.get_as<CharacterBody3D>();

// Get config values
float speed = config.movement->get_max_speed();

// Physics phase system
auto phase = Polaris::System::get_physics_phase(world);

// Process phase system
auto phase = Polaris::System::get_process_phase(world);

// Lazy entity lookup
flecs::entity e = engine->get_entity_for_node(node_ptr);

// Safe component access
const MyComp* comp = entity.try_get<MyComp>();
if (comp) { /* use comp */ }

// Mutable component access
MyComp& comp = entity.ensure<MyComp>();
```

### Reference Files

| Pattern | Example File |
|---------|--------------|
| Input Component | `pawn/component/PawnInput.h` |
| Refs Component | `pawn/component/PawnRefs.h` |
| Input in Node | `pawn/PlayerPawn.cpp` |
| System with Clear | `combat/weapon/system/WeaponSwitch.cpp` |
| Lazy Entity Lookup | `pawn/PlayerPawn.cpp` |
| Bridge Component | `combat/weapon/component/ActiveWeapon.h` |
| Entity Relationship | `inventory/InventoryNode.cpp` |
