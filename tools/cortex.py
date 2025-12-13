#!/usr/bin/env python3
"""
Cortex CLI Tool

Adds CortexFramework C++ extension support to an existing Godot project.
Creates Visual Studio solution, source files, and registers GDExtensions.

Usage:
    python tools/cortex.py                    # Interactive - select project
    python tools/cortex.py <godot_project>    # Specify project path

Example:
    python tools/cortex.py
    python tools/cortex.py C:/Games/MyGame
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
        search_paths = [
            Path.home() / 'Documents',
            Path.home() / 'Projects',
            Path.home() / 'Games',
            Path.home() / 'Godot',
            Path.cwd().parent,
            Path.cwd().parent.parent,
        ]
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

    projects.sort(key=lambda p: p.name.lower())
    return projects


def select_godot_project(projects):
    """Interactive selection of a Godot project."""
    if not projects:
        print("\nNo Godot projects found in common locations.")
        print("Enter a path manually.\n")
        path = input("Enter Godot project path: ").strip()
        if path:
            return Path(path).absolute()
        return None

    print("\n" + "=" * 50)
    print("Available Godot Projects")
    print("=" * 50)

    for i, project in enumerate(projects, 1):
        print(f"  [{i}] {project.name}")
        print(f"      {project}")

    print(f"\n  [N] Enter custom path")
    print(f"  [Q] Quit")
    print()

    while True:
        choice = input("Select project (number/N/Q): ").strip().upper()

        if choice == 'Q':
            sys.exit(0)

        if choice == 'N':
            path = input("Enter Godot project path: ").strip()
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


def get_project_name(project_path: Path):
    """Derive extension name from project folder name."""
    name = project_path.name.lower()
    name = ''.join(c if c.isalnum() or c == '_' else '_' for c in name)
    name = name.strip('_')
    if name and name[0].isdigit():
        name = 'game_' + name
    return name or 'game'


def create_directory_structure(project_path: Path):
    """Create the project directory structure."""
    dirs = [
        project_path / 'src',
        project_path / 'bin',         # DLLs go here
        project_path / 'extensions',  # .gdextension files go here
    ]
    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)
        print(f"  Created: {d}")


def generate_uuid():
    """Generate a Visual Studio compatible GUID."""
    return '{' + str(uuid.uuid4()).upper() + '}'


def write_vcxproj(project_path: Path, project_name: str, cortex_path: Path, project_guid: str):
    """Generate the Visual Studio project file."""
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
    <RootNamespace>{project_name}</RootNamespace>
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
    <OutDir>$(ProjectDir)bin\\</OutDir>
    <IntDir>$(ProjectDir)obj\\</IntDir>
    <NMakeBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons platform=windows target=template_debug debug_symbols=yes</NMakeBuildCommandLine>
    <NMakeReBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c &amp;&amp; scons platform=windows target=template_debug debug_symbols=yes</NMakeReBuildCommandLine>
    <NMakeCleanCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c</NMakeCleanCommandLine>
    <NMakeOutput>$(ProjectDir)bin\\lib{project_name}.windows.debug.x86_64.dll</NMakeOutput>
    <NMakeIncludeSearchPath>$(ProjectDir)src;{cortex_path}\\src;{cortex_path}\\godot-cpp\\include;{cortex_path}\\godot-cpp\\gen\\include;{cortex_path}\\godot-cpp\\gdextension;{cortex_path}\\flecs\\include</NMakeIncludeSearchPath>
    <NMakePreprocessorDefinitions>DEBUG_ENABLED;DEBUG_METHODS_ENABLED;WINDOWS_ENABLED;TYPED_METHOD_BIND;WIN32;_DEBUG</NMakePreprocessorDefinitions>
    <AdditionalOptions>/std:c++20</AdditionalOptions>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <OutDir>$(ProjectDir)bin\\</OutDir>
    <IntDir>$(ProjectDir)obj\\</IntDir>
    <NMakeBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons platform=windows target=template_release</NMakeBuildCommandLine>
    <NMakeReBuildCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c &amp;&amp; scons platform=windows target=template_release</NMakeReBuildCommandLine>
    <NMakeCleanCommandLine>cd /d "$(ProjectDir)" &amp;&amp; scons -c</NMakeCleanCommandLine>
    <NMakeOutput>$(ProjectDir)bin\\lib{project_name}.windows.release.x86_64.dll</NMakeOutput>
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

    path = project_path / f'{project_name}.vcxproj'
    path.write_text(content)
    print(f"  Created: {path}")


def write_sln(project_path: Path, project_name: str, project_guid: str, cortex_path: Path):
    """Generate the Visual Studio solution file with both game and Cortex projects."""
    cortex_guid = '{8A2E8F5A-0C3D-4F1E-9B5A-1234567890AB}'
    cortex_vcxproj = cortex_path / 'cortex.vcxproj'

    content = f'''Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.0.31903.59
MinimumVisualStudioVersion = 10.0.40219.1
Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{project_name}", "{project_name}.vcxproj", "{project_guid}"
	ProjectSection(ProjectDependencies) = postProject
		{cortex_guid} = {cortex_guid}
	EndProjectSection
EndProject
Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "cortex", "{cortex_vcxproj}", "{cortex_guid}"
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
		{cortex_guid}.Debug|x64.ActiveCfg = Debug|x64
		{cortex_guid}.Debug|x64.Build.0 = Debug|x64
		{cortex_guid}.Release|x64.ActiveCfg = Release|x64
		{cortex_guid}.Release|x64.Build.0 = Release|x64
	EndGlobalSection
EndGlobal
'''

    path = project_path / f'{project_name}.sln'
    path.write_text(content)
    print(f"  Created: {path}")


def write_sconstruct(project_path: Path, project_name: str, cortex_path: Path):
    """Generate the SConstruct build file."""
    content = f'''#!/usr/bin/env python
"""
SConstruct - Build script for {project_name}

