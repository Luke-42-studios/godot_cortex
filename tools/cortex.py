#!/usr/bin/env python3
"""
Cortex CLI Tool

Adds CortexFramework C++ extension support to an existing Godot project.
Creates VS Code workspace configuration, source files, and registers GDExtensions.

Usage:
    python tools/cortex.py                    # Interactive - select project
    python tools/cortex.py <godot_project>    # Specify project path
    python tools/cortex.py --godot-path PATH  # Specify Godot engine path

Example:
    python tools/cortex.py
    python tools/cortex.py C:/Games/MyGame
    python tools/cortex.py C:/Games/MyGame --godot-path C:/Workspace/Godot/engine
"""

import os
import sys
import uuid
import json
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


def find_godot_engine(search_paths=None):
    """Find Godot engine directory by looking for SConstruct and editor executable."""
    if search_paths is None:
        # Default search paths
        cortex_path = get_cortex_path()
        search_paths = [
            cortex_path.parent / 'engine',       # Sibling 'engine' folder
            cortex_path.parent / 'godot',        # Sibling 'godot' folder
            Path.home() / 'Godot' / 'engine',
            Path('C:/Godot/engine'),
            Path('C:/Workspace/Godot/engine'),
        ]

    for path in search_paths:
        if path.exists() and (path / 'SConstruct').exists():
            # Verify it's the engine by checking for core folder
            if (path / 'core').exists() or (path / 'editor').exists():
                return path.absolute()

    return None


def find_godot_executable(engine_path: Path):
    """Find the Godot editor executable in the engine bin folder."""
    bin_path = engine_path / 'bin'
    if not bin_path.exists():
        return None

    # Look for editor executables (prefer debug builds)
    patterns = [
        'godot.windows.editor.dev.x86_64.exe',
        'godot.windows.editor.x86_64.exe',
        'godot.windows.editor.dev.x86_64.console.exe',
        'godot.windows.editor.x86_64.console.exe',
        'godot.linuxbsd.editor.dev.x86_64',
        'godot.linuxbsd.editor.x86_64',
        'godot.macos.editor.dev.universal',
        'godot.macos.editor.universal',
    ]

    for pattern in patterns:
        exe = bin_path / pattern
        if exe.exists():
            return exe

    # Fallback: find any editor executable
    import glob
    for exe in bin_path.glob('godot*editor*'):
        if exe.is_file():
            return exe

    return None


def write_vscode_tasks(project_path: Path, project_name: str):
    """Generate VS Code tasks.json for building the extension."""
    vscode_dir = project_path / '.vscode'
    vscode_dir.mkdir(exist_ok=True)

    tasks = {
        "version": "2.0.0",
        "tasks": [
            {
                "label": "build_debug",
                "type": "shell",
                "command": "scons",
                "args": ["platform=windows", "target=template_debug", "debug_symbols=yes"],
                "group": {
                    "kind": "build",
                    "isDefault": True
                },
                "problemMatcher": "$msCompile",
                "detail": "Build GDExtension DLL (debug with symbols)"
            },
            {
                "label": "build_release",
                "type": "shell",
                "command": "scons",
                "args": ["platform=windows", "target=template_release"],
                "group": "build",
                "problemMatcher": "$msCompile",
                "detail": "Build GDExtension DLL (release)"
            },
            {
                "label": "clean",
                "type": "shell",
                "command": "scons",
                "args": ["-c"],
                "group": "build",
                "problemMatcher": [],
                "detail": "Clean build artifacts"
            },
            {
                "label": "rebuild_debug",
                "type": "shell",
                "command": "scons",
                "args": ["-c"],
                "group": "build",
                "problemMatcher": "$msCompile",
                "detail": "Clean and rebuild debug",
                "dependsOn": ["clean"],
                "dependsOrder": "sequence"
            }
        ]
    }

    # Add second part of rebuild task
    tasks["tasks"].append({
        "label": "rebuild_debug",
        "type": "shell",
        "command": "scons",
        "args": ["platform=windows", "target=template_debug", "debug_symbols=yes"],
        "group": "build",
        "problemMatcher": "$msCompile",
        "dependsOn": ["clean"],
        "dependsOrder": "sequence"
    })

    # Remove duplicate rebuild_debug and fix it
    tasks["tasks"] = [t for t in tasks["tasks"] if t["label"] != "rebuild_debug"]
    tasks["tasks"].append({
        "label": "rebuild_debug",
        "dependsOn": ["clean", "build_debug"],
        "dependsOrder": "sequence",
        "group": "build",
        "problemMatcher": []
    })

    path = vscode_dir / 'tasks.json'
    path.write_text(json.dumps(tasks, indent=4))
    print(f"  Created: {path}")


