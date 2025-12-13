#include "clay_3d_canvas.h"
#include "clay.h"
#include "clay_widget.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

using namespace godot;

// Internal Control class for rendering Clay commands to the SubViewport
class Clay3DCanvasRenderer : public Control {
    GDCLASS(Clay3DCanvasRenderer, Control);

public:
    Clay3DCanvas* owner = nullptr;

    static void _bind_methods() {}

    void _draw() override {
        if (owner) {
            owner->render_to_canvas(this);
        }
    }
};

// Global pointer to current Clay3DCanvas instance for callbacks
static Clay3DCanvas* g_current_clay_3d_canvas = nullptr;

// Clay error handler for 3D canvas
static void clay_3d_error_handler(Clay_ErrorData error) {
    UtilityFunctions::printerr("Clay3D Error: ", String(error.errorText.chars));
}

// Clay text measurement callback for 3D canvas
static Clay_Dimensions measure_text_3d_callback(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    Clay3DCanvas* canvas = static_cast<Clay3DCanvas*>(userData);
    if (!canvas) {
        return Clay_Dimensions{ (float)text.length * config->fontSize * 0.6f, (float)config->fontSize };
    }

    Ref<Font> font = canvas->get_font(config->fontId);
    if (!font.is_valid()) {
        font = canvas->get_default_font();
    }

    if (!font.is_valid()) {
        return Clay_Dimensions{ (float)text.length * config->fontSize * 0.6f, (float)config->fontSize };
    }

    String str;
    if (text.length > 0 && text.chars != nullptr) {
        str = String::utf8(text.chars, text.length);
    }

    Vector2 size = font->get_string_size(str, HORIZONTAL_ALIGNMENT_LEFT, -1, config->fontSize);
    float height = config->lineHeight > 0 ? config->lineHeight : config->fontSize;

    return Clay_Dimensions{ size.x, height };
}

void Clay3DCanvas::_bind_methods() {
    // Canvas settings
    ClassDB::bind_method(D_METHOD("set_canvas_size", "size"), &Clay3DCanvas::set_canvas_size);
    ClassDB::bind_method(D_METHOD("get_canvas_size"), &Clay3DCanvas::get_canvas_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "canvas_size"), "set_canvas_size", "get_canvas_size");

    ClassDB::bind_method(D_METHOD("set_world_scale", "scale"), &Clay3DCanvas::set_world_scale);
    ClassDB::bind_method(D_METHOD("get_world_scale"), &Clay3DCanvas::get_world_scale);
    ClassDB::bind_method(D_METHOD("get_world_size"), &Clay3DCanvas::get_world_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "world_scale"), "set_world_scale", "get_world_scale");

    ClassDB::bind_method(D_METHOD("set_billboard", "enabled"), &Clay3DCanvas::set_billboard);
    ClassDB::bind_method(D_METHOD("get_billboard"), &Clay3DCanvas::get_billboard);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "billboard"), "set_billboard", "get_billboard");

    ClassDB::bind_method(D_METHOD("set_double_sided", "enabled"), &Clay3DCanvas::set_double_sided);
    ClassDB::bind_method(D_METHOD("get_double_sided"), &Clay3DCanvas::get_double_sided);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "double_sided"), "set_double_sided", "get_double_sided");

    ClassDB::bind_method(D_METHOD("set_transparent_background", "enabled"), &Clay3DCanvas::set_transparent_background);
    ClassDB::bind_method(D_METHOD("get_transparent_background"), &Clay3DCanvas::get_transparent_background);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transparent_background"), "set_transparent_background", "get_transparent_background");

    // Initialization
    ClassDB::bind_method(D_METHOD("initialize_clay"), &Clay3DCanvas::initialize_clay);
    ClassDB::bind_method(D_METHOD("shutdown_clay"), &Clay3DCanvas::shutdown_clay);
    ClassDB::bind_method(D_METHOD("is_clay_initialized"), &Clay3DCanvas::is_clay_initialized);

    // Font management
    ClassDB::bind_method(D_METHOD("set_default_font", "font"), &Clay3DCanvas::set_default_font);
    ClassDB::bind_method(D_METHOD("get_default_font"), &Clay3DCanvas::get_default_font);
    ClassDB::bind_method(D_METHOD("register_font", "font_id", "font"), &Clay3DCanvas::register_font);
    ClassDB::bind_method(D_METHOD("get_font", "font_id"), &Clay3DCanvas::get_font);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_font", PROPERTY_HINT_RESOURCE_TYPE, "Font"), "set_default_font", "get_default_font");

    ClassDB::bind_method(D_METHOD("set_default_font_size", "size"), &Clay3DCanvas::set_default_font_size);
    ClassDB::bind_method(D_METHOD("get_default_font_size"), &Clay3DCanvas::get_default_font_size);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_font_size", PROPERTY_HINT_RANGE, "8,72,1"), "set_default_font_size", "get_default_font_size");

    // Texture management
    ClassDB::bind_method(D_METHOD("register_texture", "texture_id", "texture"), &Clay3DCanvas::register_texture);
    ClassDB::bind_method(D_METHOD("get_texture", "texture_id"), &Clay3DCanvas::get_texture);

    // Layout
    ClassDB::bind_method(D_METHOD("request_layout"), &Clay3DCanvas::request_layout);

    // Interaction
    ClassDB::bind_method(D_METHOD("set_interaction_camera", "camera"), &Clay3DCanvas::set_interaction_camera);
    ClassDB::bind_method(D_METHOD("get_interaction_camera"), &Clay3DCanvas::get_interaction_camera);
    ClassDB::bind_method(D_METHOD("handle_input_event", "event"), &Clay3DCanvas::handle_input_event);

    // Internal
    ClassDB::bind_method(D_METHOD("_on_canvas_draw"), &Clay3DCanvas::_on_canvas_draw);
}

