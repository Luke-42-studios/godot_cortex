#ifndef CLAY_2D_CANVAS_H
#define CLAY_2D_CANVAS_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/templates/vector.hpp>

// Forward declare Clay types
struct Clay_Arena;
struct Clay_RenderCommandArray;

namespace godot {

/**
 * Clay2DCanvas - High-performance UI layout using Clay library
 *
 * Clay is a microsecond-performance UI layout library that outputs
 * render commands which we draw using Godot's CanvasItem API.
 *
 * Features:
 * - Flexbox-like layout model
 * - Declarative C++ API
 * - Automatic text wrapping
 * - Scrolling containers
 * - Floating elements
 */
class Clay2DCanvas : public Control {
    GDCLASS(Clay2DCanvas, Control);

private:
    // Clay memory arena
    void* clay_memory = nullptr;
    uint32_t clay_memory_size = 0;
    bool clay_initialized = false;

    // Fonts
    Ref<Font> default_font;
    Dictionary fonts; // fontId -> Font
    int default_font_size = 16;

    // Images/Textures
    Dictionary textures; // pointer/id -> Texture2D

    // State
    bool needs_layout = true;
    Vector2 last_size;
    Vector2 mouse_position;
    bool mouse_down = false;
    bool mouse_pressed_this_frame = false;
    bool mouse_released_this_frame = false;

    // Scroll state
    Vector2 scroll_delta;

    // Clip stack for scissoring
    Vector<Rect2> clip_stack;

    // Debug mode
    bool debug_show_elements = false;
    Node* debug_container = nullptr;

protected:
    static void _bind_methods();

public:
    Clay2DCanvas();
    ~Clay2DCanvas();

    void _ready() override;
    void _process(double delta) override;
    void _draw() override;
    void _gui_input(const Ref<InputEvent> &event) override;
    void _notification(int p_what);

    // === Initialization ===
    void initialize_clay();
    void shutdown_clay();
    bool is_clay_initialized() const;

    // === Font Management ===
    void set_default_font(const Ref<Font> &font);
    Ref<Font> get_default_font() const;
    void register_font(int font_id, const Ref<Font> &font);
    Ref<Font> get_font(int font_id) const;

    void set_default_font_size(int size);
    int get_default_font_size() const;

    // === Texture Management ===
    void register_texture(int texture_id, const Ref<Texture2D> &texture);
    Ref<Texture2D> get_texture(int texture_id) const;

    // === Layout ===
    void request_layout();
    void begin_layout();
    void end_layout();

    // === Input State (for widgets) ===
    bool is_mouse_down() const;
    bool was_mouse_pressed() const;  // True only on the frame mouse was clicked
    bool was_mouse_released() const; // True only on the frame mouse was released

    // === Element Building API (C++ side) ===
    // These are called from C++ code to build the UI

    // Called by subclasses or C++ code to define the UI
    virtual void build_ui();

    // Build child widgets
    void build_child_widgets();

private:
    void render_clay_commands();
    void draw_rectangle(const Rect2 &rect, const Color &color, float corner_radius = 0.0f);
    void draw_border(const Rect2 &rect, const Color &color, float width, float corner_radius = 0.0f);
    void draw_text_clay(const Rect2 &rect, const String &text, const Color &color, int font_id, int font_size);
    void draw_image(const Rect2 &rect, int texture_id, const Color &tint);

    // Clipping helpers
    bool is_rect_visible(const Rect2 &rect) const;
    Rect2 clip_rect(const Rect2 &rect) const;
    Rect2 get_current_clip() const;

    // Debug helpers
    void update_widget_debug_elements(Clay_RenderCommandArray* commands);
};

} // namespace godot

#endif // CLAY_2D_CANVAS_H
