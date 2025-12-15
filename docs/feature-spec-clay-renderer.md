# Feature Specification: Clay Render System for Godot

## Overview

A high-performance UI rendering system that integrates the Clay layout library with Godot's CanvasItem drawing API, using Flecs ECS for component management and system execution.

## Architecture

### Core Components

| Type | Name | Description |
|:-----|:-----|:------------|
| **S** | SClayRenderer | System that processes Clay render commands and draws to Godot CanvasItem |
| **C** | CClayLayout | Base component that stores Clay layout state and provides element declaration API |
| **P** | PClayContext | Context struct holding Clay arena, fonts, and configuration |

### Data Flow

```
┌─────────────────┐    ┌──────────────┐    ┌───────────────┐    ┌─────────────┐
│ CClayLayout     │───▶│ Flecs System │───▶│ Clay_EndLayout│───▶│SClayRenderer│
│ Components      │    │ (collect)    │    │ (compute)     │    │ (draw)      │
└─────────────────┘    └──────────────┘    └───────────────┘    └─────────────┘
```

### Render Command Translation

| Clay Command | Godot API |
|:-------------|:----------|
| CLAY_RENDER_COMMAND_TYPE_RECTANGLE | CanvasItem::draw_rect() + StyleBoxFlat for corners |
| CLAY_RENDER_COMMAND_TYPE_BORDER | CanvasItem::draw_rect() borders + draw_arc() corners |
| CLAY_RENDER_COMMAND_TYPE_TEXT | CanvasItem::draw_string() |
| CLAY_RENDER_COMMAND_TYPE_IMAGE | CanvasItem::draw_texture_rect() |
| CLAY_RENDER_COMMAND_TYPE_SCISSOR_START | RenderingServer viewport clipping |
| CLAY_RENDER_COMMAND_TYPE_SCISSOR_END | Restore previous clip state |
| CLAY_RENDER_COMMAND_TYPE_CUSTOM | User-defined callback |

## Component Design

### CClayLayout (Base Component)

```cpp
struct CClayLayout {
    bool enabled = true;
    int16_t zIndex = 0;
    
    // Virtual method for layout declaration
    virtual void declare_layout() = 0;
};
```

### CGrayBackground (Example Component)

```cpp
struct CGrayBackground : public CClayLayout {
    Clay_Color backgroundColor = {128, 128, 128, 255};
    
    void declare_layout() override {
        CLAY(CLAY_ID("GrayBackground"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }
            },
            .backgroundColor = backgroundColor
        }) {}
    }
};
```

## System Design

### SClayRenderer

**Responsibilities:**
1. Initialize Clay context with Godot font measurement
2. Query all CClayLayout components each frame
3. Call Clay_BeginLayout() → component declarations → Clay_EndLayout()
4. Process render command array and draw to CanvasItem

**Key Methods:**

```cpp
class SClayRenderer {
    static Clay_Dimensions measure_text(Clay_StringSlice text, 
                                         Clay_TextElementConfig* config, 
                                         void* userData);
    
    void initialize(const Ref<Font>& defaultFont);
    void render(CanvasItem* target, flecs::world& world);
    void shutdown();
};
```

### Flecs Integration

```cpp
// System registration in PECSContext
world.system<CClayLayout>("ClayLayoutSystem")
    .kind(flecs::PreStore)
    .each([](flecs::entity e, CClayLayout& layout) {
        if (layout.enabled) {
            layout.declare_layout();
        }
    });
```

## Performance Considerations

### Cache-Friendly Design

- **PERF:** Clay arena is pre-allocated once, reset per-frame (zero allocations in hot path)
- **CACHE:** Font measurement cache prevents redundant text measurement
- **HOT:** Render command processing uses contiguous array iteration

### Memory Layout

```cpp
// PClayContext - singleton, allocated once
struct PClayContext {
    Clay_Arena arena;                    // Pre-allocated memory block
    std::vector<Ref<Font>> fonts;        // Font pool by fontId
    Clay_Dimensions layoutDimensions;    // Viewport size
};
```

## Color Conversion

```cpp
// PERF: Inline conversion, no branching
inline Color clay_to_godot_color(Clay_Color c) {
    return Color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

inline Rect2 clay_to_godot_rect(Clay_BoundingBox box) {
    return Rect2(box.x, box.y, box.width, box.height);
}
```

## Files

| Path | Description |
|:-----|:------------|
| `src/rendering/SClayRenderer.h` | Renderer system header |
| `src/rendering/SClayRenderer.cpp` | Renderer implementation |
| `src/rendering/PClayContext.h` | Clay context singleton |
| `src/rendering/CClayLayout.h` | Base layout component |
| `src/rendering/components/CGrayBackground.h` | Example gray background |

## Usage Example

```cpp
// In game initialization
auto& world = PECSContext::get_singleton()->get_world();

// Create entity with gray background
auto panel = world.entity("MainPanel")
    .set<CGrayBackground>({ .backgroundColor = {64, 64, 64, 255} });

// In _process or render callback
SClayRenderer::get_singleton()->render(canvas_item, world);
```

## Error Handling

- Clay errors logged via Godot's `UtilityFunctions::push_error()`
- Missing fonts fall back to default Godot font
- Scissor stack overflow protected with max depth limit

## Future Extensions

1. **Input Handling:** Clay_SetPointerState() integration with Godot input events
2. **Scroll Containers:** Clay_UpdateScrollContainers() for scrollable panels
3. **Debug View:** Clay_SetDebugModeEnabled() for layout debugging
4. **Hot Reload:** Component state preservation across scene reloads
