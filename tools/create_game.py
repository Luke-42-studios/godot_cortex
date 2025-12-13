#!/usr/bin/env python3
"""
CortexFramework Game Project Generator

Creates a new game project that links against the Cortex framework.
Can add to an existing Godot project or create a new one.

Usage:
    python tools/create_game.py                    # Interactive mode
    python tools/create_game.py <game_name>        # Create with name, select project
    python tools/create_game.py <path> <game_name> # Specify path and name

Example:
    python tools/create_game.py mygame
    python tools/create_game.py C:/Games/MyGame mygame
"""

import os
import sys
import uuid
import argparse
from pathlib import Path


def get_cortex_path():
    """Get the absolute path to the Cortex framework."""
    return Path(__file__).parent.parent.absolute()


def find_godot_projects(search_paths=None):
    """Find Godot projects by looking for .godot folders or project.godot files."""
    if search_paths is None:
        # Default search locations
        search_paths = [
            Path.home() / 'Documents',
            Path.home() / 'Projects',
            Path.home() / 'Games',
            Path.home() / 'Godot',
            Path.cwd().parent,
            Path.cwd().parent.parent,
        ]
        # Add common Windows paths
        if sys.platform == 'win32':
            search_paths.extend([
                Path('C:/Games'),
                Path('C:/Projects'),
                Path('C:/Godot'),
                Path('D:/Games'),
                Path('D:/Projects'),
            ])

    projects = []
    seen = set()

    for base_path in search_paths:
        if not base_path.exists():
            continue

        # Search up to 3 levels deep
        for depth in range(3):
            pattern = '/'.join(['*'] * (depth + 1))
            for godot_marker in base_path.glob(f'{pattern}/project.godot'):
                project_path = godot_marker.parent
                if project_path not in seen:
                    seen.add(project_path)
                    projects.append(project_path)

            for godot_marker in base_path.glob(f'{pattern}/.godot'):
                project_path = godot_marker.parent
                if project_path not in seen:
                    seen.add(project_path)
                    projects.append(project_path)

    # Sort by name
    projects.sort(key=lambda p: p.name.lower())
    return projects


def select_godot_project(projects):
    """Interactive selection of a Godot project."""
    if not projects:
        print("\nNo Godot projects found in common locations.")
        print("Enter a path manually or create a new project.\n")
        return None

    print("\n" + "=" * 50)
    print("Available Godot Projects")
    print("=" * 50)

    for i, project in enumerate(projects, 1):
        print(f"  [{i}] {project.name}")
        print(f"      {project}")

    print(f"\n  [N] Create NEW project at custom path")
    print(f"  [Q] Quit")
    print()

    while True:
        choice = input("Select project (number/N/Q): ").strip().upper()

        if choice == 'Q':
            sys.exit(0)

        if choice == 'N':
            path = input("Enter path for new project: ").strip()
            if path:
                return Path(path).absolute()
            continue

        try:
            idx = int(choice) - 1
            if 0 <= idx < len(projects):
                return projects[idx]
        except ValueError:
            pass

        print("Invalid selection. Try again.")


def get_game_name():
    """Prompt for game name."""
    while True:
        name = input("Enter game/extension name (e.g., mygame): ").strip().lower()
        name = name.replace('-', '_').replace(' ', '_')

        if name and name.isidentifier():
            return name

        print("Invalid name. Use only letters, numbers, and underscores (no leading numbers).")


def create_directory_structure(game_path: Path, game_name: str):
    """Create the game project directory structure."""
    dirs = [
        game_path / game_name / 'src',
        game_path / 'bin',
    ]
    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)
        print(f"  Created: {d}")


def generate_uuid():
    """Generate a Visual Studio compatible GUID."""
    return '{' + str(uuid.uuid4()).upper() + '}'


