#ifndef POLARIS_UTIL_TWEEN_H
#define POLARIS_UTIL_TWEEN_H

#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

namespace Polaris {

using namespace godot;

// =============================================================================
// EasingCurve - A single easing curve (transition type + ease direction)
// =============================================================================

struct EasingCurve {
    Tween::TransitionType trans = Tween::TRANS_QUAD;
    Tween::EaseType ease = Tween::EASE_OUT;

    EasingCurve() = default;
    EasingCurve(Tween::TransitionType t, Tween::EaseType e) : trans(t), ease(e) {}
};

// =============================================================================
// TweenAnimation - Two-phase animation with separate in/out curves
// =============================================================================
//
// Splits animation into two halves:
//   - First half (0 → 0.5):  Uses curve_in
//   - Second half (0.5 → 1): Uses curve_out
//
// This allows combinations like:
//   - ELASTIC/IN start → QUAD/OUT settle
//   - BOUNCE/IN start → LINEAR/OUT end
//
// =============================================================================

struct TweenAnimation {
    EasingCurve curve_in;   // How animation starts (first half)
    EasingCurve curve_out;  // How animation ends (second half)
    float duration = 1.0f;  // Total duration in seconds

    TweenAnimation() = default;
    TweenAnimation(EasingCurve in, EasingCurve out, float dur = 1.0f)
        : curve_in(in), curve_out(out), duration(dur) {}

    // Convenience: same curve for both halves
    TweenAnimation(Tween::TransitionType trans, Tween::EaseType ease, float dur = 1.0f)
        : curve_in(trans, ease), curve_out(trans, ease), duration(dur) {}
};

// =============================================================================
// Interpolation Functions
// =============================================================================

/// Interpolate using a single EasingCurve
/// @param curve The easing curve to apply
/// @param progress Progress through animation (0-1)
/// @return Eased value (0-1)
inline float interpolate_curve(const EasingCurve& curve, float progress) {
    Variant eased = Tween::interpolate_value(
        0.0, 1.0, progress, 1.0,
        curve.trans, curve.ease
    );
    return static_cast<float>(eased);
}

/// Interpolate using TweenAnimation (two-phase: in then out)
/// @param anim The animation with in/out curves
/// @param progress Progress through animation (0-1)
/// @return Eased value (0-1)
inline float interpolate_tween(const TweenAnimation& anim, float progress) {
    // Clamp progress to 0-1
    progress = std::fmax(0.0f, std::fmin(1.0f, progress));

    if (progress <= 0.5f) {
        // First half: use curve_in, map 0-0.5 → 0-1
        float local_progress = progress * 2.0f;
        float eased = interpolate_curve(anim.curve_in, local_progress);
        // Scale to 0-0.5 range
        return eased * 0.5f;
    } else {
        // Second half: use curve_out, map 0.5-1 → 0-1
        float local_progress = (progress - 0.5f) * 2.0f;
        float eased = interpolate_curve(anim.curve_out, local_progress);
        // Scale to 0.5-1 range
        return 0.5f + eased * 0.5f;
    }
}

/// Interpolate a value range using TweenAnimation
/// @param anim The animation with in/out curves
/// @param start Starting value
/// @param end Ending value
/// @param progress Progress through animation (0-1)
/// @return Interpolated value between start and end
inline float interpolate_tween_value(const TweenAnimation& anim,
                                      float start, float end, float progress) {
    float t = interpolate_tween(anim, progress);
    return start + (end - start) * t;
}

// =============================================================================
// Tick Functions - Update time and return eased value
// =============================================================================

/// Tick a looping eased animation
/// @param curve The easing curve to apply
/// @param time Current time (updated in place)
/// @param dt Delta time to advance
/// @param duration Animation duration in seconds
/// @param pause Pause time at end before looping (default 0)
/// @return Eased value (0-1)
inline float tick_easing(const EasingCurve& curve, float& time,
                         float dt, float duration, float pause = 0.0f) {
    time += dt;

    float total_cycle = duration + pause;
    if (time >= total_cycle) {
        time = 0.0f;
    }

    float progress = std::fmin(time / duration, 1.0f);
    return interpolate_curve(curve, progress);
}

/// Result from one-shot easing tick
struct EasingResult {
    float value;      // Eased value (0-1)
    bool completed;   // True when animation finished
};

/// Tick a one-shot eased animation (plays once, doesn't loop)
/// @param curve The easing curve to apply
/// @param time Current time (updated in place)
/// @param dt Delta time to advance
/// @param duration Animation duration in seconds
/// @return Result with eased value and completion flag
inline EasingResult tick_easing_oneshot(const EasingCurve& curve, float& time,
                                        float dt, float duration) {
    bool was_complete = time >= duration;
    time += dt;

    float progress = std::fmin(time / duration, 1.0f);
    float value = interpolate_curve(curve, progress);

    return { value, !was_complete && time >= duration };
}

/// Tick a looping two-phase eased animation
/// @param anim The animation with in/out curves
/// @param time Current time (updated in place)
/// @param dt Delta time to advance
/// @param pause Pause time at end before looping (default 0)
/// @return Eased value (0-1)
inline float tick_tween(const TweenAnimation& anim, float& time,
                        float dt, float pause = 0.0f) {
    time += dt;

    float total_cycle = anim.duration + pause;
    if (time >= total_cycle) {
        time = 0.0f;
    }

    float progress = std::fmin(time / anim.duration, 1.0f);
    return interpolate_tween(anim, progress);
}

} // namespace Polaris

#endif // POLARIS_UTIL_TWEEN_H
