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
godot_cpp_path = os.environ.get('GODOT_CPP_PATH', 'godot-cpp')

# Add godot-cpp to the build environment
env = SConscript(os.path.join(godot_cpp_path, 'SConstruct'))

# Project configuration
project_name = 'cortex'
src_dir = 'src'
flecs_dir = 'flecs'

# C++ and C source files (recursive)
env.Append(CPPPATH=[src_dir])
sources = []
for root, dirs, files in os.walk(src_dir):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.c'):
            sources.append(os.path.join(root, f))

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

sources.extend(flecs_sources)

# Flecs configuration - build as static library embedded in our DLL
env.Append(CPPDEFINES=['flecs_STATIC'])

# Clay UI library requires C++20 for designated initializers
if env['platform'] == 'windows':
    env.Append(CXXFLAGS=['/std:c++20'])
else:
    env.Append(CXXFLAGS=['-std=c++20'])

# Output directory - build to lib/ for game projects to link against
lib_output_dir = 'lib'
demo_output_dir = os.path.join('demo', 'bin')

# Platform-specific library name and extension
if env['platform'] == 'windows':
    lib_suffix = '.dll'
elif env['platform'] == 'macos':
    lib_suffix = '.dylib'
else:
    lib_suffix = '.so'

# Map SCons target to simple name (template_debug -> debug)
target_name = env['target'].replace('template_', '')

# Library naming convention
# e.g., libcortex.windows.debug.x86_64.dll
library_name = 'lib{}.{}.{}.{}{}'.format(
    project_name,
    env['platform'],
    target_name,
    env['arch'],
    lib_suffix
)

# Build to lib/ directory
lib_output_path = os.path.join(lib_output_dir, library_name)

# Build the shared library
library = env.SharedLibrary(
    target=lib_output_path,
    source=sources
)

Default(library)

# Also copy to demo/bin for standalone testing
def copy_to_demo(target, source, env):
    import shutil
    os.makedirs(demo_output_dir, exist_ok=True)
    for src in source:
        src_path = str(src)
        if not os.path.exists(src_path):
            continue
        filename = os.path.basename(src_path)
        dst_path = os.path.join(demo_output_dir, filename)
        print(f"Copying {src_path} -> {dst_path}")
        shutil.copy2(src_path, dst_path)
        # Also copy PDB if it exists (Windows debug symbols)
        pdb_path = src_path.replace('.dll', '.pdb')
        if os.path.exists(pdb_path):
            pdb_dst = os.path.join(demo_output_dir, os.path.basename(pdb_path))
            shutil.copy2(pdb_path, pdb_dst)

copy_demo = env.Command('copy_to_demo', library, copy_to_demo)
Default(copy_demo)

# Export variables for game projects that import this SConstruct
Export('env', 'project_name', 'lib_output_dir', 'library_name')

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

Output:
    lib/                Contains the built Cortex library (for linking)
    demo/bin/           Contains a copy for standalone demo testing

For game projects, use: python tools/cortex.py
""")
