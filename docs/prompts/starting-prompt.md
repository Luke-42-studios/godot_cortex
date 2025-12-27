can you read through this file:///c:/Workspace/Godot/polaris/docs/architecture.md look at
file:///c:/Workspace/Godot/source--control/src/SampleContext.cpp for how we implment a system that runs on the game
tick and the physics tick. I want us to think about how to make this FPS control how a ECS. What should be a
component and what should be a system. We need to design it and then implement it.
C:\Workspace\Godot\project-clay\src\FPSCameraNode.cpp C:\Workspace\Godot\project-clay\src\FPSCrosshairNode.cpp
C:\Workspace\Godot\project-clay\src\FPSWeaponNode.cpp C:\Workspace\Godot\project-clay\src\PlayerController.cpp. Any
variables we need to edit or have to us as a game play program in godot but be exposed as a component in godot. You
does it to design a player.tscn which is a prefab of the player and then implement ECS which allow game developers
to edit certain attributes of the components for game play tweaking. Can we start designing this? Also each system
should be seen at its own function for better readablility.