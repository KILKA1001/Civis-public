# Building Civis

## Requirements

- Windows x64; 32-bit targets are rejected during configuration.
- CMake 3.24 or newer and Ninja.
- An x64 C/C++20 toolchain. Development uses LLVM-MinGW with strict
  floating-point options. Other toolchains need their own validation.
- A graphics driver supporting the OpenGL features used by the renderer.

No original game installation is needed to configure and compile the source.
The source includes Lua, SQLite, zlib, and the LZMA decoder subset.

## Configure

Run in the repository root with compiler and Ninja binaries on PATH:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Use an x64 toolchain. Compiler paths and `CMAKE_MAKE_PROGRAM` may also be passed
as absolute paths. Generated files remain under `build/`.

When passing LLVM-MinGW compiler paths explicitly, keep its `bin/` directory on
PATH for compiler runtime DLLs and resource tools. Set `CMAKE_RC_COMPILER` to the
toolchain's `x86_64-w64-mingw32-windres.exe` when it is not found automatically.

## Targets

```powershell
# Small self-contained terrain and asset tests
cmake --build build --target civ5_terrain_tests civ5_asset_tests civ5_lean_tests
ctest --test-dir build -R '^(terrain\.height_addressing|assets\.)' --output-on-failure

# Windows front end
cmake --build build --target civis_frontend

# Save inspection CLI
cmake --build build --target civis_inspect
```

The front end is produced in `build/civ5/frontend/`. CLI tools are produced in
`build/civ5/runtime/`. Exact output subdirectories can vary with the generator.

## Content-dependent validation

Some tests require selected original content or locally generated reference
fixtures. Those files are deliberately absent from the source distribution.
See [content configuration](CONTENT.md). Do not interpret missing private
fixtures as passed compatibility tests.

The optional `CIVIS_WARRIOR_CLIP_FIXTURE` and `CIVIS_ICE_WEIGHT_FIXTURE` CMake
cache entries accept explicitly supplied local captures. Their tests register
only when the supplied file exists.
