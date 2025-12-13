// Must define CLAY_IMPLEMENTATION before including clay.h in exactly one file
#define CLAY_IMPLEMENTATION
#include "clay.h"

#include "clay_2d_canvas.h"
#include "clay_widget.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

using namespace godot;

// Global pointer to current Clay2DCanvas instance for callbacks
static Clay2DCanvas* g_current_clay_canvas = nullptr;

// Clay error handler
static void clay_error_handler(Clay_ErrorData error) {
    UtilityFunctions::printerr("Clay Error: ", String(error.errorText.chars));
}

// Clay text measurement callback
static Clay_Dimensions measure_text_callback(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    Clay2DCanvas* canvas = static_cast<Clay2DCanvas*>(userData);
    if (!canvas) {
        return Clay_Dimensions{ (float)text.length * config->fontSize * 0.6f, (float)config->fontSize };
    }

    Ref<Font> font = canvas->get_font(config->fontId);
    if (!font.is_valid()) {
        font = canvas->get_default_font();
    }

    if (!font.is_valid()) {
        // Fallback: estimate based on font size
        return Clay_Dimensions{ (float)text.length * config->fontSize * 0.6f, (float)config->fontSize };
    }

    // Convert Clay_StringSlice to Godot String
    String str;
    if (text.length > 0 && text.chars != nullptr) {
        str = String::utf8(text.chars, text.length);
    }

    Vector2 size = font->get_string_size(str, HORIZONTAL_ALIGNMENT_LEFT, -1, config->fontSize);

    float height = config->lineHeight > 0 ? config->lineHeight : config->fontSize;

    return Clay_Dimensions{ size.x, height };
}

void Clay2DCanvas::_bind_methods() {
    // Initialization
    ClassDB::bind_method(D_METHOD("initialize_clay"), &Clay2DCanvas::initialize_clay);
    ClassDB::bind_method(D_METHOD("shutdown_clay"), &Clay2DCanvas::shutdown_clay);
    ClassDB::bind_method(D_METHOD("is_clay_initialized"), &Clay2DCanvas::is_clay_initialized);

    // Font management
    ClassDB::bind_method(D_METHOD("set_default_font", "font"), &Clay2DCanvas::set_default_font);
    ClassDB::bind_method(D_METHOD("get_default_font"), &Clay2DCanvas::get_default_font);
    ClassDB::bind_method(D_METHOD("register_font", "font_id", "font"), &Clay2DCanvas::register_font);
    ClassDB::bind_method(D_METHOD("get_font", "font_id"), &Clay2DCanvas::get_font);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_font", PROPERTY_HINT_RESOURCE_TYPE, "Font"), "set_default_font", "get_default_font");

    ClassDB::bind_method(D_METHOD("set_default_font_size", "size"), &Clay2DCanvas::set_default_font_size);
    ClassDB::bind_method(D_METHOD("get_default_font_size"), &Clay2DCanvas::get_default_font_size);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_font_size", PROPERTY_HINT_RANGE, "8,72,1"), "set_default_font_size", "get_default_font_size");

    // Texture management
    ClassDB::bind_method(D_METHOD("register_texture", "texture_id", "texture"), &Clay2DCanvas::register_texture);
    ClassDB::bind_method(D_METHOD("get_texture", "texture_id"), &Clay2DCanvas::get_texture);

    // Layout
    ClassDB::bind_method(D_METHOD("request_layout"), &Clay2DCanvas::request_layout);

}

Clay2DCanvas::Clay2DCanvas() {
}

Clay2DCanvas::~Clay2DCanvas() {
    shutdown_clay();
}

void Clay2DCanvas::_ready() {
    set_process(true);
    set_process_input(true);

    // Try to get default font from theme
    if (!default_font.is_valid()) {
        default_font = ThemeDB::get_singleton()->get_fallback_font();
    }

    initialize_clay();
}

void Clay2DCanvas::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE:
            // Initialize when entering tree (works in editor too)
            if (!clay_initialized) {
                if (!default_font.is_valid()) {
                    default_font = ThemeDB::get_singleton()->get_fallback_font();
                }
                initialize_clay();
            }
            break;
        case NOTIFICATION_RESIZED:
            needs_layout = true;
            queue_redraw();
            break;
        case NOTIFICATION_VISIBILITY_CHANGED:
            if (is_visible()) {
                needs_layout = true;
                queue_redraw();
            }
            break;
    }
}

