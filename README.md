# Civis

An experimental 64-bit C++ engine and tools for Civilization V compatibility.

Civis is being developed around native resource loading, save inspection,
rules and Lua integration, terrain rendering, and a Windows front end.
Full game continuation and broad mod compatibility remain in development.

## Current source snapshot

- C++20 libraries for terrain, materials, asset containers, content, and saves.
- A Windows x64 front end and command-line inspection tools.
- Unit tests with project-created synthetic archive fixtures.
- Vendored Lua, SQLite, zlib, and LZMA decoder source with upstream notices.

## Build

The current target platform is Windows x64. You need CMake 3.24 or newer,
Ninja, and a compatible x64 C/C++ toolchain. See [build instructions](docs/BUILD.md)
for configuration, individual targets, and tests.

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target civ5_terrain_tests civ5_asset_tests civ5_lean_tests
ctest --test-dir build -R '^(terrain\.height_addressing|assets\.)' --output-on-failure
```

## Game content

Original game assets, saves, executables, and third-party mod packages are not
distributed here. Content-dependent tools require the user's own legally
obtained installation and explicitly selected inputs. See
[content configuration](docs/CONTENT.md).

## Project status

See [current scope and limitations](docs/STATUS.md). Build and test results apply
to the targets and inputs actually exercised; they do not establish complete
gameplay compatibility. The [source layout](docs/ARCHITECTURE.md) describes the
main modules.

## License and contributions

Project-owned source is provided under [MPL-2.0](LICENSE). Third-party components
retain their own terms; see [third-party notices](THIRD_PARTY_NOTICES.md).
The project license grants no rights to external game content.

Read [contribution guidelines](CONTRIBUTING.md) before submitting changes.
