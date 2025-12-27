# Polaris Custom Pipelines

## Overview

Polaris uses **custom Flecs pipelines** to separate physics and process systems. Instead of all systems running every time `world.progress()` is called, systems are grouped into pipelines that run only during their designated Godot callback.

```
┌─────────────────────────────────────────────────────────────────┐
│                    Godot Frame Loop                             │
│                                                                 │
│   _physics_process(delta)          _process(delta)             │
│         │                                │                      │
│         ▼                                ▼                      │
│   run_physics_pipeline()          run_process_pipeline()       │
│         │                                │                      │
│         ▼                                ▼                      │
│   ┌─────────────────┐            ┌─────────────────┐           │
│   │ Physics Systems │            │ Process Systems │           │
│   │ • Movement      │            │ • Interpolation │           │
│   │ • Collision     │            │ • Animation     │           │
│   │ • Physics apply │            │ • UI updates    │           │
│   └─────────────────┘            └─────────────────┘           │
└─────────────────────────────────────────────────────────────────┘
```

## Why Custom Pipelines?

### The Problem

Previously, all systems ran during both `_physics_process` and `_process`:

```cpp
// Old approach - manual phase checking
world.system("Movement")
    .kind(flecs::OnUpdate)
    .run([](flecs::iter& it) {
        // Check phase every frame, skip if wrong phase
        const auto* phase = world.get<CurrentPhase>();
        if (phase->phase != FramePhase::Physics) {
            return;  // Wasteful - system still "runs" but does nothing
        }
        // Actual work...
    });
```

Issues:
- Systems execute twice per frame (once in physics, once in process)
- Manual phase checking in every system
- Boilerplate code repeated across all systems
- Harder to reason about system execution order

### The Solution

Custom pipelines ensure systems only run during their intended phase:

```cpp
// New approach - pipeline-based
world.system("Movement")
    .kind(get_physics_phase(world))  // Registers to physics pipeline
    .each([](flecs::entity e, Movement& m) {
        // Only runs during _physics_process
        // No phase check needed
    });
```

Benefits:
- Systems run exactly once, during correct phase
- No manual phase checking
- Cleaner system code
- Explicit about when systems execute

## Usage

### Registering a Physics System

Physics systems run during `_physics_process` at a fixed timestep (default 60Hz):

```cpp
#include "system/FrameTicker.h"

flecs::entity register_movement_system(flecs::world& world) {
    auto physics_phase = Polaris::System::get_physics_phase(world);

    return world.system<Velocity, Position>("Movement")
        .kind(physics_phase)
        .each([](flecs::entity e, Velocity& vel, Position& pos) {
            // Get delta from PhysicsFrame singleton
            auto* engine = Polaris::PolarisEngine::get_singleton();
            const auto* pf = engine->get_world().get<Polaris::System::PhysicsFrame>();
            float delta = static_cast<float>(pf->delta);

            pos.value += vel.value * delta;
        });
}
```

### Registering a Process System

Process systems run during `_process` at the render framerate (variable):

```cpp
flecs::entity register_interpolation_system(flecs::world& world) {
    auto process_phase = Polaris::System::get_process_phase(world);

    return world.system<Transform, RenderPosition>("Interpolation")
        .kind(process_phase)
        .each([](flecs::entity e, Transform& t, RenderPosition& rp) {
            // Get delta from ProcessFrame singleton
            auto* engine = Polaris::PolarisEngine::get_singleton();
            const auto* pf = engine->get_world().get<Polaris::System::ProcessFrame>();
            float delta = static_cast<float>(pf->delta);

            // Smooth interpolation for rendering
            rp.value = rp.value.lerp(t.value, delta * 10.0f);
        });
}
```

### Complete Context Example

