#ifndef CLAY_WIDGET_SCRIPT_H
#define CLAY_WIDGET_SCRIPT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include "clay.h"

namespace godot {

class ClayWidget;
class Clay2DCanvas;
class Clay3DCanvas;

/**
 * ClayWidgetScript - Base class for Clay UI component scripts
 *
 * Attach this as a child of a ClayWidget to add logic/behavior.
 * The parent widget will automatically find and call this script's methods.
 *
 * This allows you to separate UI building logic from widget definitions,
 * making it easy to create reusable behaviors that can be attached to any widget.
 *
 * Example:
 *   class ButtonLogic : public ClayWidgetScript {
 *       GDCLASS(ButtonLogic, ClayWidgetScript);
 *   public:
 *       void on_build() override {
 *           if (is_hovered() && was_clicked()) {
 *               // Handle click
 *           }
 *       }
 *   };
 */
class ClayWidgetScript : public Node {
    GDCLASS(ClayWidgetScript, Node);

private:
    ClayWidget* parent_widget = nullptr;
    bool enabled = true;

protected:
    static void _bind_methods();

public:
    ClayWidgetScript();
    virtual ~ClayWidgetScript();

    // Godot lifecycle
    void _ready() override;
    void _enter_tree() override;
    void _exit_tree() override;

    // === Script Lifecycle (override these) ===

    // Called once when the script is ready
    virtual void on_start();

    // Called before the widget's build() - use for state updates
    virtual void on_pre_build(double delta);

    // Called during the widget's build() - add your Clay UI here
    virtual void on_build();

    // Called after the widget's build() completes
    virtual void on_post_build();

    // === State ===
    void set_enabled(bool p_enabled);
    bool is_enabled() const;

    // === Parent Widget Access ===
    ClayWidget* get_widget() const;
    Clay2DCanvas* get_canvas_2d() const;
    Clay3DCanvas* get_canvas_3d() const;

    // === Helper Methods (forwarded from parent widget) ===
    bool is_hovered() const;
    bool is_element_hovered(const String& element_id) const;
    Vector2 get_mouse_position() const;
    bool is_mouse_pressed() const;
    bool was_clicked() const;

    // === Font/Texture Helpers ===
    Ref<Font> get_font(int font_id) const;
    Ref<Texture2D> get_texture(int texture_id) const;

private:
    void find_parent_widget();
};

} // namespace godot

#endif // CLAY_WIDGET_SCRIPT_H
