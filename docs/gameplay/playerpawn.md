# PlayerPawn Implementation Guide

GoldSrc-style first-person player movement using the Polaris ECS framework.

## Architecture Overview

```
                           COMPOSITION HIERARCHY
    +-----------------------------------------------------------------+
    |                                                                 |
    |   Polaris::Composition                                          |
    |         |                                                       |
    |         v                                                       |
    |   +-------------------------------------------------------------+
    |   |  Pawn (base)                                            |   |
    |   |  +-- Gd::Node3D, Gd::CharacterBody3D                    |   |
    |   |  +-- Health, Velocity, PawnMovement                     |   |
    |   |  +-- Tag::Grounded                                      |   |
    |   |  +-- Properties: max_speed, friction, gravity, etc.     |   |
    |   +-------------------------------------------------------------+
    |         |                                                       |
    |         v                                                       |
    |   +-------------------------------------------------------------+
    |   |  PlayerPawn (extends Pawn)                              |   |
    |   |  +-- Gd::Camera3D                                       |   |
    |   |  +-- PlayerInputState, PlayerLook, PlayerBunnyHop       |   |
    |   |  +-- Tag::Player                                        |   |
    |   |  +-- Properties: sensitivity, autohop, bhop_cap_mode    |   |
    |   +-------------------------------------------------------------+
    |                                                                 |
    +-----------------------------------------------------------------+


                              SYSTEM PIPELINE
    +-----------------------------------------------------------------+
    |                                                                 |
    |   Phase_Input (from _input callback)                            |
    |   +-- CapturePlayerInput: Godot Input -> PlayerInputState       |
    |                                                                 |
    |   Phase_Physics (from _physics_process callback)                |
    |   |                                                             |
    |   +-- PlayerMovement.Input -----------------------------+       |
    |   |   +-- PlayerLook:     mouse delta -> body yaw, cam pitch    |
    |   |   +-- PlayerFriction: apply ground friction                 |
    |   |   +-- PlayerAccel:    WASD -> velocity (ground/air)         |
    |   |                                                             |
    |   +-- PlayerMovement.Apply -----------------------------+       |
    |   |   +-- PlayerGravity:  apply downward acceleration           |
    |   |   +-- PlayerJump:     jump + bunny hop cap                  |
    |   |   +-- PlayerApply:    velocity -> move_and_slide()          |
    |   |                                                             |
    |   +-- PlayerMovement.Resolve ---------------------------+       |
    |       +-- PlayerResolve:  collision -> velocity, grounded       |
    |                                                                 |
    +-----------------------------------------------------------------+


                           GOLDSRC MOVEMENT
    +-----------------------------------------------------------------+
    |                                                                 |
    |   GROUND                          AIR (strafe jumping)          |
    |   +------------------+            +------------------+          |
    |   | wish_speed: 320  |            | wish_speed: 30   | <- KEY!  |
    |   | acceleration: 10 |            | acceleration: 10 |          |
    |   | friction: 4      |            | (no friction)    |          |
    |   +------------------+            +------------------+          |
    |                                                                 |
    |   The 30 HU air speed cap enables strafe jumping:               |
    |   - Moving perpendicular adds velocity in that direction        |
    |   - Total speed can exceed ground max_speed                     |
    |   - Skilled players reach 2-3x normal speed                     |
    |                                                                 |
    +-----------------------------------------------------------------+
```

## Components

### Shared (all pawns) - `pawn/Components.h`

```cpp
// Health state
struct Health {
    float current;
    float max;
};

// Current velocity
struct Velocity {
    godot::Vector3 linear;
};

// Movement parameters (baked from Composition)
struct PawnMovement {
    float max_speed;      // Ground max speed (320 HU = 8.128m)
    float acceleration;   // Ground accel multiplier
    float friction;       // Ground friction
    float stop_speed;     // Friction threshold (100 HU = 2.54m)
    float max_air_speed;  // Air wish cap (30 HU = 0.762m) - enables strafe jumping
    float air_accel;      // Air accel multiplier
    float gravity;        // Downward accel (800 HU/s^2 = 20.32m/s^2)
    float jump_speed;     // Initial jump velocity
};

namespace Tag {
    struct Grounded {};   // Entity is on the ground
}
```

