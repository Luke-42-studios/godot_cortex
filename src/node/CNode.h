#ifndef CNODE_H
#define CNODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// =============================================================================
// Context - Base class for ECS-compatible node contexts
// Inherit from Resource so it appears in the editor's resource picker
// =============================================================================
class Context : public Resource {
    GDCLASS(Context, Resource)

protected:
    static void _bind_methods();

public:
    Context();
    virtual ~Context();

    // Override in subclasses to provide custom initialization
    virtual void on_context_ready(Node* owner) {}

    // Override in subclasses to provide custom cleanup
    virtual void on_context_exit(Node* owner) {}

    // Override to handle per-frame updates if needed
    virtual void on_context_process(Node* owner, double delta) {}
};

// =============================================================================
// CNode - A Node that can hold an ECS Context
// The context can be set in the editor via the inspector
//
// LIFECYCLE: Context start/stop is controlled by Polaris::Engine, NOT by
// Godot notifications. This ensures the ECS entity exists before context
// methods are called.
// =============================================================================
class CNode : public Node {
    GDCLASS(CNode, Node)

private:
    Ref<Context> m_context;
    bool m_context_started = false;

protected:
    static void _bind_methods();

    void _notification(int p_what);

public:
    CNode();
    virtual ~CNode();

    // Context property accessors
    void set_context(const Ref<Context>& context);
    Ref<Context> get_context() const;

    // Check if a context is assigned
    bool has_context() const { return m_context.is_valid(); }

    // =========================================================================
    // Lifecycle Control (called by Polaris::Engine)
    // =========================================================================

    // Start the context - called by Polaris after entity is created
    void start_context();

    // Stop the context - called by Polaris before entity is destroyed
    void stop_context();

    // Check if context has been started
    bool is_context_started() const { return m_context_started; }
};

#endif // CNODE_H
