#ifndef CNODE_H
#define CNODE_H

#include "gd_macros.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {

// Forward declaration
class CNode;

// ============================================================================
// NodeContext - Base class for swappable behavior/logic
// ============================================================================
// This is a Resource so it can be assigned in the editor!
// It receives lifecycle events just like a Node would.
class NodeContext : public Resource {
    GDCLASS(NodeContext, Resource);

protected:
    Node* owner = nullptr;

    static void _bind_methods() {
        // Lifecycle methods - subclasses override these
        ClassDB::bind_method(D_METHOD("ctx_ready"), &NodeContext::ctx_ready);
        ClassDB::bind_method(D_METHOD("ctx_process", "delta"), &NodeContext::ctx_process);
        ClassDB::bind_method(D_METHOD("ctx_physics_process", "delta"), &NodeContext::ctx_physics_process);
        ClassDB::bind_method(D_METHOD("ctx_input", "event"), &NodeContext::ctx_input);
        ClassDB::bind_method(D_METHOD("ctx_unhandled_input", "event"), &NodeContext::ctx_unhandled_input);

        // Utility
        ClassDB::bind_method(D_METHOD("get_owner_node"), &NodeContext::get_owner_node_variant);
    }

public:
    NodeContext() {}
    virtual ~NodeContext() {}

    // Called by owner node to set the reference
    void _set_owner(Node* node) { owner = node; }

    // Generic typed access to owner node
    // Usage: get_node<ClayButtonNode>() returns ClayButtonNode*
    template<typename T>
    T* get_node() const {
        return Object::cast_to<T>(owner);
    }

    // Non-templated version for GDScript
    Node* get_owner_node() const { return owner; }
    Variant get_owner_node_variant() const { return Variant(owner); }

    // Virtual lifecycle methods - override these in subclasses
    // Named with ctx_ prefix to avoid confusion with Node methods
    virtual void ctx_ready() {}
    virtual void ctx_process(double delta) {}
    virtual void ctx_physics_process(double delta) {}
    virtual void ctx_input(const Ref<InputEvent>& event) {}
    virtual void ctx_unhandled_input(const Ref<InputEvent>& event) {}

    // Called when context is attached/detached
    virtual void ctx_enter() {}
    virtual void ctx_exit() {}
};

// ============================================================================
// CNode - Base node class with swappable context
// ============================================================================
class CNode : public Node {
    GDCLASS(CNode, Node);

private:
    Ref<NodeContext> context;

protected:
    static void _bind_methods() {
        // Context property - assignable in editor
        ClassDB::bind_method(D_METHOD("set_context", "context"), &CNode::set_context);
        ClassDB::bind_method(D_METHOD("get_context"), &CNode::get_context);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "context", PROPERTY_HINT_RESOURCE_TYPE, "NodeContext"),
                     "set_context", "get_context");
    }

public:
    CNode() {}
    virtual ~CNode() {}

    // Context getter/setter
    void set_context(const Ref<NodeContext>& p_context) {
        // Detach old context
        if (context.is_valid()) {
            context->ctx_exit();
            context->_set_owner(nullptr);
        }

        context = p_context;

        // Attach new context
        if (context.is_valid()) {
            context->_set_owner(this);
            context->ctx_enter();

            // If we're already in the tree, call ready
            if (is_inside_tree()) {
                context->ctx_ready();
            }
        }
    }

    Ref<NodeContext> get_context() const { return context; }

    void _ready() override {
        if (context.is_valid()) {
            context->_set_owner(this);
            context->ctx_ready();
        }
    }

    void _process(double delta) override {
        if (context.is_valid()) {
            context->ctx_process(delta);
        }
    }

    void _physics_process(double delta) override {
        if (context.is_valid()) {
            context->ctx_physics_process(delta);
        }
    }

    void _input(const Ref<InputEvent>& event) override {
        if (context.is_valid()) {
            context->ctx_input(event);
        }
    }

    void _unhandled_input(const Ref<InputEvent>& event) override {
        if (context.is_valid()) {
            context->ctx_unhandled_input(event);
        }
    }
};

} // namespace godot

// ============================================================================
// Macro to easily bind a context property with type restriction
// ============================================================================
// Usage in _bind_methods():
//   GD_BIND_CONTEXT(MyNodeClass, MyContextClass, context);
//
// This will:
// - Bind set_context/get_context methods
// - Add property with dropdown filtered to MyContextClass and subclasses
#define GD_BIND_CONTEXT(node_class, context_class, prop_name) \
    ClassDB::bind_method(D_METHOD("set_" #prop_name, #prop_name), &node_class::set_##prop_name); \
    ClassDB::bind_method(D_METHOD("get_" #prop_name), &node_class::get_##prop_name); \
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, #prop_name, PROPERTY_HINT_RESOURCE_TYPE, #context_class), \
                 "set_" #prop_name, "get_" #prop_name);

#endif // CNODE_H
