#!/usr/bin/env python3
"""
Generate wrapper .c files for Lexbor sources to avoid object file name collisions.
Each wrapper simply #includes the original source file.
"""

import os
import sys
from pathlib import Path

def main():
    script_dir = Path(__file__).parent
    lexbor_root = script_dir / "../../../../external/lexbor"
    lexbor_src = lexbor_root / "source"
    output_dir = script_dir / "generated"
    
    # Ensure output directory exists
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Find all .c files excluding windows_nt port
    sources = []
    for path in lexbor_src.rglob("lexbor/**/*.c"):
        rel = path.relative_to(lexbor_src)
        if "ports/windows_nt" in str(rel):
            continue
        sources.append((path, rel))
    
    # Generate wrapper files with unique names based on path
    wrapper_files = []
    for abs_path, rel_path in sources:
        # Create unique name: lexbor_css_state.c instead of state.c
        unique_name = str(rel_path).replace("/", "_").replace("\\", "_")
        wrapper_path = output_dir / unique_name
        
        # Write wrapper that includes the original using relative path from generated/ dir
        # Path from generated/ to external/lexbor/source/
        rel_include = os.path.relpath(abs_path, output_dir)
        with open(wrapper_path, "w") as f:
            f.write(f'/* Auto-generated wrapper for {rel_path} */\n')
            f.write(f'#include "{rel_include}"\n')
        
        wrapper_files.append(wrapper_path.name)
    
    # Write CMakeLists.txt fragment listing all sources
    cmake_path = output_dir / "sources.cmake"
    with open(cmake_path, "w") as f:
        f.write("# Auto-generated list of Lexbor wrapper sources\n")
        f.write("set(LEXBOR_GENERATED_SRCS\n")
        for name in sorted(wrapper_files):
            f.write(f"    ${{CMAKE_CURRENT_LIST_DIR}}/{name}\n")
        f.write(")\n")
    
    print(f"Generated {len(wrapper_files)} wrapper files in {output_dir}")
    print(f"Include {cmake_path} in your CMakeLists.txt")

if __name__ == "__main__":
    main()