### Player-specific - `pawn/player/PlayerComponents.h`

Uses **Config/State separation**: Config structs are inspector-bound and serialized; State structs hold runtime data.

```cpp
// ==========================================
// Look - Camera rotation
// ==========================================

// Config: Inspector values (editor units)
struct PlayerLook {
    float sensitivity_pct{};    // 0-100 percentage
    bool invert_y{};
};

// State: Runtime values
struct PlayerLookState {
    float yaw{};
    float pitch{};
};

// X-macro for POLARIS_ACCESSORS/BIND
#define PLAYER_LOOK_PROPS(X, C, M) \
    X(sensitivity_pct, float, FLOAT, "look/sensitivity", "", C, M) \
    X(invert_y,        bool,  BOOL,  "look/invert_y",    "", C, M)

// ==========================================
// Bunny Hop - Advanced movement options
// ==========================================

// Config only (no state needed)
struct PlayerBunnyHop {
    bool autohop{};
    int cap_mode{};         // 0=None, 1=Hard, 2=Soft
    float threshold{};
    float drop{};
};

#define PLAYER_BUNNY_HOP_PROPS(X, C, M) \
    X(autohop,   bool,  BOOL,  "bhop/autohop",   "",                       C, M) \
    X(cap_mode,  int,   INT,   "bhop/cap_mode",  "None,Hard Cap,Soft Cap", C, M) \
    X(threshold, float, FLOAT, "bhop/threshold", "",                       C, M) \
    X(drop,      float, FLOAT, "bhop/drop",      "",                       C, M)

// ==========================================
// Input State - Runtime only (not serialized)
// ==========================================

struct PlayerInputState {
    godot::Vector2 move;
    godot::Vector2 look_delta;
    godot::Vector2 look_stick;
    bool jump_pressed{};
    bool jump_held{};
};
```

**Pattern notes:**
- Config structs: Plain POD, inspector-bound via X-macro
- State structs: Runtime only, initialized fresh in compose()
- Systems do all unit conversions (e.g., `sensitivity_pct` → raw sensitivity)

## File Structure

```
src/
+-- pawn/
|   +-- Components.h              # Shared pawn components (Health, Velocity, PawnMovement, Tag::Grounded)
|   +-- Pawn.h                    # Base pawn composition
|   +-- Pawn.cpp
|   +-- player/
|       +-- PlayerComponents.h    # Player-specific (PlayerInputState, PlayerLook, PlayerBunnyHop)
|       +-- PlayerPawn.h          # Player pawn composition
|       +-- PlayerPawn.cpp
|       +-- PlayerSystems.h       # Systems header
|       +-- PlayerSystems.cpp     # Input + movement systems combined
+-- GameInit.h                    # System registration
+-- register_types.cpp            # Godot class registration

scenes/pawn/player_pawn.tscn
resources/pawn/player_pawn.tres
```

## Implementation

### 1. Shared Components (`src/pawn/Components.h`)

```cpp
#ifndef GAME_PAWN_COMPONENTS_H
#define GAME_PAWN_COMPONENTS_H

#include <godot_cpp/variant/vector3.hpp>

namespace Game {

struct Health {
    float current = 100.0f;
    float max = 100.0f;
};

struct Velocity {
    godot::Vector3 linear;
};

struct PawnMovement {
    float max_speed = 8.128f;      // 320 HU
    float acceleration = 10.0f;
    float friction = 4.0f;
    float stop_speed = 2.54f;      // 100 HU
    float max_air_speed = 0.762f;  // 30 HU - enables strafe jumping
    float air_accel = 10.0f;
    float gravity = 20.32f;        // 800 HU/s^2
    float jump_speed = 6.858f;
};

namespace Tag {
    struct Grounded {};
}

} // namespace Game

#endif // GAME_PAWN_COMPONENTS_H
```

