#ifndef NODE_WATCHER_SYSTEM_H
#define NODE_WATCHER_SYSTEM_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/// --------------------------------------------------------------------
/// NodeWatcherSystem – watches the whole scene‑tree, keeps a flat list of
/// live nodes and can **optionally** dump that list to the Godot debugger each frame.
/// --------------------------------------------------------------------
class NodeWatcherSystem : public Object {
    GDCLASS(NodeWatcherSystem, Object)

private:
    static inline NodeWatcherSystem *singleton_instance = nullptr;

    static void _bind_methods();

    // Prevent copying
    NodeWatcherSystem(const NodeWatcherSystem&) = delete;
    // NodeWatcherSystem& operator=(const NodeWatcherSystem&) = delete;

public:
    // Fast C++ access (avoids Engine string lookup)
    static NodeWatcherSystem* get_singleton() noexcept {
        return singleton_instance;
    }

    // ----------------------------------------------------------------
    // Construction / destruction
    // ----------------------------------------------------------------
    NodeWatcherSystem();          // registers to the current scene‑tree (if any)
    ~NodeWatcherSystem();

    void _process(double delta); 

    // Static methods for module integration
    static NodeWatcherSystem* create_global_instance();
    static void destroy_global_instance();
};

#endif // NODE_WATCHER_SYSTEM_H