```cpp
class MyContext : public Context {
    GDCLASS(MyContext, Context)

    flecs::entity m_physics_system;
    flecs::entity m_process_system;

public:
    void _on_ecs_ready(Node* owner) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();

        // Physics system - runs at fixed 60Hz
        auto physics_phase = Polaris::System::get_physics_phase(world);
        m_physics_system = world.system<Velocity>("ApplyVelocity")
            .kind(physics_phase)
            .each([](Velocity& v) {
                // Physics logic here
            });

        // Process system - runs every render frame
        auto process_phase = Polaris::System::get_process_phase(world);
        m_process_system = world.system<Visual>("UpdateVisuals")
            .kind(process_phase)
            .each([](Visual& vis) {
                // Visual/render logic here
            });
    }

    void _on_ecs_exit(Node* owner) {
        if (m_physics_system.is_valid()) {
            m_physics_system.destruct();
        }
        if (m_process_system.is_valid()) {
            m_process_system.destruct();
        }
    }
};
```

## API Reference

### Header: `system/FrameTicker.h`

```cpp
namespace Polaris::System {

/// Initialize pipelines - called automatically by TickerNode
void init_pipelines(flecs::world& world);

/// Get the physics phase entity for system registration
/// Use with .kind() when creating physics systems
flecs::entity get_physics_phase(flecs::world& world);

/// Get the process phase entity for system registration
/// Use with .kind() when creating process systems
flecs::entity get_process_phase(flecs::world& world);

/// Run the physics pipeline (called by TickerNode during _physics_process)
void run_physics_pipeline(flecs::world& world, float delta);

/// Run the process pipeline (called by TickerNode during _process)
void run_process_pipeline(flecs::world& world, float delta);

}
```

### Frame Singletons

Systems can access frame timing through singleton components:

```cpp
// Physics frame data (updated during _physics_process)
struct PhysicsFrame {
    double delta = 0.0;   // Time since last physics frame (~0.0167 at 60Hz)
    uint64_t frame = 0;   // Physics frame counter
    double time = 0.0;    // Total elapsed physics time
};

// Process frame data (updated during _process)
struct ProcessFrame {
    double delta = 0.0;   // Time since last render frame (variable)
    uint64_t frame = 0;   // Process frame counter
    double time = 0.0;    // Total elapsed process time
};
```

## How It Works

### Pipeline Initialization

When `TickerNode` initializes, it creates two custom pipelines:

```cpp
void init_pipelines(flecs::world& world) {
    // Create phase entities tagged with flecs::Phase
    physics_phase = world.entity("Polaris::PhysicsPhase")
        .add(flecs::Phase)
        .add(flecs::DependsOn, flecs::OnUpdate);

    process_phase = world.entity("Polaris::ProcessPhase")
        .add(flecs::Phase)
        .add(flecs::DependsOn, flecs::OnUpdate);

    // Create pipelines that match systems registered to each phase
    physics_pipeline = world.pipeline()
        .with(flecs::System)
        .with(flecs::DependsOn, physics_phase)
        .build();

    process_pipeline = world.pipeline()
        .with(flecs::System)
        .with(flecs::DependsOn, process_phase)
        .build();
}
```

### System Registration

When you call `.kind(physics_phase)`, Flecs adds a `DependsOn` relationship:

```cpp
world.system("MySystem")
    .kind(physics_phase)  // Adds: DependsOn(PhysicsPhase)
    .each([](/* ... */) { /* ... */ });
```

This relationship is what the pipeline query matches against.

### Pipeline Execution

During Godot callbacks, the appropriate pipeline is set and run:

```cpp
// In TickerNode::_physics_process(delta)
void run_physics_pipeline(flecs::world& world, float delta) {
    world.set_pipeline(physics_pipeline);
    world.progress(delta);  // Only runs systems with DependsOn(PhysicsPhase)
}

// In TickerNode::_process(delta)
void run_process_pipeline(flecs::world& world, float delta) {
    world.set_pipeline(process_pipeline);
    world.progress(delta);  // Only runs systems with DependsOn(ProcessPhase)
}
```

## Adding New Pipelines

The system is designed to be extensible. To add a new pipeline (e.g., for input or networking):

### 1. Add to PipelineRegistry

```cpp
// In FrameTicker.h
struct PipelineRegistry {
    flecs::entity physics_phase;
    flecs::entity process_phase;
    flecs::entity input_phase;      // New

    flecs::entity physics_pipeline;
    flecs::entity process_pipeline;
    flecs::entity input_pipeline;   // New

    bool initialized = false;
};
```

