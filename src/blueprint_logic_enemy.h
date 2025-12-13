#ifndef BLUEPRINT_LOGIC_ENEMY_H
#define BLUEPRINT_LOGIC_ENEMY_H

#include "blueprint_node.h"

namespace godot {

// ============================================================================
// EnemyLogic - Makes a BlueprintNode act like an enemy
// ============================================================================
class EnemyLogic : public BlueprintLogic {
    GDCLASS(EnemyLogic, BlueprintLogic);

    // Enemy-specific properties
    GD_PROPERTY(float, patrol_radius, 5.0f)
    GD_PROPERTY(float, patrol_speed, 2.0f)
    GD_PROPERTY(float, aggro_range, 10.0f)
    GD_PROPERTY(int, attack_damage, 10)
    GD_PROPERTY(Color, idle_color, Color(0.5f, 0.5f, 0.5f, 1.0f))
    GD_PROPERTY(Color, aggro_color, Color(1.0f, 0.0f, 0.0f, 1.0f))
    GD_PROPERTY(Color, hurt_color, Color(1.0f, 1.0f, 1.0f, 1.0f))

private:
    enum State { IDLE, PATROL, AGGRO, DEAD };
    State current_state = IDLE;
    float patrol_angle = 0.0f;
    float hurt_flash_timer = 0.0f;
    Vector3 start_position;

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(EnemyLogic, float, patrol_radius);
        GD_BIND_PROPERTY(EnemyLogic, float, patrol_speed);
        GD_BIND_PROPERTY(EnemyLogic, float, aggro_range);
        GD_BIND_PROPERTY(EnemyLogic, int, attack_damage);
        GD_BIND_PROPERTY(EnemyLogic, Color, idle_color);
        GD_BIND_PROPERTY(EnemyLogic, Color, aggro_color);
        GD_BIND_PROPERTY(EnemyLogic, Color, hurt_color);

        GD_BIND_METHOD(EnemyLogic, get_state_name);
        GD_BIND_METHOD(EnemyLogic, is_dead);

        GD_BIND_SIGNAL(enemy_died);
        GD_BIND_SIGNAL_1(enemy_attacked, Variant::INT, damage);
    }

public:
    EnemyLogic() {}
    ~EnemyLogic() {}

    void on_ready() override {
        UtilityFunctions::print("EnemyLogic initialized - patrolling!");
        current_state = PATROL;
        if (owner) {
            start_position = owner->get_position();
            owner->set_mesh_color(_idle_color);
        }
    }

    void on_process(double delta) override {
        if (!owner || current_state == DEAD) return;

        // Handle hurt flash
        if (hurt_flash_timer > 0) {
            hurt_flash_timer -= delta;
            if (hurt_flash_timer <= 0) {
                update_color_for_state();
            }
        }

        // Patrol behavior - move in a circle
        if (current_state == PATROL) {
            patrol_angle += _patrol_speed * delta;
            Vector3 offset(
                Math::cos(patrol_angle) * _patrol_radius,
                0,
                Math::sin(patrol_angle) * _patrol_radius
            );
            owner->set_position(start_position + offset);

            // Bob up and down slightly
            MeshInstance3D* mesh = owner->get_mesh_instance();
            if (mesh) {
                Vector3 pos = mesh->get_position();
                pos.y = Math::sin(patrol_angle * 2) * 0.1f;
                mesh->set_position(pos);
            }
        }
    }

    void on_interact() override {
        if (current_state == DEAD) return;

        // Player interacted - enemy attacks!
        UtilityFunctions::print("Enemy attacks for ", _attack_damage, " damage!");
        emit_signal("enemy_attacked", _attack_damage);
    }

    void on_damage(int amount) override {
        UtilityFunctions::print("Enemy took ", amount, " damage!");

        // Flash white when hurt
        if (owner) owner->set_mesh_color(_hurt_color);
        hurt_flash_timer = 0.15f;

        // Check if dead
        if (owner && owner->get_health() <= 0) {
            current_state = DEAD;
            UtilityFunctions::print("Enemy died!");
            emit_signal("enemy_died");

            // Scale down to show death
            MeshInstance3D* mesh = owner->get_mesh_instance();
            if (mesh) {
                mesh->set_scale(Vector3(1.0f, 0.1f, 1.0f));
            }
        }
    }

    String get_state_name() const {
        switch (current_state) {
            case IDLE: return "Idle";
            case PATROL: return "Patrol";
            case AGGRO: return "Aggro";
            case DEAD: return "Dead";
            default: return "Unknown";
        }
    }

    bool is_dead() const { return current_state == DEAD; }

private:
    void update_color_for_state() {
        if (!owner) return;
        switch (current_state) {
            case AGGRO:
                owner->set_mesh_color(_aggro_color);
                break;
            default:
                owner->set_mesh_color(_idle_color);
                break;
        }
    }
};

} // namespace godot

#endif // BLUEPRINT_LOGIC_ENEMY_H
