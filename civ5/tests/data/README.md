# Mod archive fixtures

`mod-archive-store.7z` and `mod-archive-solid.7z` contain only project-created
synthetic data: `abcdef.txt` = `hello\n`, `ghijkl.txt` = `world\n`, and a small
`Fixture.modinfo` declaring `civis.archive.fixture` version 7 with both MD5s.
No original game or third-party mod content is included.
`mod-archive-stale-md5.zip` is another synthetic fixture: its declared SQL
MD5 is deliberately stale, exercising explicit pinned-archive exceptions and
cache receipt verification without bundling any third-party mod content.

Created independently of the runtime using the official LZMA SDK 26.03 x64
`7zr.exe`: `a -t7z -m0=Copy -mhc=off -mtc=off -mtm=off -mta=off` and
`a -t7z -m0=LZMA2 -ms=on -mtc=off -mtm=off -mta=off`, respectively.
`mod-archive-deflate.7z` contains a 1,500,000-byte synthetic `rows.sql`:
`SELECT 1;\n` repeated 150,000 times. It was created with the official 7-Zip
26.03 x64 `7za.exe`: `a -t7z -m0=Deflate -ms=off`. Its small compressed size
exercises 7z Deflate output in more than one decoder chunk.
`mod-archive-folder.zip` wraps that 7z file with a root `/` entry, as some
shared-folder downloads do. Python's `zipfile` created it with stored entries.
`mod-archive-multipackage.zip` contains two synthetic mod roots copied from
`mod-archive-store.7z`; the second manifest has a different ID. It verifies
that selecting one package does not publish the other.
`mod-archive-multipackage-invalid-neighbor.zip` adds a malformed manifest in
the unrequested root. Explicit root selection still installs the requested
package; whole-archive validation rejects the same ZIP.
The tests repair header CRCs with an independent bitwise implementation when
mutating paths, so the traversal/alias cases reach the actual path checks.
