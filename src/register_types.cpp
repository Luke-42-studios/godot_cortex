#include "register_types.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "cpp_node.h"
#include "c_node.h"
#include "simple_node.h"
#include "cpp_script.h"
#include "player_controller.h"
#include "physics_body_3d_setup.h"
#include "custom_mesh_3d.h"
#include "clay_2d_canvas.h"
#include "clay_3d_canvas.h"
#include "clay_widget.h"
#include "clay_widget_script.h"
#include "clay_scroll_view.h"
#include "clay_debug_element.h"
#include "clay_demo_widget.h"
#include "clay_main_demo.h"
#include "flecs_world.h"
#include "cnode.h"
#include "clay_button_node.h"
#include "start_game_context.h"
#include "quit_game_context.h"
#include "option_menu_context.h"

using namespace godot;

// Singleton instance
static FlecsWorld* flecs_world_singleton = nullptr;

void initialize_cortex_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Register custom classes
    GDREGISTER_CLASS(CPP_Node);
    GDREGISTER_CLASS(C_Node);
    GDREGISTER_CLASS(SimpleNode);
    GDREGISTER_CLASS(CPPScript);
    GDREGISTER_CLASS(PlayerController);
    GDREGISTER_CLASS(PhysicsBody3DSetup);
    GDREGISTER_CLASS(CustomMesh3D);

    // Clay UI classes
    GDREGISTER_CLASS(Clay2DCanvas);
    GDREGISTER_CLASS(Clay3DCanvas);
    GDREGISTER_CLASS(ClayWidget);
    GDREGISTER_CLASS(ClayWidgetScript);
    GDREGISTER_CLASS(ClayScrollView);
    GDREGISTER_CLASS(ClayDebugElement);
    GDREGISTER_CLASS(ClayPanel);
    GDREGISTER_CLASS(ClayDemoWidget);
    GDREGISTER_CLASS(ClayMainDemo);

    // Flecs ECS system (singleton)
    GDREGISTER_CLASS(FlecsWorld);

    // CNode context system - composable node + swappable context
    GDREGISTER_CLASS(NodeContext);       // Base context class
    GDREGISTER_CLASS(CNode);             // Base node with context

    // Clay button with context system
    GDREGISTER_CLASS(ClayButtonContext); // Base button context
    GDREGISTER_CLASS(ClayButtonNode);    // The button widget
    GDREGISTER_CLASS(StartGameContext);   // Context: prints "Starting Game!"
    GDREGISTER_CLASS(QuitGameContext);    // Context: prints "Quitting Game!"
    GDREGISTER_CLASS(OptionMenuContext);  // Context: full options menu logic (.cpp)

    // Create and register the FlecsWorld singleton
    flecs_world_singleton = memnew(FlecsWorld);
    Engine::get_singleton()->register_singleton("FlecsWorld", flecs_world_singleton);

    // Initialize Flecs world
    flecs_world_singleton->initialize();
}

void uninitialize_cortex_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Cleanup FlecsWorld singleton
    if (flecs_world_singleton) {
        Engine::get_singleton()->unregister_singleton("FlecsWorld");
        memdelete(flecs_world_singleton);
        flecs_world_singleton = nullptr;
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT cortex_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {

        godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_cortex_module);
        init_obj.register_terminator(uninitialize_cortex_module);
        init_obj.set_minimum_library_initialization_level(
            MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
