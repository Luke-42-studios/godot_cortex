#include "Engine.h"
#include "polaris_init.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/context.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void PolarisEngine::_bind_methods() {
    
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

}

void PolarisEngine::shutdown() {
    
}


// =============================================================================
// Singleton Management
// =============================================================================

PolarisEngine* PolarisEngine::create_global_instance() {
    // std::call_once guarantees thread-safe one-time initialization.
    //
    // WHY WE NEED THIS:
    // If two threads call create_global_instance() simultaneously without
    // protection, both might see singleton_instance as nullptr and both
    // would create instances - causing a memory leak and undefined behavior.
    //
    // std::call_once uses an internal mutex to ensure the lambda runs
    // exactly once, even under concurrent access. The std::once_flag
    // tracks whether initialization has completed.
    //
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
