// ECSWorldSystem.cpp - Enhanced implementation with performance optimizations
#include "PECSContext.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void PECSContext::_bind_methods() {
    // Expose methods to GDScript if needed
}

PECSContext::PECSContext() 
    : m_world(std::make_unique<flecs::world>()) {  
    
    singleton_instance = this;
    
    // PERF: Defer expensive initialization until _initialize() is called by Godot
    // This allows proper lifecycle management and batching of operations
}

PECSContext::~PECSContext() {
    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
    
    // Cleanup handled in _shutdown()
    _shutdown();
}

void PECSContext::_initialize() {
    // COLD PATH: Initialization can take time, optimize for clarity over micro-performance
    auto& world = get_world();
    world.set_target_fps(60.0f);
    
    // TODO: Add component registration here
    // Register core ECS components following Flecs patterns
}

void PECSContext::_shutdown() {
    // HOT PATH: Cleanup should be efficient but complete
    if (m_world) {
        m_world->quit();
    }
}

// Global instance management with proper memory management
PECSContext* PECSContext::create_global_instance() {
    static std::once_flag init_once;
    
    std::call_once(init_once, []() {
        singleton_instance = memnew(PECSContext);
        Engine::get_singleton()->register_singleton("FlecsWorld", singleton_instance);
    });
    
    return singleton_instance;
}

void PECSContext::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("FlecsWorld");
        
        // PERF: Ensure proper cleanup order
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}
