#include "clay_widget.h"
#include "clay_widget_script.h"
#include "clay_debug_element.h"
#include "clay_2d_canvas.h"
#include "clay_3d_canvas.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

using namespace godot;

// ============================================================================
// ClayWidget
// ============================================================================

void ClayWidget::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ClayWidget::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ClayWidget::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &ClayWidget::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("is_debug_enabled"), &ClayWidget::is_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "is_debug_enabled");

    ClassDB::bind_method(D_METHOD("is_hovered"), &ClayWidget::is_hovered);
    ClassDB::bind_method(D_METHOD("is_element_hovered", "element_id"), &ClayWidget::is_element_hovered);
    ClassDB::bind_method(D_METHOD("get_mouse_position"), &ClayWidget::get_mouse_position);
    ClassDB::bind_method(D_METHOD("is_mouse_pressed"), &ClayWidget::is_mouse_pressed);
    ClassDB::bind_method(D_METHOD("invalidate_scripts"), &ClayWidget::invalidate_scripts);
    ClassDB::bind_method(D_METHOD("clear_debug_elements"), &ClayWidget::clear_debug_elements);
}

ClayWidget::ClayWidget() {
}

ClayWidget::~ClayWidget() {
}

void ClayWidget::_ready() {
    collect_scripts();
}

void ClayWidget::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_CHILD_ORDER_CHANGED:
            scripts_dirty = true;
            break;
        case NOTIFICATION_ENTER_TREE:
            // When entering tree in editor, update debug element owners
            if (Engine::get_singleton()->is_editor_hint() && debug_enabled) {
                update_debug_owners();
            }
            break;
    }
}

void ClayWidget::collect_scripts() {
    scripts.clear();
    int child_count = get_child_count();
    for (int i = 0; i < child_count; i++) {
        ClayWidgetScript* script = Object::cast_to<ClayWidgetScript>(get_child(i));
        if (script) {
            scripts.push_back(script);
        }
    }
    scripts_dirty = false;
}

void ClayWidget::call_script_pre_build(double delta) {
    if (scripts_dirty) {
        collect_scripts();
    }
    for (int i = 0; i < scripts.size(); i++) {
        if (scripts[i]->is_enabled()) {
            scripts[i]->on_pre_build(delta);
        }
    }
}

void ClayWidget::call_script_build() {
    if (scripts_dirty) {
        collect_scripts();
    }
    for (int i = 0; i < scripts.size(); i++) {
        if (scripts[i]->is_enabled()) {
            scripts[i]->on_build();
        }
    }
}

void ClayWidget::call_script_post_build() {
    if (scripts_dirty) {
        collect_scripts();
    }
    for (int i = 0; i < scripts.size(); i++) {
        if (scripts[i]->is_enabled()) {
            scripts[i]->on_post_build();
        }
    }
}

void ClayWidget::invalidate_scripts() {
    scripts_dirty = true;
}

void ClayWidget::build() {
    // Call attached scripts' build methods
    call_script_build();
}

void ClayWidget::pre_build(double delta) {
    // Call attached scripts' pre_build methods
    call_script_pre_build(delta);
}

void ClayWidget::set_enabled(bool p_enabled) {
    enabled = p_enabled;
}

bool ClayWidget::is_enabled() const {
    return enabled;
}

void ClayWidget::set_parent_canvas(Clay2DCanvas* canvas) {
    parent_canvas_2d = canvas;
    parent_canvas_3d = nullptr;
}

void ClayWidget::set_parent_canvas_3d(Clay3DCanvas* canvas) {
    parent_canvas_3d = canvas;
    parent_canvas_2d = nullptr;
}

Clay2DCanvas* ClayWidget::get_parent_canvas() const {
    return parent_canvas_2d;
}

Clay3DCanvas* ClayWidget::get_parent_canvas_3d() const {
    return parent_canvas_3d;
}

bool ClayWidget::is_hovered() const {
    return Clay_Hovered();
}

