#include "clay_scroll_view.h"
#include "clay_2d_canvas.h"
#include "clay_3d_canvas.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void ClayScrollView::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_scroll_id", "id"), &ClayScrollView::set_scroll_id);
    ClassDB::bind_method(D_METHOD("get_scroll_id"), &ClayScrollView::get_scroll_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "scroll_id"), "set_scroll_id", "get_scroll_id");

    ClassDB::bind_method(D_METHOD("set_scroll_horizontal", "enabled"), &ClayScrollView::set_scroll_horizontal);
    ClassDB::bind_method(D_METHOD("get_scroll_horizontal"), &ClayScrollView::get_scroll_horizontal);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_horizontal"), "set_scroll_horizontal", "get_scroll_horizontal");

    ClassDB::bind_method(D_METHOD("set_scroll_vertical", "enabled"), &ClayScrollView::set_scroll_vertical);
    ClassDB::bind_method(D_METHOD("get_scroll_vertical"), &ClayScrollView::get_scroll_vertical);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_vertical"), "set_scroll_vertical", "get_scroll_vertical");

    ClassDB::bind_method(D_METHOD("set_background_color", "color"), &ClayScrollView::set_background_color);
    ClassDB::bind_method(D_METHOD("get_background_color"), &ClayScrollView::get_background_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_background_color", "get_background_color");

    ClassDB::bind_method(D_METHOD("set_padding", "padding"), &ClayScrollView::set_padding);
    ClassDB::bind_method(D_METHOD("get_padding"), &ClayScrollView::get_padding);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "padding", PROPERTY_HINT_RANGE, "0,100,1"), "set_padding", "get_padding");

    ClassDB::bind_method(D_METHOD("set_child_gap", "gap"), &ClayScrollView::set_child_gap);
    ClassDB::bind_method(D_METHOD("get_child_gap"), &ClayScrollView::get_child_gap);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "child_gap", PROPERTY_HINT_RANGE, "0,50,1"), "set_child_gap", "get_child_gap");

    ClassDB::bind_method(D_METHOD("set_fixed_width", "width"), &ClayScrollView::set_fixed_width);
    ClassDB::bind_method(D_METHOD("get_fixed_width"), &ClayScrollView::get_fixed_width);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fixed_width"), "set_fixed_width", "get_fixed_width");

    ClassDB::bind_method(D_METHOD("set_fixed_height", "height"), &ClayScrollView::set_fixed_height);
    ClassDB::bind_method(D_METHOD("get_fixed_height"), &ClayScrollView::get_fixed_height);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fixed_height"), "set_fixed_height", "get_fixed_height");
}

ClayScrollView::ClayScrollView() {
}

void ClayScrollView::build() {
    Clay_Color bg = {
        (float)(background_color.r * 255),
        (float)(background_color.g * 255),
        (float)(background_color.b * 255),
        (float)(background_color.a * 255)
    };

    CharString id_utf8 = scroll_id.utf8();
    Clay_ElementId clay_id = Clay_GetElementId(Clay_String{
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(id_utf8.get_data()),
        .chars = id_utf8.get_data()
    });

    // Determine sizing
    Clay_SizingAxis width_sizing = fixed_width > 0
        ? CLAY_SIZING_FIXED(fixed_width)
        : CLAY_SIZING_GROW(0);

    Clay_SizingAxis height_sizing = fixed_height > 0
        ? CLAY_SIZING_FIXED(fixed_height)
        : CLAY_SIZING_GROW(0);

    // Create the scroll container with clip config
    CLAY(clay_id, {
        .layout = {
            .sizing = { width_sizing, height_sizing },
            .padding = { (uint16_t)padding, (uint16_t)padding, (uint16_t)padding, (uint16_t)padding },
            .childGap = (uint16_t)child_gap,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = bg,
        .clip = {
            .horizontal = scroll_horizontal,
            .vertical = scroll_vertical,
            .childOffset = Clay_GetScrollOffset()
        }
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

void ClayScrollView::set_scroll_id(const String& id) {
    scroll_id = id;
}

String ClayScrollView::get_scroll_id() const {
    return scroll_id;
}

void ClayScrollView::set_scroll_horizontal(bool enabled) {
    scroll_horizontal = enabled;
}

bool ClayScrollView::get_scroll_horizontal() const {
    return scroll_horizontal;
}

void ClayScrollView::set_scroll_vertical(bool enabled) {
    scroll_vertical = enabled;
}

bool ClayScrollView::get_scroll_vertical() const {
    return scroll_vertical;
}

void ClayScrollView::set_background_color(const Color& color) {
    background_color = color;
}

Color ClayScrollView::get_background_color() const {
    return background_color;
}

void ClayScrollView::set_padding(int p_padding) {
    padding = p_padding;
}

int ClayScrollView::get_padding() const {
    return padding;
}

void ClayScrollView::set_child_gap(int gap) {
    child_gap = gap;
}

int ClayScrollView::get_child_gap() const {
    return child_gap;
}

void ClayScrollView::set_fixed_width(float width) {
    fixed_width = width;
}

float ClayScrollView::get_fixed_width() const {
    return fixed_width;
}

void ClayScrollView::set_fixed_height(float height) {
    fixed_height = height;
}

float ClayScrollView::get_fixed_height() const {
    return fixed_height;
}