void Clay2DCanvas::_process(double delta) {
    // Check if size changed
    Vector2 current_size = get_size();
    if (current_size != last_size) {
        last_size = current_size;
        needs_layout = true;
    }

    if (needs_layout) {
        queue_redraw();
    }
}

void Clay2DCanvas::_gui_input(const Ref<InputEvent> &event) {
    Ref<InputEventMouseMotion> motion = event;
    if (motion.is_valid()) {
        mouse_position = motion->get_position();
        needs_layout = true;
    }

    Ref<InputEventMouseButton> button = event;
    if (button.is_valid()) {
        if (button->get_button_index() == MOUSE_BUTTON_LEFT) {
            bool was_down = mouse_down;
            mouse_down = button->is_pressed();

            // Track press/release events for single-frame detection
            if (mouse_down && !was_down) {
                mouse_pressed_this_frame = true;
            }
            if (!mouse_down && was_down) {
                mouse_released_this_frame = true;
            }
            needs_layout = true;
        }
        // Handle scroll wheel
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

void Clay2DCanvas::_draw() {
    if (!clay_initialized) {
        // Try to initialize if not done yet
        if (!default_font.is_valid()) {
            default_font = ThemeDB::get_singleton()->get_fallback_font();
        }
        initialize_clay();
        if (!clay_initialized) {
            return;
        }
    }

    g_current_clay_canvas = this;

    // Update Clay state
    Vector2 size = get_size();

    // Ensure we have a valid size
    if (size.x <= 0 || size.y <= 0) {
        size = Vector2(800, 600);
    }

    Clay_SetLayoutDimensions(Clay_Dimensions{ size.x, size.y });
    Clay_SetPointerState(Clay_Vector2{ mouse_position.x, mouse_position.y }, mouse_down);
    Clay_UpdateScrollContainers(true, Clay_Vector2{ scroll_delta.x, scroll_delta.y }, get_process_delta_time());

    // Reset scroll delta after applying
    scroll_delta = Vector2();

    // Begin layout
    Clay_BeginLayout();

    // Build UI (override this in subclasses or set demo_mode)
    build_ui();

    // End layout and get render commands
    Clay_RenderCommandArray commands = Clay_EndLayout();

    // Clear clip stack before rendering
    clip_stack.clear();

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
                float radius = cmd->renderData.rectangle.cornerRadius.topLeft;
                draw_rectangle(rect, color, radius);
            } break;

            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Color color(
                    cmd->renderData.border.color.r / 255.0f,
                    cmd->renderData.border.color.g / 255.0f,
                    cmd->renderData.border.color.b / 255.0f,
                    cmd->renderData.border.color.a / 255.0f
                );
                float width = cmd->renderData.border.width.left; // Use left width for all sides
                float radius = cmd->renderData.border.cornerRadius.topLeft;
                draw_border(rect, color, width, radius);
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
                draw_text_clay(rect, text, color, cmd->renderData.text.fontId, cmd->renderData.text.fontSize);
            } break;

            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                Color tint(
                    cmd->renderData.image.backgroundColor.r / 255.0f,
                    cmd->renderData.image.backgroundColor.g / 255.0f,
                    cmd->renderData.image.backgroundColor.b / 255.0f,
                    cmd->renderData.image.backgroundColor.a / 255.0f
                );
                // Use imageData as texture ID
                intptr_t tex_id = reinterpret_cast<intptr_t>(cmd->renderData.image.imageData);
                draw_image(rect, (int)tex_id, tint);
            } break;

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                // Push clip rect onto stack
                clip_stack.push_back(rect);
            } break;

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                // Pop clip rect from stack
                if (clip_stack.size() > 0) {
                    clip_stack.resize(clip_stack.size() - 1);
                }
            } break;

            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                // Custom rendering - could be extended
            } break;

            default:
                break;
        }
    }

    needs_layout = false;

    // Update debug elements on widgets that have debug enabled
    update_widget_debug_elements(&commands);

    g_current_clay_canvas = nullptr;

    // Reset single-frame flags after drawing
    mouse_pressed_this_frame = false;
    mouse_released_this_frame = false;
}

