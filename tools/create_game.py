#!/usr/bin/env python3
"""
CortexFramework Game Project Generator

Creates a new game project that links against the Cortex framework.
The generated project includes:
- SConstruct build file that links to Cortex
- register_types.cpp for GDExtension registration
- Example context files to get started
- .gdextension file for Godot

Usage:
    python tools/create_game.py <game_path> <game_name>

Example:
    python tools/create_game.py C:/Games/MyGame mygame

This creates:
    C:/Games/MyGame/
        mygame/                    # C++ extension source
            src/
                register_types.cpp
                register_types.h
                start_button_context.h
            SConstruct
        bin/                       # Built libraries go here
            mygame.gdextension
"""

import os
import sys
import argparse
from pathlib import Path


def get_cortex_path():
    """Get the absolute path to the Cortex framework."""
    return Path(__file__).parent.parent.absolute()


def create_directory_structure(game_path: Path, game_name: str):
    """Create the game project directory structure."""
    dirs = [
        game_path / game_name / 'src',
        game_path / 'bin',
    ]
    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)
        print(f"Created: {d}")


def write_sconstruct(game_path: Path, game_name: str, cortex_path: Path):
    """Generate the SConstruct build file."""
    content = f'''#!/usr/bin/env python
"""
SConstruct - Build script for {game_name}

This game project links against CortexFramework.
"""

import os
import sys
import shutil

# Path to Cortex framework (adjust if needed)
cortex_path = r'{cortex_path}'
godot_cpp_path = os.path.join(cortex_path, 'godot-cpp')

# Add godot-cpp to the build environment
env = SConscript(os.path.join(godot_cpp_path, 'SConstruct'))

# Project configuration
project_name = '{game_name}'
src_dir = 'src'

# Include paths - Cortex headers and godot-cpp
env.Append(CPPPATH=[
    src_dir,
    os.path.join(cortex_path, 'src'),
    os.path.join(cortex_path, 'flecs', 'include'),
])

# Link against Cortex library
env.Append(LIBPATH=[os.path.join(cortex_path, 'lib')])

# C++20 for Clay UI compatibility
if env['platform'] == 'windows':
    env.Append(CXXFLAGS=['/std:c++20'])
else:
    env.Append(CXXFLAGS=['-std=c++20'])

# Collect source files
sources = Glob(os.path.join(src_dir, '*.cpp'))
sources += Glob(os.path.join(src_dir, '*.c'))

# Output configuration
output_dir = os.path.join('..', 'bin')

# Platform-specific settings
if env['platform'] == 'windows':
    lib_suffix = '.dll'
    cortex_lib = 'libcortex.{{}}.{{}}.{{}}'.format(env['platform'], env['target'], env['arch'])
elif env['platform'] == 'macos':
    lib_suffix = '.dylib'
    cortex_lib = 'cortex.{{}}.{{}}.{{}}'.format(env['platform'], env['target'], env['arch'])
else:
    lib_suffix = '.so'
    cortex_lib = 'cortex.{{}}.{{}}.{{}}'.format(env['platform'], env['target'], env['arch'])

# Link against Cortex
env.Append(LIBS=[cortex_lib])

# Library name
library_name = 'lib{{}}.{{}}.{{}}.{{}}{{}}'.format(
    project_name,
    env['platform'],
    env['target'],
    env['arch'],
    lib_suffix
)

output_path = os.path.join(output_dir, library_name)

# Build the shared library
library = env.SharedLibrary(
    target=output_path,
    source=sources
)

Default(library)

# Copy Cortex library to bin/ after build
def copy_cortex_lib(target, source, env):
    cortex_lib_dir = os.path.join(cortex_path, 'lib')
    bin_dir = os.path.abspath(output_dir)
    os.makedirs(bin_dir, exist_ok=True)

    # Find and copy all Cortex libraries for current platform/target
    for f in os.listdir(cortex_lib_dir):
        if f.startswith('libcortex.') and env['platform'] in f and env['target'] in f:
            src = os.path.join(cortex_lib_dir, f)
            dst = os.path.join(bin_dir, f)
            print(f"Copying Cortex: {{src}} -> {{dst}}")
            shutil.copy2(src, dst)
            # Copy PDB for Windows
            if f.endswith('.dll'):
                pdb = src.replace('.dll', '.pdb')
                if os.path.exists(pdb):
                    shutil.copy2(pdb, os.path.join(bin_dir, os.path.basename(pdb)))

copy_cortex = env.Command('copy_cortex', library, copy_cortex_lib)
Default(copy_cortex)

Help("""
{game_name} Build System
========================

Game built with CortexFramework.

Build:
    scons                           # Build debug
    scons target=template_release   # Build release

Output goes to ../bin/ and includes both game and Cortex libraries.

Before building, ensure Cortex is built:
    cd {cortex_path}
    scons
""")
'''

    sconstruct_path = game_path / game_name / 'SConstruct'
    sconstruct_path.write_text(content)
    print(f"Created: {sconstruct_path}")


