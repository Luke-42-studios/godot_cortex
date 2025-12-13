#include "clay_main_demo.h"
#include "clay_2d_canvas.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ClayMainDemo::_bind_methods() {
    // No additional properties needed
}

ClayMainDemo::ClayMainDemo() {
}

void ClayMainDemo::build() {
    // Colors - using const values directly avoids string/memory issues
    const Clay_Color COLOR_WHITE = { 255, 255, 255, 255 };
    const Clay_Color COLOR_DARK = { 40, 40, 45, 255 };
    const Clay_Color COLOR_DARKER = { 30, 30, 35, 255 };
    const Clay_Color COLOR_ACCENT = { 100, 140, 200, 255 };
    const Clay_Color COLOR_ACCENT_HOVER = { 120, 160, 220, 255 };
    const Clay_Color COLOR_TEXT = { 220, 220, 225, 255 };
    const Clay_Color COLOR_TEXT_DIM = { 150, 150, 160, 255 };
    const Clay_Color COLOR_SUCCESS = { 80, 180, 120, 255 };
    const Clay_Color COLOR_WARNING = { 220, 180, 80, 255 };
    const Clay_Color COLOR_TRANSPARENT = { 0, 0, 0, 0 };
    const Clay_Color COLOR_CARD = { 50, 50, 55, 255 };
    const Clay_Color COLOR_CARD_HOVER = { 60, 60, 65, 255 };

    // Root container - full screen dark background
    CLAY(CLAY_ID("MainDemoRoot"), {
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = COLOR_DARK
    }) {
        // Header
        CLAY(CLAY_ID("MainDemoHeader"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
            },
            .backgroundColor = COLOR_DARKER,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            CLAY_TEXT(CLAY_STRING("Clay UI + Godot"), CLAY_TEXT_CONFIG({
                .textColor = COLOR_WHITE,
                .fontSize = 28
            }));

            CLAY_TEXT(CLAY_STRING("High-performance UI layout in C++"), CLAY_TEXT_CONFIG({
                .textColor = COLOR_TEXT_DIM,
                .fontSize = 14
            }));
        }

        // Main content area with sidebar
        CLAY(CLAY_ID("MainDemoArea"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .childGap = 16
            }
        }) {
            // Sidebar
            CLAY(CLAY_ID("MainDemoSidebar"), {
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(12),
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = COLOR_DARKER,
                .cornerRadius = CLAY_CORNER_RADIUS(8)
            }) {
                CLAY_TEXT(CLAY_STRING("Navigation"), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_TEXT_DIM,
                    .fontSize = 12
                }));

                // Menu items - using static strings
                // Use Clay_PointerOver with element ID to check hover before declaring element
                Clay_ElementId menu0_id = CLAY_ID("MainDemoMenuItem0");
                bool menu0_hovered = Clay_PointerOver(menu0_id);
                CLAY(menu0_id, {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = { 12, 12, 10, 10 }
                    },
                    .backgroundColor = menu0_hovered ? COLOR_ACCENT : COLOR_TRANSPARENT,
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(CLAY_STRING("Dashboard"), CLAY_TEXT_CONFIG({
                        .textColor = menu0_hovered ? COLOR_WHITE : COLOR_TEXT,
                        .fontSize = 14
                    }));
                }

                Clay_ElementId menu1_id = CLAY_ID("MainDemoMenuItem1");
                bool menu1_hovered = Clay_PointerOver(menu1_id);
                CLAY(menu1_id, {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = { 12, 12, 10, 10 }
                    },
                    .backgroundColor = menu1_hovered ? COLOR_ACCENT : COLOR_TRANSPARENT,
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(CLAY_STRING("Components"), CLAY_TEXT_CONFIG({
                        .textColor = menu1_hovered ? COLOR_WHITE : COLOR_TEXT,
                        .fontSize = 14
                    }));
                }

                Clay_ElementId menu2_id = CLAY_ID("MainDemoMenuItem2");
                bool menu2_hovered = Clay_PointerOver(menu2_id);
                CLAY(menu2_id, {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = { 12, 12, 10, 10 }
                    },
                    .backgroundColor = menu2_hovered ? COLOR_ACCENT : COLOR_TRANSPARENT,
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(CLAY_STRING("Layout"), CLAY_TEXT_CONFIG({
                        .textColor = menu2_hovered ? COLOR_WHITE : COLOR_TEXT,
                        .fontSize = 14
                    }));
                }

                Clay_ElementId menu3_id = CLAY_ID("MainDemoMenuItem3");
                bool menu3_hovered = Clay_PointerOver(menu3_id);
                CLAY(menu3_id, {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = { 12, 12, 10, 10 }
                    },
                    .backgroundColor = menu3_hovered ? COLOR_ACCENT : COLOR_TRANSPARENT,
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(CLAY_STRING("Settings"), CLAY_TEXT_CONFIG({
                        .textColor = menu3_hovered ? COLOR_WHITE : COLOR_TEXT,
                        .fontSize = 14
                    }));
                }
            }

            // Content area
            CLAY(CLAY_ID("MainDemoContent"), {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(16),
                    .childGap = 16,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = COLOR_DARKER,
                .cornerRadius = CLAY_CORNER_RADIUS(8)
            }) {
                CLAY_TEXT(CLAY_STRING("Features Demo"), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_WHITE,
                    .fontSize = 20
                }));

                // Feature cards row
                CLAY(CLAY_ID("MainDemoFeatureCards"), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .childGap = 12
                    }
                }) {
                    // Card 1 - Performance
                    CLAY(CLAY_ID("MainDemoCard1"), {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .padding = CLAY_PADDING_ALL(16),
                            .childGap = 8,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .backgroundColor = COLOR_CARD,
                        .cornerRadius = CLAY_CORNER_RADIUS(8),
                        .border = { .color = COLOR_SUCCESS, .width = { 0, 0, 3, 0, 0 } }
                    }) {
                        CLAY_TEXT(CLAY_STRING("Microsecond Layout"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_SUCCESS,
                            .fontSize = 16
                        }));
                        CLAY_TEXT(CLAY_STRING("Sub-millisecond layout computation"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_TEXT_DIM,
                            .fontSize = 12
                        }));
                    }

                    // Card 2 - Flexbox
                    CLAY(CLAY_ID("MainDemoCard2"), {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .padding = CLAY_PADDING_ALL(16),
                            .childGap = 8,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .backgroundColor = COLOR_CARD,
                        .cornerRadius = CLAY_CORNER_RADIUS(8),
                        .border = { .color = COLOR_ACCENT, .width = { 0, 0, 3, 0, 0 } }
                    }) {
                        CLAY_TEXT(CLAY_STRING("Flexbox Layout"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_ACCENT,
                            .fontSize = 16
                        }));
                        CLAY_TEXT(CLAY_STRING("Familiar flex-based layout model"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_TEXT_DIM,
                            .fontSize = 12
                        }));
                    }

                    // Card 3 - Renderer Agnostic
                    CLAY(CLAY_ID("MainDemoCard3"), {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                            .padding = CLAY_PADDING_ALL(16),
                            .childGap = 8,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .backgroundColor = COLOR_CARD,
                        .cornerRadius = CLAY_CORNER_RADIUS(8),
                        .border = { .color = COLOR_WARNING, .width = { 0, 0, 3, 0, 0 } }
                    }) {
                        CLAY_TEXT(CLAY_STRING("Godot Renderer"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_WARNING,
                            .fontSize = 16
                        }));
                        CLAY_TEXT(CLAY_STRING("Uses Godot CanvasItem drawing"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_TEXT_DIM,
                            .fontSize = 12
                        }));
                    }
                }

                // Buttons row
                CLAY(CLAY_ID("MainDemoButtons"), {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .padding = { 0, 0, 16, 0 },
                        .childGap = 12
                    }
                }) {
                    // Primary button
                    Clay_ElementId btn1_id = CLAY_ID("MainDemoPrimaryBtn");
                    bool btn1_hovered = Clay_PointerOver(btn1_id);
                    CLAY(btn1_id, {
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .padding = { 24, 24, 12, 12 }
                        },
                        .backgroundColor = btn1_hovered ? COLOR_ACCENT_HOVER : COLOR_ACCENT,
                        .cornerRadius = CLAY_CORNER_RADIUS(6)
                    }) {
                        CLAY_TEXT(CLAY_STRING("Primary Action"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_WHITE,
                            .fontSize = 14
                        }));
                    }

                    // Secondary button
                    Clay_ElementId btn2_id = CLAY_ID("MainDemoSecondaryBtn");
                    bool btn2_hovered = Clay_PointerOver(btn2_id);
                    CLAY(btn2_id, {
                        .layout = {
                            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                            .padding = { 24, 24, 12, 12 }
                        },
                        .backgroundColor = btn2_hovered ? COLOR_CARD_HOVER : COLOR_CARD,
                        .cornerRadius = CLAY_CORNER_RADIUS(6),
                        .border = { .color = COLOR_TEXT_DIM, .width = { 1, 1, 1, 1, 0 } }
                    }) {
                        CLAY_TEXT(CLAY_STRING("Secondary"), CLAY_TEXT_CONFIG({
                            .textColor = COLOR_TEXT,
                            .fontSize = 14
                        }));
                    }
                }

                // Info text
                CLAY_TEXT(CLAY_STRING("Hover over elements to see interaction. This UI is built with Clay."), CLAY_TEXT_CONFIG({
                    .textColor = COLOR_TEXT_DIM,
                    .fontSize = 13
                }));
            }
        }

        // Footer
        CLAY(CLAY_ID("MainDemoFooter"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = { 16, 16, 8, 8 },
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER }
            }
        }) {
            CLAY_TEXT(CLAY_STRING("Clay UI Library - github.com/nicbarker/clay"), CLAY_TEXT_CONFIG({
                .textColor = COLOR_TEXT_DIM,
                .fontSize = 12
            }));
        }
    }
}