void Clay2DCanvas::initialize_clay() {
    if (clay_initialized) {
        return;
    }

    clay_memory_size = Clay_MinMemorySize();
    clay_memory = memalloc(clay_memory_size);

    if (!clay_memory) {
        UtilityFunctions::printerr("Clay2DCanvas: Failed to allocate memory for Clay");
        return;
    }

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_memory_size, clay_memory);

    Vector2 size = get_size();
    if (size.x <= 0) size.x = 800;
    if (size.y <= 0) size.y = 600;

    Clay_Initialize(arena, Clay_Dimensions{ size.x, size.y }, Clay_ErrorHandler{ clay_error_handler });
    Clay_SetMeasureTextFunction(measure_text_callback, this);

    clay_initialized = true;
    UtilityFunctions::print("Clay2DCanvas: Initialized with ", clay_memory_size, " bytes");
}

void Clay2DCanvas::shutdown_clay() {
    if (clay_memory) {
        memfree(clay_memory);
        clay_memory = nullptr;
        clay_memory_size = 0;
    }
    clay_initialized = false;
}

bool Clay2DCanvas::is_clay_initialized() const {
    return clay_initialized;
}

void Clay2DCanvas::set_default_font(const Ref<Font> &font) {
    default_font = font;
    needs_layout = true;
}

Ref<Font> Clay2DCanvas::get_default_font() const {
    return default_font;
}

void Clay2DCanvas::register_font(int font_id, const Ref<Font> &font) {
    fonts[font_id] = font;
}

Ref<Font> Clay2DCanvas::get_font(int font_id) const {
    if (fonts.has(font_id)) {
        return fonts[font_id];
    }
    return default_font;
}

void Clay2DCanvas::set_default_font_size(int size) {
    default_font_size = size;
    needs_layout = true;
}

int Clay2DCanvas::get_default_font_size() const {
    return default_font_size;
}

void Clay2DCanvas::register_texture(int texture_id, const Ref<Texture2D> &texture) {
    textures[texture_id] = texture;
}

Ref<Texture2D> Clay2DCanvas::get_texture(int texture_id) const {
    if (textures.has(texture_id)) {
        return textures[texture_id];
    }
    return Ref<Texture2D>();
}

void Clay2DCanvas::request_layout() {
    needs_layout = true;
    queue_redraw();
}

bool Clay2DCanvas::is_mouse_down() const {
    return mouse_down;
}

bool Clay2DCanvas::was_mouse_pressed() const {
    return mouse_pressed_this_frame;
}

bool Clay2DCanvas::was_mouse_released() const {
    return mouse_released_this_frame;
}

void Clay2DCanvas::build_child_widgets() {
    // Find and build all ClayWidget children
    int child_count = get_child_count();
    for (int i = 0; i < child_count; i++) {
        ClayWidget* widget = Object::cast_to<ClayWidget>(get_child(i));
        if (widget && widget->is_enabled()) {
            widget->set_parent_canvas(this);
            widget->pre_build(get_process_delta_time());
            widget->build();
        }
    }
}

void Clay2DCanvas::build_ui() {
    // Build all child widgets - Clay2DCanvas only handles children now
    build_child_widgets();
}

void Clay2DCanvas::draw_rectangle(const Rect2 &rect, const Color &color, float corner_radius) {
    // If no clipping active, draw directly
    if (clip_stack.size() == 0) {
        draw_rect(rect, color);
        return;
    }

    if (!is_rect_visible(rect)) {
        return;
    }

    Rect2 clipped = clip_rect(rect);
    if (clipped.size.x <= 0 || clipped.size.y <= 0) {
        return;
    }

    // TODO: Handle corner_radius with clipping properly
    draw_rect(clipped, color);
}

void Clay2DCanvas::draw_border(const Rect2 &rect, const Color &color, float width, float corner_radius) {
    if (width <= 0) return;

    // Draw border as 4 rectangles (top, bottom, left, right)
    Rect2 top(rect.position.x, rect.position.y, rect.size.x, width);
    Rect2 bottom(rect.position.x, rect.position.y + rect.size.y - width, rect.size.x, width);
    Rect2 left(rect.position.x, rect.position.y + width, width, rect.size.y - width * 2);
    Rect2 right(rect.position.x + rect.size.x - width, rect.position.y + width, width, rect.size.y - width * 2);

    // If no clipping, draw directly
    if (clip_stack.size() == 0) {
        draw_rect(top, color);
        draw_rect(bottom, color);
        draw_rect(left, color);
        draw_rect(right, color);
        return;
    }

    if (!is_rect_visible(rect)) return;

    Rect2 clipped_top = clip_rect(top);
    Rect2 clipped_bottom = clip_rect(bottom);
    Rect2 clipped_left = clip_rect(left);
    Rect2 clipped_right = clip_rect(right);

    if (clipped_top.size.x > 0 && clipped_top.size.y > 0) draw_rect(clipped_top, color);
    if (clipped_bottom.size.x > 0 && clipped_bottom.size.y > 0) draw_rect(clipped_bottom, color);
    if (clipped_left.size.x > 0 && clipped_left.size.y > 0) draw_rect(clipped_left, color);
    if (clipped_right.size.x > 0 && clipped_right.size.y > 0) draw_rect(clipped_right, color);
}