Clay3DCanvas::Clay3DCanvas() {
}

Clay3DCanvas::~Clay3DCanvas() {
    shutdown_clay();
}

void Clay3DCanvas::_ready() {
    // _ready is called in both editor and runtime
    setup_viewport();
    setup_mesh();

    if (!default_font.is_valid()) {
        default_font = ThemeDB::get_singleton()->get_fallback_font();
    }

    initialize_clay();
    needs_layout = true;

    // Force initial redraw
    if (canvas_renderer) {
        canvas_renderer->queue_redraw();
    }
}

void Clay3DCanvas::_enter_tree() {
    // Enable processing in editor to render the canvas
    set_process(true);

    // In editor, _ready may not be called on re-entering tree, so setup here too
    if (Engine::get_singleton()->is_editor_hint()) {
        setup_viewport();
        setup_mesh();

        if (!default_font.is_valid()) {
            default_font = ThemeDB::get_singleton()->get_fallback_font();
        }

        initialize_clay();
        needs_layout = true;

        if (canvas_renderer) {
            canvas_renderer->queue_redraw();
        }
    }
}

void Clay3DCanvas::_exit_tree() {
    // Clean up internal nodes
    if (canvas_renderer) {
        canvas_renderer->queue_free();
        canvas_renderer = nullptr;
    }
    if (viewport) {
        viewport->queue_free();
        viewport = nullptr;
    }
    if (mesh_instance) {
        mesh_instance->queue_free();
        mesh_instance = nullptr;
    }
    quad_mesh.unref();
    material.unref();

    // Shutdown clay so it can be reinitialized on re-enter
    shutdown_clay();
}

