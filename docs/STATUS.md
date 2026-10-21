# Scope and current limitations

Civis is in active development. This repository contains the current native C++
implementation.

| Area | Scope present in source |
| --- | --- |
| Assets | Containers, texture readers and resource selection |
| Saves | Parsing, inspection and typed state structures |
| Runtime | Rules data, Lua host and selected compatibility paths |
| Rendering | Terrain, materials, water, fog and UI integration |
| Front end | Windows source selection, menus and scene startup |
| Tests | Unit tests, synthetic archive inputs and optional integration paths |

Complete saved-game continuation, full turn progression, arbitrary mod support,
and complete rendering equivalence remain unfinished. Individual implemented
paths do not establish these larger capabilities.

Content-dependent validation needs explicitly provided local inputs.
