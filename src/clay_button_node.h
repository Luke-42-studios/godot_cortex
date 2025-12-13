#ifndef CLAY_BUTTON_NODE_H
#define CLAY_BUTTON_NODE_H

#include "cnode.h"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/font.hpp>

namespace godot {

// Forward declaration
class ClayButtonNode;

// ============================================================================
// ClayButtonContext - Base context that knows about ClayButtonNode
// ============================================================================
// The context automatically gets typed access to the button.
// Override the virtual methods to implement behavior.
class ClayButtonContext : public NodeContext {
    GDCLASS(ClayButtonContext, NodeContext);

protected:
    static void _bind_methods() {
        // Expose virtuals to GDScript if needed
        ClassDB::bind_method(D_METHOD("on_pressed"), &ClayButtonContext::on_pressed);
        ClassDB::bind_method(D_METHOD("on_released"), &ClayButtonContext::on_released);
        ClassDB::bind_method(D_METHOD("on_hover_enter"), &ClayButtonContext::on_hover_enter);
        ClassDB::bind_method(D_METHOD("on_hover_exit"), &ClayButtonContext::on_hover_exit);
    }

public:
    ClayButtonContext() {}
    virtual ~ClayButtonContext() {}

    // =========================================================================
    // Convenience accessor for button contexts
    // =========================================================================
    // Shorthand for get_node<ClayButtonNode>()
    ClayButtonNode* get_button() const {
        return get_node<ClayButtonNode>();
    }

    // =========================================================================
    // Button interface - override these in subclasses
    // =========================================================================
    // These match the events that ClayButtonNode emits
    virtual void on_pressed() {}
    virtual void on_released() {}
    virtual void on_hover_enter() {}
    virtual void on_hover_exit() {}
};

// ============================================================================
// ClayButtonNode - A Clay-style button with swappable context
// ============================================================================
// The button defines the interface (pressed, released, hover).
// The context provides the implementation.
class ClayButtonNode : public Control {
    GDCLASS(ClayButtonNode, Control);

    // Button properties exposed to editor
    GD_PROPERTY(String, label, "Button")
    GD_PROPERTY(Color, normal_color, Color(0.2f, 0.2f, 0.2f, 1.0f))
    GD_PROPERTY(Color, hover_color, Color(0.3f, 0.3f, 0.3f, 1.0f))
    GD_PROPERTY(Color, pressed_color, Color(0.1f, 0.1f, 0.1f, 1.0f))
    GD_PROPERTY(Color, text_color, Color(1.0f, 1.0f, 1.0f, 1.0f))
    GD_PROPERTY(int, font_size, 16)
    GD_PROPERTY(float, corner_radius, 4.0f)
    GD_PROPERTY(Vector2, padding, Vector2(16, 8))

private:
    Ref<ClayButtonContext> context;
    bool is_hovered = false;
    bool is_pressed_state = false;

protected:
    static void _bind_methods() {
        // Properties
        GD_BIND_PROPERTY(ClayButtonNode, String, label);
        GD_BIND_PROPERTY(ClayButtonNode, Color, normal_color);
        GD_BIND_PROPERTY(ClayButtonNode, Color, hover_color);
        GD_BIND_PROPERTY(ClayButtonNode, Color, pressed_color);
        GD_BIND_PROPERTY(ClayButtonNode, Color, text_color);
        GD_BIND_PROPERTY(ClayButtonNode, int, font_size);
        GD_BIND_PROPERTY(ClayButtonNode, float, corner_radius);
        GD_BIND_PROPERTY(ClayButtonNode, Vector2, padding);

        // Context property - only shows ClayButtonContext and subclasses in dropdown
        GD_BIND_CONTEXT(ClayButtonNode, ClayButtonContext, context);

        // State accessors
        GD_BIND_METHOD(ClayButtonNode, is_button_hovered);
        GD_BIND_METHOD(ClayButtonNode, is_button_pressed);

        // Signals - these define the interface!
        GD_BIND_SIGNAL(pressed);
        GD_BIND_SIGNAL(released);
        GD_BIND_SIGNAL(hover_entered);
        GD_BIND_SIGNAL(hover_exited);
    }

