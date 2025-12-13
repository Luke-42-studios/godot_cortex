#ifndef CLAY_SCROLL_VIEW_H
#define CLAY_SCROLL_VIEW_H

#include "clay_widget.h"
#include <godot_cpp/variant/color.hpp>

namespace godot {

/**
 * ClayScrollView - A scrollable container widget
 *
 * Wraps content in a scrollable area with optional scrollbars.
 * Uses Clay's built-in scroll container functionality.
 *
 * Features:
 * - Vertical and/or horizontal scrolling
 * - Automatic content clipping
 * - Optional background color
 * - Configurable size
 */
class ClayScrollView : public ClayWidget {
    GDCLASS(ClayScrollView, ClayWidget);

private:
    String scroll_id = "ScrollView";
    bool scroll_horizontal = false;
    bool scroll_vertical = true;
    Color background_color = Color(0.1f, 0.1f, 0.12f, 1.0f);
    int padding = 8;
    int child_gap = 4;
    float fixed_width = 0;  // 0 = grow
    float fixed_height = 300; // Default fixed height for scroll container

protected:
    static void _bind_methods();

public:
    ClayScrollView();

    void build() override;

    // Scroll ID (must be unique)
    void set_scroll_id(const String& id);
    String get_scroll_id() const;

    // Scroll directions
    void set_scroll_horizontal(bool enabled);
    bool get_scroll_horizontal() const;

    void set_scroll_vertical(bool enabled);
    bool get_scroll_vertical() const;

    // Appearance
    void set_background_color(const Color& color);
    Color get_background_color() const;

    void set_padding(int p_padding);
    int get_padding() const;

    void set_child_gap(int gap);
    int get_child_gap() const;

    // Size
    void set_fixed_width(float width);
    float get_fixed_width() const;

    void set_fixed_height(float height);
    float get_fixed_height() const;
};

} // namespace godot

#endif // CLAY_SCROLL_VIEW_H
