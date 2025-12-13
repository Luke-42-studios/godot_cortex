#include "option_menu_context.h"
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/audio_server.hpp>

namespace godot {

// ============================================================================
// Static data
// ============================================================================
const OptionMenuContext::Resolution OptionMenuContext::RESOLUTIONS[RESOLUTION_COUNT] = {
    { 1280, 720,  "1280x720 (720p)" },
    { 1920, 1080, "1920x1080 (1080p)" },
    { 2560, 1440, "2560x1440 (1440p)" },
    { 3840, 2160, "3840x2160 (4K)" }
};

// ============================================================================
// Bindings
// ============================================================================
void OptionMenuContext::_bind_methods() {
    // Properties
    GD_BIND_PROPERTY(OptionMenuContext, String, config_path);
    GD_BIND_PROPERTY(OptionMenuContext, bool, fullscreen);
    GD_BIND_PROPERTY_HINT(OptionMenuContext, float, master_volume, PROPERTY_HINT_RANGE, "0,1,0.01");
    GD_BIND_PROPERTY_HINT(OptionMenuContext, float, music_volume, PROPERTY_HINT_RANGE, "0,1,0.01");
    GD_BIND_PROPERTY_HINT(OptionMenuContext, float, sfx_volume, PROPERTY_HINT_RANGE, "0,1,0.01");
    GD_BIND_PROPERTY_HINT(OptionMenuContext, int, resolution_index, PROPERTY_HINT_RANGE, "0,3,1");

    // Methods exposed to Godot
    GD_BIND_METHOD(OptionMenuContext, load_settings);
    GD_BIND_METHOD(OptionMenuContext, save_settings);
    GD_BIND_METHOD(OptionMenuContext, apply_settings);
    GD_BIND_METHOD(OptionMenuContext, reset_to_defaults);
    GD_BIND_METHOD(OptionMenuContext, get_current_resolution_name);
    GD_BIND_METHOD(OptionMenuContext, cycle_resolution);
    GD_BIND_METHOD(OptionMenuContext, toggle_fullscreen);
    GD_BIND_METHOD(OptionMenuContext, is_menu_open);
    GD_BIND_METHOD(OptionMenuContext, has_unsaved_changes);

    // Signals
    ADD_SIGNAL(MethodInfo("settings_changed"));
    ADD_SIGNAL(MethodInfo("settings_saved"));
    ADD_SIGNAL(MethodInfo("settings_loaded"));
    ADD_SIGNAL(MethodInfo("menu_toggled", PropertyInfo(Variant::BOOL, "is_open")));
}

// ============================================================================
// Constructor / Destructor
// ============================================================================
OptionMenuContext::OptionMenuContext() {
    UtilityFunctions::print("[OptionMenuContext] Created");
}

OptionMenuContext::~OptionMenuContext() {
    UtilityFunctions::print("[OptionMenuContext] Destroyed");
}

// ============================================================================
// Lifecycle
// ============================================================================
void OptionMenuContext::ctx_ready() {
    UtilityFunctions::print("[OptionMenuContext] Ready - loading settings from: ", _config_path);
    load_settings();
}

void OptionMenuContext::ctx_process(double delta) {
    // Could do things like auto-save after a delay
    // or animate menu transitions
}

// ============================================================================
// Button Interface
// ============================================================================
void OptionMenuContext::on_pressed() {
    menu_open = !menu_open;

    UtilityFunctions::print("========================================");
    UtilityFunctions::print("  OPTIONS MENU ", menu_open ? "OPENED" : "CLOSED");
    UtilityFunctions::print("========================================");

    if (menu_open) {
        UtilityFunctions::print("  Current Settings:");
        UtilityFunctions::print("    Resolution: ", get_current_resolution_name());
        UtilityFunctions::print("    Fullscreen: ", _fullscreen ? "ON" : "OFF");
        UtilityFunctions::print("    Master Vol: ", String::num(_master_volume * 100, 0), "%");
        UtilityFunctions::print("    Music Vol:  ", String::num(_music_volume * 100, 0), "%");
        UtilityFunctions::print("    SFX Vol:    ", String::num(_sfx_volume * 100, 0), "%");
    } else {
        if (settings_dirty) {
            UtilityFunctions::print("  (Unsaved changes detected)");
        }
    }
    UtilityFunctions::print("========================================");

    emit_signal("menu_toggled", menu_open);

    // Access the button
    ClayButtonNode* btn = get_node<ClayButtonNode>();
    if (btn) {
        btn->set_label(menu_open ? "Close Options" : "Options");
    }
}

void OptionMenuContext::on_released() {
    // Nothing special on release
}

// ============================================================================
// Settings Management
// ============================================================================
void OptionMenuContext::load_settings() {
    UtilityFunctions::print("[OptionMenuContext] Loading settings...");

    Ref<FileAccess> file = FileAccess::open(_config_path, FileAccess::READ);
    if (file.is_valid()) {
        String content = file->get_as_text();
        file->close();

        Ref<JSON> json;
        json.instantiate();
        Error err = json->parse(content);

        if (err == OK) {
            current_settings = json->get_data();

            // Apply loaded values
            if (current_settings.has("fullscreen")) {
                _fullscreen = current_settings["fullscreen"];
            }
            if (current_settings.has("master_volume")) {
                _master_volume = current_settings["master_volume"];
            }
            if (current_settings.has("music_volume")) {
                _music_volume = current_settings["music_volume"];
            }
            if (current_settings.has("sfx_volume")) {
                _sfx_volume = current_settings["sfx_volume"];
            }
            if (current_settings.has("resolution_index")) {
                _resolution_index = current_settings["resolution_index"];
            }

            UtilityFunctions::print("[OptionMenuContext] Settings loaded successfully");
            emit_signal("settings_loaded");
        } else {
            UtilityFunctions::print("[OptionMenuContext] Failed to parse settings JSON");
        }
    } else {
        UtilityFunctions::print("[OptionMenuContext] No settings file found, using defaults");
    }

    settings_dirty = false;
}

void OptionMenuContext::save_settings() {
    UtilityFunctions::print("[OptionMenuContext] Saving settings...");

    // Build settings dictionary
    current_settings["fullscreen"] = _fullscreen;
    current_settings["master_volume"] = _master_volume;
    current_settings["music_volume"] = _music_volume;
    current_settings["sfx_volume"] = _sfx_volume;
    current_settings["resolution_index"] = _resolution_index;

    // Convert to JSON
    String json_string = JSON::stringify(current_settings, "  ");

    // Write to file
    Ref<FileAccess> file = FileAccess::open(_config_path, FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_string(json_string);
        file->close();

        UtilityFunctions::print("[OptionMenuContext] Settings saved to: ", _config_path);
        settings_dirty = false;
        emit_signal("settings_saved");
    } else {
        UtilityFunctions::print("[OptionMenuContext] ERROR: Could not save settings!");
    }
}

void OptionMenuContext::apply_settings() {
    UtilityFunctions::print("[OptionMenuContext] Applying settings...");

    // Apply fullscreen
    DisplayServer* ds = DisplayServer::get_singleton();
    if (ds) {
        if (_fullscreen) {
            ds->window_set_mode(DisplayServer::WINDOW_MODE_FULLSCREEN);
        } else {
            ds->window_set_mode(DisplayServer::WINDOW_MODE_WINDOWED);
            // Set resolution
            if (_resolution_index >= 0 && _resolution_index < RESOLUTION_COUNT) {
                Vector2i size(RESOLUTIONS[_resolution_index].width,
                             RESOLUTIONS[_resolution_index].height);
                ds->window_set_size(size);
            }
        }
    }

    // Apply audio volumes
    AudioServer* audio = AudioServer::get_singleton();
    if (audio) {
        // Master bus is typically index 0
        int master_idx = audio->get_bus_index("Master");
        if (master_idx >= 0) {
            // Convert linear to dB (0 = -80dB, 1 = 0dB)
            float db = _master_volume > 0 ? 20.0f * log10(_master_volume) : -80.0f;
            audio->set_bus_volume_db(master_idx, db);
        }

        // Music bus
        int music_idx = audio->get_bus_index("Music");
        if (music_idx >= 0) {
            float db = _music_volume > 0 ? 20.0f * log10(_music_volume) : -80.0f;
            audio->set_bus_volume_db(music_idx, db);
        }

        // SFX bus
        int sfx_idx = audio->get_bus_index("SFX");
        if (sfx_idx >= 0) {
            float db = _sfx_volume > 0 ? 20.0f * log10(_sfx_volume) : -80.0f;
            audio->set_bus_volume_db(sfx_idx, db);
        }
    }

    UtilityFunctions::print("[OptionMenuContext] Settings applied");
    emit_signal("settings_changed");
}

void OptionMenuContext::reset_to_defaults() {
    UtilityFunctions::print("[OptionMenuContext] Resetting to defaults...");

    _fullscreen = false;
    _master_volume = 1.0f;
    _music_volume = 0.8f;
    _sfx_volume = 0.8f;
    _resolution_index = 1;  // 1080p

    settings_dirty = true;
    emit_signal("settings_changed");
}

// ============================================================================
// Resolution helpers
// ============================================================================
String OptionMenuContext::get_current_resolution_name() const {
    if (_resolution_index >= 0 && _resolution_index < RESOLUTION_COUNT) {
        return RESOLUTIONS[_resolution_index].name;
    }
    return "Unknown";
}

void OptionMenuContext::cycle_resolution() {
    _resolution_index = (_resolution_index + 1) % RESOLUTION_COUNT;
    settings_dirty = true;

    UtilityFunctions::print("[OptionMenuContext] Resolution: ", get_current_resolution_name());
    emit_signal("settings_changed");
}

void OptionMenuContext::toggle_fullscreen() {
    _fullscreen = !_fullscreen;
    settings_dirty = true;

    UtilityFunctions::print("[OptionMenuContext] Fullscreen: ", _fullscreen ? "ON" : "OFF");
    emit_signal("settings_changed");
}

// ============================================================================
// Volume controls
// ============================================================================
void OptionMenuContext::set_master_volume_normalized(float value) {
    _master_volume = Math::clamp(value, 0.0f, 1.0f);
    settings_dirty = true;
    emit_signal("settings_changed");
}

void OptionMenuContext::set_music_volume_normalized(float value) {
    _music_volume = Math::clamp(value, 0.0f, 1.0f);
    settings_dirty = true;
    emit_signal("settings_changed");
}

void OptionMenuContext::set_sfx_volume_normalized(float value) {
    _sfx_volume = Math::clamp(value, 0.0f, 1.0f);
    settings_dirty = true;
    emit_signal("settings_changed");
}

} // namespace godot