    // =========================================================================
    // Interface methods - called by input handling, dispatch to context
    // =========================================================================
    void _on_pressed() {
        emit_signal("pressed");
        if (context.is_valid()) {
            context->on_pressed();
        }
    }

    void _on_released() {
        emit_signal("released");
        if (context.is_valid()) {
            context->on_released();
        }
    }

    void _on_hover_enter() {
        emit_signal("hover_entered");
        if (context.is_valid()) {
            context->on_hover_enter();
        }
    }

    void _on_hover_exit() {
        emit_signal("hover_exited");
        if (context.is_valid()) {
            context->on_hover_exit();
        }
    }

public:
    ClayButtonNode() {
        set_mouse_filter(MOUSE_FILTER_STOP);
    }
    ~ClayButtonNode() {}

    // Context getter/setter
    void set_context(const Ref<ClayButtonContext>& p_context) {
        if (context.is_valid()) {
            context->ctx_exit();
            context->_set_owner(nullptr);
        }

        context = p_context;

        if (context.is_valid()) {
            context->_set_owner(this);
            context->ctx_enter();
            if (is_inside_tree()) {
                context->ctx_ready();
            }
        }
    }

    Ref<ClayButtonContext> get_context() const { return context; }

    // State accessors
    bool is_button_hovered() const { return is_hovered; }
    bool is_button_pressed() const { return is_pressed_state; }

    void _ready() override {
        UtilityFunctions::print("ClayButtonNode ready: ", _label);

        if (context.is_valid()) {
            context->_set_owner(this);
            context->ctx_ready();
        }
    }

    void _process(double delta) override {
        if (context.is_valid()) {
            context->ctx_process(delta);
        }
    }

    void _draw() override {
        Color bg_color = _normal_color;
        if (is_pressed_state) {
            bg_color = _pressed_color;
        } else if (is_hovered) {
            bg_color = _hover_color;
        }

        Rect2 rect(Vector2(), get_size());
        draw_rect(rect, bg_color);

        Ref<Font> font = get_theme_default_font();
        if (font.is_valid()) {
            Vector2 text_size = font->get_string_size(_label, HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size);
            Vector2 text_pos = (get_size() - text_size) / 2;
            text_pos.y += text_size.y * 0.75f;
            draw_string(font, text_pos, _label, HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size, _text_color);
        }
    }

    void _gui_input(const Ref<InputEvent>& event) override {
        Ref<InputEventMouseButton> mb = event;
        if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
            if (mb->is_pressed()) {
                is_pressed_state = true;
                _on_pressed();  // Dispatch to interface
                queue_redraw();
            } else {
                is_pressed_state = false;
                _on_released();  // Dispatch to interface
                queue_redraw();
            }
            accept_event();
        }

        if (context.is_valid()) {
            context->ctx_input(event);
        }
    }

    void _notification(int p_what) {
        switch (p_what) {
            case NOTIFICATION_MOUSE_ENTER:
                is_hovered = true;
                _on_hover_enter();  // Dispatch to interface
                queue_redraw();
                break;

            case NOTIFICATION_MOUSE_EXIT:
                is_hovered = false;
                is_pressed_state = false;
                _on_hover_exit();  // Dispatch to interface
                queue_redraw();
                break;
        }
    }

    Vector2 _get_minimum_size() const {
        Ref<Font> font = get_theme_default_font();
        if (font.is_valid()) {
            Vector2 text_size = font->get_string_size(_label, HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size);
            return text_size + _padding * 2;
        }
        return Vector2(100, 40);
    }
};

} // namespace godot

#endif // CLAY_BUTTON_NODE_H
