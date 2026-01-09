// examples/domains/game_init.cpp
// Main game initialization - wires all domain modules together
//
// This is the ONLY file that knows about all domains.
// Each domain is self-contained and isolated.
//
// NAMESPACE CONVENTION:
//   Polaris::          - Engine code (Runtime, Gd::, Tag::)
//   Game::             - Your game code
//   Game::Movement::   - Movement domain
//   Game::Combat::     - Combat domain
//   etc.
//
// For brevity, these examples use flat namespaces (Movement:: instead of Game::Movement::)

#include "polaris/core/Runtime.h"

// Domain module headers (in production, these would be .h files)
#include "input.cpp"
#include "movement.cpp"
#include "combat.cpp"
#include "ai.cpp"
#include "animation.cpp"

using namespace Polaris;  // Access Runtime, Gd::, Tag::, etc.

// ============================================================================
// Game Initialization
// ============================================================================
//
// Domain modules register themselves.
// Order here determines sub-phase dependencies between domains.
//
// Execution order within Phase_Physics:
//
//   Phase_Physics
//       |
//       +-- Movement.Input     (player calculates direction)
//       +-- Movement.Apply     (gravity, move_and_slide)
//       +-- Movement.Resolve   (check grounded)
//       |
//       +-- AI.Sense           (detect targets)
//       +-- AI.Decide          (state machine)
//       +-- AI.Act             (apply AI velocity)
//       |
//       +-- Combat systems     (damage, death)
//

void register_game_systems(Runtime* rt) {
    // Each domain initializes itself
    Input::init(rt);        // Phase_Input
    Movement::init(rt);     // Phase_Physics.Movement.*
    AI::init(rt);           // Phase_Physics.AI.*
    Combat::init(rt);       // Phase_Physics (simple, no sub-phases)
    Animation::init(rt);    // Phase_Process
}

// ============================================================================
// Alternative: Domain Dependencies
// ============================================================================
//
// If AI needs to run AFTER Movement resolves (to see updated positions),
// add an explicit dependency:
//
//   AI::Phase::Sense.depends_on(Movement::Phase::Resolve);
//
// This makes the data flow explicit:
//
//   Movement.Input -> Movement.Apply -> Movement.Resolve
//                                              |
//                                              v
//                                       AI.Sense -> AI.Decide -> AI.Act
//

void register_game_systems_with_dependencies(Runtime* rt) {
    // Initialize domains
    Input::init(rt);
    Movement::init(rt);
    AI::init(rt);
    Combat::init(rt);
    Animation::init(rt);

    // AI sees Movement results before sensing
    AI::Phase::Sense.add(flecs::DependsOn, Movement::Phase::Resolve);
}