void Clay3DCanvas::_process(double delta) {
    // Skip interaction updates in editor (no camera)
    bool in_editor = Engine::get_singleton()->is_editor_hint();

    // Update billboard orientation if enabled
    if (!in_editor && billboard && interaction_camera) {
        Vector3 cam_pos = interaction_camera->get_global_position();
        look_at(cam_pos, Vector3(0, 1, 0));
        rotate_y(Math_PI); // Face the camera
    }

    // Handle mouse interaction if we have a camera (runtime only)
    if (!in_editor && interaction_camera) {
        // Get mouse position from the main window viewport
        Viewport* main_viewport = get_tree()->get_root();
        if (main_viewport) {
            Vector2 mouse_pos = main_viewport->get_mouse_position();

            Vector3 ray_origin = interaction_camera->project_ray_origin(mouse_pos);
            Vector3 ray_dir = interaction_camera->project_ray_normal(mouse_pos);

            Vector2 canvas_pos;
            if (raycast_to_canvas(ray_origin, ray_dir, canvas_pos)) {
                if (canvas_pos != mouse_position) {
                    mouse_position = canvas_pos;
                    needs_layout = true;
                }
            }
        }
    }

    // Always render in editor, or when layout needed at runtime
    if (in_editor || needs_layout) {
        if (canvas_renderer) {
            canvas_renderer->queue_redraw();
        }
    }
}

void Clay3DCanvas::setup_viewport() {
    // Avoid recreating if already exists
    if (viewport) {
        return;
    }

    // Create SubViewport as child
    viewport = memnew(SubViewport);
    viewport->set_name("_Clay3DViewport");
    viewport->set_size(Vector2i((int)canvas_size.x, (int)canvas_size.y));
    viewport->set_transparent_background(transparent_background);
    viewport->set_update_mode(SubViewport::UPDATE_ALWAYS);
    viewport->set_clear_mode(SubViewport::CLEAR_MODE_ALWAYS);
    viewport->set_handle_input_locally(false);
    add_child(viewport, false, Node::INTERNAL_MODE_BACK);

    // Create a Control inside the viewport for drawing
    canvas_renderer = memnew(Control);
    canvas_renderer->set_name("_Clay3DRenderer");
    canvas_renderer->set_anchors_preset(Control::PRESET_FULL_RECT);
    canvas_renderer->connect("draw", Callable(this, "_on_canvas_draw"));
    viewport->add_child(canvas_renderer, false, Node::INTERNAL_MODE_BACK);
}

void Clay3DCanvas::setup_mesh() {
    // Avoid recreating if already exists
    if (mesh_instance) {
        return;
    }

    // Create MeshInstance3D
    mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_name("_Clay3DMesh");

    // Create QuadMesh
    quad_mesh.instantiate();
    quad_mesh->set_size(get_world_size());
    mesh_instance->set_mesh(quad_mesh);

    // Create material
    material.instantiate();
    material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
    material->set_cull_mode(double_sided ? StandardMaterial3D::CULL_DISABLED : StandardMaterial3D::CULL_BACK);

    if (transparent_background) {
        material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
    }

    // Set viewport texture as albedo
    if (viewport) {
        Ref<ViewportTexture> vp_tex = viewport->get_texture();
        material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, vp_tex);
    }

    mesh_instance->set_material_override(material);
    add_child(mesh_instance, false, Node::INTERNAL_MODE_BACK);
}

void Clay3DCanvas::update_material() {
    if (material.is_valid()) {
        material->set_cull_mode(double_sided ? StandardMaterial3D::CULL_DISABLED : StandardMaterial3D::CULL_BACK);

        if (transparent_background) {
            material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
        } else {
            material->set_transparency(StandardMaterial3D::TRANSPARENCY_DISABLED);
        }
    }

    if (quad_mesh.is_valid()) {
        quad_mesh->set_size(get_world_size());
    }

    if (viewport) {
        viewport->set_size(Vector2i((int)canvas_size.x, (int)canvas_size.y));
        viewport->set_transparent_background(transparent_background);
    }
}

void Clay3DCanvas::_on_canvas_draw() {
    render_to_canvas(canvas_renderer);
}

