// ECSWorldSystem.cpp - Enhanced implementation with performance optimizations
#include "PECSContext.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>

using namespace godot;

void PECSContext::_bind_methods() {
    ClassDB::bind_method(D_METHOD("print_state"), &PECSContext::print_state);
}

PECSContext::PECSContext()
    : m_world(std::make_unique<flecs::world>()) {

    singleton_instance = this;

    // Register components immediately
    _initialize();
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

    // Register core ECS components with Flecs
    // This gives them proper names for debugging and enables queries
    world.component<CGodotNode>();
    world.component<CNodeDepth>();
    world.component<CTreeId>();

    UtilityFunctions::print("[PECSContext] Components registered: CGodotNode, CNodeDepth, CTreeId");
}

void PECSContext::_shutdown() {
    // HOT PATH: Cleanup should be efficient but complete
    if (m_world) {
        m_world->quit();
    }
}

void PECSContext::print_state() const {
    UtilityFunctions::print("[PECSContext] ==========================================");
    UtilityFunctions::print("[PECSContext] Flecs World State");
    UtilityFunctions::print("[PECSContext] ==========================================");

    if (!m_world) {
        UtilityFunctions::print("[PECSContext] <world not initialized>");
        return;
    }

    int entity_count = 0;
    int component_count = 0;

    // Query entities that have CGodotNode component (our node entities)
    m_world->each<CGodotNode>([&](flecs::entity e, CGodotNode& gn) {
        entity_count++;

        const char* name = e.name().c_str();
        String entity_name = (name && name[0] != '\0') ? String(name) : String("<unnamed>");

        String node_class = "?";
        if (gn.ptr) {
            node_class = gn.ptr->get_class();
        }

        UtilityFunctions::print("[PECSContext] Entity ", e.id(), ": ", entity_name, " [", node_class, "]");

        // Print all components/tags on this entity
        e.each([&](flecs::id id) {
            component_count++;

            String comp_str;

            if (id.is_pair()) {
                // Relationship pair
                flecs::entity first = id.first();
                flecs::entity second = id.second();
                const char* first_name = first.name().c_str();
                const char* second_name = second.name().c_str();
                comp_str = String("  (") + (first_name ? first_name : "?") +
                           ", " + (second_name ? second_name : "?") + ")";
            } else if (id.is_entity()) {
                // Component or tag
                flecs::entity comp = id.entity();
                const char* comp_name = comp.name().c_str();
                if (comp_name && comp_name[0] != '\0') {
                    comp_str = String("  - ") + comp_name;
                } else {
                    comp_str = String("  - <id:") + String::num_int64(comp.id()) + ">";
                }
            }

            if (!comp_str.is_empty()) {
                UtilityFunctions::print("[PECSContext]", comp_str);
            }
        });

        UtilityFunctions::print("[PECSContext]");  // Blank line between entities
    });

    UtilityFunctions::print("[PECSContext] ------------------------------------------");
    UtilityFunctions::print("[PECSContext] Total entities: ", entity_count);
    UtilityFunctions::print("[PECSContext] Total components/tags: ", component_count);
    UtilityFunctions::print("[PECSContext] ==========================================");
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