### 2. Player Components (`src/pawn/player/PlayerComponents.h`)

```cpp
#ifndef GAME_PLAYER_COMPONENTS_H
#define GAME_PLAYER_COMPONENTS_H

#include <godot_cpp/variant/vector2.hpp>

namespace Game {

struct PlayerInputState {
    godot::Vector2 move;
    godot::Vector2 look_delta;
    bool jump_pressed = false;
    bool jump_held = false;
};

struct PlayerLook {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float sensitivity = 0.002f;
    bool invert_y = false;
};

struct PlayerBunnyHop {
    bool autohop = false;
    int cap_mode = 0;
    float threshold = 1.7f;
    float drop = 1.1f;
};

} // namespace Game

#endif // GAME_PLAYER_COMPONENTS_H
```

### 3. Pawn Base (`src/pawn/Pawn.h`)

```cpp
#ifndef GAME_PAWN_H
#define GAME_PAWN_H

#include "composition/Composition.h"

namespace Game {

class Pawn : public Polaris::Composition {
    GDCLASS(Pawn, Polaris::Composition)

protected:
    static void _bind_methods();

    float m_max_health = 100.0f;
    float m_max_speed = 8.128f;       // 320 HU
    float m_acceleration = 10.0f;
    float m_friction = 4.0f;
    float m_stop_speed = 2.54f;       // 100 HU
    float m_max_air_speed = 0.762f;   // 30 HU
    float m_air_accel = 10.0f;
    float m_gravity = 20.32f;         // 800 HU/s^2
    float m_jump_speed = 6.858f;

public:
    void compose(flecs::entity e, godot::Node* node) override;
    void decompose(flecs::entity e) override;

    // Property accessors...
};

} // namespace Game

#endif // GAME_PAWN_H
```

### 4. Pawn Base (`src/pawn/Pawn.cpp`)

```cpp
#include "Pawn.h"
#include "PawnComponents.h"
#include "components/Gd.h"
#include <godot_cpp/classes/character_body3d.hpp>

namespace Game {

// Composition Lifecycle (main logic - comes first)
void Pawn::compose(flecs::entity e, godot::Node* node) {
    Polaris::Composition::compose(e, node);

    // Godot bindings
    if (auto* body = godot::Object::cast_to<godot::CharacterBody3D>(node)) {
        e.set<Polaris::Gd::CharacterBody3D>(Polaris::Gd::CharacterBody3D::create(body));
    }

    // Components
    e.set<Health>({ m_max_health, m_max_health });
    e.set<Velocity>({ godot::Vector3() });
    e.set<PawnMovement>({
        m_max_speed, m_acceleration, m_friction, m_stop_speed,
        m_max_air_speed, m_air_accel, m_gravity, m_jump_speed
    });
    e.add<Tag::Grounded>();
}

void Pawn::decompose(flecs::entity e) {
    if (auto* n = e.get_mut<Polaris::Gd::Node3D>()) n->unbind();
    if (auto* b = e.get_mut<Polaris::Gd::CharacterBody3D>()) b->unbind();
    Polaris::Composition::decompose(e);
}

// Godot Bindings (boilerplate - comes last)
void Pawn::_bind_methods() {
    // Bind all properties to inspector
}

} // namespace Game
```

### 5. PlayerPawn (`src/pawn/player/PlayerPawn.h`)

Uses **POLARIS_ACCESSORS** to generate property accessors from X-macros.

