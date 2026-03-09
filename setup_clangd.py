import os
Import("env")

# Include toolchain paths in the compilation database
env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=False)

# Optional: Override the path to ensure it's in the root
env.Replace(COMPILATIONDB_PATH="compile_commands.json")