def write_register_types_h(game_path: Path, game_name: str):
    """Generate register_types.h"""
    content = f'''#ifndef {game_name.upper()}_REGISTER_TYPES_H
#define {game_name.upper()}_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_{game_name}_module(ModuleInitializationLevel p_level);
void uninitialize_{game_name}_module(ModuleInitializationLevel p_level);

#endif // {game_name.upper()}_REGISTER_TYPES_H
'''

    path = game_path / game_name / 'src' / 'register_types.h'
    path.write_text(content)
    print(f"Created: {path}")


def write_register_types_cpp(game_path: Path, game_name: str):
    """Generate register_types.cpp"""
    # Convert game_name to PascalCase for class names
    class_prefix = ''.join(word.capitalize() for word in game_name.split('_'))

    content = f'''#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

// Include your game contexts here
#include "start_button_context.h"

using namespace godot;

void initialize_{game_name}_module(ModuleInitializationLevel p_level) {{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {{
        return;
    }}

    // Register your game-specific contexts
    ClassDB::register_class<{class_prefix}StartButtonContext>();

    // Add more context registrations here as you create them
    // ClassDB::register_class<{class_prefix}EnemyContext>();
    // ClassDB::register_class<{class_prefix}PlayerContext>();
}}

void uninitialize_{game_name}_module(ModuleInitializationLevel p_level) {{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {{
        return;
    }}
}}

extern "C" {{
    GDExtensionBool GDE_EXPORT {game_name}_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {{
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_{game_name}_module);
        init_obj.register_terminator(uninitialize_{game_name}_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }}
}}
'''

    path = game_path / game_name / 'src' / 'register_types.cpp'
    path.write_text(content)
    print(f"Created: {path}")


