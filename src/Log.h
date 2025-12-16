#ifndef POLARIS_LOG_H
#define POLARIS_LOG_H

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string.hpp>

namespace Polaris {

// =============================================================================
// Log - Conditional Debug Logging Utility
// =============================================================================
//
// Provides runtime-controlled debug logging per class.
//
// USAGE:
//   // In your class:
//   bool m_debug_enabled = false;
//
//   // In your methods:
//   Polaris::Log::print(m_debug_enabled, "[MyClass] Value: ", value);
//
//   // From GDScript:
//   Engine.debug_enabled = true
//
// =============================================================================

namespace Log {

using namespace godot;

/// Print a debug message if the debug flag is enabled
template<typename... Args>
inline void print(bool debug_enabled, Args&&... args) {
    if (debug_enabled) {
        UtilityFunctions::print(std::forward<Args>(args)...);
    }
}

/// Print a warning message if the debug flag is enabled
template<typename... Args>
inline void warn(bool debug_enabled, Args&&... args) {
    if (debug_enabled) {
        UtilityFunctions::push_warning(std::forward<Args>(args)...);
    }
}

/// Print an error message (ALWAYS prints, ignores debug flag)
template<typename... Args>
inline void error(Args&&... args) {
    UtilityFunctions::push_error(std::forward<Args>(args)...);
}

/// Print unconditionally (for important lifecycle messages)
template<typename... Args>
inline void info(Args&&... args) {
    UtilityFunctions::print(std::forward<Args>(args)...);
}

} // namespace Log
} // namespace Polaris

#endif // POLARIS_LOG_H
