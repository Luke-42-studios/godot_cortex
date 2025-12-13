# CortexFramework

A native C++ game framework for Godot using the **CNS (Context-Node-System)** architecture.

## Overview

CortexFramework lets you build game logic entirely in C++ while leveraging Godot as the runtime. Instead of attaching GDScript to nodes, you attach **Contexts** - native C++ classes that define behavior.

### Core Technologies

- **Godot 4.2+** - Game engine runtime
- **Flecs** - High-performance Entity Component System
- **Clay** - Immediate-mode 2D UI rendering
- **C++20** - Modern C++ features

## CNS Architecture

CNS (Context-Node-System) maps familiar ECS concepts to Godot's node-based workflow:

| CNS | ECS Equivalent | Description |
|-----|----------------|-------------|
| **Context** | Component | Swappable behavior attached to nodes |
| **Node** | Entity | Godot node that hosts contexts |
| **System** | System | Flecs systems that process entities |

### How It Works

```
┌─────────────────────────────────────────────────────────────┐
│  ClayButtonNode (the visual widget)                         │
│  ─────────────────────────────────────────────────────────  │
│  Properties: label, colors, font_size, padding              │
│                                                             │
│  Interface (defined by node):                               │
│    • on_pressed()                                           │
│    • on_released()                                          │
│    • on_hover_enter()                                       │
│    • on_hover_exit()                                        │
│                                                             │
│  context: Ref<ClayButtonContext>  ◄── assign in editor      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  StartGameContext : ClayButtonContext                       │
│  ─────────────────────────────────────────────────────────  │
│  get_node<ClayButtonNode>() → typed access to owner         │
│                                                             │
│  void on_pressed() override {                               │
│      print("Starting game!");                               │
│      // Access button properties                            │
│      auto* btn = get_node<ClayButtonNode>();                │
│      print(btn->get_label());                               │
│  }                                                          │
└─────────────────────────────────────────────────────────────┘
```

## Example: Start/Quit Buttons

Here's how to create two buttons with different behaviors using the same node type:

### 1. The Node (ClayButtonNode)

The node defines the visual appearance and interface:

```cpp
// clay_button_node.h
class ClayButtonNode : public Control {
    GDCLASS(ClayButtonNode, Control);

    // Properties exposed to editor
    GD_PROPERTY(String, label, "Button")
    GD_PROPERTY(Color, normal_color, Color(0.2f, 0.2f, 0.2f, 1.0f))
    GD_PROPERTY(Color, hover_color, Color(0.3f, 0.3f, 0.3f, 1.0f))

private:
    Ref<ClayButtonContext> context;

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(ClayButtonNode, String, label);
        GD_BIND_PROPERTY(ClayButtonNode, Color, normal_color);

        // Only ClayButtonContext subclasses appear in dropdown
        GD_BIND_CONTEXT(ClayButtonNode, ClayButtonContext, context);
    }
};
```

### 2. The Base Context (ClayButtonContext)

Defines the interface that all button behaviors must implement:

```cpp
// clay_button_node.h
class ClayButtonContext : public NodeContext {
    GDCLASS(ClayButtonContext, NodeContext);

public:
    // Typed access to owner - use get_node<T>() for any type
    ClayButtonNode* get_button() const {
        return get_node<ClayButtonNode>();
    }

    // Interface - override these in subclasses
    virtual void on_pressed() {}
    virtual void on_released() {}
    virtual void on_hover_enter() {}
    virtual void on_hover_exit() {}
};
```

### 3. StartGameContext

Implements "Start Game" behavior:

```cpp
// start_game_context.h
class StartGameContext : public ClayButtonContext {
    GDCLASS(StartGameContext, ClayButtonContext);

    // Context-specific properties (editable in inspector)
    GD_PROPERTY(String, game_scene, "res://scenes/game.tscn")
    GD_PROPERTY(bool, show_loading, true)

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(StartGameContext, String, game_scene);
        GD_BIND_PROPERTY(StartGameContext, bool, show_loading);
        GD_BIND_SIGNAL(game_starting);
    }

public:
    void on_pressed() override {
        UtilityFunctions::print("========================================");
        UtilityFunctions::print("  STARTING GAME!");
        UtilityFunctions::print("  Loading scene: ", _game_scene);
        UtilityFunctions::print("========================================");

        emit_signal("game_starting");

        // Access button through generic get_node<T>()
        ClayButtonNode* btn = get_node<ClayButtonNode>();
        if (btn) {
            UtilityFunctions::print("  Button label: ", btn->get_label());
        }
    }

    void on_hover_enter() override {
        UtilityFunctions::print("[StartGame] Ready to start!");
    }
};
```