def write_vscode_launch(project_path: Path, project_name: str, godot_exe: Path):
    """Generate VS Code launch.json for debugging."""
    vscode_dir = project_path / '.vscode'
    vscode_dir.mkdir(exist_ok=True)

    # Convert to forward slashes for JSON
    godot_exe_str = str(godot_exe).replace('\\', '/')
    project_path_str = str(project_path).replace('\\', '/')

    launch = {
        "version": "0.2.0",
        "configurations": [
            {
                "name": "Build & Launch Godot Editor",
                "type": "cppvsdbg",
                "request": "launch",
                "program": godot_exe_str,
                "args": ["--editor", "--path", project_path_str],
                "preLaunchTask": "build_debug",
                "stopAtEntry": False,
                "cwd": project_path_str,
                "environment": [],
                "console": "integratedTerminal"
            },
            {
                "name": "Build Only (Hot Reload)",
                "type": "cppvsdbg",
                "request": "launch",
                "program": godot_exe_str,
                "args": ["--version"],
                "preLaunchTask": "build_debug",
                "stopAtEntry": False,
                "cwd": project_path_str,
                "environment": [],
                "console": "integratedTerminal"
            },
            {
                "name": "Launch Godot Editor (no build)",
                "type": "cppvsdbg",
                "request": "launch",
                "program": godot_exe_str,
                "args": ["--editor", "--path", project_path_str],
                "stopAtEntry": False,
                "cwd": project_path_str,
                "environment": [],
                "console": "integratedTerminal"
            },
            {
                "name": "Launch Game (debug)",
                "type": "cppvsdbg",
                "request": "launch",
                "program": godot_exe_str,
                "args": ["--path", project_path_str],
                "preLaunchTask": "build_debug",
                "stopAtEntry": False,
                "cwd": project_path_str,
                "environment": [],
                "console": "integratedTerminal"
            },
            {
                "name": "Attach to Godot",
                "type": "cppvsdbg",
                "request": "attach",
                "processId": "${command:pickProcess}"
            }
        ]
    }

    path = vscode_dir / 'launch.json'
    path.write_text(json.dumps(launch, indent=4))
    print(f"  Created: {path}")


def write_vscode_cpp_properties(project_path: Path, project_name: str, cortex_path: Path):
    """Generate VS Code c_cpp_properties.json for IntelliSense."""
    vscode_dir = project_path / '.vscode'
    vscode_dir.mkdir(exist_ok=True)

    # Convert paths to forward slashes
    cortex_str = str(cortex_path).replace('\\', '/')
    project_str = str(project_path).replace('\\', '/')

    properties = {
        "configurations": [
            {
                "name": "Win32",
                "includePath": [
                    f"{project_str}/src",
                    f"{cortex_str}/src",
                    f"{cortex_str}/godot-cpp/include",
                    f"{cortex_str}/godot-cpp/gen/include",
                    f"{cortex_str}/godot-cpp/gdextension",
                    f"{cortex_str}/flecs/include"
                ],
                "defines": [
                    "DEBUG_ENABLED",
                    "DEBUG_METHODS_ENABLED",
                    "WINDOWS_ENABLED",
                    "TYPED_METHOD_BIND",
                    "WIN32",
                    "_DEBUG"
                ],
                "windowsSdkVersion": "10.0.22621.0",
                "compilerPath": "cl.exe",
                "cStandard": "c17",
                "cppStandard": "c++20",
                "intelliSenseMode": "windows-msvc-x64"
            }
        ],
        "version": 4
    }

    path = vscode_dir / 'c_cpp_properties.json'
    path.write_text(json.dumps(properties, indent=4))
    print(f"  Created: {path}")


