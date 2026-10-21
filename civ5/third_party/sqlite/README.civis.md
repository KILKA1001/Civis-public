# SQLite 3.53.4

Unmodified `sqlite3.c` and `sqlite3.h` from the official amalgamation:
https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip

Archive SHA3-256 (verified against https://www.sqlite.org/download.html):
`628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e`

Archive SHA-256:
`1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d`

SQLite is public domain; the upstream notice is preserved in both source files.
See https://www.sqlite.org/copyright.html .

Civis compiles this source as a private static x64 library. No dependency on
the original game's x86 DLL or the system SQLite version. Runtime byte counts,
row counters, bound integers, and memory sizes use 64-bit types. SQLite's limits
on a single SQL statement/row are not game world or process address-space limits.
Extension loading is omitted. The in-memory rules database denies ATTACH,
DETACH, PRAGMA and caller transaction control; SQL file behavior outside that
boundary is reported as unsupported, not silently ignored.
