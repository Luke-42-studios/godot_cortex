#include "TestComposition.h"
#include "util/Log.h"

namespace Polaris {

// =============================================================================
// Class Registration
// =============================================================================

void TestComposition::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_initial_value", "value"), &TestComposition::set_initial_value);
    ClassDB::bind_method(D_METHOD("get_initial_value"), &TestComposition::get_initial_value);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "initial_value"), "set_initial_value", "get_initial_value");

    ClassDB::bind_method(D_METHOD("set_label", "label"), &TestComposition::set_label);
    ClassDB::bind_method(D_METHOD("get_label"), &TestComposition::get_label);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "label"), "set_label", "get_label");
}

// =============================================================================
// Entity Lifecycle
// =============================================================================

void TestComposition::compose(flecs::entity entity, Node* root) {
    // Call base class to set up Gd::Node component
    Composition::compose(entity, root);

    // Add test components
    entity.add<TestTag>();
    entity.set<TestData>({ m_initial_value, 0 });

    // Set entity name from label
    std::string name = m_label.utf8().get_data();
    entity.set_name(name.c_str());

    Log::info("[TestComposition] Composed entity '", m_label, "' with initial_value=", m_initial_value);
}

void TestComposition::decompose(flecs::entity entity) {
    Log::info("[TestComposition] Decomposing entity '", m_label, "'");

    // Call base class to null out Gd::Node
    Composition::decompose(entity);
}

} // namespace Polaris
