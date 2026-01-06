# Getting Started

> Create your first Polaris entity in 10 minutes

---

## Prerequisites

- Godot 4.x project
- Polaris GDExtension built and added to your project

---

## Step 1: Setup Autoload

Add the Polaris autoload to your project:

1. Open **Project → Project Settings → Autoload**
2. Click the folder icon and select `src/polaris/scenes/Polaris.tscn`
3. Name it `Polaris`
4. Click **Add**

```
┌─────────────────────────────────────────────────────────────────────────────┐
│   Project Settings → Autoload                                               │
│                                                                             │
│   Name              Path                                    Enabled         │
│   ─────────────────────────────────────────────────────────────────         │
│   Polaris           res://src/polaris/scenes/Polaris.tscn   ✓               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

When the game runs, Polaris will:
- Initialize the ECS world
- Watch for nodes with composition metadata
- Run systems each frame

---

## Step 2: Create Your First Data Component

Data components are pure structs that hold game state. Create a simple `Health` component:

**File:** `game/components/Data.h`

```cpp
#pragma once

struct Health {
    float current = 100.0f;
    float max = 100.0f;

    bool is_alive() const { return current > 0; }
    float percentage() const { return current / max; }
};
```

That's it — no registration needed. Flecs discovers components automatically.

---

## Step 3: Create Your First Gd:: Component

Gd:: components hold pointers to Godot nodes. They follow a standard pattern:

**File:** `game/components/GdPhysics.h`

```cpp
#pragma once

#include <godot_cpp/classes/character_body3d.hpp>

namespace Gd::Physics {

struct CharacterController {
    godot::CharacterBody3D* body = nullptr;

    // Factory method — finds/casts the node
    static CharacterController create(godot::Node* root) {
        return { godot::Object::cast_to<godot::CharacterBody3D>(root) };
    }

    // Validity check — always check before using
    bool is_valid() const { return body != nullptr; }

    // Cleanup — call in decompose()
    void unbind() { body = nullptr; }

    // Convenience methods
    void move_and_slide() {
        if (is_valid()) body->move_and_slide();
    }

    godot::Vector3 get_velocity() const {
        return is_valid() ? body->get_velocity() : godot::Vector3();
    }

    void set_velocity(const godot::Vector3& vel) {
        if (is_valid()) body->set_velocity(vel);
    }
};

}
```

### Gd:: Component Pattern

```
┌─────────────────────────────────────────────────────────────────────────────┐
│   GD:: COMPONENT PATTERN                                                    │
│                                                                             │
│   struct ComponentName {                                                    │
│       NodeType* ptr = nullptr;           // Pointer to Godot node           │
│                                                                             │
│       static ComponentName create(Node* root);   // Factory from root       │
│       bool is_valid() const;                     // Safety check            │
│       void unbind();                             // Cleanup for decompose   │
│                                                                             │
│       // ... convenience methods wrapping node API                          │
│   };                                                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

| Method | Purpose |
|--------|---------|
| `create(root)` | Static factory — casts root node or finds child |
| `is_valid()` | Check pointer before use — prevents crashes |
| `unbind()` | Set pointer to nullptr — call in `decompose()` |

---

## Step 4: Create Your First Composition

A Composition defines what components an entity has. Let's create a `PlayerPawn` that uses both our `Health` data component and `Gd::Physics::CharacterController`:

**File:** `game/compositions/PlayerPawn.h`

```cpp
#pragma once

#include "composition/Composition.h"
#include "game/components/Data.h"
#include "game/components/GdPhysics.h"

namespace Game {

class PlayerPawn : public Polaris::Composition {
    GDCLASS(PlayerPawn, Polaris::Composition)

    // Exposed to Godot inspector
    float m_max_health = 100.0f;

public:
    void compose(flecs::entity e, godot::Node* root) override;
    void decompose(flecs::entity e) override;

    // Property accessors
    float get_max_health() const { return m_max_health; }
    void set_max_health(float value) { m_max_health = value; }

protected:
    static void _bind_methods();
};

}
```

**File:** `game/compositions/PlayerPawn.cpp`

```cpp
#include "PlayerPawn.h"
#include "components/Gd.h"
#include "util/Log.h"

namespace Game {

using namespace godot;
using namespace Polaris;

void PlayerPawn::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_max_health"), &PlayerPawn::get_max_health);
    ClassDB::bind_method(D_METHOD("set_max_health", "value"), &PlayerPawn::set_max_health);

    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "max_health", PROPERTY_HINT_RANGE, "1,1000,1"),
        "set_max_health", "get_max_health"
    );
}

void PlayerPawn::compose(flecs::entity e, Node* root) {
    // Call parent — sets Gd::Node automatically for 3D
    Composition::compose(e, root);

    // Add data component
    e.set<Health>({
        .current = m_max_health,
        .max = m_max_health
    });

    // Add Gd:: component using create() factory
    e.set<Gd::Physics::CharacterController>(
        Gd::Physics::CharacterController::create(root)
    );

    Log::info("[PlayerPawn] Composed entity: ", root->get_name().utf8().get_data());
}

void PlayerPawn::decompose(flecs::entity e) {
    // Unbind Gd:: components first
    if (auto* controller = e.try_get_mut<Gd::Physics::CharacterController>()) {
        controller->unbind();
    }

    // Call parent — clears Gd::Node
    Composition::decompose(e);

    Log::info("[PlayerPawn] Decomposed entity");
}

}
```

