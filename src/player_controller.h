#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include "cpp_script.h"
#include <godot_cpp/classes/input.hpp>

namespace godot {

/**
 * PlayerController - Example C++ gameplay script
 *
 * Attach this to a Node2D or Node3D to add player movement.
 * This demonstrates how to write gameplay logic in C++.
 */
class PlayerController : public CPPScript {
    GDCLASS(PlayerController, CPPScript);

private:
    float move_speed = 200.0f;
    float rotation_speed = 3.0f;
    Vector2 velocity;

protected:
    static void _bind_methods();

    // Gameplay logic
    void on_start() override;
    void on_update(double delta) override;
    void on_fixed_update(double delta) override;

public:
    PlayerController();
    ~PlayerController();

    // Properties
    void set_move_speed(float speed);
    float get_move_speed() const;

    void set_rotation_speed(float speed);
    float get_rotation_speed() const;

    // Methods
    Vector2 get_velocity() const;
};

} // namespace godot

#endif // PLAYER_CONTROLLER_H
