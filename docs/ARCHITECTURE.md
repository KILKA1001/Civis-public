# Source layout

| Path | Responsibility |
| --- | --- |
| `civ5/include/civ5/` | Native module interfaces |
| `civ5/src/assets/` | Read-only asset containers and resource sources |
| `civ5/src/content/` | Content selection and rules data |
| `civ5/src/saves/` | Save formats and state import |
| `civ5/src/runtime/` | Host services and compatibility implementations |
| `civ5/src/terrain/` | Terrain geometry and world data |
| `civ5/src/materials/` | Material preparation and cache management |
| `civ5/src/graphics/` | Native graphics resources and rendering |
| `civ5/src/ui/` | UI presentation, input and front-end behavior |
| `civ5/runtime/` | Command-line entry points and runtime build definitions |
| `civ5/frontend/` | Windows front-end build target |
| `civ5/validation/` | Validation programs and rendering code shared with the viewer |
| `civ5/tests/` | Unit and integration test source |
| `civ5/third_party/` | Vendored libraries and their notices |
| `civ5/config/` | Runtime source metadata and external decoder identity pins |
| `shaders/` | GLSL source used by UI rendering |

Public native interfaces live under `include/civ5`. Assets are supplied to the
engine through selected resource sources rather than bundled with this repository.
Save parsing, rules setup, and scene presentation are separate modules; success
in one module does not imply that a complete game session can continue.
