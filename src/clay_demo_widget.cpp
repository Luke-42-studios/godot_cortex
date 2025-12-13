#include "clay_demo_widget.h"
#include "clay_2d_canvas.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void ClayDemoWidget::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_title", "title"), &ClayDemoWidget::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &ClayDemoWidget::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_accent_color", "color"), &ClayDemoWidget::set_accent_color);
    ClassDB::bind_method(D_METHOD("get_accent_color"), &ClayDemoWidget::get_accent_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "accent_color"), "set_accent_color", "get_accent_color");

    ClassDB::bind_method(D_METHOD("increment_counter"), &ClayDemoWidget::increment_counter);
    ClassDB::bind_method(D_METHOD("set_selected_tab", "tab"), &ClayDemoWidget::set_selected_tab);
    ClassDB::bind_method(D_METHOD("get_selected_tab"), &ClayDemoWidget::get_selected_tab);
    ClassDB::bind_method(D_METHOD("get_click_count"), &ClayDemoWidget::get_click_count);
}

ClayDemoWidget::ClayDemoWidget() {
}

void ClayDemoWidget::pre_build(double delta) {
    // Called every frame before build - can update state here
}

void ClayDemoWidget::build() {
    // Colors
    const Clay_Color COLOR_WHITE = { 255, 255, 255, 255 };
    const Clay_Color COLOR_BG = { 35, 35, 40, 255 };
    const Clay_Color COLOR_CARD = { 45, 45, 50, 255 };
    const Clay_Color COLOR_CARD_HOVER = { 55, 55, 60, 255 };
    const Clay_Color COLOR_TEXT = { 220, 220, 225, 255 };
    const Clay_Color COLOR_TEXT_DIM = { 140, 140, 150, 255 };
    const Clay_Color COLOR_TRANSPARENT = { 0, 0, 0, 0 };

    Clay_Color COLOR_ACCENT = {
        (float)(accent_color.r * 255),
        (float)(accent_color.g * 255),
        (float)(accent_color.b * 255),
        (float)(accent_color.a * 255)
    };
    Clay_Color COLOR_ACCENT_HOVER = {
        (float)(accent_color.r * 255 * 1.2f),
        (float)(accent_color.g * 255 * 1.2f),
        (float)(accent_color.b * 255 * 1.2f),
        255
    };

    // Store UTF-8 buffers at function scope to keep them alive
    // These MUST remain in scope for the entire build() function
    CharString title_utf8 = title.utf8();
    String counter_text = "Clicks: " + String::num_int64(click_count);
    CharString counter_utf8 = counter_text.utf8();

    // Create Clay string for counter (title_str created inline where needed)
    int counter_len = counter_utf8.get_data() ? (int)strlen(counter_utf8.get_data()) : 0;
    Clay_String counter_str = { .isStaticallyAllocated = false, .length = counter_len, .chars = counter_utf8.get_data() };

    // Root container
    CLAY(CLAY_ID("DemoWidget"), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(20),
            .childGap = 16,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = COLOR_BG
    }) {
        // Title bar
        CLAY(CLAY_ID("DemoTitleBar"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = { 16, 16, 12, 12 },
                .childGap = 12,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = COLOR_CARD,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            // Use strlen to get actual string length (CharString includes null terminator in some cases)
            int title_len = title_utf8.get_data() ? (int)strlen(title_utf8.get_data()) : 0;
            Clay_String safe_title_str = { .isStaticallyAllocated = false, .length = title_len, .chars = title_utf8.get_data() };
            CLAY_TEXT(safe_title_str, CLAY_TEXT_CONFIG({
                .textColor = COLOR_WHITE,
                .fontSize = 22
            }));
        }

        // Tab bar - track which tab to select at end to avoid mid-loop state changes
        int new_selected_tab = selected_tab;

        CLAY(CLAY_ID("DemoTabBar"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childGap = 4
            }
        }) {
            const char* tabs[] = { "Overview", "Settings", "About" };
            for (int i = 0; i < 3; i++) {
                bool is_selected = (i == selected_tab);

                // Base colors - no hover effect to avoid flicker issues
                Clay_Color tab_bg = is_selected ? COLOR_ACCENT : COLOR_CARD;
                Clay_Color tab_text = is_selected ? COLOR_WHITE : COLOR_TEXT;

                CLAY(CLAY_IDI("DemoTab", i), {
                    .layout = {
                        .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .padding = { 20, 20, 10, 10 }
                    },
                    .backgroundColor = tab_bg,
                    .cornerRadius = { 6, 6, 0, 0 }
                }) {
                    Clay_String tab_str = { .isStaticallyAllocated = true, .length = (int32_t)strlen(tabs[i]), .chars = tabs[i] };
                    CLAY_TEXT(tab_str, CLAY_TEXT_CONFIG({
                        .textColor = tab_text,
                        .fontSize = 14
                    }));

                    // Check hover INSIDE the CLAY block - this is when the element is "open"
                    if (Clay_Hovered() && was_clicked()) {
                        new_selected_tab = i;
                    }
                }
            }
        }

        // Apply tab selection after the loop to avoid changing state mid-render
        selected_tab = new_selected_tab;

        // Content area
        CLAY(CLAY_ID("Content"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = COLOR_CARD,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            if (selected_tab == 0) {
                // Overview tab
                CLAY_TEXT(CLAY_STRING("Welcome to Clay UI!"), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_WHITE,
                    .fontSize = 18
                }));

                CLAY_TEXT(CLAY_STRING("This widget demonstrates how to build UI using Clay's declarative C++ API. All elements are laid out using flexbox-like rules."), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_TEXT_DIM,
                    .fontSize = 13
                }));

                // Counter section
                CLAY(CLAY_ID("CounterSection"), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(12),
                        .childGap = 12,
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = { 55, 55, 60, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(6)
                }) {
                    CLAY_TEXT(counter_str, CLAY_TEXT_CONFIG({
                        .textColor = COLOR_TEXT,
                        .fontSize = 16
                    }));

                    // Increment button
                    Clay_ElementId btn_id = CLAY_ID("IncrementBtn");
                    bool btn_hovered = Clay_PointerOver(btn_id);
                    CLAY(btn_id, {
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .padding = { 16, 16, 8, 8 }
                        },
                        .backgroundColor = btn_hovered ? COLOR_ACCENT_HOVER : COLOR_ACCENT,
                        .cornerRadius = CLAY_CORNER_RADIUS(4)
                    }) {
                        CLAY_TEXT(CLAY_STRING("Click Me!"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_WHITE,
                            .fontSize = 14
                        }));
                    }

                    if (btn_hovered && was_clicked()) {
                        click_count++;
                    }
                }

            } else if (selected_tab == 1) {
                // Settings tab
                CLAY_TEXT(CLAY_STRING("Settings"), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_WHITE,
                    .fontSize = 18
                }));

                CLAY_TEXT(CLAY_STRING("Configure your widget properties in the Godot Inspector panel. You can change the title and accent color."), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_TEXT_DIM,
                    .fontSize = 13
                }));

                // Show current settings
                CLAY(CLAY_ID("SettingsInfo"), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(12),
                        .childGap = 8,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    },
                    .backgroundColor = { 55, 55, 60, 255 },
                    .cornerRadius = CLAY_CORNER_RADIUS(6)
                }) {
                    CLAY_TEXT(CLAY_STRING("Current accent color is shown in tabs and buttons."), CLAY_TEXT_CONFIG({
                        .textColor = COLOR_TEXT,
                        .fontSize = 12
                    }));

                    // Color preview
                    CLAY(CLAY_ID("ColorPreview"), {
                        .layout = {
                            .sizing = { CLAY_SIZING_FIXED(100), CLAY_SIZING_FIXED(30) }
                        },
                        .backgroundColor = COLOR_ACCENT,
                        .cornerRadius = CLAY_CORNER_RADIUS(4)
                    }) {}
                }

            } else {
                // About tab
                CLAY_TEXT(CLAY_STRING("About Clay"), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_WHITE,
                    .fontSize = 18
                }));

                CLAY_TEXT(CLAY_STRING("Clay is a high-performance UI layout library created by Nic Barker. It computes layouts in microseconds and outputs render commands that can be drawn by any renderer."), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_TEXT_DIM,
                    .fontSize = 13
                }));

                CLAY_TEXT(CLAY_STRING("This Godot integration renders Clay's output using Godot's CanvasItem drawing API."), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_TEXT_DIM,
                    .fontSize = 13
                }));

                CLAY(CLAY_ID("Link"), {
                    .layout = {
                        .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                        .padding = { 0, 0, 8, 0 }
                    }
                }) {
                    CLAY_TEXT(CLAY_STRING("github.com/nicbarker/clay"), CLAY_TEXT_CONFIG({
                        .textColor = COLOR_ACCENT,
                        .fontSize = 13
                    }));
                }
            }
        }

        // Footer
        CLAY(CLAY_ID("Footer"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = { 8, 8, 4, 4 },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
            }
        }) {
            CLAY_TEXT(CLAY_STRING("ClayWidget Example - Extend this class to create your own widgets"), CLAY_TEXT_CONFIG({
                .textColor = COLOR_TEXT_DIM,
                .fontSize = 11
            }));
        }
    }
}

void ClayDemoWidget::set_title(const String& p_title) {
    title = p_title;
}

String ClayDemoWidget::get_title() const {
    return title;
}

void ClayDemoWidget::set_accent_color(const Color& color) {
    accent_color = color;
}

Color ClayDemoWidget::get_accent_color() const {
    return accent_color;
}

void ClayDemoWidget::increment_counter() {
    click_count++;
}

void ClayDemoWidget::set_selected_tab(int tab) {
    selected_tab = tab;
}

int ClayDemoWidget::get_selected_tab() const {
    return selected_tab;
}

int ClayDemoWidget::get_click_count() const {
    return click_count;
}
