#include "clay_widget_script.h"
#include "clay_widget.h"
#include "clay_2d_canvas.h"
#include "clay_3d_canvas.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void ClayWidgetScript::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ClayWidgetScript::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ClayWidgetScript::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("get_widget"), &ClayWidgetScript::get_widget);
    ClassDB::bind_method(D_METHOD("get_canvas_2d"), &ClayWidgetScript::get_canvas_2d);
    ClassDB::bind_method(D_METHOD("get_canvas_3d"), &ClayWidgetScript::get_canvas_3d);

    ClassDB::bind_method(D_METHOD("is_hovered"), &ClayWidgetScript::is_hovered);
    ClassDB::bind_method(D_METHOD("is_element_hovered", "element_id"), &ClayWidgetScript::is_element_hovered);
    ClassDB::bind_method(D_METHOD("get_mouse_position"), &ClayWidgetScript::get_mouse_position);
    ClassDB::bind_method(D_METHOD("is_mouse_pressed"), &ClayWidgetScript::is_mouse_pressed);
    ClassDB::bind_method(D_METHOD("was_clicked"), &ClayWidgetScript::was_clicked);

    ClassDB::bind_method(D_METHOD("get_font", "font_id"), &ClayWidgetScript::get_font);
    ClassDB::bind_method(D_METHOD("get_texture", "texture_id"), &ClayWidgetScript::get_texture);
}

ClayWidgetScript::ClayWidgetScript() {
}

ClayWidgetScript::~ClayWidgetScript() {
}

void ClayWidgetScript::_ready() {
    find_parent_widget();
    if (parent_widget) {
        on_start();
    }
}

void ClayWidgetScript::_enter_tree() {
    find_parent_widget();
}

void ClayWidgetScript::_exit_tree() {
    parent_widget = nullptr;
}

void ClayWidgetScript::find_parent_widget() {
    Node* parent = get_parent();
    if (parent) {
        parent_widget = Object::cast_to<ClayWidget>(parent);
    }
}

void ClayWidgetScript::on_start() {
    // Override in derived classes
}

void ClayWidgetScript::on_pre_build(double delta) {
    // Override in derived classes
}

void ClayWidgetScript::on_build() {
    // Override in derived classes
}

void ClayWidgetScript::on_post_build() {
    // Override in derived classes
}

void ClayWidgetScript::set_enabled(bool p_enabled) {
    enabled = p_enabled;
}

bool ClayWidgetScript::is_enabled() const {
    return enabled;
}

ClayWidget* ClayWidgetScript::get_widget() const {
    return parent_widget;
}

Clay2DCanvas* ClayWidgetScript::get_canvas_2d() const {
    if (parent_widget) {
        return parent_widget->get_parent_canvas();
    }
    return nullptr;
}

Clay3DCanvas* ClayWidgetScript::get_canvas_3d() const {
    if (parent_widget) {
        return parent_widget->get_parent_canvas_3d();
    }
    return nullptr;
}

bool ClayWidgetScript::is_hovered() const {
    if (parent_widget) {
        return parent_widget->is_hovered();
    }
    return false;
}

bool ClayWidgetScript::is_element_hovered(const String& element_id) const {
    if (parent_widget) {
        return parent_widget->is_element_hovered(element_id);
    }
    return false;
}

Vector2 ClayWidgetScript::get_mouse_position() const {
    if (parent_widget) {
        return parent_widget->get_mouse_position();
    }
    return Vector2();
}

bool ClayWidgetScript::is_mouse_pressed() const {
    if (parent_widget) {
        return parent_widget->is_mouse_pressed();
    }
    return false;
}

bool ClayWidgetScript::was_clicked() const {
    if (parent_widget) {
        return parent_widget->was_clicked();
    }
    return false;
}

Ref<Font> ClayWidgetScript::get_font(int font_id) const {
    if (parent_widget) {
        return parent_widget->get_font(font_id);
    }
    return Ref<Font>();
}

Ref<Texture2D> ClayWidgetScript::get_texture(int texture_id) const {
    if (parent_widget) {
        return parent_widget->get_texture(texture_id);
    }
    return Ref<Texture2D>();
}