bool ClayWidget::is_element_hovered(const String& element_id) const {
    CharString utf8 = element_id.utf8();
    Clay_ElementId id = Clay_GetElementId(Clay_String{ .isStaticallyAllocated = false, .length = (int32_t)utf8.length(), .chars = utf8.get_data() });
    return Clay_PointerOver(id);
}

Vector2 ClayWidget::get_mouse_position() const {
    if (parent_canvas_2d) {
        return parent_canvas_2d->get_local_mouse_position();
    }
    // 3D canvas doesn't have get_local_mouse_position in the same way
    return Vector2();
}

bool ClayWidget::is_mouse_pressed() const {
    if (parent_canvas_2d) {
        return parent_canvas_2d->is_mouse_down();
    }
    if (parent_canvas_3d) {
        return parent_canvas_3d->is_mouse_down();
    }
    return false;
}

bool ClayWidget::was_clicked() const {
    if (parent_canvas_2d) {
        return parent_canvas_2d->was_mouse_pressed();
    }
    if (parent_canvas_3d) {
        return parent_canvas_3d->was_mouse_pressed();
    }
    return false;
}

Ref<Font> ClayWidget::get_font(int font_id) const {
    if (parent_canvas_2d) {
        return parent_canvas_2d->get_font(font_id);
    }
    if (parent_canvas_3d) {
        return parent_canvas_3d->get_font(font_id);
    }
    return Ref<Font>();
}

Ref<Texture2D> ClayWidget::get_texture(int texture_id) const {
    if (parent_canvas_2d) {
        return parent_canvas_2d->get_texture(texture_id);
    }
    if (parent_canvas_3d) {
        return parent_canvas_3d->get_texture(texture_id);
    }
    return Ref<Texture2D>();
}

void ClayWidget::set_debug_enabled(bool p_enabled) {
    if (debug_enabled == p_enabled) {
        return;
    }
    debug_enabled = p_enabled;

    if (!debug_enabled) {
        clear_debug_elements();
    } else {
        // Create the debug container immediately so it shows in editor
        ensure_debug_container();
    }
}

bool ClayWidget::is_debug_enabled() const {
    return debug_enabled;
}

void ClayWidget::ensure_debug_container() {
    if (debug_container) {
        return;
    }

    debug_container = memnew(Node);
    debug_container->set_name("_DebugElements");

    // Add as regular child (not internal) so it shows in editor
    add_child(debug_container, false, Node::INTERNAL_MODE_DISABLED);

    // Set owner so it appears in the scene tree in editor
    if (Engine::get_singleton()->is_editor_hint() && is_inside_tree()) {
        Node* scene_root = get_tree()->get_edited_scene_root();
        if (scene_root) {
            debug_container->set_owner(scene_root);
        }
    }
}

ClayDebugElement* ClayWidget::get_or_create_debug_element(uint32_t element_id) {
    if (debug_elements.has(element_id)) {
        return debug_elements[element_id];
    }

    ensure_debug_container();

    ClayDebugElement* element = memnew(ClayDebugElement);
    element->set_element_hash(element_id);
    debug_container->add_child(element, false, Node::INTERNAL_MODE_DISABLED);
    debug_elements[element_id] = element;

    // Set owner so it appears in the scene tree in editor
    if (Engine::get_singleton()->is_editor_hint() && is_inside_tree()) {
        Node* scene_root = get_tree()->get_edited_scene_root();
        if (scene_root) {
            element->set_owner(scene_root);
        }
    }

    return element;
}