def write_example_context(game_path: Path, game_name: str):
    """Generate an example button context."""
    # Convert game_name to PascalCase for class names
    class_prefix = ''.join(word.capitalize() for word in game_name.split('_'))

    content = f'''#ifndef {game_name.upper()}_START_BUTTON_CONTEXT_H
#define {game_name.upper()}_START_BUTTON_CONTEXT_H

// Include Cortex framework headers
#include "clay_button_node.h"
#include "gd_macros.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/**
 * {class_prefix}StartButtonContext
 *
 * Example game-specific context that extends ClayButtonContext.
 * This demonstrates how to create custom button behaviors in your game.
 *
 * Usage in Godot Editor:
 * 1. Add a ClayButtonNode to your scene
 * 2. In the Inspector, set the Context property to a new {class_prefix}StartButtonContext
 * 3. Configure the game_scene property to point to your game scene
 */
class {class_prefix}StartButtonContext : public ClayButtonContext {{
    GDCLASS({class_prefix}StartButtonContext, ClayButtonContext);

    // Properties editable in Godot Inspector
    GD_PROPERTY(String, game_scene, "res://scenes/game.tscn")
    GD_PROPERTY(String, transition_effect, "fade")
    GD_PROPERTY(float, transition_duration, 0.5f)

protected:
    static void _bind_methods() {{
        // Bind properties so they appear in editor
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, game_scene);
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, transition_effect);
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, transition_duration);
    }}

public:
    void on_pressed() override {{
        UtilityFunctions::print("[{class_prefix}] Start button pressed!");

        // Get the button node for additional info
        ClayButtonNode* btn = get_node<ClayButtonNode>();
        if (btn) {{
            UtilityFunctions::print("  Button label: ", btn->get_label());
        }}

        // Example: Change to game scene
        // Uncomment when you have a game scene set up:
        // if (btn && !game_scene.is_empty()) {{
        //     btn->get_tree()->change_scene_to_file(game_scene);
        // }}
    }}

    void on_hover_enter() override {{
        UtilityFunctions::print("[{class_prefix}] Hovering start button");
    }}

    void on_hover_exit() override {{
        UtilityFunctions::print("[{class_prefix}] Stopped hovering start button");
    }}
}};

#endif // {game_name.upper()}_START_BUTTON_CONTEXT_H
'''

    path = game_path / game_name / 'src' / 'start_button_context.h'
    path.write_text(content)
    print(f"Created: {path}")


def write_gdextension(game_path: Path, game_name: str):
    """Generate the .gdextension file."""
    content = f'''[configuration]
entry_symbol = "{game_name}_library_init"
compatibility_minimum = "4.2"

[libraries]
; Windows
windows.debug.x86_64 = "res://bin/lib{game_name}.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/lib{game_name}.windows.template_release.x86_64.dll"

; Linux
linux.debug.x86_64 = "res://bin/lib{game_name}.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://bin/lib{game_name}.linux.template_release.x86_64.so"

; macOS
macos.debug = "res://bin/lib{game_name}.macos.template_debug.universal.dylib"
macos.release = "res://bin/lib{game_name}.macos.template_release.universal.dylib"

[dependencies]
; Cortex framework library (must be in same bin/ folder)
windows.debug.x86_64 = {{"res://bin/libcortex.windows.template_debug.x86_64.dll": ""}}
windows.release.x86_64 = {{"res://bin/libcortex.windows.template_release.x86_64.dll": ""}}
linux.debug.x86_64 = {{"res://bin/libcortex.linux.template_debug.x86_64.so": ""}}
linux.release.x86_64 = {{"res://bin/libcortex.linux.template_release.x86_64.so": ""}}
macos.debug = {{"res://bin/libcortex.macos.template_debug.universal.dylib": ""}}
macos.release = {{"res://bin/libcortex.macos.template_release.universal.dylib": ""}}
'''

    path = game_path / 'bin' / f'{game_name}.gdextension'
    path.write_text(content)
    print(f"Created: {path}")


def write_cortex_gdextension(game_path: Path):
    """Generate the cortex.gdextension file for the dependency."""
    content = '''[configuration]
entry_symbol = "cortex_library_init"
compatibility_minimum = "4.2"

[libraries]
; Windows
windows.debug.x86_64 = "res://bin/libcortex.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/libcortex.windows.template_release.x86_64.dll"

; Linux
linux.debug.x86_64 = "res://bin/libcortex.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://bin/libcortex.linux.template_release.x86_64.so"

; macOS
macos.debug = "res://bin/libcortex.macos.template_debug.universal.dylib"
macos.release = "res://bin/libcortex.macos.template_release.universal.dylib"
'''

    path = game_path / 'bin' / 'cortex.gdextension'
    path.write_text(content)
    print(f"Created: {path}")


