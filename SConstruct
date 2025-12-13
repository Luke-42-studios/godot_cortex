#!/usr/bin/env python
"""
SConstruct - Build script for CortexFramework

CortexFramework is a native C++ game framework for Godot using the CNS
(Context-Node-System) architecture, powered by Flecs ECS and Clay UI.

Build commands:
    scons                           # Build debug
    scons target=template_release   # Build release
    scons platform=windows          # Build for Windows
    scons platform=linux            # Build for Linux
    scons platform=macos            # Build for macOS
"""

import os
import sys

# Get the path to godot-cpp
# Can be customized via GODOT_CPP_PATH environment variable
godot_cpp_path = os.environ.get('GODOT_CPP_PATH', 'godot-cpp')

# Add godot-cpp to the build environment
env = SConscript(os.path.join(godot_cpp_path, 'SConstruct'))

# Project configuration
project_name = 'cortex'
src_dir = 'src'
flecs_dir = 'flecs'

# C++ and C source files
env.Append(CPPPATH=[src_dir])
sources = Glob(os.path.join(src_dir, '*.cpp'))
sources += Glob(os.path.join(src_dir, '*.c'))

# Flecs ECS library
flecs_include = os.path.join(flecs_dir, 'include')
flecs_src = os.path.join(flecs_dir, 'src')
env.Append(CPPPATH=[flecs_include])

# Collect Flecs C source files (recursive)
flecs_sources = []
for root, dirs, files in os.walk(flecs_src):
    for f in files:
        if f.endswith('.c'):
            flecs_sources.append(os.path.join(root, f))

# Add Flecs sources to build
sources.extend(flecs_sources)

# Flecs configuration - build as static library embedded in our DLL
env.Append(CPPDEFINES=['flecs_STATIC'])

# Clay UI library requires C++20 for designated initializers
if env['platform'] == 'windows':
    env.Append(CXXFLAGS=['/std:c++20'])
else:
    env.Append(CXXFLAGS=['-std=c++20'])

# Output directory
output_dir = os.path.join('demo', 'bin')

# Platform-specific library name and extension
if env['platform'] == 'windows':
    lib_suffix = '.dll'
elif env['platform'] == 'macos':
    lib_suffix = '.dylib'
else:
    lib_suffix = '.so'

# Determine the library name with platform and target
# e.g., libgdframework.windows.template_debug.x86_64.dll
library_name = 'lib{}.{}.{}.{}{}'.format(
    project_name,
    env['platform'],
    env['target'],
    env['arch'],
    lib_suffix
)

# For macOS, we create a framework structure
if env['platform'] == 'macos':
    output_path = os.path.join(output_dir, 'lib{}.{}.{}.framework'.format(
        project_name,
        env['platform'],
        env['target']
    ), library_name)
else:
    output_path = os.path.join(output_dir, library_name)

# Build the shared library
library = env.SharedLibrary(
    target=output_path,
    source=sources
)

Default(library)

# Copy to project folder after build
project_bin_dir = os.path.abspath(r'C:\Workspace\Godot\cpp-sample\bin')

def copy_to_project(target, source, env):
    import shutil
    for src in source:
        src_path = str(src)
        filename = os.path.basename(src_path)
        dst_path = os.path.join(project_bin_dir, filename)
        print(f"Copying {src_path} -> {dst_path}")
        shutil.copy2(src_path, dst_path)
        # Also copy PDB if it exists (Windows debug symbols)
        pdb_path = src_path.replace('.dll', '.pdb')
        if os.path.exists(pdb_path):
            pdb_dst = os.path.join(project_bin_dir, os.path.basename(pdb_path))
            print(f"Copying {pdb_path} -> {pdb_dst}")
            shutil.copy2(pdb_path, pdb_dst)

if os.path.isdir(project_bin_dir):
    copy_command = env.Command('copy_to_project', library, copy_to_project)
    Default(copy_command)

# Help text
Help("""
CortexFramework Build System
============================

Native C++ game framework for Godot using CNS architecture.

Targets:
    scons               Build the extension (debug by default)
    scons -c            Clean build files

Options:
    target=<target>     Build target: template_debug (default), template_release, editor
    platform=<platform> Target platform: windows, linux, macos
    arch=<arch>         Target architecture: x86_64, arm64
    use_mingw=yes       Use MinGW on Windows (default is MSVC)
    debug_symbols=yes   Include debug symbols

Examples:
    scons target=template_release platform=windows
    scons target=editor platform=linux
    scons platform=macos arch=arm64

Requirements:
    - Python 3.x
    - SCons
    - godot-cpp (in 'godot-cpp' subdirectory)
    - Platform-appropriate compiler (MSVC/MinGW for Windows, GCC/Clang for Linux/macOS)
""")