void Clay3DCanvas::render_to_canvas(Control* target) {
    if (!clay_initialized || !target) {
        return;
    }

    g_current_clay_3d_canvas = this;
    current_render_target = target;

    // Update Clay state
    Clay_SetLayoutDimensions(Clay_Dimensions{ canvas_size.x, canvas_size.y });
    Clay_SetPointerState(Clay_Vector2{ mouse_position.x, mouse_position.y }, mouse_down);
    Clay_UpdateScrollContainers(true, Clay_Vector2{ scroll_delta.x, scroll_delta.y }, get_process_delta_time());

    scroll_delta = Vector2();

    // Begin layout
    Clay_BeginLayout();

    // Build UI
    build_ui();

    // End layout and get render commands
    Clay_RenderCommandArray commands = Clay_EndLayout();

    // Render all commands
    for (int i = 0; i < commands.length; i++) {
        Clay_RenderCommand* cmd = &commands.internalArray[i];
        Rect2 rect(cmd->boundingBox.x, cmd->boundingBox.y, cmd->boundingBox.width, cmd->boundingBox.height);

        switch (cmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Color color(
                    cmd->renderData.rectangle.backgroundColor.r / 255.0f,
                    cmd->renderData.rectangle.backgroundColor.g / 255.0f,
                    cmd->renderData.rectangle.backgroundColor.b / 255.0f,
                    cmd->renderData.rectangle.backgroundColor.a / 255.0f
                );
                target->draw_rect(rect, color);
            } break;

            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Color color(
                    cmd->renderData.border.color.r / 255.0f,
                    cmd->renderData.border.color.g / 255.0f,
                    cmd->renderData.border.color.b / 255.0f,
                    cmd->renderData.border.color.a / 255.0f
                );
                float width = cmd->renderData.border.width.left;
                if (width > 0) {
                    // Draw border as 4 rectangles
                    target->draw_rect(Rect2(rect.position.x, rect.position.y, rect.size.x, width), color);
                    target->draw_rect(Rect2(rect.position.x, rect.position.y + rect.size.y - width, rect.size.x, width), color);
                    target->draw_rect(Rect2(rect.position.x, rect.position.y + width, width, rect.size.y - width * 2), color);
                    target->draw_rect(Rect2(rect.position.x + rect.size.x - width, rect.position.y + width, width, rect.size.y - width * 2), color);
                }
            } break;

            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                String text;
                if (cmd->renderData.text.stringContents.length > 0) {
                    text = String::utf8(cmd->renderData.text.stringContents.chars, cmd->renderData.text.stringContents.length);
                }
                Color color(
                    cmd->renderData.text.textColor.r / 255.0f,
                    cmd->renderData.text.textColor.g / 255.0f,
                    cmd->renderData.text.textColor.b / 255.0f,
                    cmd->renderData.text.textColor.a / 255.0f
                );

                Ref<Font> font = get_font(cmd->renderData.text.fontId);
                if (!font.is_valid()) {
                    font = default_font;
                }
                if (font.is_valid()) {
                    float ascent = font->get_ascent(cmd->renderData.text.fontSize);
                    Vector2 pos(rect.position.x, rect.position.y + ascent);
                    target->draw_string(font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, rect.size.x, cmd->renderData.text.fontSize, color);
                }
            } break;

            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                Color tint(
                    cmd->renderData.image.backgroundColor.r / 255.0f,
                    cmd->renderData.image.backgroundColor.g / 255.0f,
                    cmd->renderData.image.backgroundColor.b / 255.0f,
                    cmd->renderData.image.backgroundColor.a / 255.0f
                );
                intptr_t tex_id = reinterpret_cast<intptr_t>(cmd->renderData.image.imageData);
                Ref<Texture2D> tex = get_texture((int)tex_id);
                if (tex.is_valid()) {
                    Color actual_tint = tint.a > 0 ? tint : Color(1, 1, 1, 1);
                    target->draw_texture_rect(tex, rect, false, actual_tint);
                }
            } break;

            default:
                break;
        }
    }

    needs_layout = false;

    // Update debug elements on widgets that have debug enabled
    update_widget_debug_elements(&commands);

    g_current_clay_3d_canvas = nullptr;
    current_render_target = nullptr;

    // Reset single-frame flags after render (when widgets have had a chance to check them)
    mouse_pressed_this_frame = false;
    mouse_released_this_frame = false;
}