### 2. Create Phase and Pipeline

```cpp
// In init_pipelines()
s_registry.input_phase = world.entity("Polaris::InputPhase")
    .add(flecs::Phase)
    .add(flecs::DependsOn, flecs::OnUpdate);

s_registry.input_pipeline = world.pipeline()
    .with(flecs::System)
    .with(flecs::DependsOn, s_registry.input_phase)
    .build();
```

### 3. Add Accessor Function

```cpp
flecs::entity get_input_phase(flecs::world& world) {
    if (!s_registry.initialized) {
        Log::info("[Polaris::Pipelines] WARNING: Pipelines not initialized");
        return flecs::entity();
    }
    return s_registry.input_phase;
}

void run_input_pipeline(flecs::world& world, float delta) {
    if (!s_registry.initialized) return;
    world.set_pipeline(s_registry.input_pipeline);
    world.progress(delta);
}
```

### 4. Call from Appropriate Location

```cpp
// In TickerNode or wherever appropriate
void _input(const Ref<InputEvent>& event) {
    run_input_pipeline(*m_world, 0.0f);
}
```

## Best Practices

### Choose the Right Pipeline

| Use Case | Pipeline | Why |
|----------|----------|-----|
| Movement, physics, collision | Physics | Fixed timestep, deterministic |
| Gravity, jumping, friction | Physics | Consistent behavior |
| Visual interpolation | Process | Smooth at any framerate |
| UI updates | Process | Responds to render rate |
| Camera smoothing | Process | Visual feedback |
| Animation blending | Process | Smooth transitions |

### Accessing Delta Time

Always get delta from the appropriate singleton:

```cpp
// In physics systems
const auto* pf = world.get<PhysicsFrame>();
float delta = pf->delta;  // Fixed ~0.0167

// In process systems
const auto* pf = world.get<ProcessFrame>();
float delta = pf->delta;  // Variable, depends on framerate
```

### System Ordering Within a Pipeline

Systems within the same pipeline run in registration order by default. For explicit ordering, use Flecs phases:

```cpp
// Create ordered phases within physics
auto early_physics = world.entity()
    .add(flecs::Phase)
    .add(flecs::DependsOn, physics_phase);

auto late_physics = world.entity()
    .add(flecs::Phase)
    .add(flecs::DependsOn, early_physics);

// Register systems to specific sub-phases
world.system("Input").kind(early_physics).each(/* ... */);
world.system("Movement").kind(early_physics).each(/* ... */);
world.system("Apply").kind(late_physics).each(/* ... */);
```

## Migration from Manual Phase Checks

### Before (Manual Checking)

```cpp
world.system("OldSystem")
    .kind(flecs::OnUpdate)
    .run([](flecs::iter& it) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* phase = world.get<CurrentPhase>();
        if (phase->phase != FramePhase::Physics) {
            return;
        }

        const auto* pf = world.get<PhysicsFrame>();
        // ... system logic
    });
```

### After (Pipeline-Based)

```cpp
auto physics_phase = Polaris::System::get_physics_phase(world);

world.system("NewSystem")
    .kind(physics_phase)
    .run([](flecs::iter& it) {
        auto& world = Polaris::PolarisEngine::get_singleton()->get_world();
        const auto* pf = world.get<PhysicsFrame>();
        // ... system logic (no phase check needed)
    });
```

## Debugging

Enable Polaris debug logging to see pipeline activity:

```gdscript
Polaris.debug_enabled = true
```

Output:
```
[Polaris::Pipelines] Initializing custom pipelines...
[Polaris::Pipelines] Physics and Process pipelines created
```

To verify systems are running in the correct pipeline, add logging:

```cpp
world.system("DebugPhysics")
    .kind(physics_phase)
    .run([](flecs::iter& it) {
        static int count = 0;
        if (++count % 60 == 0) {
            Log::info("[Physics] Frame ", count);
        }
    });
```
