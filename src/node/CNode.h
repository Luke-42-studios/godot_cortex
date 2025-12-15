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
// =============================================================================
class CNode : public Node {
    GDCLASS(CNode, Node)

private:
    Ref<Context> m_context;

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
};

#endif // CNODE_H