def write_readme(game_path: Path, game_name: str, cortex_path: Path):
    """Generate a README for the game project."""
    content = f'''# {game_name}

Game project built with CortexFramework.

## Project Structure

```
{game_path.name}/
    {game_name}/              # C++ extension source
        src/
            register_types.cpp
            register_types.h
            start_button_context.h  # Example context
        SConstruct
    bin/                      # Built libraries (add to Godot project)
        {game_name}.gdextension
        cortex.gdextension
```

## Building

### Prerequisites

1. Build Cortex framework first:
   ```bash
   cd {cortex_path}
   scons
   ```

2. Then build your game:
   ```bash
   cd {game_path / game_name}
   scons
   ```

### Build Commands

```bash
scons                           # Debug build
scons target=template_release   # Release build
```

## Adding to Godot

1. Copy the entire `bin/` folder to your Godot project root
2. Godot will detect both `.gdextension` files automatically
3. Both Cortex and your game libraries will be loaded

## Creating New Contexts

1. Create a new header in `src/`, e.g., `my_context.h`
2. Extend a Cortex base context (e.g., `ClayButtonContext`, `NodeContext`)
3. Register it in `register_types.cpp`
4. Rebuild with `scons`

Example:
```cpp
#include "clay_button_node.h"
#include "gd_macros.h"

class MyButtonContext : public ClayButtonContext {{
    GDCLASS(MyButtonContext, ClayButtonContext);

    GD_PROPERTY(String, my_property, "default")

protected:
    static void _bind_methods() {{
        GD_BIND_PROPERTY(MyButtonContext, my_property);
    }}

public:
    void on_pressed() override {{
        // Your button logic
    }}
}};
```

## Cortex Framework

Located at: {cortex_path}

For framework documentation, see the Cortex README.
'''

    path = game_path / 'README.md'
    path.write_text(content)
    print(f"Created: {path}")


def main():
    parser = argparse.ArgumentParser(
        description='Create a new game project using CortexFramework',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
    python tools/create_game.py C:/Games/MyGame mygame
    python tools/create_game.py ./projects/puzzle_game puzzle
    python tools/create_game.py /home/dev/space_shooter space_shooter
        '''
    )

    parser.add_argument('game_path', help='Path where the game project will be created')
    parser.add_argument('game_name', help='Name of the game (used for library naming)')

    args = parser.parse_args()

    game_path = Path(args.game_path).absolute()
    game_name = args.game_name.lower().replace('-', '_').replace(' ', '_')
    cortex_path = get_cortex_path()

    print(f"\nCortexFramework Game Project Generator")
    print(f"======================================")
    print(f"Game Path:   {game_path}")
    print(f"Game Name:   {game_name}")
    print(f"Cortex Path: {cortex_path}")
    print()

    # Validate game name
    if not game_name.isidentifier():
        print(f"Error: '{game_name}' is not a valid identifier.")
        print("Use only letters, numbers, and underscores (no leading numbers).")
        sys.exit(1)

    # Create structure
    print("Creating project structure...")
    create_directory_structure(game_path, game_name)

    print("\nGenerating files...")
    write_sconstruct(game_path, game_name, cortex_path)
    write_register_types_h(game_path, game_name)
    write_register_types_cpp(game_path, game_name)
    write_example_context(game_path, game_name)
    write_gdextension(game_path, game_name)
    write_cortex_gdextension(game_path)
    write_readme(game_path, game_name, cortex_path)

    print(f"\n{'='*50}")
    print(f"Game project '{game_name}' created successfully!")
    print(f"{'='*50}")
    print(f"""
Next steps:

1. Build Cortex (if not already built):
   cd {cortex_path}
   scons

2. Build your game:
   cd {game_path / game_name}
   scons

3. Copy bin/ folder to your Godot project:
   {game_path / 'bin'} -> YourGodotProject/bin/

4. Open Godot and your extensions will be loaded!

5. Add custom contexts in {game_path / game_name / 'src'}
""")


if __name__ == '__main__':
    main()
