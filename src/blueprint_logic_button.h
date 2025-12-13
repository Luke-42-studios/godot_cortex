#ifndef BLUEPRINT_LOGIC_BUTTON_H
#define BLUEPRINT_LOGIC_BUTTON_H

#include "blueprint_node.h"

namespace godot {

// ============================================================================
// ButtonLogic - Makes a BlueprintNode act like a pressable button
// ============================================================================
class ButtonLogic : public BlueprintLogic {
    GDCLASS(ButtonLogic, BlueprintLogic);

    // Button-specific properties (also editable in inspector!)
    GD_PROPERTY(float, press_depth, 0.2f)
    GD_PROPERTY(float, press_speed, 5.0f)
    GD_PROPERTY(Color, pressed_color, Color(0.2f, 0.8f, 0.2f, 1.0f))
    GD_PROPERTY(Color, default_color, Color(0.8f, 0.2f, 0.2f, 1.0f))

private:
    bool is_pressed = false;
    float current_depth = 0.0f;
    int press_count = 0;

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(ButtonLogic, float, press_depth);
        GD_BIND_PROPERTY(ButtonLogic, float, press_speed);
        GD_BIND_PROPERTY(ButtonLogic, Color, pressed_color);
        GD_BIND_PROPERTY(ButtonLogic, Color, default_color);

        GD_BIND_METHOD(ButtonLogic, get_press_count);
        GD_BIND_METHOD(ButtonLogic, is_button_pressed);

        GD_BIND_SIGNAL(button_pressed);
        GD_BIND_SIGNAL(button_released);
    }

public:
    ButtonLogic() {}
    ~ButtonLogic() {}

    void on_ready() override {
        UtilityFunctions::print("ButtonLogic initialized!");
        if (owner) {
            owner->set_mesh_color(_default_color);
        }
    }

    void on_process(double delta) override {
        if (!owner) return;

        // Animate button press/release
        float target_depth = is_pressed ? _press_depth : 0.0f;
        current_depth = Math::lerp(current_depth, target_depth, (float)delta * _press_speed);

        // Move the mesh down when pressed
        MeshInstance3D* mesh = owner->get_mesh_instance();
        if (mesh) {
            Vector3 pos = mesh->get_position();
            pos.y = -current_depth;
            mesh->set_position(pos);
        }
    }

    void on_interact() override {
        // Toggle pressed state
        is_pressed = !is_pressed;

        if (is_pressed) {
            press_count++;
            UtilityFunctions::print("Button PRESSED! Count: ", press_count);
            if (owner) owner->set_mesh_color(_pressed_color);
            emit_signal("button_pressed");
        } else {
            UtilityFunctions::print("Button released");
            if (owner) owner->set_mesh_color(_default_color);
            emit_signal("button_released");
        }
    }

    int get_press_count() const { return press_count; }
    bool is_button_pressed() const { return is_pressed; }
};

} // namespace godot

#endif // BLUEPRINT_LOGIC_BUTTON_H