```cpp
#ifndef GAME_PLAYER_PAWN_H
#define GAME_PLAYER_PAWN_H

#include "pawn/Pawn.h"
#include "PlayerComponents.h"
#include "polaris/src/core/PropertyMacros.h"

namespace Game {

class PlayerPawn : public Pawn {
    GDCLASS(PlayerPawn, Pawn)

protected:
    static void _bind_methods();

    // Config members (serialized to .tres, synced to ECS on change)
    PlayerLook m_look;
    PlayerBunnyHop m_bunny_hop;

public:
    PlayerPawn() = default;
    virtual ~PlayerPawn() = default;

    void compose(flecs::entity e, godot::Node* node) override;
    void decompose(flecs::entity e) override;

    // Generated accessors (passthrough + sync)
    POLARIS_ACCESSORS(PlayerLook, m_look, PLAYER_LOOK_PROPS)
    POLARIS_ACCESSORS(PlayerBunnyHop, m_bunny_hop, PLAYER_BUNNY_HOP_PROPS)
};

} // namespace Game

#endif // GAME_PLAYER_PAWN_H
```

### 6. PlayerPawn (`src/pawn/player/PlayerPawn.cpp`)

```cpp
#include "PlayerPawn.h"
#include "pawn/PawnComponents.h"
#include "components/Gd.h"
#include <godot_cpp/classes/camera3d.hpp>

namespace Game {

void PlayerPawn::compose(flecs::entity e, godot::Node* node) {
    Pawn::compose(e, node);  // Base setup

    // Camera
    if (auto* cam_node = node->get_node_or_null(godot::NodePath("Camera3D"))) {
        if (auto* cam = godot::Object::cast_to<godot::Camera3D>(cam_node)) {
            e.set<Polaris::Gd::Camera3D>(Polaris::Gd::Camera3D::create(cam));
        }
    }

    // Config components (from .tres) - no conversion, just copy
    e.set<PlayerLook>(m_look);
    e.set<PlayerBunnyHop>(m_bunny_hop);

    // State components (fresh) - runtime values start at defaults
    e.set<PlayerLookState>({});
    e.set<PlayerInputState>({});

    e.add<Polaris::Tag::Player>();
}

void PlayerPawn::decompose(flecs::entity e) {
    if (auto* c = e.get_mut<Polaris::Gd::Camera3D>()) c->unbind();
    Pawn::decompose(e);
}

void PlayerPawn::_bind_methods() {
    ADD_GROUP("Look", "look/");
    POLARIS_BIND(PlayerPawn, PLAYER_LOOK_PROPS)

    ADD_GROUP("Bunny Hop", "bhop/");
    POLARIS_BIND(PlayerPawn, PLAYER_BUNNY_HOP_PROPS)
}

} // namespace Game
```

### 7. Player Systems (`src/pawn/player/PlayerSystems.h`)

```cpp
#ifndef GAME_PLAYER_SYSTEMS_H
#define GAME_PLAYER_SYSTEMS_H

namespace Polaris {
    class Runtime;
}

namespace Game::PlayerSystems {

/// Initialize all player systems (input capture + movement)
void init(Polaris::Runtime* rt);

/// Queue mouse delta for next frame (call from _input callback)
void queue_mouse_delta(float dx, float dy);

} // namespace Game::PlayerSystems

#endif // GAME_PLAYER_SYSTEMS_H
```

### 8. Player Systems (`src/pawn/player/PlayerSystems.cpp`)

