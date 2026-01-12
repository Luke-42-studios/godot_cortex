#ifndef POLARIS_DEBUG_REGISTRY_H
#define POLARIS_DEBUG_REGISTRY_H

#include <typeindex>
#include <unordered_map>

namespace Polaris
{

    // =============================================================================
    // DebugRegistry - Static registry mapping state types to debug flags
    // =============================================================================
    //
    // PURPOSE: Allow per-state-type debug toggles without adding debug fields to
    // every state struct. Debug flags are set via Godot inspector (debug builds)
    // and read by systems via Debuggable<T>::debug().
    //
    // USAGE:
    //   // Set (from Godot accessor):
    //   DebugRegistry::set<MyState_Base>(true);
    //
    //   // Get (from system via Debuggable<T>):
    //   if (state.debug()) { ... }
    //
    // =============================================================================

    class DebugRegistry
    {
        inline static std::unordered_map<std::type_index, bool> s_flags;

    public:
        template <typename T>
        static bool get()
        {
            auto it = s_flags.find(typeid(T));
            return it != s_flags.end() ? it->second : false;
        }

        template <typename T>
        static void set(bool v)
        {
            s_flags[typeid(T)] = v;
        }

        /// Clear all debug flags (useful for cleanup/testing)
        static void clear() { s_flags.clear(); }
    };

    // =============================================================================
    // Debuggable<T> - Template wrapper that adds debug() to any state
    // =============================================================================
    //
    // PURPOSE: Wrap state structs to add .debug() method without modifying the
    // original struct definition. In release builds, debug() always returns false
    // and the compiler optimizes away debug-only code paths.
    //
    // USAGE:
    //   STATE(MyState) {       // Uses Debuggable<MyState_Base> via alias
    //       float value{};
    //   };
    //
    //   // In system:
    //   if (state.debug()) {   // Reads from DebugRegistry
    //       print("value=", state.value);
    //   }
    //
    // =============================================================================

    template <typename T>
    struct Debuggable : T
    {
#ifdef DEBUG_ENABLED
        bool debug() const { return DebugRegistry::get<T>(); }
#else
        bool debug() const { return false; }
#endif
    };

} // namespace Polaris

#endif // POLARIS_DEBUG_REGISTRY_H
