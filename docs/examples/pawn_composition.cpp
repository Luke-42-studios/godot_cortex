// examples/pawn_composition.cpp
// Complete Pawn hierarchy showing Composition pattern in Polaris
//
// NAMESPACE CONVENTION:
//   Polaris::      - Engine code (Runtime, Gd::, Tag::)
//   Game::         - Your game code (components flat, domains nested)
//
// Polaris provides:  Gd::Node3D, Gd::CharacterBody3D, Tag::Player, etc.
// You define:        Health, Velocity, AI, and game-specific tags

#include "polaris/core/Runtime.h"
#include "polaris/composition/Composition.h"
#include "polaris/components/Gd.h"  // Provides Polaris::Gd::

using namespace Polaris;  // Access Gd::, Tag::, runtime(), etc.

namespace Game {

// ============================================================================
// DATA COMPONENTS (flat in Game:: namespace)
// ============================================================================

struct Health {
    float current = 100.0f;
    float max = 100.0f;
};

struct Velocity {
    godot::Vector3 linear;
    godot::Vector3 angular;
};

struct MoveSpeed {
    float value = 5.0f;
};

struct InputState {
    godot::Vector2 move;
    godot::Vector2 look;
    bool jump = false;
    bool attack = false;
};

enum class AIState : uint8_t {
    Idle, Patrol, Chase, Attack
};

struct AI {
    flecs::entity target;
    float timer = 0.0f;
    float aggro_range = 10.0f;
    AIState state = AIState::Idle;
    uint8_t padding[3];
};

// ============================================================================
// GAME-SPECIFIC TAGS (extend Polaris::Tag:: with your own)
// ============================================================================
// Note: Polaris provides Tag::Player, Tag::Enemy, Tag::Grounded, Tag::Dead
// Add game-specific tags here:

namespace Tag {
    struct Interactable {};
    struct QuestGiver {};
}

// ============================================================================
// Gd:: COMPONENT EXAMPLES
// ============================================================================
// In production, Polaris::Gd:: provides these. Shown here for reference.

namespace GdExample {
    struct Node3D {
        godot::Node3D* root = nullptr;

        static Node3D create(godot::Node* n) {
            return { godot::Object::cast_to<godot::Node3D>(n) };
        }

        bool is_valid() const { return root != nullptr; }
        void unbind() { root = nullptr; }

        godot::Vector3 get_position() const {
            return is_valid() ? root->get_position() : godot::Vector3();
        }

        void set_position(const godot::Vector3& pos) {
            if (is_valid()) root->set_position(pos);
        }

        godot::Vector3 get_forward() const {
            return is_valid() ? -root->get_global_transform().basis.get_column(2) : godot::Vector3();
        }
    };

    struct CharacterBody3D {
        godot::CharacterBody3D* ptr = nullptr;

        static CharacterBody3D create(godot::Node* n) {
            return { godot::Object::cast_to<godot::CharacterBody3D>(n) };
        }

        bool is_valid() const { return ptr != nullptr; }
        void unbind() { ptr = nullptr; }
    };

    struct AnimationPlayer {
        godot::AnimationPlayer* ptr = nullptr;

        static AnimationPlayer create(godot::Node* root, const char* child_name = "AnimationPlayer") {
            if (!root) return { nullptr };
            auto* child = root->find_child(godot::String(child_name));
            return { godot::Object::cast_to<godot::AnimationPlayer>(child) };
        }

        bool is_valid() const { return ptr != nullptr; }
        void unbind() { ptr = nullptr; }
    };
}

// ============================================================================
// PAWN - Base class for all characters
// ============================================================================

class Pawn : public Polaris::Composition {
    GDCLASS(Pawn, Polaris::Composition)

protected:
    // Godot properties - exposed to inspector
    float max_health = 100.0f;
    float move_speed = 5.0f;

public:
    // -------------------------------------------------------------------------
    // Composition Lifecycle (comes FIRST - this is the main logic)
    // -------------------------------------------------------------------------

    // Called when node enters scene tree
    void compose(flecs::entity e, godot::Node* node) override {
        // Base Composition sets Gd::Node (auto-detected)
        Polaris::Composition::compose(e, node);

        // Set Gd:: components - bridge to Godot scene tree
        e.set<Gd::Node3D>(Gd::Node3D::create(node));
        e.set<Gd::CharacterBody3D>(Gd::CharacterBody3D::create(node));
        e.set<Gd::AnimationPlayer>(Gd::AnimationPlayer::create(node, "AnimationPlayer"));

        // Set data components - game state
        e.set<Health>({ max_health, max_health });
        e.set<Velocity>({ godot::Vector3(), godot::Vector3() });
        e.set<MoveSpeed>({ move_speed });
    }