This game project compiles Cortex sources directly into a single DLL.
This avoids cross-extension inheritance issues.
"""

import os
import sys

# Path to Cortex framework
cortex_path = r'{cortex_path}'
godot_cpp_path = os.path.join(cortex_path, 'godot-cpp')
flecs_dir = os.path.join(cortex_path, 'flecs')

# Add godot-cpp to the build environment
env = SConscript(os.path.join(godot_cpp_path, 'SConstruct'))

# Project configuration
project_name = '{project_name}'
src_dir = 'src'

# Include paths
env.Append(CPPPATH=[
    src_dir,
    os.path.join(cortex_path, 'src'),
    os.path.join(flecs_dir, 'include'),
])

# C++20 for Clay UI compatibility
if env['platform'] == 'windows':
    env.Append(CXXFLAGS=['/std:c++20'])
else:
    env.Append(CXXFLAGS=['-std=c++20'])

# Collect game source files
sources = Glob(os.path.join(src_dir, '*.cpp'))
sources += Glob(os.path.join(src_dir, '*.c'))

# Include Cortex source files directly (single DLL approach)
cortex_src_dir = os.path.join(cortex_path, 'src')
sources += Glob(os.path.join(cortex_src_dir, '*.cpp'))
sources += Glob(os.path.join(cortex_src_dir, '*.c'))

# Include Flecs source files
flecs_src_dir = os.path.join(flecs_dir, 'src')
for root, dirs, files in os.walk(flecs_src_dir):
    for f in files:
        if f.endswith('.c'):
            sources.append(os.path.join(root, f))

# Flecs configuration - build as static embedded in our DLL
env.Append(CPPDEFINES=['flecs_STATIC'])

# Output configuration
output_dir = 'bin'

# Map SCons target to simple name (template_debug -> debug)
target_name = env['target'].replace('template_', '')

# Platform-specific settings
if env['platform'] == 'windows':
    lib_suffix = '.dll'
elif env['platform'] == 'macos':
    lib_suffix = '.dylib'
else:
    lib_suffix = '.so'

# Library name (without template_ prefix)
library_name = 'lib{{}}.{{}}.{{}}.{{}}{{}}'.format(
    project_name,
    env['platform'],
    target_name,
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

Help("""
{project_name} - CortexFramework Game
=====================================