```cpp
#include "PlayerSystems.h"
#include "PlayerComponents.h"
#include "pawn/PawnComponents.h"
#include "core/Runtime.h"
#include "components/Gd.h"
#include <godot_cpp/classes/input.hpp>
#include <cmath>

namespace Game::PlayerSystems {

using namespace Polaris;

static godot::Vector2 s_pending_mouse_delta;

void queue_mouse_delta(float dx, float dy) {
    s_pending_mouse_delta.x += dx;
    s_pending_mouse_delta.y += dy;
}

namespace Phase {
    static flecs::entity Input, Apply, Resolve;
}

// Ground acceleration (no cap)
static void ground_accelerate(godot::Vector3& vel, const godot::Vector3& wish_dir,
                              float wish_speed, float accel, float dt) {
    float current = vel.dot(wish_dir);
    float add = wish_speed - current;
    if (add <= 0) return;
    vel += wish_dir * std::fmin(accel * wish_speed * dt, add);
}

// Air acceleration (capped - enables strafe jumping)
static void air_accelerate(godot::Vector3& vel, const godot::Vector3& wish_dir,
                           float wish_speed, float max_air, float accel, float dt) {
    float capped = std::fmin(wish_speed, max_air);
    float current = vel.dot(wish_dir);
    float add = capped - current;
    if (add <= 0) return;
    vel += wish_dir * std::fmin(accel * wish_speed * dt, add);
}

// System implementations...
// (capture_input, look_system, friction_system, accel_system,
//  gravity_system, jump_system, apply_system, resolve_system)

void init(Runtime* rt) {
    auto& w = rt->world();
    auto physics = rt->get_phase(Phase_Physics).id;

    Phase::Input = w.entity("PlayerMovement.Input").add(flecs::Phase).depends_on(physics);
    Phase::Apply = w.entity("PlayerMovement.Apply").add(flecs::Phase).depends_on(Phase::Input);
    Phase::Resolve = w.entity("PlayerMovement.Resolve").add(flecs::Phase).depends_on(Phase::Apply);

    // Input capture
    w.system<PlayerInputState>("CapturePlayerInput")
        .kind(physics).with<Tag::Player>().each(capture_input);

    // Input phase
    w.system<PlayerInputState, PlayerLook, Gd::Node3D, Gd::Camera3D>("PlayerLook")
        .kind(Phase::Input).with<Tag::Player>().each(look);
    w.system<Velocity, const PawnMovement>("PlayerFriction")
        .kind(Phase::Input).with<Tag::Player>().with<Tag::Grounded>().each(friction);
    w.system<const PlayerInputState, Velocity, const PawnMovement, const PlayerLook>("PlayerAccel")
        .kind(Phase::Input).with<Tag::Player>().each(accelerate);

    // Apply phase
    w.system<Velocity, const PawnMovement>("PlayerGravity")
        .kind(Phase::Apply).with<Tag::Player>().without<Tag::Grounded>().each(gravity);
    w.system<const PlayerInputState, Velocity, const PawnMovement, const PlayerBunnyHop>("PlayerJump")
        .kind(Phase::Apply).with<Tag::Player>().with<Tag::Grounded>().each(jump);
    w.system<const Velocity, Gd::CharacterBody3D>("PlayerApply")
        .kind(Phase::Apply).with<Tag::Player>().each(apply);

    // Resolve phase
    w.system<Velocity, Gd::CharacterBody3D>("PlayerResolve")
        .kind(Phase::Resolve).with<Tag::Player>().each(resolve);
}

} // namespace Game::PlayerSystems
```

### 9. Game Init (`src/GameInit.h`)

```cpp
#ifndef GAME_INIT_H
#define GAME_INIT_H

#include "core/Runtime.h"
#include "pawn/player/PlayerSystems.h"

namespace Game {

inline void init_game_systems() {
    auto* rt = Polaris::Runtime::get();
    if (!rt) return;

    PlayerSystems::init(rt);
}

} // namespace Game

#endif // GAME_INIT_H
```

## Scene Setup

### player_pawn.tscn

```
PlayerPawn (CharacterBody3D)
+-- CollisionShape3D (CapsuleShape3D: r=0.4, h=1.8)
+-- Camera3D (y=1.6, fov=90)
+-- metadata/composition = player_pawn.tres
```

### Input Actions

| Action | Key |
|--------|-----|
| move_forward | W |
| move_back | S |
| move_left | A |
| move_right | D |
| jump | Space |

## GoldSrc Defaults

| Parameter | Meters | Hammer Units |
|-----------|--------|--------------|
| max_speed | 8.128 | 320 |
| max_air_speed | 0.762 | 30 |
| stop_speed | 2.54 | 100 |
| gravity | 20.32 | 800 |
| jump_speed | 6.858 | ~270 |

**Unit conversion:** 1 HU = 0.0254m (1 inch)
