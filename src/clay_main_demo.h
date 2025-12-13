#ifndef CLAY_MAIN_DEMO_H
#define CLAY_MAIN_DEMO_H

#include "clay_widget.h"

namespace godot {

/**
 * ClayMainDemo - The main demo widget showing Clay's capabilities
 *
 * This is the demo that was previously built into ClayUI's demo_mode.
 * It demonstrates:
 * - Full-screen layouts with sidebar
 * - Cards and panels
 * - Hover states
 * - Text rendering
 */
class ClayMainDemo : public ClayWidget {
    GDCLASS(ClayMainDemo, ClayWidget);

protected:
    static void _bind_methods();

public:
    ClayMainDemo();

    void build() override;
};

} // namespace godot

#endif // CLAY_MAIN_DEMO_H
