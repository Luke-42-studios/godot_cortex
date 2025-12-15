#include "CNode.h"
#include <godot_cpp/core/memory.hpp>

// =============================================================================
// Context Implementation
// =============================================================================

void Context::_bind_methods() {
    // Base context has no properties, but subclasses can add their own
    // Virtual methods are not exposed to GDScript by default
}

Context::Context() {
    // Base initialization
}

Context::~Context() {
    // Base cleanup
}

// =============================================================================
// CNode Implementation
// =============================================================================

void CNode::_bind_methods() {
    // Bind the context property
    ClassDB::bind_method(D_METHOD("set_context", "context"), &CNode::set_context);
    ClassDB::bind_method(D_METHOD("get_context"), &CNode::get_context);
    ClassDB::bind_method(D_METHOD("has_context"), &CNode::has_context);

    // Register the property - this makes it visible in the editor
    // PROPERTY_HINT_RESOURCE_TYPE allows selecting Context or any subclass
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "context", PROPERTY_HINT_RESOURCE_TYPE, "Context"),
        "set_context", "get_context"
    );
}

CNode::CNode() {
    // Processing will be enabled in NOTIFICATION_READY if context needs it
}

CNode::~CNode() {
    // Context cleanup happens in _notification(NOTIFICATION_EXIT_TREE)
}

void CNode::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY:
            // Only enable processing if we have a context
            if (m_context.is_valid()) {
                set_process(true);
                m_context->on_context_ready(this);
            }
            break;

        case NOTIFICATION_PROCESS:
            if (m_context.is_valid()) {
                m_context->on_context_process(this, get_process_delta_time());
            }
            break;

        case NOTIFICATION_EXIT_TREE:
            if (m_context.is_valid()) {
                m_context->on_context_exit(this);
            }
            break;
    }
}

void CNode::set_context(const Ref<Context>& context) {
    // If we're in the tree and had a previous context, call exit on it
    if (is_inside_tree() && m_context.is_valid() && m_context != context) {
        m_context->on_context_exit(this);
    }

    m_context = context;

    // If we're already in the tree and have a new context, initialize it
    if (is_inside_tree() && m_context.is_valid()) {
        m_context->on_context_ready(this);
    }
}

Ref<Context> CNode::get_context() const {
    return m_context;
}
