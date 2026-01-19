#ifndef POLARIS_UNITS_H
#define POLARIS_UNITS_H

namespace Polaris {

// =============================================================================
// Units - Domain-based conversion utilities
// =============================================================================
//
// PURPOSE:
//   Convert between "designer units" (inspector-friendly) and "Godot units"
//   (what the engine/math needs). Conversions happen in compose() and setters.
//
// DESIGN PRINCIPLE:
//   Each domain (Speed, Distance, Rotation, etc.) has a rule:
//   - What unit designers see in the inspector
//   - What unit systems use internally
//   - Why that choice makes sense
//
// NAMING CONVENTION:
//   to_godot()  - Designer unit → Engine unit (use in compose/setters)
//   to_source() - Engine unit → Designer unit (use for debugging/display)
//
// =============================================================================

namespace Units {

// =============================================================================
// Speed Domain
// =============================================================================
//
// RULE: Designers work in HU/s (Hammer Units per second)
//
// WHY:
//   - All Source Engine documentation uses HU/s
//   - Community knowledge: "320 = walk speed" is universal
//   - Easy to look up reference values from Half-Life, CS, TF2
//
// EXAMPLES:
//   Walk speed:    320 HU/s  →  8.128 m/s
//   Run speed:     400 HU/s  →  10.16 m/s
//   Air speed cap:  30 HU/s  →  0.762 m/s (enables strafe jumping)
//
// INSPECTOR PROPERTY:
//   float m_max_speed_hu = 320.0f;
//   ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_speed_hu", PROPERTY_HINT_RANGE, "0,500,1"), ...);
//
namespace Speed {
    constexpr float kHuToMeters = 0.0254f;    // 1 HU = 1 inch = 0.0254m
    constexpr float kMetersToHu = 39.3701f;   // 1m = 39.37 HU

    /// Use when: Setting velocity/speed components from inspector values
    /// Example: move.max_speed = Speed::to_godot(m_max_speed_hu);
    inline float to_godot(float hu_per_sec) {
        return hu_per_sec * kHuToMeters;
    }

    /// Use when: Displaying current speed to designers, debug output
    /// Example: print("Speed: ", Speed::to_source(velocity.length()), " HU/s");
    inline float to_source(float m_per_sec) {
        return m_per_sec * kMetersToHu;
    }
}

// =============================================================================
// Distance Domain (Large)
// =============================================================================
//
// RULE: Large distances in HU (player height, jump height, room dimensions)
//
// WHY:
//   - Consistent with speed units (both HU-based)
//   - Source Engine level design uses HU
//   - "72 HU player height" matches real Source games
//
// EXAMPLES:
//   Player height (stand): 72 HU  →  1.8288 m
//   Player height (crouch): 36 HU →  0.9144 m
//   Eye height (stand):     64 HU →  1.6256 m
//   Door height:           108 HU →  2.7432 m
//
// INSPECTOR PROPERTY:
//   float m_stand_height_hu = 72.0f;
//   ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stand_height_hu", PROPERTY_HINT_RANGE, "36,96,1"), ...);
//
namespace Distance {
    constexpr float kHuToMeters = 0.0254f;
    constexpr float kMetersToHu = 39.3701f;
    constexpr float kCmToMeters = 0.01f;
    constexpr float kMetersToCm = 100.0f;

    /// Use when: Setting large distances (player height, collision, level geometry)
    /// Example: crouch.stand_height = Distance::hu_to_godot(m_stand_height_hu);
    inline float hu_to_godot(float hu) {
        return hu * kHuToMeters;
    }

    /// Use when: Converting godot distance back to HU for display/debugging
    inline float godot_to_hu(float meters) {
        return meters * kMetersToHu;
    }

    /// Use when: Setting small distances (bob amplitude, camera offsets, sway)
    /// These are too small for HU to be intuitive - use centimeters instead
    /// Example: bob.vertical_amp = Distance::cm_to_godot(m_bob_vertical_cm);
    inline float cm_to_godot(float cm) {
        return cm * kCmToMeters;
    }

    /// Use when: Converting small distances back to cm for display
    inline float godot_to_cm(float meters) {
        return meters * kMetersToCm;
    }
}

// =============================================================================
// Rotation Domain
// =============================================================================
//
// RULE: Designers work in degrees, systems use radians
//
// WHY:
//   - Everyone understands degrees (90° = right angle)
//   - Radians are for internal math only
//   - No mental conversion needed in inspector
//
// EXAMPLES:
//   Pitch limit: 89°    →  1.553 rad
//   Roll angle:  0.65°  →  0.0113 rad
//   FOV:         90°    →  1.571 rad
//
// INSPECTOR PROPERTY:
//   float m_roll_angle_deg = 0.65f;
//   ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "roll_angle_deg", PROPERTY_HINT_RANGE, "0,5,0.05"), ...);
//
namespace Rotation {
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kDegToRad = kPi / 180.0f;
    constexpr float kRadToDeg = 180.0f / kPi;

    /// Use when: Setting any angle component from inspector degrees
    /// Example: roll.max_angle = Rotation::to_godot(m_roll_angle_deg);
    inline float to_godot(float degrees) {
        return degrees * kDegToRad;
    }

