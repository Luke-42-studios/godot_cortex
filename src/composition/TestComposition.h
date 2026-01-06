#ifndef POLARIS_TEST_COMPOSITION_H
#define POLARIS_TEST_COMPOSITION_H

#include "Composition.h"

namespace Polaris {

// =============================================================================
// Test Components
// =============================================================================

struct TestTag {};

struct TestData {
    float value = 0.0f;
    int counter = 0;
};

// =============================================================================
// TestComposition - Simple composition for testing the pipeline
// =============================================================================
//
// Adds TestTag and TestData components to demonstrate the composition system.
// Use this to verify that nodes are being detected and entities created.
//
// USAGE:
//   1. In GDScript, create and attach:
//      var comp = TestComposition.new()
//      comp.initial_value = 42.0
//      node.set_meta("composition", comp)
//
//   2. Or create a .tres resource in the editor
//
// =============================================================================

class TestComposition : public Composition {
    GDCLASS(TestComposition, Composition)

private:
    float m_initial_value = 1.0f;
    String m_label = "TestEntity";

protected:
    static void _bind_methods();

public:
    TestComposition() = default;
    virtual ~TestComposition() = default;

    // =========================================================================
    // Entity Lifecycle
    // =========================================================================

    void compose(flecs::entity entity, Node* root) override;
    void decompose(flecs::entity entity) override;

    // =========================================================================
    // Properties
    // =========================================================================

    void set_initial_value(float value) { m_initial_value = value; }
    float get_initial_value() const { return m_initial_value; }

    void set_label(const String& label) { m_label = label; }
    String get_label() const { return m_label; }
};

} // namespace Polaris

#endif // POLARIS_TEST_COMPOSITION_H
