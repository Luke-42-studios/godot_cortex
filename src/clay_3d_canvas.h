#ifndef CLAY_3D_CANVAS_H
#define CLAY_3D_CANVAS_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>

// Forward declare Clay types
struct Clay_Arena;
struct Clay_RenderCommandArray;

namespace godot {

class ClayWidget;

/**
 * Clay3DCanvas - High-performance UI layout in 3D world space
 *
 * Renders Clay UI to a SubViewport texture displayed on a quad mesh.
 * Supports mouse interaction via raycasting from 3D camera.
 *
 * Features:
 * - Same ClayWidget children as Clay2DCanvas
 * - Configurable canvas size and pixel density
 * - Billboard mode option
 * - 3D mouse interaction via raycast
 */
class Clay3DCanvas : public Node3D {
    GDCLASS(Clay3DCanvas, Node3D);

private:
    // Viewport for rendering
    SubViewport* viewport = nullptr;
    MeshInstance3D* mesh_instance = nullptr;
    Ref<QuadMesh> quad_mesh;
    Ref<StandardMaterial3D> material;

    // Canvas settings
    Vector2 canvas_size = Vector2(1024, 768);
    float world_scale = 2.0f; // Width of the canvas in 3D world units (meters). Height is calculated from aspect ratio.
    bool billboard = false;
    bool double_sided = true;
    bool transparent_background = false;

    // Clay memory arena
    void* clay_memory = nullptr;
    uint32_t clay_memory_size = 0;
    bool clay_initialized = false;

    // Fonts
    Ref<Font> default_font;
    Dictionary fonts;
    int default_font_size = 16;

    // Textures
    Dictionary textures;

    // State
    bool needs_layout = true;
    Vector2 mouse_position;
    bool mouse_down = false;
    bool mouse_pressed_this_frame = false;
    bool mouse_released_this_frame = false;
    Vector2 scroll_delta;

    // Interaction
    Camera3D* interaction_camera = nullptr;

protected:
    static void _bind_methods();

public:
    Clay3DCanvas();
    ~Clay3DCanvas();

    void _ready() override;
    void _process(double delta) override;
    void _enter_tree() override;
    void _exit_tree() override;

    // === Canvas Settings ===
    void set_canvas_size(const Vector2& size);
    Vector2 get_canvas_size() const;

    void set_world_scale(float scale);
    float get_world_scale() const;

    // Helper to get computed world size
    Vector2 get_world_size() const;

    void set_billboard(bool enabled);
    bool get_billboard() const;

    void set_double_sided(bool enabled);
    bool get_double_sided() const;

    void set_transparent_background(bool enabled);
    bool get_transparent_background() const;

    // === Initialization ===
    void initialize_clay();
    void shutdown_clay();
    bool is_clay_initialized() const;

    // === Font Management ===
    void set_default_font(const Ref<Font>& font);
    Ref<Font> get_default_font() const;
    void register_font(int font_id, const Ref<Font>& font);
    Ref<Font> get_font(int font_id) const;

    void set_default_font_size(int size);
    int get_default_font_size() const;

    // === Texture Management ===
    void register_texture(int texture_id, const Ref<Texture2D>& texture);
    Ref<Texture2D> get_texture(int texture_id) const;

    // === Layout ===
    void request_layout();

    // === Input State (for widgets) ===
    bool is_mouse_down() const;
    bool was_mouse_pressed() const;
    bool was_mouse_released() const;

    // === Interaction ===
    void set_interaction_camera(Camera3D* camera);
    Camera3D* get_interaction_camera() const;

    // Convert 3D ray to canvas position (returns true if hit)
    bool raycast_to_canvas(const Vector3& ray_origin, const Vector3& ray_direction, Vector2& out_canvas_pos);

    // Handle input event (call from _input or _unhandled_input)
    void handle_input_event(const Ref<InputEvent>& event);

    // === UI Building ===
    virtual void build_ui();
    void build_child_widgets();

    // Get the viewport (for advanced use)
    SubViewport* get_viewport() const;

    // Called by internal renderer to draw Clay commands
    void render_to_canvas(Control* target);

    // Called when canvas_renderer needs to draw
    void _on_canvas_draw();

private:
    // Internal renderer Control
    Control* canvas_renderer = nullptr;
    Control* current_render_target = nullptr;

    void setup_viewport();
    void setup_mesh();
    void update_material();
    void update_widget_debug_elements(Clay_RenderCommandArray* commands);
};

} // namespace godot

#endif // CLAY_3D_CANVAS_H