String ClayWidget::get_render_command_type_name(int type) const {
    switch (type) {
        case CLAY_RENDER_COMMAND_TYPE_NONE: return "None";
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: return "Rectangle";
        case CLAY_RENDER_COMMAND_TYPE_BORDER: return "Border";
        case CLAY_RENDER_COMMAND_TYPE_TEXT: return "Text";
        case CLAY_RENDER_COMMAND_TYPE_IMAGE: return "Image";
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: return "ScissorStart";
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: return "ScissorEnd";
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

void ClayWidget::update_debug_elements(Clay_RenderCommandArray* render_commands) {
    if (!debug_enabled || !render_commands) {
        return;
    }

    // Track which elements we've seen this frame
    HashMap<uint32_t, bool> seen_elements;

    for (int32_t i = 0; i < render_commands->length; i++) {
        Clay_RenderCommand* cmd = Clay_RenderCommandArray_Get(render_commands, i);
        if (!cmd || cmd->id == 0) {
            continue;
        }

        // Skip scissor commands as they don't have meaningful IDs
        if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_START ||
            cmd->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_END) {
            continue;
        }

        seen_elements[cmd->id] = true;

        ClayDebugElement* debug_elem = get_or_create_debug_element(cmd->id);

        // Update element data
        Rect2 bounds(cmd->boundingBox.x, cmd->boundingBox.y,
                     cmd->boundingBox.width, cmd->boundingBox.height);
        debug_elem->set_bounding_box(bounds);
        debug_elem->set_element_type(get_render_command_type_name(cmd->commandType));
        debug_elem->set_z_index(cmd->zIndex);

        // Try to create a readable name from the hash
        // Format: ElementType_Hash
        String name = get_render_command_type_name(cmd->commandType) + "_" + String::num_uint64(cmd->id);
        debug_elem->set_name(name);
        debug_elem->set_element_id(name);
    }

    // Remove elements that weren't seen this frame
    Vector<uint32_t> to_remove;
    for (const KeyValue<uint32_t, ClayDebugElement*>& kv : debug_elements) {
        if (!seen_elements.has(kv.key)) {
            to_remove.push_back(kv.key);
        }
    }

    for (int i = 0; i < to_remove.size(); i++) {
        uint32_t id = to_remove[i];
        ClayDebugElement* elem = debug_elements[id];
        if (elem) {
            elem->queue_free();
        }
        debug_elements.erase(id);
    }

    // Update owners so debug elements appear in editor scene tree
    if (Engine::get_singleton()->is_editor_hint()) {
        update_debug_owners();
    }
}

void ClayWidget::clear_debug_elements() {
    for (const KeyValue<uint32_t, ClayDebugElement*>& kv : debug_elements) {
        if (kv.value) {
            kv.value->queue_free();
        }
    }
    debug_elements.clear();

    if (debug_container) {
        debug_container->queue_free();
        debug_container = nullptr;
    }
}

void ClayWidget::update_debug_owners() {
    if (!is_inside_tree()) {
        return;
    }

    Node* scene_root = get_tree()->get_edited_scene_root();
    if (!scene_root) {
        return;
    }

    if (debug_container && debug_container->get_owner() != scene_root) {
        debug_container->set_owner(scene_root);
    }

    for (const KeyValue<uint32_t, ClayDebugElement*>& kv : debug_elements) {
        if (kv.value && kv.value->get_owner() != scene_root) {
            kv.value->set_owner(scene_root);
        }
    }
}

// ============================================================================
// ClayPanel
// ============================================================================

void ClayPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_background_color", "color"), &ClayPanel::set_background_color);
    ClassDB::bind_method(D_METHOD("get_background_color"), &ClayPanel::get_background_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_background_color", "get_background_color");

    ClassDB::bind_method(D_METHOD("set_border_color", "color"), &ClayPanel::set_border_color);
    ClassDB::bind_method(D_METHOD("get_border_color"), &ClayPanel::get_border_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "border_color"), "set_border_color", "get_border_color");

    ClassDB::bind_method(D_METHOD("set_border_width", "width"), &ClayPanel::set_border_width);
    ClassDB::bind_method(D_METHOD("get_border_width"), &ClayPanel::get_border_width);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "border_width", PROPERTY_HINT_RANGE, "0,10,0.5"), "set_border_width", "get_border_width");

    ClassDB::bind_method(D_METHOD("set_corner_radius", "radius"), &ClayPanel::set_corner_radius);
    ClassDB::bind_method(D_METHOD("get_corner_radius"), &ClayPanel::get_corner_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "corner_radius", PROPERTY_HINT_RANGE, "0,50,1"), "set_corner_radius", "get_corner_radius");

    ClassDB::bind_method(D_METHOD("set_padding", "padding"), &ClayPanel::set_padding);
    ClassDB::bind_method(D_METHOD("get_padding"), &ClayPanel::get_padding);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "padding", PROPERTY_HINT_RANGE, "0,100,1"), "set_padding", "get_padding");

    ClassDB::bind_method(D_METHOD("set_child_gap", "gap"), &ClayPanel::set_child_gap);
    ClassDB::bind_method(D_METHOD("get_child_gap"), &ClayPanel::get_child_gap);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "child_gap", PROPERTY_HINT_RANGE, "0,50,1"), "set_child_gap", "get_child_gap");

    ClassDB::bind_method(D_METHOD("set_vertical_layout", "vertical"), &ClayPanel::set_vertical_layout);
    ClassDB::bind_method(D_METHOD("get_vertical_layout"), &ClayPanel::get_vertical_layout);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vertical_layout"), "set_vertical_layout", "get_vertical_layout");

    ClassDB::bind_method(D_METHOD("set_panel_id", "id"), &ClayPanel::set_panel_id);
    ClassDB::bind_method(D_METHOD("get_panel_id"), &ClayPanel::get_panel_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "panel_id"), "set_panel_id", "get_panel_id");
}