void Clay2DCanvas::draw_text_clay(const Rect2 &rect, const String &text, const Color &color, int font_id, int font_size) {
    Ref<Font> font = get_font(font_id);
    if (!font.is_valid()) {
        font = default_font;
    }
    if (!font.is_valid()) {
        return;
    }

    float ascent = font->get_ascent(font_size);
    Vector2 pos(rect.position.x, rect.position.y + ascent);

    // If no clipping, draw directly
    if (clip_stack.size() == 0) {
        draw_string(font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, rect.size.x, font_size, color);
        return;
    }

    if (!is_rect_visible(rect)) {
        return;
    }

    // For text, we use draw_string with clipping via the clip width
    Rect2 clipped = clip_rect(rect);
    if (clipped.size.x <= 0 || clipped.size.y <= 0) {
        return;
    }

    // Adjust position if clipped on the left
    pos.x = clipped.position.x;

    // Only draw if the baseline is within the clip region
    if (pos.y >= clipped.position.y && pos.y - ascent <= clipped.position.y + clipped.size.y) {
        draw_string(font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, clipped.size.x, font_size, color);
    }
}

void Clay2DCanvas::draw_image(const Rect2 &rect, int texture_id, const Color &tint) {
    Ref<Texture2D> tex = get_texture(texture_id);
    if (!tex.is_valid()) {
        return;
    }

    Color actual_tint = tint;
    if (tint.a <= 0) {
        actual_tint = Color(1, 1, 1, 1);
    }

    // If no clipping, draw directly
    if (clip_stack.size() == 0) {
        draw_texture_rect(tex, rect, false, actual_tint);
        return;
    }

    if (!is_rect_visible(rect)) {
        return;
    }

    // For images, we need to clip both the rect and adjust the source region
    Rect2 clipped = clip_rect(rect);
    if (clipped.size.x <= 0 || clipped.size.y <= 0) {
        return;
    }

    // If fully visible, draw normally
    if (clipped == rect) {
        draw_texture_rect(tex, rect, false, actual_tint);
    } else {
        // Calculate the source region based on how much was clipped
        Vector2 tex_size = tex->get_size();
        float x_ratio = tex_size.x / rect.size.x;
        float y_ratio = tex_size.y / rect.size.y;

        Rect2 src_rect;
        src_rect.position.x = (clipped.position.x - rect.position.x) * x_ratio;
        src_rect.position.y = (clipped.position.y - rect.position.y) * y_ratio;
        src_rect.size.x = clipped.size.x * x_ratio;
        src_rect.size.y = clipped.size.y * y_ratio;

        draw_texture_rect_region(tex, clipped, src_rect, actual_tint);
    }
}

Rect2 Clay2DCanvas::get_current_clip() const {
    if (clip_stack.size() > 0) {
        return clip_stack[clip_stack.size() - 1];
    }
    return Rect2(0, 0, get_size().x, get_size().y);
}

bool Clay2DCanvas::is_rect_visible(const Rect2 &rect) const {
    if (clip_stack.size() == 0) {
        return true;
    }
    Rect2 clip = get_current_clip();
    return rect.intersects(clip);
}

Rect2 Clay2DCanvas::clip_rect(const Rect2 &rect) const {
    if (clip_stack.size() == 0) {
        return rect;
    }
    Rect2 clip = get_current_clip();
    return rect.intersection(clip);
}

void Clay2DCanvas::update_widget_debug_elements(Clay_RenderCommandArray* commands) {
    // Find and update all ClayWidget children that have debug enabled
    int child_count = get_child_count();
    for (int i = 0; i < child_count; i++) {
        ClayWidget* widget = Object::cast_to<ClayWidget>(get_child(i));
        if (widget && widget->is_debug_enabled()) {
            widget->update_debug_elements(commands);
        }
    }
}