    // Called when node exits scene tree
    void decompose(flecs::entity e) override {
        // Unbind Godot pointers to prevent dangling references
        if (auto* n = e.try_get_mut<Gd::Node3D>()) n->unbind();
        if (auto* b = e.try_get_mut<Gd::CharacterBody3D>()) b->unbind();
        if (auto* a = e.try_get_mut<Gd::AnimationPlayer>()) a->unbind();

        // Base decompose destroys entity
        Polaris::Composition::decompose(e);
    }

    // -------------------------------------------------------------------------
    // Godot Bindings (comes LAST - boilerplate)
    // -------------------------------------------------------------------------

protected:
    static void _bind_methods() {
        // Expose properties to Godot inspector
        ClassDB::bind_method(D_METHOD("get_max_health"), &Pawn::get_max_health);
        ClassDB::bind_method(D_METHOD("set_max_health", "value"), &Pawn::set_max_health);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_health"), "set_max_health", "get_max_health");

        ClassDB::bind_method(D_METHOD("get_move_speed"), &Pawn::get_move_speed);
        ClassDB::bind_method(D_METHOD("set_move_speed", "value"), &Pawn::set_move_speed);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed"), "set_move_speed", "get_move_speed");
    }

    float get_max_health() const { return max_health; }
    void set_max_health(float value) { max_health = value; }
    float get_move_speed() const { return move_speed; }
    void set_move_speed(float value) { move_speed = value; }
};

// ============================================================================
// PLAYER PAWN - Player-controlled character
// ============================================================================

class PlayerPawn : public Pawn {
    GDCLASS(PlayerPawn, Pawn)

public:
    // Composition Lifecycle
    void compose(flecs::entity e, godot::Node* node) override {
        // Call base to set up common components
        Pawn::compose(e, node);

        // Add player-specific components
        e.set<InputState>({});
        e.add<Tag::Player>();

        // Player-specific configuration
        auto* speed = e.get_mut<MoveSpeed>();
        if (speed) speed->value = move_speed * 1.2f;  // Players move faster
    }

    // Godot Bindings
protected:
    static void _bind_methods() {
        // Inherit parent properties
    }
};

// ============================================================================
// AI PAWN - AI-controlled character
// ============================================================================

class AIPawn : public Pawn {
    GDCLASS(AIPawn, Pawn)

protected:
    float aggro_range = 10.0f;
    float patrol_radius = 5.0f;

public:
    // Composition Lifecycle
    void compose(flecs::entity e, godot::Node* node) override {
        // Call base to set up common components
        Pawn::compose(e, node);

        // Add AI-specific components
        e.set<AI>({
            .target = {},
            .timer = 0.0f,
            .aggro_range = aggro_range,
            .state = AIState::Idle
        });
        e.add<Tag::Enemy>();

        // AI-specific configuration
        auto* hp = e.get_mut<Health>();
        if (hp) {
            hp->current = max_health * 0.8f;  // AI starts slightly damaged
            hp->max = max_health * 0.8f;
        }
    }

    // Godot Bindings
protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("get_aggro_range"), &AIPawn::get_aggro_range);
        ClassDB::bind_method(D_METHOD("set_aggro_range", "value"), &AIPawn::set_aggro_range);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "aggro_range"), "set_aggro_range", "get_aggro_range");

        ClassDB::bind_method(D_METHOD("get_patrol_radius"), &AIPawn::get_patrol_radius);
        ClassDB::bind_method(D_METHOD("set_patrol_radius", "value"), &AIPawn::set_patrol_radius);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "patrol_radius"), "set_patrol_radius", "get_patrol_radius");
    }

    float get_aggro_range() const { return aggro_range; }
    void set_aggro_range(float value) { aggro_range = value; }
    float get_patrol_radius() const { return patrol_radius; }
    void set_patrol_radius(float value) { patrol_radius = value; }
};

// ============================================================================
// USAGE IN GODOT
// ============================================================================

/*
 * 1. Create scene tree:
 *    
 *    PlayerPawn (CharacterBody3D)
 *    ├── MeshInstance3D
 *    ├── CollisionShape3D
 *    └── AnimationPlayer
 *
 * 2. Attach PlayerPawn script to root node
 * 
 * 3. Set metadata in inspector:
 *    metadata["composition"] = "PlayerPawn"
 *
 * 4. When node enters tree:
 *    - CompositionFactory detects metadata
 *    - Calls PlayerPawn::compose()
 *    - Entity created with all components
 *    - Systems start processing entity
 *
 * 5. When node exits tree:
 *    - CompositionFactory calls PlayerPawn::decompose()
 *    - Godot pointers unbound
 *    - Entity destroyed
 */

}
