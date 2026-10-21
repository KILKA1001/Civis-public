# LZMA SDK 26.03

C decoder subset from the official Igor Pavlov release linked at
https://www.7-zip.org/sdk.html :
https://github.com/ip7z/7zip/releases/download/26.03/lzma2603.7z

Downloaded archive SHA-256:
`86c213f752520ab5325c310f50bef63ec344b56dd1c80b0246d06dc6cec953b2`

The upstream source files retain their public-domain notices. Only decoder
sources are compiled, statically for x64; no downloaded executable is shipped
or used by Civis. File offsets, buffer sizes and allocation accounting are
64-bit. The upstream archive-entry index is local to one archive, not a game
object ID. The Civis adapter validates paths and rejects links, Windows devices,
collisions and CRC errors before accepting a package. All extraction budgets
are explicit 64-bit values and may be configured by the caller.

Local change: `CpuArch.h` adds `CIVIS_LZMA_PORTABLE_BYTE_ACCESS` to select the
SDK's existing byte-wise helpers instead of unaligned integer dereferences.
CMake enables this in every build. UBSan found an unaligned `GetUi64` load at
7z header offset 12; the option fixes that access without disabling sanitizer
checks. All other vendored decoder sources remain unmodified.
