// NodeWatcherSystem.cpp
#include "system/node_watcher_system.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

/* --------------------------------------------------------------
   GDCLASS registration – makes the class visible to GDScript.
   -------------------------------------------------------------- */
void NodeWatcherSystem::_bind_methods() {
}

/* --------------------------------------------------------------
   Construction / destruction
   -------------------------------------------------------------- */
NodeWatcherSystem::NodeWatcherSystem() {                     
    singleton_instance = this;
    UtilityFunctions::print("[NodeWatcher] Init");
}

NodeWatcherSystem::~NodeWatcherSystem() {
    if (singleton_instance == this)
        singleton_instance = nullptr;
}

void NodeWatcherSystem::_process(double delta) {
    // Process logic here
}

// Global instance management
NodeWatcherSystem* NodeWatcherSystem::create_global_instance() {
    if (!singleton_instance) {
        singleton_instance = memnew(NodeWatcherSystem);
        // Register as a singleton with the engine so it's accessible globally
        Engine::get_singleton()->register_singleton("NodeWatcherSystem", singleton_instance);
    }
    return singleton_instance;
}

void NodeWatcherSystem::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("NodeWatcherSystem");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}
