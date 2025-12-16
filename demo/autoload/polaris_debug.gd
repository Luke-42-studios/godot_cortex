extends Node

## Polaris Debug Controller
## Attach to a node or use as autoload to control debug logging.

@export var polaris_debug: bool = false:
	set(value):
		polaris_debug = value
		if Engine.has_singleton("Polaris"):
			Engine.get_singleton("Polaris").call("set_debug_enabled", value)

@export var ecs_world_debug: bool = false:
	set(value):
		ecs_world_debug = value
		if Engine.has_singleton("ECSWorld"):
			Engine.get_singleton("ECSWorld").call("set_debug_enabled", value)

@export var node_watcher_debug: bool = false:
	set(value):
		node_watcher_debug = value
		if Engine.has_singleton("NodeWatcher"):
			Engine.get_singleton("NodeWatcher").call("set_debug_enabled", value)

@export var all_debug: bool = false:
	set(value):
		all_debug = value
		polaris_debug = value
		ecs_world_debug = value
		node_watcher_debug = value


func _ready() -> void:
	# Apply initial values
	if polaris_debug:
		self.polaris_debug = polaris_debug
	if ecs_world_debug:
		self.ecs_world_debug = ecs_world_debug
	if node_watcher_debug:
		self.node_watcher_debug = node_watcher_debug


func _input(event: InputEvent) -> void:
	# Toggle all debug with F3
	if event.is_action_pressed("toggle_polaris_debug"):
		all_debug = not all_debug
		print("[PolarisDebug] All debug: ", all_debug)


## Print current ECS world state
func print_ecs_state() -> void:
	if Engine.has_singleton("ECSWorld"):
		Engine.get_singleton("ECSWorld").call("print_state")


## Get entity count
func get_entity_count() -> int:
	if Engine.has_singleton("Polaris"):
		return Engine.get_singleton("Polaris").call("get_entity_count")
	return 0
