#ifndef POLARIS_ENGINE_H
#define POLARIS_ENGINE_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <flecs.h>
#include <unordered_map>
#include <mutex>

#include "Log.h"


namespace Polaris {

using namespace godot;

/// PolarisEngine - Main Polaris Coordinator

class PolarisEngine : public Object {
    GDCLASS(PolarisEngine, Object)

private:
    static inline PolarisEngine* singleton_instance = nullptr;

    PolarisEngine(const PolarisEngine&) = delete;

protected:
    static void _bind_methods();

public:
    PolarisEngine();
    ~PolarisEngine();

    // =========================================================================
    // Lifecycle
    // =========================================================================

    void initialize();
    void shutdown();


    // =========================================================================
    // Singleton
    // =========================================================================

    static PolarisEngine* get_singleton() noexcept { return singleton_instance; }
    static PolarisEngine* create_global_instance();
    static void destroy_global_instance();

};

} // namespace Polaris

#endif // POLARIS_ENGINE_H
