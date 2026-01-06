#include "ECSWorld.h"
#include "util/Log.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/memory.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void ECSWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_entity_count"), &ECSWorld::get_entity_count);
    ClassDB::bind_method(D_METHOD("progress", "delta"), &ECSWorld::progress);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "entity_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_entity_count");
}

// =============================================================================
// Construction / Destruction
// =============================================================================

ECSWorld::ECSWorld() {
    singleton_instance = this;

    // Don't let flecs manage timing - Godot handles frame rate
    m_world.set_target_fps(0);

    Log::info("[ECSWorld] Created - flecs world initialized");
}

ECSWorld::~ECSWorld() {
    Log::info("[ECSWorld] Destroying flecs world");
    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// GDScript API
// =============================================================================

int ECSWorld::get_entity_count() const {
    // Get entity count from world info
    const ecs_world_info_t* info = ecs_get_world_info(m_world.c_ptr());
    return info ? static_cast<int>(info->last_component_id) : 0;
}

void ECSWorld::progress(float delta) {
    m_world.progress(delta);
}

// =============================================================================
// Singleton Management
// =============================================================================

ECSWorld* ECSWorld::create_global_instance() {
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(ECSWorld);
        godot::Engine::get_singleton()->register_singleton("ECSWorld", singleton_instance);
        Log::info("[ECSWorld] Registered as Godot singleton");
    });

    return singleton_instance;
}

void ECSWorld::destroy_global_instance() {
    if (singleton_instance) {
        godot::Engine::get_singleton()->unregister_singleton("ECSWorld");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace Polaris