def write_vcxproj(game_path: Path, game_name: str, cortex_path: Path, project_guid: str):
    """Generate the Visual Studio project file."""
    # Convert game_name to PascalCase for display
    display_name = ''.join(word.capitalize() for word in game_name.split('_'))

    content = f'''<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>17.0</VCProjectVersion>
    <ProjectGuid>{project_guid}</ProjectGuid>
    <RootNamespace>{game_name}</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
    <Keyword>MakeFileProj</Keyword>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>Makefile</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>Makefile</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <OutDir>$(ProjectDir)..\\bin\\</OutDir>
    <IntDir>$(ProjectDir)obj\\</IntDir>
    <NMakeBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons platform=windows target=template_debug debug_symbols=yes</NMakeBuildCommandLine>
    <NMakeReBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c &amp;&amp; scons platform=windows target=template_debug debug_symbols=yes</NMakeReBuildCommandLine>
    <NMakeCleanCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c</NMakeCleanCommandLine>
    <NMakeOutput>$(ProjectDir)..\\bin\\lib{game_name}.windows.template_debug.x86_64.dll</NMakeOutput>
    <NMakeIncludeSearchPath>$(ProjectDir)src;{cortex_path}\\src;{cortex_path}\\godot-cpp\\include;{cortex_path}\\godot-cpp\\gen\\include;{cortex_path}\\godot-cpp\\gdextension;{cortex_path}\\flecs\\include</NMakeIncludeSearchPath>
    <NMakePreprocessorDefinitions>DEBUG_ENABLED;DEBUG_METHODS_ENABLED;WINDOWS_ENABLED;TYPED_METHOD_BIND;WIN32;_DEBUG</NMakePreprocessorDefinitions>
    <AdditionalOptions>/std:c++20</AdditionalOptions>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <OutDir>$(ProjectDir)..\\bin\\</OutDir>
    <IntDir>$(ProjectDir)obj\\</IntDir>
    <NMakeBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons platform=windows target=template_release</NMakeBuildCommandLine>
    <NMakeReBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c &amp;&amp; scons platform=windows target=template_release</NMakeReBuildCommandLine>
    <NMakeCleanCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c</NMakeCleanCommandLine>
    <NMakeOutput>$(ProjectDir)..\\bin\\lib{game_name}.windows.template_release.x86_64.dll</NMakeOutput>
    <NMakeIncludeSearchPath>$(ProjectDir)src;{cortex_path}\\src;{cortex_path}\\godot-cpp\\include;{cortex_path}\\godot-cpp\\gen\\include;{cortex_path}\\godot-cpp\\gdextension;{cortex_path}\\flecs\\include</NMakeIncludeSearchPath>
    <NMakePreprocessorDefinitions>WINDOWS_ENABLED;TYPED_METHOD_BIND;WIN32;NDEBUG</NMakePreprocessorDefinitions>
    <AdditionalOptions>/std:c++20</AdditionalOptions>
  </PropertyGroup>
  <ItemGroup>
    <ClInclude Include="src\\register_types.h" />
    <ClInclude Include="src\\start_button_context.h" />
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="src\\register_types.cpp" />
  </ItemGroup>
  <ItemGroup>
    <None Include="SConstruct" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />
</Project>
'''

    path = game_path / game_name / f'{game_name}.vcxproj'
    path.write_text(content)
    print(f"  Created: {path}")
    return path


def write_sln(game_path: Path, game_name: str, project_guid: str):
    """Generate the Visual Studio solution file."""
    display_name = ''.join(word.capitalize() for word in game_name.split('_'))

    content = f'''Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.0.31903.59
MinimumVisualStudioVersion = 10.0.40219.1
Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{game_name}", "{game_name}\\{game_name}.vcxproj", "{project_guid}"
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|x64 = Debug|x64
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
		{project_guid}.Debug|x64.ActiveCfg = Debug|x64
		{project_guid}.Debug|x64.Build.0 = Debug|x64
		{project_guid}.Release|x64.ActiveCfg = Release|x64
		{project_guid}.Release|x64.Build.0 = Release|x64
	EndGlobalSection
EndGlobal
'''

    # Solution file goes in the game root (Godot project root)
    path = game_path / f'{game_name}.sln'
    path.write_text(content)
    print(f"  Created: {path}")
    return path


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

# Path to Cortex framework
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
    if os.path.exists(cortex_lib_dir):
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

Or use Visual Studio: Open {game_name}.sln and build (F7)

