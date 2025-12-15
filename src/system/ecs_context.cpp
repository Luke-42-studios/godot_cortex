// ECSWorldSystem.cpp - Enhanced implementation
#include "ecs_context.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>

void ECSContext::_bind_methods() {
    // Expose methods to GDScript if needed
}

ECSContext::ECSContext() 
    : ecs_world() {  
    singleton_instance = this;
    
    // Auto-initialize with engine defaults
    _initialize();
}

ECSContext::~ECSContext() {
    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
    
    // Cleanup handled in _shutdown()
    _shutdown();
}

void ECSContext::_initialize() {
    ecs_world.set_target_fps(60.0f);
    // Add your ECS initialization here
}

void ECSContext::_shutdown() {
    ecs_world.quit();
}

// Global instance management
ECSContext* ECSContext::create_global_instance() {
    if (!singleton_instance) {
        singleton_instance = memnew(ECSContext);
        Engine::get_singleton()->register_singleton("FlecsWorld", singleton_instance);
    }
    return singleton_instance;
}

void ECSContext::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("FlecsWorld");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}