    /// Use when: Displaying angles to designers, debug output
    /// Example: print("Pitch: ", Rotation::to_source(look.pitch), " degrees");
    inline float to_source(float radians) {
        return radians * kRadToDeg;
    }
}

// =============================================================================
// Sensitivity Domain
// =============================================================================
//
// RULE: 0-100 percentage scale for mouse/stick sensitivity
//
// WHY:
//   - Raw values (0.001-0.01) are meaningless to designers
//   - "20% sensitivity" is intuitive and adjustable
//   - Easy to communicate: "try 15% for precision, 40% for fast play"
//
// CALIBRATION:
//   1%   = 0.0001 raw (extremely slow)
//   20%  = 0.002 raw  (typical default)
//   50%  = 0.005 raw  (fast)
//   100% = 0.01 raw   (maximum reasonable)
//
// INSPECTOR PROPERTY:
//   float m_sensitivity_pct = 20.0f;
//   ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sensitivity_pct", PROPERTY_HINT_RANGE, "1,100,1"), ...);
//
namespace Sensitivity {
    constexpr float kPctToRaw = 0.0001f;    // 1% = 0.0001 raw
    constexpr float kRawToPct = 10000.0f;   // 0.0001 raw = 1%

    /// Use when: Setting look sensitivity from inspector percentage
    /// Example: look.sensitivity = Sensitivity::to_godot(m_sensitivity_pct);
    inline float to_godot(float percent) {
        return percent * kPctToRaw;
    }

    /// Use when: Displaying sensitivity as percentage
    inline float to_source(float raw) {
        return raw * kRawToPct;
    }
}

// =============================================================================
// Multiplier Domain
// =============================================================================
//
// RULE: 0-100 percentage for any scaling factor
//
// WHY:
//   - "50% speed" is clearer than "0.5 multiplier"
//   - Consistent with sensitivity (both use percentage)
//   - Natural for designers: "crouch at 40% speed"
//
// EXAMPLES:
//   Crouch speed mult: 40%  →  0.4
//   Damage mult:       150% →  1.5
//   Volume:            80%  →  0.8
//
// INSPECTOR PROPERTY:
//   float m_crouch_speed_pct = 40.0f;
//   ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crouch_speed_pct", PROPERTY_HINT_RANGE, "0,100,1"), ...);
//
namespace Multiplier {
    constexpr float kPctToMult = 0.01f;     // 100% = 1.0
    constexpr float kMultToPct = 100.0f;    // 1.0 = 100%

    /// Use when: Setting any multiplier/scale factor from percentage
    /// Example: crouch.speed_mult = Multiplier::to_godot(m_crouch_speed_pct);
    inline float to_godot(float percent) {
        return percent * kPctToMult;
    }

    /// Use when: Displaying multiplier as percentage
    inline float to_source(float mult) {
        return mult * kMultToPct;
    }
}

// =============================================================================
// Acceleration Domain
// =============================================================================
//
// RULE: Designers work in HU/s² (consistent with speed)
//
// WHY:
//   - "800 gravity" is iconic from Source Engine
//   - Matches speed units (both HU-based)
//   - Easy to reference from game documentation
//
// EXAMPLES:
//   Gravity:      800 HU/s²  →  20.32 m/s²
//   Ground accel: 10 (multiplier, not converted)
//   Air accel:    10 (multiplier, not converted)
//
// NOTE: acceleration and friction values in Source are multipliers,
//       not direct HU/s² values. Only gravity uses this conversion.
//
// INSPECTOR PROPERTY:
//   float m_gravity_hu = 800.0f;
//   ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity_hu", PROPERTY_HINT_RANGE, "0,1200,10"), ...);
//
namespace Accel {
    constexpr float kHuToMeters = 0.0254f;
    constexpr float kMetersToHu = 39.3701f;

    /// Use when: Setting gravity or direct acceleration values
    /// Example: move.gravity = Accel::to_godot(m_gravity_hu);
    inline float to_godot(float hu_per_sec2) {
        return hu_per_sec2 * kHuToMeters;
    }

    /// Use when: Displaying acceleration as HU/s²
    inline float to_source(float m_per_sec2) {
        return m_per_sec2 * kMetersToHu;
    }
}

// =============================================================================
// Quick Reference - GoldSrc Standard Values
// =============================================================================
//
// | Property           | Source (HU) | Godot (m)  | Notes                    |
// |--------------------|-------------|------------|--------------------------|
// | Walk speed         | 320 HU/s    | 8.128 m/s  | cl_forwardspeed default  |
// | Run speed          | 400 HU/s    | 10.16 m/s  | +speed modifier          |
// | Air speed cap      | 30 HU/s     | 0.762 m/s  | Enables strafe jumping!  |
// | Stop speed         | 100 HU/s    | 2.54 m/s   | Friction threshold       |
// | Gravity            | 800 HU/s²   | 20.32 m/s² | sv_gravity default       |
// | Jump velocity      | ~270 HU/s   | 6.858 m/s  | Varies by game           |
// | Player height      | 72 HU       | 1.8288 m   | Standing                 |
// | Crouch height      | 36 HU       | 0.9144 m   | Crouched                 |
// | Eye height (stand) | 64 HU       | 1.6256 m   | Camera position          |
// | Eye height (crouch)| 28 HU       | 0.7112 m   | Camera when crouched     |
// | Player width       | 32 HU       | 0.8128 m   | Collision hull           |
//
// =============================================================================

} // namespace Units
} // namespace Polaris

#endif // POLARIS_UNITS_H
