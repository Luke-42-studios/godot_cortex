#ifndef CLAY_WIDGET_H
#define CLAY_WIDGET_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>

// Include Clay header (without implementation)
#include "clay.h"

namespace godot {

class Clay2DCanvas;
class Clay3DCanvas;
class ClayWidgetScript;
class ClayDebugElement;

/**
 * ClayWidget - Base class for building Clay UI in C++
 *
 * Subclass this to create your own UI widgets. Override build() to define
 * your UI using Clay's declarative macros.
 *
 * Usage:
 * 1. Create a class that extends ClayWidget
 * 2. Override the build() method
 * 3. Add your widget as a child of a Clay2DCanvas or Clay3DCanvas node
 *
 * Example:
 *   class MyWidget : public ClayWidget {
 *       GDCLASS(MyWidget, ClayWidget);
 *   public:
 *       void build() override {
 *           CLAY(CLAY_ID("MyBox"), { .backgroundColor = {255,0,0,255} }) {
 *               CLAY_TEXT(CLAY_STRING("Hello!"), CLAY_TEXT_CONFIG({ .textColor = {255,255,255,255}, .fontSize = 16 }));
 *           }
 *       }
 *   };
 */
class ClayWidget : public Node {
    GDCLASS(ClayWidget, Node);

private:
    Clay2DCanvas* parent_canvas_2d = nullptr;
    Clay3DCanvas* parent_canvas_3d = nullptr;
    bool enabled = true;
    Vector<ClayWidgetScript*> scripts;
    bool scripts_dirty = true;

    // Debug visualization
    bool debug_enabled = false;
    Node* debug_container = nullptr;
    HashMap<uint32_t, ClayDebugElement*> debug_elements;

protected:
    static void _bind_methods();

    // Collect all ClayWidgetScript children
    void collect_scripts();

    // Call script hooks
    void call_script_pre_build(double delta);
    void call_script_build();
    void call_script_post_build();

public:
    ClayWidget();
    virtual ~ClayWidget();

    // Godot lifecycle
    void _ready() override;
    void _notification(int p_what);

    // Called by ClayUI to build this widget's UI
    // Override this in subclasses to define your UI
    virtual void build();

    // Called before build() each frame - use for state updates
    virtual void pre_build(double delta);

    // Mark scripts as needing to be recollected (call when children change)
    void invalidate_scripts();

    // === State ===
    void set_enabled(bool p_enabled);
    bool is_enabled() const;

    // === Parent Access ===
    void set_parent_canvas(Clay2DCanvas* canvas);
    void set_parent_canvas_3d(Clay3DCanvas* canvas);
    Clay2DCanvas* get_parent_canvas() const;
    Clay3DCanvas* get_parent_canvas_3d() const;

    // === Helper Methods ===
    // Check if mouse is over an element (call during build)
    bool is_hovered() const;

    // Check if mouse is over a specific element by ID
    bool is_element_hovered(const String& element_id) const;

    // Get mouse position relative to the ClayUI
    Vector2 get_mouse_position() const;

    // Check if mouse button is currently held down
    bool is_mouse_pressed() const;

    // Check if mouse was clicked THIS FRAME (use for button clicks)
    bool was_clicked() const;

    // === Font/Texture Helpers ===
    // These forward to the parent canvas (2D or 3D)
    Ref<Font> get_font(int font_id) const;
    Ref<Texture2D> get_texture(int texture_id) const;

    // === Debug Visualization ===
    void set_debug_enabled(bool enabled);
    bool is_debug_enabled() const;

    // Called after layout to update debug elements from render commands
    void update_debug_elements(Clay_RenderCommandArray* render_commands);

    // Clear all debug elements
    void clear_debug_elements();

private:
    void ensure_debug_container();
    ClayDebugElement* get_or_create_debug_element(uint32_t element_id);
    String get_render_command_type_name(int type) const;
    void update_debug_owners();
};

/**
 * ClayPanel - A simple container widget with background and padding
 *
 * Add other ClayWidget children to this to create panels/sections.
 */
class ClayPanel : public ClayWidget {
    GDCLASS(ClayPanel, ClayWidget);

private:
    Color background_color = Color(0.15f, 0.15f, 0.17f, 1.0f);
    Color border_color = Color(0.3f, 0.3f, 0.35f, 1.0f);
    float border_width = 0.0f;
    float corner_radius = 8.0f;
    int padding = 16;
    int child_gap = 8;
    bool vertical_layout = true;
    String panel_id = "Panel";

protected:
    static void _bind_methods();

public:
    ClayPanel();

    void build() override;

    void set_background_color(const Color& color);
    Color get_background_color() const;

    void set_border_color(const Color& color);
    Color get_border_color() const;

    void set_border_width(float width);
    float get_border_width() const;

    void set_corner_radius(float radius);
    float get_corner_radius() const;

    void set_padding(int p_padding);
    int get_padding() const;

    void set_child_gap(int gap);
    int get_child_gap() const;

    void set_vertical_layout(bool vertical);
    bool get_vertical_layout() const;

    void set_panel_id(const String& id);
    String get_panel_id() const;
};

} // namespace godot

#endif // CLAY_WIDGET_H