### 4. QuitGameContext

Implements "Quit Game" behavior:

```cpp
// quit_game_context.h
class QuitGameContext : public ClayButtonContext {
    GDCLASS(QuitGameContext, ClayButtonContext);

    GD_PROPERTY(bool, confirm_quit, true)
    GD_PROPERTY(String, quit_message, "Thanks for playing!")

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(QuitGameContext, bool, confirm_quit);
        GD_BIND_PROPERTY(QuitGameContext, String, quit_message);
        GD_BIND_SIGNAL(game_quitting);
    }

public:
    void on_pressed() override {
        UtilityFunctions::print("========================================");
        UtilityFunctions::print("  QUITTING GAME!");
        UtilityFunctions::print("  Message: ", _quit_message);
        UtilityFunctions::print("========================================");

        emit_signal("game_quitting");

        // Could actually quit:
        // get_node<ClayButtonNode>()->get_tree()->quit();
    }

    void on_hover_enter() override {
        UtilityFunctions::print("[QuitGame] Are you sure?");
    }
};
```

### 5. Using in Godot Editor

1. Add a `ClayButtonNode` to your scene
2. Set `label` to "Start Game"
3. Click the `context` dropdown → **New StartGameContext**
4. Configure context properties (`game_scene`, `show_loading`)

Add another button:
1. Add another `ClayButtonNode`
2. Set `label` to "Quit"
3. Click `context` dropdown → **New QuitGameContext**
4. Configure its properties (`confirm_quit`, `quit_message`)

**Same node, different context = different behavior!**

## Helper Macros

CortexFramework provides macros to reduce boilerplate:

```cpp
// Declare property with auto getter/setter
GD_PROPERTY(String, message, "Hello!")
GD_PROPERTY(int, health, 100)
GD_PROPERTY(float, speed, 5.0f)

// Bind properties in _bind_methods()
GD_BIND_PROPERTY(MyClass, String, message);
GD_BIND_PROPERTY_HINT(MyClass, float, speed, PROPERTY_HINT_RANGE, "0,100,0.1");

// Bind methods
GD_BIND_METHOD(MyClass, do_something);
GD_BIND_METHOD_1(MyClass, take_damage, amount);

// Bind signals
GD_BIND_SIGNAL(died);
GD_BIND_SIGNAL_1(damaged, Variant::INT, amount);

// Bind context property (filters dropdown to specific context type)
GD_BIND_CONTEXT(MyNode, MyContext, context);
```

## Building

### Requirements

- Python 3.x
- SCons
- godot-cpp (in `godot-cpp` subdirectory)
- C++20 compiler (MSVC, GCC, or Clang)

### Build Commands

```bash
# Debug build
scons platform=windows target=template_debug

# Release build
scons platform=windows target=template_release

# Linux
scons platform=linux target=template_debug

# macOS
scons platform=macos target=template_debug
```

## Project Structure

```
cortex/
├── src/
│   ├── cnode.h                 # Base NodeContext class
│   ├── gd_macros.h             # Helper macros
│   ├── clay_button_node.h      # Button widget + ClayButtonContext
│   ├── start_game_context.h    # Start game behavior
│   ├── quit_game_context.h     # Quit game behavior
│   ├── option_menu_context.h   # Complex context example (.h)
│   ├── option_menu_context.cpp # Complex context example (.cpp)
│   ├── flecs_world.h/cpp       # Flecs ECS singleton
│   └── register_types.cpp      # Class registration
├── flecs/                      # Flecs ECS library
├── godot-cpp/                  # Godot C++ bindings
├── demo/bin/                   # Built libraries
│   └── cortex.gdextension      # Godot extension config
├── SConstruct                  # Build script
└── README.md
```

## License

MIT License
