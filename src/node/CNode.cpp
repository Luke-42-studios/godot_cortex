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

    // Lifecycle methods (called by Polaris::Engine)
    ClassDB::bind_method(D_METHOD("start_context"), &CNode::start_context);
    ClassDB::bind_method(D_METHOD("stop_context"), &CNode::stop_context);
    ClassDB::bind_method(D_METHOD("is_context_started"), &CNode::is_context_started);

    // Register the property - this makes it visible in the editor
    // PROPERTY_HINT_RESOURCE_TYPE allows selecting Context or any subclass
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "context", PROPERTY_HINT_RESOURCE_TYPE, "Context"),
        "set_context", "get_context"
    );
}

CNode::CNode() {
    // Processing is enabled when context starts
}

CNode::~CNode() {
    // Safety: ensure context is stopped
    stop_context();
}

void CNode::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_PROCESS:
            // Only process if context is started
            if (m_context_started && m_context.is_valid()) {
                m_context->on_context_process(this, get_process_delta_time());
            }
            break;

        case NOTIFICATION_EXIT_TREE:
            // Safety fallback: stop context if still running when exiting tree
            // Normally Polaris::Engine calls stop_context() before this
            stop_context();
            break;
    }
}

void CNode::set_context(const Ref<Context>& context) {
    // If context was started, stop it first
    if (m_context_started && m_context.is_valid() && m_context != context) {
        stop_context();
    }

    m_context = context;

    // Note: We do NOT auto-start here. Polaris::Engine controls lifecycle.
}

Ref<Context> CNode::get_context() const {
    return m_context;
}

// =============================================================================
// Lifecycle Control
// =============================================================================

void CNode::start_context() {
    if (m_context_started) {
        return; // Already started
    }

    if (!m_context.is_valid()) {
        return; // No context to start
    }

    m_context_started = true;
    set_process(true);
    m_context->on_context_ready(this);
}

void CNode::stop_context() {
    if (!m_context_started) {
        return; // Already stopped
    }

    m_context_started = false;
    set_process(false);

    if (m_context.is_valid()) {
        m_context->on_context_exit(this);
    }
}