def write_vscode_settings(project_path: Path):
    """Generate VS Code settings.json with helpful defaults."""
    vscode_dir = project_path / '.vscode'
    vscode_dir.mkdir(exist_ok=True)

    settings = {
        "files.associations": {
            "*.gdextension": "ini",
            "SConstruct": "python",
            "*.h": "cpp",
            "*.hpp": "cpp"
        },
        "C_Cpp.default.cppStandard": "c++20",
        "editor.formatOnSave": True
    }

    path = vscode_dir / 'settings.json'
    # Only create if doesn't exist to not overwrite user settings
    if not path.exists():
        path.write_text(json.dumps(settings, indent=4))
        print(f"  Created: {path}")
    else:
        print(f"  Skipped: {path} (already exists)")


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
for root, dirs, files in os.walk(cortex_src_dir):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.c'):
            sources.append(os.path.join(root, f))

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
    python tools/cortex.py --godot-path C:/Workspace/Godot/engine

What this tool does:
    1. Creates src/ folder with starter C++ files
    2. Creates VS Code workspace (.vscode/) with:
       - tasks.json      (build debug/release DLL)
       - launch.json     (launch Godot editor, attach debugger)
       - c_cpp_properties.json (IntelliSense)
    3. Adds .gdextension files so Godot loads the extensions
    4. Sets up SConstruct for building with SCons
        '''
    )

    parser.add_argument('project_path', nargs='?', help='Path to Godot project')
    parser.add_argument('--godot-path', '-g', help='Path to Godot engine source (for debugging)')

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

    # Find Godot engine path
    if args.godot_path:
        godot_engine_path = Path(args.godot_path).absolute()
    else:
        print("Searching for Godot engine...")
        godot_engine_path = find_godot_engine()

    godot_exe = None
    if godot_engine_path:
        godot_exe = find_godot_executable(godot_engine_path)
        if godot_exe:
            print(f"  Found Godot: {godot_exe}")
        else:
            print(f"  Godot engine found at: {godot_engine_path}")
            print("  WARNING: No editor executable found in bin/")
            print("  You may need to build Godot first: scons target=editor dev_build=yes")
    else:
        print("  WARNING: Godot engine not found.")
        print("  Use --godot-path to specify the engine location.")
        godot_path_input = input("  Enter Godot engine path (or press Enter to skip): ").strip()
        if godot_path_input:
            godot_engine_path = Path(godot_path_input).absolute()
            godot_exe = find_godot_executable(godot_engine_path)

    print()
    print("-" * 60)
    print(f"  Project:  {project_path}")
    print(f"  Name:     {project_name}")
    if godot_exe:
        print(f"  Godot:    {godot_exe}")
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
    write_gdextension(project_path, project_name)

    # Generate VS Code configuration
    print()
    print("Creating VS Code workspace...")
    write_vscode_tasks(project_path, project_name)
    write_vscode_cpp_properties(project_path, project_name, cortex_path)
    write_vscode_settings(project_path)

    if godot_exe:
        write_vscode_launch(project_path, project_name, godot_exe)
    else:
        print("  Skipped: launch.json (no Godot executable found)")
        print("  Run again with --godot-path after building Godot engine")

    print()
    print("=" * 60)
    print("  SUCCESS!")
    print("=" * 60)

    launch_note = ""
    if godot_exe:
        launch_note = f"""
  Debug configurations (F5 in VS Code):
    - Build & Launch Godot Editor  (builds DLL, launches editor)
    - Build Only (Hot Reload)      (builds DLL, prints version)
    - Launch Godot Editor          (no build, just launch)
    - Launch Game                  (run game directly)
    - Attach to Godot              (attach to running process)
"""
    else:
        launch_note = """
  NOTE: launch.json was not created (Godot exe not found).
  Run the tool again with --godot-path after building the engine.
"""

    print(f'''
  Files created in {project_path}:

    .vscode/
        tasks.json              <- Build tasks (Ctrl+Shift+B)
        launch.json             <- Debug configurations (F5)
        c_cpp_properties.json   <- IntelliSense config
        settings.json           <- Editor settings
    SConstruct                  <- SCons build script
    src/
        register_types.cpp
        register_types.h
        start_button_context.h  <- Example context
    bin/                        <- DLLs go here after build
    extensions/
        {project_name}.gdextension
{launch_note}
  Next steps:

  1. Open folder in VS Code:
     code "{project_path}"

  2. Build the extension:
     - Press Ctrl+Shift+B and select "build_debug"
     - Or run: scons platform=windows target=template_debug

  3. Debug your extension:
     - Set breakpoints in your C++ code
     - Press F5 to build and launch Godot with debugger attached
     - Or use "Attach to Godot" to attach to a running instance

  4. Open project in Godot:
     - The extension loads automatically from extensions/ folder
     - Your custom classes appear in the editor
''')


if __name__ == '__main__':
    main()