void Clay3DCanvas::initialize_clay() {
    if (clay_initialized) {
        return;
    }

    clay_memory_size = Clay_MinMemorySize();
    clay_memory = memalloc(clay_memory_size);

    if (!clay_memory) {
        UtilityFunctions::printerr("Clay3DCanvas: Failed to allocate memory for Clay");
        return;
    }

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_memory_size, clay_memory);

    Clay_Initialize(arena, Clay_Dimensions{ canvas_size.x, canvas_size.y }, Clay_ErrorHandler{ clay_3d_error_handler });
    Clay_SetMeasureTextFunction(measure_text_3d_callback, this);

    clay_initialized = true;
    UtilityFunctions::print("Clay3DCanvas: Initialized with ", clay_memory_size, " bytes");
}

void Clay3DCanvas::shutdown_clay() {
    if (clay_memory) {
        memfree(clay_memory);
        clay_memory = nullptr;
        clay_memory_size = 0;
    }
    clay_initialized = false;
}

bool Clay3DCanvas::is_clay_initialized() const {
    return clay_initialized;
}

void Clay3DCanvas::set_canvas_size(const Vector2& size) {
    canvas_size = size;
    update_material();
    needs_layout = true;
    if (canvas_renderer) {
        canvas_renderer->queue_redraw();
    }
}

Vector2 Clay3DCanvas::get_canvas_size() const {
    return canvas_size;
}

void Clay3DCanvas::set_world_scale(float scale) {
    world_scale = scale;
    update_material();
    needs_layout = true;
    if (canvas_renderer) {
        canvas_renderer->queue_redraw();
    }
}

float Clay3DCanvas::get_world_scale() const {
    return world_scale;
}

Vector2 Clay3DCanvas::get_world_size() const {
    // world_scale is the width in meters, height is calculated from aspect ratio
    float aspect = canvas_size.y / canvas_size.x;
    return Vector2(world_scale, world_scale * aspect);
}

void Clay3DCanvas::set_billboard(bool enabled) {
    billboard = enabled;
}

bool Clay3DCanvas::get_billboard() const {
    return billboard;
}

void Clay3DCanvas::set_double_sided(bool enabled) {
    double_sided = enabled;
    update_material();
}

bool Clay3DCanvas::get_double_sided() const {
    return double_sided;
}

void Clay3DCanvas::set_transparent_background(bool enabled) {
    transparent_background = enabled;
    update_material();
}

bool Clay3DCanvas::get_transparent_background() const {
    return transparent_background;
}

void Clay3DCanvas::set_default_font(const Ref<Font>& font) {
    default_font = font;
    needs_layout = true;
}

Ref<Font> Clay3DCanvas::get_default_font() const {
    return default_font;
}

void Clay3DCanvas::register_font(int font_id, const Ref<Font>& font) {
    fonts[font_id] = font;
}

Ref<Font> Clay3DCanvas::get_font(int font_id) const {
    if (fonts.has(font_id)) {
        return fonts[font_id];
    }
    return default_font;
}

void Clay3DCanvas::set_default_font_size(int size) {
    default_font_size = size;
    needs_layout = true;
}

int Clay3DCanvas::get_default_font_size() const {
    return default_font_size;
}

void Clay3DCanvas::register_texture(int texture_id, const Ref<Texture2D>& texture) {
    textures[texture_id] = texture;
}

Ref<Texture2D> Clay3DCanvas::get_texture(int texture_id) const {
    if (textures.has(texture_id)) {
        return textures[texture_id];
    }
    return Ref<Texture2D>();
}

void Clay3DCanvas::request_layout() {
    needs_layout = true;
    if (canvas_renderer) {
        canvas_renderer->queue_redraw();
    }
}

bool Clay3DCanvas::is_mouse_down() const {
    return mouse_down;
}

bool Clay3DCanvas::was_mouse_pressed() const {
    return mouse_pressed_this_frame;
}

bool Clay3DCanvas::was_mouse_released() const {
    return mouse_released_this_frame;
}

void Clay3DCanvas::set_interaction_camera(Camera3D* camera) {
    interaction_camera = camera;
}

Camera3D* Clay3DCanvas::get_interaction_camera() const {
    return interaction_camera;
}

