# Local game content

The repository contains source and synthetic tests. The following inputs stay
outside Git:

- Civilization V executables, DLLs, archives, textures, models, fonts and audio.
- Original or captured shader binaries and diagnostic memory dumps.
- User saves, extracted content, third-party mod packages and package caches.

Supply your own installation and selected save to content-dependent tools.
Runtime command-line options are available from the selected executable's help.
Common CMake configuration entries are:

| Entry | Purpose |
| --- | --- |
| `CIV5_ASSETS_ROOT` | Local extracted asset tree for content-dependent tests |
| `CIV5_BNW_INSTALL_ROOT` | Original installed BNW content for integration tests |
| `CIV5_BNW_SELECTED_SAVE` | Explicitly selected BNW save |
| `CIV5_BNW_TEST_UI_LANGUAGE` | Installed language used for UI tests |
| `CIV5_BNW_REFERENCE_EXE` | Optional locally supplied reference executable |

Source configuration files under `civ5/config/` contain decoder identity pins
and mod source metadata. They are not packages of the referenced third-party
software or game content. External decoders and downloads retain their own
licenses and are not part of this source snapshot.

Use `scripts/check_public_tree.py` before committing to check the tracked file
list for excluded content. This is a file-policy check, not a review of the
rights or provenance of code contained in allowed source files.