### Compose/Decompose Pattern

```
┌─────────────────────────────────────────────────────────────────────────────┐
│   COMPOSE / DECOMPOSE PATTERN                                               │
│                                                                             │
│   compose(entity, root)                   decompose(entity)                 │
│   ─────────────────────                   ──────────────────                │
│   1. Call parent compose()                1. Unbind custom Gd:: components  │
│   2. Add data components                  2. Call parent decompose()        │
│   3. Add Gd:: components via create()                                       │
│                                                                             │
│   e.set<Gd::Physics::CharacterController>(    if (auto* c = e.try_get_mut   │
│       Gd::Physics::CharacterController::          <Gd::Physics::Controller  │
│           create(root)                                >()) {                │
│   );                                              c->unbind();              │
│                                               }                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Step 5: Register Your Composition

Add your composition to the GDExtension registration:

**File:** `src/register_types.cpp` (or your game's init file)

```cpp
#include "game/compositions/PlayerPawn.h"

void register_game_types() {
    GDREGISTER_CLASS(Game::PlayerPawn);
}
```

---

## Step 6: Create the Resource

1. In Godot, right-click your `resources/` folder
2. Select **New Resource**
3. Search for `PlayerPawn`
4. Save as `player.tres`
5. In the inspector, set `max_health` to `100`

```
┌─────────────────────────────────────────────────────────────────────────────┐
│   Inspector                                                                 │
│                                                                             │
│   PlayerPawn                                                                │
│   ─────────────────────────────────────────────────────────────────         │
│   Max Health         [====100====]                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Step 7: Create Your Scene

Create a simple player scene:

1. Create a new scene with a `CharacterBody3D` root
2. Add child nodes: `MeshInstance3D`, `CollisionShape3D`
3. Save as `player.tscn`

```
CharacterBody3D "Player"
├── MeshInstance3D (add a capsule mesh)
└── CollisionShape3D (add a capsule shape)
```

---

## Step 8: Attach the Composition

Attach your composition to the node:

1. Select the `CharacterBody3D` root node
2. In the Inspector, scroll to **Meta** section (or click the meta icon)
3. Add a new meta key: `composition`
4. Set the value to your `player.tres` resource

```
┌─────────────────────────────────────────────────────────────────────────────┐
│   Inspector → CharacterBody3D                                               │
│                                                                             │
│   ▼ Meta                                                                    │
│     composition    [player.tres]                                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Step 9: Run the Game

1. Add your player scene to your main scene
2. Press **F5** to run

In the output, you should see:

```
[Polaris] Framework classes registered
[ECSWorld] Created - flecs world initialized
[PlayerPawn] Composed entity: Player
```

Your entity now exists in the ECS world with:
- `Gd::Node` — pointing to the CharacterBody3D (auto-set by base)
- `Gd::Physics::CharacterController` — with physics API access
- `Health` — with current: 100, max: 100

---

## What's Next?

Now that you have an entity, you can:

1. **Query it in systems** — See [3. Lifecycle](3.%20Lifecycle.md)
2. **Add more components** — See [2.0 Entity](2.0.Entity.md)
3. **Create custom Gd:: components** — See [1.0 Composition](1.0%20Composition.md)

### Quick System Example

To move your player with physics, create a movement system using the CharacterController:

```cpp
// In your game initialization
flecs::world& world = ECSWorld::get()->world();

world.system<Gd::Physics::CharacterController, Health>()
    .each([](Gd::Physics::CharacterController& controller, Health& hp) {
        if (controller.is_valid() && hp.is_alive()) {
            // Apply gravity and move
            Vector3 vel = controller.get_velocity();
            vel.y -= 9.8f * 0.016f;  // gravity * delta
            controller.set_velocity(vel);
            controller.move_and_slide();
        }
    });
```

---

## Summary

| Step | What You Did |
|------|--------------|
| 1 | Added Polaris.tscn to Autoload |
| 2 | Created a `Health` data component (pure struct) |
| 3 | Created a `Gd::Physics::CharacterController` component (node pointer) |
| 4 | Created a `PlayerPawn` composition (adds both components) |
| 5 | Registered the composition with Godot |
| 6 | Created a `player.tres` resource with settings |
| 7 | Built a player scene with CharacterBody3D |
| 8 | Attached the composition via metadata |
| 9 | Ran the game — entity created automatically |

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│   THE POLARIS PATTERN                                                       │
│                                                                             │
│   ┌─────────────┐     ┌─────────────┐     ┌──────────────────────┐          │
│   │ .tres       │     │ Godot Node  │     │ ECS Entity           │          │
│   │ Resource    │ ──> │ + metadata  │ ──> │                      │          │
│   │             │     │             │     │ Health (data)        │          │
│   │ PlayerPawn  │     │ Player.tscn │     │ Gd::Node (base)      │          │
│   │ max_hp: 100 │     │ composition │     │ Gd::Physics::        │          │
│   └─────────────┘     └─────────────┘     │   CharacterController│          │
│                                           └──────────────────────┘          │
│                                                                             │
│   Designer creates     Artist builds      Systems query components          │
│   .tres variants       the scene          and call Godot APIs               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```