ClayPanel::ClayPanel() {
}

void ClayPanel::build() {
    Clay_Color bg = {
        (float)(background_color.r * 255),
        (float)(background_color.g * 255),
        (float)(background_color.b * 255),
        (float)(background_color.a * 255)
    };

    Clay_Color border = {
        (float)(border_color.r * 255),
        (float)(border_color.g * 255),
        (float)(border_color.b * 255),
        (float)(border_color.a * 255)
    };

    CharString id_utf8 = panel_id.utf8();
    Clay_ElementId clay_id = Clay_GetElementId(Clay_String{ .isStaticallyAllocated = false, .length = (int32_t)id_utf8.length(), .chars = id_utf8.get_data() });

    Clay_BorderWidth border_w = { 0, 0, 0, 0, 0 };
    if (border_width > 0) {
        border_w = { (uint16_t)border_width, (uint16_t)border_width, (uint16_t)border_width, (uint16_t)border_width, 0 };
    }

    CLAY(clay_id, {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
            .padding = { (uint16_t)padding, (uint16_t)padding, (uint16_t)padding, (uint16_t)padding },
            .childGap = (uint16_t)child_gap,
            .layoutDirection = vertical_layout ? CLAY_TOP_TO_BOTTOM : CLAY_LEFT_TO_RIGHT
        },
        .backgroundColor = bg,
        .cornerRadius = CLAY_CORNER_RADIUS(corner_radius),
        .border = { .color = border, .width = border_w }
    }) {
        // Build all child widgets
        int child_count = get_child_count();
        for (int i = 0; i < child_count; i++) {
            ClayWidget* child_widget = Object::cast_to<ClayWidget>(get_child(i));
            if (child_widget && child_widget->is_enabled()) {
                child_widget->build();
            }
        }
    }
}

void ClayPanel::set_background_color(const Color& color) {
    background_color = color;
}

Color ClayPanel::get_background_color() const {
    return background_color;
}

void ClayPanel::set_border_color(const Color& color) {
    border_color = color;
}

Color ClayPanel::get_border_color() const {
    return border_color;
}

void ClayPanel::set_border_width(float width) {
    border_width = width;
}

float ClayPanel::get_border_width() const {
    return border_width;
}

void ClayPanel::set_corner_radius(float radius) {
    corner_radius = radius;
}

float ClayPanel::get_corner_radius() const {
    return corner_radius;
}

void ClayPanel::set_padding(int p_padding) {
    padding = p_padding;
}

int ClayPanel::get_padding() const {
    return padding;
}

void ClayPanel::set_child_gap(int gap) {
    child_gap = gap;
}

int ClayPanel::get_child_gap() const {
    return child_gap;
}

void ClayPanel::set_vertical_layout(bool vertical) {
    vertical_layout = vertical;
}

bool ClayPanel::get_vertical_layout() const {
    return vertical_layout;
}

void ClayPanel::set_panel_id(const String& id) {
    panel_id = id;
}

String ClayPanel::get_panel_id() const {
    return panel_id;
}
