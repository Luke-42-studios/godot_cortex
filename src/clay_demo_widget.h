#ifndef CLAY_DEMO_WIDGET_H
#define CLAY_DEMO_WIDGET_H

#include "clay_widget.h"

namespace godot {

/**
 * ClayDemoWidget - Example widget showing how to build UI with Clay
 *
 * This demonstrates:
 * - Creating UI elements with Clay macros
 * - Handling hover states
 * - Using state variables
 * - Building complex layouts
 */
class ClayDemoWidget : public ClayWidget {
    GDCLASS(ClayDemoWidget, ClayWidget);

private:
    // Widget state
    int click_count = 0;
    int selected_tab = 0;
    String title = "Demo Widget";
    Color accent_color = Color(0.4f, 0.55f, 0.8f, 1.0f);

protected:
    static void _bind_methods();

public:
    ClayDemoWidget();

    void build() override;
    void pre_build(double delta) override;

    // Properties
    void set_title(const String& p_title);
    String get_title() const;

    void set_accent_color(const Color& color);
    Color get_accent_color() const;

    // Methods
    void increment_counter();
    void set_selected_tab(int tab);
    int get_selected_tab() const;
    int get_click_count() const;
};

} // namespace godot

#endif // CLAY_DEMO_WIDGET_H
