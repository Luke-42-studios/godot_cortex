extends SceneTracker
## SceneTracker Autoload
##
## This autoload extends the C++ SceneTracker class to provide additional
## functionality specific to this project.
##
## The SceneTracker automatically monitors all nodes entering and exiting
## the scene tree, providing efficient querying capabilities.

# EntityRegistry for game entity management
var entity_registry: EntityRegistry

# ComponentSystem for data-oriented component management  
var component_system: ComponentSystem


func _ready() -> void:
	super._ready()
	
	# Initialize subsystems
	entity_registry = EntityRegistry.new()
	component_system = ComponentSystem.new()
	
	# Connect signals for logging/debugging
	node_tracked.connect(_on_node_tracked)
	node_untracked.connect(_on_node_untracked)
	
	print("[SceneTrackerAutoload] Initialized with EntityRegistry and ComponentSystem")


func _on_node_tracked(node: Node) -> void:
	# Optional: Auto-register certain node types as entities
	if node.is_in_group("entity"):
		var entity_type = node.get("entity_type") if "entity_type" in node else "generic"
		entity_registry.register_entity(node, entity_type)


func _on_node_untracked(node: Node) -> void:
	# Clean up entity registration when nodes are removed
	if entity_registry.is_node_registered(node):
		entity_registry.unregister_node(node)
	
	# Clean up component data
	component_system.remove_all_components(node)


## Quick access to entity registry
func get_entity_registry() -> EntityRegistry:
	return entity_registry


## Quick access to component system
func get_component_system() -> ComponentSystem:
	return component_system


## Convenience method: Register a game entity
func register_game_entity(node: Node, entity_type: StringName, tags: Array[StringName] = []) -> int:
	return entity_registry.register_entity_with_tags(node, entity_type, tags)


## Convenience method: Add component to a node
func add_component(node: Node, component_type: StringName, data: Dictionary) -> void:
	component_system.add_component(node, component_type, data)


## Convenience method: Get component from a node
func get_component(node: Node, component_type: StringName) -> Dictionary:
	return component_system.get_component(node, component_type)


## Print debug information
func print_debug_info() -> void:
	var tracker_stats = get_tracking_stats()
	var entity_stats = entity_registry.get_stats()
	var component_stats = component_system.get_stats()
	
	print("=== GDFramework Debug Info ===")
	print("SceneTracker: ", tracker_stats)
	print("EntityRegistry: ", entity_stats)
	print("ComponentSystem: ", component_stats)
	print("==============================")
