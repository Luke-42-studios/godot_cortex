#ifndef CLAY_DEBUG_ELEMENT_H
#define CLAY_DEBUG_ELEMENT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

/**
 * ClayDebugElement - Debug visualization node for Clay UI elements
 *
 * These nodes are automatically created when debug mode is enabled on a ClayWidget.
 * They appear in the Godot scene tree to help visualize the Clay element hierarchy.
 *
 * Each debug element shows:
 * - Element ID (string name like "DemoWidget", "DemoTitleBar", etc.)
 * - Bounding box (position and size)
 * - Element type (rectangle, text, border, etc.)
 */
class ClayDebugElement : public Node {
    GDCLASS(ClayDebugElement, Node);

private:
    String element_id;
    uint32_t element_hash = 0;
    Rect2 bounding_box;
    String element_type;
    int z_index = 0;

protected:
    static void _bind_methods();

public:
    ClayDebugElement();
    ~ClayDebugElement();

    // === Setters (called by ClayWidget during debug update) ===
    void set_element_id(const String& id);
    void set_element_hash(uint32_t hash);
    void set_bounding_box(const Rect2& box);
    void set_element_type(const String& type);
    void set_z_index(int z);

    // === Getters (for inspector visibility) ===
    String get_element_id() const;
    uint32_t get_element_hash() const;
    Rect2 get_bounding_box() const;
    String get_element_type() const;
    int get_z_index() const;

    // === Utility ===
    String get_debug_info() const;
};

} // namespace godot

#endif // CLAY_DEBUG_ELEMENT_H