Build:
    scons                           # Debug build
    scons target=template_release   # Release build

Or open {project_name}.sln in Visual Studio and build (F7)

Note: Cortex framework is compiled directly into this DLL (single extension).
""")
'''

    path = project_path / 'SConstruct'
    path.write_text(content)
    print(f"  Created: {path}")


def write_register_types_h(project_path: Path, project_name: str):
    """Generate register_types.h"""
    content = f'''#ifndef {project_name.upper()}_REGISTER_TYPES_H
#define {project_name.upper()}_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_{project_name}_module(ModuleInitializationLevel p_level);
void uninitialize_{project_name}_module(ModuleInitializationLevel p_level);

#endif
'''

    path = project_path / 'src' / 'register_types.h'
    path.write_text(content)
    print(f"  Created: {path}")


def write_register_types_cpp(project_path: Path, project_name: str):
    """Generate register_types.cpp"""
    class_prefix = ''.join(word.capitalize() for word in project_name.split('_'))

    content = f'''#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Cortex framework initialization helper
#include "cortex_init.h"

// Game contexts
#include "start_button_context.h"

using namespace godot;

void initialize_{project_name}_module(ModuleInitializationLevel p_level) {{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {{
        return;
    }}

    // Register Cortex framework classes
    cortex_register_classes();

    // Register game contexts
    GDREGISTER_CLASS({class_prefix}StartButtonContext);

    UtilityFunctions::print("[{class_prefix}] Extension loaded successfully!");
}}

void uninitialize_{project_name}_module(ModuleInitializationLevel p_level) {{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {{
        return;
    }}

    // Cleanup Cortex
    cortex_unregister_classes();
}}

extern "C" {{
    GDExtensionBool GDE_EXPORT {project_name}_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {{
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_{project_name}_module);
        init_obj.register_terminator(uninitialize_{project_name}_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }}
}}
'''

    path = project_path / 'src' / 'register_types.cpp'
    path.write_text(content)
    print(f"  Created: {path}")


def write_example_context(project_path: Path, project_name: str):
    """Generate an example button context."""
    class_prefix = ''.join(word.capitalize() for word in project_name.split('_'))

    content = f'''#ifndef {project_name.upper()}_START_BUTTON_CONTEXT_H
#define {project_name.upper()}_START_BUTTON_CONTEXT_H

#include "clay_button_node.h"
#include "gd_macros.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {{

class {class_prefix}StartButtonContext : public ClayButtonContext {{
    GDCLASS({class_prefix}StartButtonContext, ClayButtonContext);

    GD_PROPERTY(String, game_scene, "res://scenes/game.tscn")

protected:
    static void _bind_methods() {{
        GD_BIND_PROPERTY({class_prefix}StartButtonContext, String, game_scene);
    }}

public:
    {class_prefix}StartButtonContext() {{}}
    ~{class_prefix}StartButtonContext() {{}}

    void on_pressed() override {{
        UtilityFunctions::print("[{class_prefix}] Button pressed!");

        // Change scene example:
        // ClayButtonNode* btn = get_node<ClayButtonNode>();
        // if (btn && !_game_scene.is_empty()) {{
        //     btn->get_tree()->change_scene_to_file(_game_scene);
        // }}
    }}

    void on_hover_enter() override {{
        UtilityFunctions::print("[{class_prefix}] Hover");
    }}
}};

}} // namespace godot

#endif
'''

    path = project_path / 'src' / 'start_button_context.h'
    path.write_text(content)
    print(f"  Created: {path}")


def write_gdextension(project_path: Path, project_name: str):
    """Generate the .gdextension file for the game in extensions folder.

    Single DLL approach - Cortex is compiled into the game DLL, no dependencies.
    """
    content = f'''[configuration]
entry_symbol = "{project_name}_library_init"
compatibility_minimum = "4.2"

[libraries]
windows.debug.x86_64 = "res://bin/lib{project_name}.windows.debug.x86_64.dll"
windows.release.x86_64 = "res://bin/lib{project_name}.windows.release.x86_64.dll"
linux.debug.x86_64 = "res://bin/lib{project_name}.linux.debug.x86_64.so"
linux.release.x86_64 = "res://bin/lib{project_name}.linux.release.x86_64.so"
macos.debug = "res://bin/lib{project_name}.macos.debug.universal.dylib"
macos.release = "res://bin/lib{project_name}.macos.release.universal.dylib"
'''

    # Place .gdextension in extensions folder
    path = project_path / 'extensions' / f'{project_name}.gdextension'
    path.write_text(content)
    print(f"  Created: {path}")


def main():
    parser = argparse.ArgumentParser(
        description='Cortex CLI - Add C++ extension support to a Godot project',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
    python tools/cortex.py                  # Interactive - select from found projects
    python tools/cortex.py C:/Games/MyGame  # Specify project path directly

What this tool does:
    1. Creates src/ folder with starter C++ files
    2. Creates Visual Studio solution (includes Cortex for debugging)
    3. Adds .gdextension files so Godot loads the extensions
    4. Sets up SConstruct for building with SCons
        '''
    )

    parser.add_argument('project_path', nargs='?', help='Path to Godot project')

    args = parser.parse_args()
    cortex_path = get_cortex_path()

    print()
    print("=" * 60)
    print("  Cortex - Add C++ Extensions to Godot Project")
    print("=" * 60)
    print(f"  Cortex Framework: {cortex_path}")
    print()

    # Get project path
    if args.project_path:
        project_path = Path(args.project_path).absolute()
    else:
        print("Scanning for Godot projects...")
        projects = find_godot_projects()
        project_path = select_godot_project(projects)

        if project_path is None:
            print("No project selected. Exiting.")
            sys.exit(1)

    # Verify it's a Godot project
    if not (project_path / 'project.godot').exists() and not (project_path / '.godot').exists():
        print(f"\nWARNING: No project.godot found in {project_path}")
        print("This may not be a Godot project.")
        confirm = input("Continue anyway? [y/N]: ").strip().lower()
        if confirm != 'y':
            print("Cancelled.")
            sys.exit(0)

    # Derive project name from folder
    project_name = get_project_name(project_path)
    project_guid = generate_uuid()

    print()
    print("-" * 60)
    print(f"  Project:  {project_path}")
    print(f"  Name:     {project_name}")
    print("-" * 60)
    print()

    # Check if already set up
    if (project_path / 'SConstruct').exists():
        print("WARNING: This project already has C++ support!")
        confirm = input("Overwrite existing files? [y/N]: ").strip().lower()
        if confirm != 'y':
            print("Cancelled.")
            sys.exit(0)
    else:
        confirm = input("Add C++ extension support? [Y/n]: ").strip().lower()
        if confirm and confirm != 'y':
            print("Cancelled.")
            sys.exit(0)

    print()
    print("Creating files...")
    create_directory_structure(project_path)
    write_sconstruct(project_path, project_name, cortex_path)
    write_register_types_h(project_path, project_name)
    write_register_types_cpp(project_path, project_name)
    write_example_context(project_path, project_name)
    write_vcxproj(project_path, project_name, cortex_path, project_guid)
    write_sln(project_path, project_name, project_guid, cortex_path)
    write_gdextension(project_path, project_name)

    print()
    print("=" * 60)
    print("  SUCCESS!")
    print("=" * 60)
    print(f'''
  Files created in {project_path}:

    {project_name}.sln              <- Open in Visual Studio
    {project_name}.vcxproj
    SConstruct
    src/
        register_types.cpp
        register_types.h
        start_button_context.h      <- Example context
    bin/                            <- Single DLL goes here after build
    extensions/                     <- .gdextension file
        {project_name}.gdextension

  Solution contains:
    - {project_name}  (your game code)
    - cortex          (framework - editable & debuggable)

  Next steps:

  1. Open {project_name}.sln in Visual Studio

  2. Build solution (F7)
     - Builds Cortex first, then your game
     - Copies all DLLs to bin/

  3. Open project in Godot
     - Extensions load automatically from extensions/ folder
     - Your custom contexts appear in the editor

  4. To debug: Debug > Attach to Process > Godot
''')


if __name__ == '__main__':
    main()
