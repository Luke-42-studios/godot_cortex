#ifndef OPTION_MENU_CONTEXT_H
#define OPTION_MENU_CONTEXT_H

#include "clay_button_node.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>

namespace godot {

// ============================================================================
// OptionMenuContext - A more complex context with .cpp implementation
// ============================================================================
// This demonstrates separating declaration from implementation.
// The logic here doesn't interact with the editor - it's pure runtime behavior.
class OptionMenuContext : public ClayButtonContext {
    GDCLASS(OptionMenuContext, ClayButtonContext);

    // Properties
    GD_PROPERTY(String, config_path, "user://settings.json")
    GD_PROPERTY(bool, fullscreen, false)
    GD_PROPERTY(float, master_volume, 1.0f)
    GD_PROPERTY(float, music_volume, 0.8f)
    GD_PROPERTY(float, sfx_volume, 0.8f)
    GD_PROPERTY(int, resolution_index, 0)

private:
    // Internal state
    bool settings_dirty = false;
    bool menu_open = false;
    Dictionary current_settings;

    // Available resolutions
    static const int RESOLUTION_COUNT = 4;
    struct Resolution { int width; int height; const char* name; };
    static const Resolution RESOLUTIONS[RESOLUTION_COUNT];

protected:
    static void _bind_methods();

public:
    OptionMenuContext();
    ~OptionMenuContext();

    // Lifecycle
    void ctx_ready() override;
    void ctx_process(double delta) override;

    // Button interface
    void on_pressed() override;
    void on_released() override;

    // Settings management (implemented in .cpp)
    void load_settings();
    void save_settings();
    void apply_settings();
    void reset_to_defaults();

    // Getters for UI
    String get_current_resolution_name() const;
    void cycle_resolution();
    void toggle_fullscreen();

    // Volume controls
    void set_master_volume_normalized(float value);
    void set_music_volume_normalized(float value);
    void set_sfx_volume_normalized(float value);

    // State
    bool is_menu_open() const { return menu_open; }
    bool has_unsaved_changes() const { return settings_dirty; }
};

} // namespace godot

#endif // OPTION_MENU_CONTEXT_H