Output goes to ../bin/ (your Godot project's bin folder).
""")
'''

    path = game_path / game_name / 'SConstruct'
    path.write_text(content)
    print(f"  Created: {path}")


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
    print(f"  Created: {path}")


def write_register_types_cpp(game_path: Path, game_name: str):
    """Generate register_types.cpp"""
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

    // Add more context registrations here:
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
    print(f"  Created: {path}")


def write_example_context(game_path: Path, game_name: str):
    """Generate an example button context."""
    class_prefix = ''.join(word.capitalize() for word in game_name.split('_'))

    content = f'''#ifndef {game_name.upper()}_START_BUTTON_CONTEXT_H
#define {game_name.upper()}_START_BUTTON_CONTEXT_H

// Cortex framework headers
#include "clay_button_node.h"
#include "gd_macros.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/**
 * {class_prefix}StartButtonContext
 *
 * Example game-specific context that extends ClayButtonContext.
 * Demonstrates how to create custom button behaviors.
 *
 * Usage in Godot Editor:
 * 1. Add a ClayButtonNode to your scene
 * 2. In Inspector, set Context to a new {class_prefix}StartButtonContext
 * 3. Configure properties as needed
 */
class {class_prefix}StartButtonContext : public ClayButtonContext {{
    GDCLASS({class_prefix}StartButtonContext, ClayButtonContext);

    // Properties editable in Godot Inspector
    GD_PROPERTY(String, game_scene, "res://scenes/game.tscn")
    GD_PROPERTY(String, transition_effect, "fade")
    GD_PROPERTY(float, transition_duration, 0.5f)

protected:
    static void _bind_methods() {{
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, game_scene);
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, transition_effect);
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, transition_duration);
    }}

public:
    void on_pressed() override {{
        UtilityFunctions::print("[{class_prefix}] Start button pressed!");

        ClayButtonNode* btn = get_node<ClayButtonNode>();
        if (btn) {{
            UtilityFunctions::print("  Button label: ", btn->get_label());
        }}

        // Change scene example (uncomment when ready):
        // if (btn && !game_scene.is_empty()) {{
        //     btn->get_tree()->change_scene_to_file(game_scene);
        // }}
    }}

    void on_hover_enter() override {{
        UtilityFunctions::print("[{class_prefix}] Hover enter");
    }}

    void on_hover_exit() override {{
        UtilityFunctions::print("[{class_prefix}] Hover exit");
    }}
}};

#endif // {game_name.upper()}_START_BUTTON_CONTEXT_H
'''

    path = game_path / game_name / 'src' / 'start_button_context.h'
    path.write_text(content)
    print(f"  Created: {path}")


def write_gdextension(game_path: Path, game_name: str):
    """Generate the .gdextension file for the game."""
    content = f'''[configuration]
entry_symbol = "{game_name}_library_init"
compatibility_minimum = "4.2"

[libraries]
windows.debug.x86_64 = "res://bin/lib{game_name}.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/lib{game_name}.windows.template_release.x86_64.dll"
linux.debug.x86_64 = "res://bin/lib{game_name}.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://bin/lib{game_name}.linux.template_release.x86_64.so"
macos.debug = "res://bin/lib{game_name}.macos.template_debug.universal.dylib"
macos.release = "res://bin/lib{game_name}.macos.template_release.universal.dylib"

[dependencies]
windows.debug.x86_64 = {{"res://bin/libcortex.windows.template_debug.x86_64.dll": ""}}
windows.release.x86_64 = {{"res://bin/libcortex.windows.template_release.x86_64.dll": ""}}
linux.debug.x86_64 = {{"res://bin/libcortex.linux.template_debug.x86_64.so": ""}}
linux.release.x86_64 = {{"res://bin/libcortex.linux.template_release.x86_64.so": ""}}
macos.debug = {{"res://bin/libcortex.macos.template_debug.universal.dylib": ""}}
macos.release = {{"res://bin/libcortex.macos.template_release.universal.dylib": ""}}
'''

    path = game_path / 'bin' / f'{game_name}.gdextension'
    path.write_text(content)
    print(f"  Created: {path}")


def write_cortex_gdextension(game_path: Path):
    """Generate the cortex.gdextension file."""
    content = '''[configuration]
entry_symbol = "cortex_library_init"
compatibility_minimum = "4.2"

[libraries]
windows.debug.x86_64 = "res://bin/libcortex.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/libcortex.windows.template_release.x86_64.dll"
linux.debug.x86_64 = "res://bin/libcortex.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://bin/libcortex.linux.template_release.x86_64.so"
macos.debug = "res://bin/libcortex.macos.template_debug.universal.dylib"
macos.release = "res://bin/libcortex.macos.template_release.universal.dylib"
'''

    path = game_path / 'bin' / 'cortex.gdextension'
    path.write_text(content)
    print(f"  Created: {path}")


def write_gitignore(game_path: Path, game_name: str):
    """Generate .gitignore for the game extension folder."""
    content = '''# Build artifacts
obj/
*.obj
*.o
*.pdb
*.ilk
*.exp
*.lib

# Visual Studio
.vs/
*.suo
*.user
*.ncb
*.sdf
*.opensdf
*.VC.db
*.VC.VC.opendb

# SCons
.sconsign.dblite
*.pyc
'''

    path = game_path / game_name / '.gitignore'
    path.write_text(content)
    print(f"  Created: {path}")


def main():
    parser = argparse.ArgumentParser(
        description='Create a new game project using CortexFramework',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
    python tools/create_game.py                     # Interactive mode
    python tools/create_game.py mygame              # Name only, select project
    python tools/create_game.py C:/Games/MyGame mygame  # Full specification
        '''
    )

    parser.add_argument('args', nargs='*', help='[game_path] game_name')

    args = parser.parse_args()
    cortex_path = get_cortex_path()

    print()
    print("=" * 60)
    print("  CortexFramework Game Project Generator")
    print("=" * 60)
    print(f"  Cortex: {cortex_path}")
    print()

    # Parse arguments
    game_path = None
    game_name = None

    if len(args.args) == 0:
        # Full interactive mode
        pass
    elif len(args.args) == 1:
        # Just name provided, will select project
        game_name = args.args[0].lower().replace('-', '_').replace(' ', '_')
    else:
        # Path and name provided
        game_path = Path(args.args[0]).absolute()
        game_name = args.args[1].lower().replace('-', '_').replace(' ', '_')

    # Find and select Godot project if not specified
    if game_path is None:
        print("Scanning for Godot projects...")
        projects = find_godot_projects()
        game_path = select_godot_project(projects)

        if game_path is None:
            print("No project selected. Exiting.")
            sys.exit(1)

    # Get game name if not specified
    if game_name is None:
        game_name = get_game_name()

    # Validate
    if not game_name.isidentifier():
        print(f"Error: '{game_name}' is not a valid identifier.")
        sys.exit(1)

    project_guid = generate_uuid()

    print()
    print("-" * 60)
    print(f"  Project Path: {game_path}")
    print(f"  Game Name:    {game_name}")
    print(f"  Cortex Path:  {cortex_path}")
    print("-" * 60)
    print()

    # Confirm
    confirm = input("Create project? [Y/n]: ").strip().lower()
    if confirm and confirm != 'y':
        print("Cancelled.")
        sys.exit(0)

    print()
    print("Creating project structure...")
    create_directory_structure(game_path, game_name)

    print()
    print("Generating files...")
    write_sconstruct(game_path, game_name, cortex_path)
    write_register_types_h(game_path, game_name)
    write_register_types_cpp(game_path, game_name)
    write_example_context(game_path, game_name)
    write_vcxproj(game_path, game_name, cortex_path, project_guid)
    write_sln(game_path, game_name, project_guid)
    write_gdextension(game_path, game_name)
    write_cortex_gdextension(game_path)
    write_gitignore(game_path, game_name)

    print()
    print("=" * 60)
    print(f"  SUCCESS! Game project '{game_name}' created.")
    print("=" * 60)
    print(f'''
  Project structure:
    {game_path}/
        {game_name}.sln              <- Open in Visual Studio
        {game_name}/
            src/
                register_types.cpp
                start_button_context.h
            SConstruct
            {game_name}.vcxproj
        bin/
            {game_name}.gdextension
            cortex.gdextension

  Next steps:

  1. Build Cortex (if not already):
     cd {cortex_path}
     scons

  2. Build your game (choose one):
     - Visual Studio: Open {game_path / f'{game_name}.sln'} and build (F7)
     - Command line:  cd {game_path / game_name} && scons

  3. Open your Godot project - extensions load automatically!

  4. Add custom contexts in {game_path / game_name / 'src'}
''')


if __name__ == '__main__':
    main()
