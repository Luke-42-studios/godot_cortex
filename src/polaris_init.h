#ifndef POLARIS_INIT_H
#define POLARIS_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Core singletons
#include "core/Engine.h"
#include "core/ECSWorld.h"
#include "core/PipelineNode.h"
#include "core/CompositionFactory.h"

// Composition system
#include "composition/Composition.h"
#include "composition/TestComposition.h"

namespace godot {

// ============================================================================
// Module Initialization - Polaris Framework
// ============================================================================

/// @brief Registers all Polaris framework classes with Godot's class system
inline void polaris_register_classes() {
    // Core singletons
    GDREGISTER_CLASS(Polaris::PolarisEngine);
    GDREGISTER_CLASS(Polaris::ECSWorld);

    // Core nodes
    GDREGISTER_CLASS(Polaris::PipelineNode);
    GDREGISTER_CLASS(Polaris::CompositionFactory);

    // Composition resources
    GDREGISTER_CLASS(Polaris::Composition);
    GDREGISTER_CLASS(Polaris::TestComposition);

    UtilityFunctions::print("[Polaris] Framework classes registered");

    // Create Engine singleton - this initializes ECSWorld internally
    Polaris::PolarisEngine::create_global_instance();
}

// ============================================================================
// Module Cleanup
// ============================================================================

inline void polaris_unregister_classes() {
    // Engine handles shutdown of all subsystems (including ECSWorld)
    Polaris::PolarisEngine::destroy_global_instance();
}

} // namespace godot

#endif // POLARIS_INIT_H