bool Clay3DCanvas::raycast_to_canvas(const Vector3& ray_origin, const Vector3& ray_direction, Vector2& out_canvas_pos) {
    if (!mesh_instance) {
        return false;
    }

    Vector2 ws = get_world_size();

    // Get the plane of the quad in world space
    Transform3D global_transform = get_global_transform();
    Vector3 plane_normal = global_transform.basis.get_column(2); // Z axis is the normal
    Vector3 plane_origin = global_transform.origin;

    // Ray-plane intersection
    float denom = plane_normal.dot(ray_direction);
    if (Math::abs(denom) < 0.0001f) {
        return false;
    }

    float t = (plane_origin - ray_origin).dot(plane_normal) / denom;
    if (t < 0) {
        return false;
    }

    Vector3 hit_point = ray_origin + ray_direction * t;

    // Convert hit point to local space
    Vector3 local_hit = global_transform.affine_inverse().xform(hit_point);

    // Check if within quad bounds
    float half_width = ws.x / 2.0f;
    float half_height = ws.y / 2.0f;

    if (local_hit.x < -half_width || local_hit.x > half_width ||
        local_hit.y < -half_height || local_hit.y > half_height) {
        return false;
    }

    // Convert to canvas coordinates
    out_canvas_pos.x = ((local_hit.x + half_width) / ws.x) * canvas_size.x;
    out_canvas_pos.y = ((half_height - local_hit.y) / ws.y) * canvas_size.y;

    return true;
}

void Clay3DCanvas::handle_input_event(const Ref<InputEvent>& event) {
    // First check if we have a camera and the mouse is over the canvas
    if (!interaction_camera) {
        return;
    }

    Ref<InputEventMouseButton> button = event;
    if (button.is_valid()) {
        // Check if mouse is over canvas
        Vector2 mouse_pos = button->get_position();
        Vector3 ray_origin = interaction_camera->project_ray_origin(mouse_pos);
        Vector3 ray_dir = interaction_camera->project_ray_normal(mouse_pos);

        Vector2 canvas_pos;
        bool on_canvas = raycast_to_canvas(ray_origin, ray_dir, canvas_pos);

        if (button->get_button_index() == MOUSE_BUTTON_LEFT) {
            bool was_down = mouse_down;

            if (button->is_pressed()) {
                // Only register press if on canvas
                if (on_canvas) {
                    mouse_down = true;
                    if (!was_down) {
                        mouse_pressed_this_frame = true;
                        UtilityFunctions::print("Clay3DCanvas: Mouse pressed at ", canvas_pos);
                    }
                }
            } else {
                // Always register release
                mouse_down = false;
                if (was_down) {
                    mouse_released_this_frame = true;
                }
            }

            needs_layout = true;
            if (canvas_renderer) {
                canvas_renderer->queue_redraw();
            }
        }

        if (on_canvas) {
            if (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP) {
                scroll_delta.y += 30.0f;
                needs_layout = true;
            }
            if (button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN) {
                scroll_delta.y -= 30.0f;
                needs_layout = true;
            }
        }
    }
}

SubViewport* Clay3DCanvas::get_viewport() const {
    return viewport;
}

void Clay3DCanvas::build_child_widgets() {
    int child_count = get_child_count();
    for (int i = 0; i < child_count; i++) {
        ClayWidget* widget = Object::cast_to<ClayWidget>(get_child(i));
        if (widget && widget->is_enabled()) {
            widget->set_parent_canvas_3d(const_cast<Clay3DCanvas*>(this));
            widget->pre_build(get_process_delta_time());
            widget->build();
        }
    }
}

void Clay3DCanvas::build_ui() {
    build_child_widgets();
}

void Clay3DCanvas::update_widget_debug_elements(Clay_RenderCommandArray* commands) {
    // Find and update all ClayWidget children that have debug enabled
    int child_count = get_child_count();
    for (int i = 0; i < child_count; i++) {
        ClayWidget* widget = Object::cast_to<ClayWidget>(get_child(i));
        if (widget && widget->is_debug_enabled()) {
            widget->update_debug_elements(commands);
        }
    }
}
