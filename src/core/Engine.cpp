#include "Engine.h"
#include "util/Log.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void PolarisEngine::_bind_methods() {
    // Expose ecs() to GDScript if needed
    // ClassDB::bind_method(D_METHOD("get_ecs"), &PolarisEngine::ecs);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

PolarisEngine::PolarisEngine() {
    singleton_instance = this;
    Log::info("[PolarisEngine] Created");
}

PolarisEngine::~PolarisEngine() {
    shutdown();

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// Lifecycle Management
// =============================================================================

void PolarisEngine::initialize() {
    Log::info("[PolarisEngine] Initializing subsystems...");

    // Initialize ECS first - everything depends on this
    ECSWorld::create_global_instance();

    Log::info("[PolarisEngine] All subsystems initialized");
}

void PolarisEngine::shutdown() {
    Log::info("[PolarisEngine] Shutting down subsystems...");

    // Shutdown in reverse order
    ECSWorld::destroy_global_instance();

    Log::info("[PolarisEngine] Shutdown complete");
}

// =============================================================================
// Singleton Management
// =============================================================================

PolarisEngine* PolarisEngine::create_global_instance() {
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(PolarisEngine);
        godot::Engine::get_singleton()->register_singleton("Polaris", singleton_instance);
        singleton_instance->initialize();
    });

    return singleton_instance;
}

void PolarisEngine::destroy_global_instance() {
    if (singleton_instance) {
        godot::Engine::get_singleton()->unregister_singleton("Polaris");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace Polaris
