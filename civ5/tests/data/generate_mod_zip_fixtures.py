"""Regenerate the small, authored ZIP fixtures used by mod acquisition tests."""
from pathlib import Path
from struct import pack
from zipfile import ZIP_DEFLATED, ZIP_STORED, ZipFile, ZipInfo
import zlib


ROOT = Path(__file__).resolve().parent


def member(name: str, method: int) -> ZipInfo:
    info = ZipInfo(name, (1980, 1, 1, 0, 0, 0))
    info.compress_type = method
    info.create_system = 0
    info.external_attr = (0x10 if name.endswith("/") else 0x20) << 16
    return info


with ZipFile(ROOT / "mod-archive-mixed.zip", "w") as output:
    output.writestr(
        member("Fixture.modinfo", ZIP_DEFLATED),
        '<Mod id="civis.zip.fixture" version="9"><Files><File import="1">'
        "data/rows.sql</File></Files></Mod>",
    )
    output.writestr(member("data/", ZIP_STORED), b"")
    output.writestr(member("data/rows.sql", ZIP_STORED), b"SELECT 1;\n")


# The authored manifest can lag behind its own archive. The SQL bytes remain
# part of a pinned archive and are recorded separately in the cache receipt.
with ZipFile(ROOT / "mod-archive-stale-md5.zip", "w") as output:
    output.writestr(
        member("Fixture.modinfo", ZIP_DEFLATED),
        '<Mod id="civis.stale.fixture" version="11"><Files><File md5="'
        '00000000000000000000000000000000" import="1">rows.sql</File>'
        '</Files><Actions><OnModActivated><UpdateDatabase>rows.sql'
        '</UpdateDatabase></OnModActivated></Actions></Mod>',
    )
    output.writestr(member("rows.sql", ZIP_STORED), b"SELECT 2;\n")


# A pinned author archive can omit a declared visual asset, but cannot omit
# executable SQL/XML/Lua/native content under the same exception.
with ZipFile(ROOT / "mod-archive-missing-asset.zip", "w") as output:
    output.writestr(
        member("Fixture.modinfo", ZIP_DEFLATED),
        '<Mod id="civis.missing.asset.fixture" version="12"><Files>'
        '<File md5="00000000000000000000000000000000" import="1">'
        'art/unused.dds</File>'
        '<File import="1">art/portrait.dds</File>'
        '<File import="1">rows.sql</File>'
        '</Files><Actions><OnModActivated><UpdateDatabase>rows.sql'
        '</UpdateDatabase></OnModActivated></Actions></Mod>',
    )
    output.writestr(member("art/", ZIP_STORED), b"")
    output.writestr(member("art/portrait.dds", ZIP_STORED), b"PICTURE")
    output.writestr(member("rows.sql", ZIP_STORED), b"SELECT 4;\n")

with ZipFile(ROOT / "mod-archive-missing-sql.zip", "w") as output:
    output.writestr(
        member("Fixture.modinfo", ZIP_DEFLATED),
        '<Mod id="civis.missing.sql.fixture" version="13"><Files>'
        '<File import="0">rows.sql</File></Files>'
        '<Actions><OnModActivated><UpdateDatabase>rows.sql'
        '</UpdateDatabase></OnModActivated></Actions></Mod>',
    )

with ZipFile(ROOT / "mod-archive-duplicate-declared.zip", "w") as output:
    output.writestr(
        member("Fixture.modinfo", ZIP_DEFLATED),
        '<Mod id="civis.duplicate.declared.fixture" version="14"><Files>'
        '<File md5="00000000000000000000000000000000" import="1">rows.sql</File>'
        '<File md5="00000000000000000000000000000000" import="1">rows.sql</File>'
        '</Files><Actions><OnModActivated><UpdateDatabase>rows.sql'
        '</UpdateDatabase></OnModActivated></Actions></Mod>',
    )
    output.writestr(member("rows.sql", ZIP_STORED), b"SELECT 5;\n")


with ZipFile(ROOT / "mod-archive-duplicate.zip", "w") as output:
    output.writestr(member("A.lua", ZIP_STORED), b"a")
    output.writestr(member("a.LUA", ZIP_STORED), b"b")


with ZipFile(ROOT / "mod-archive-symlink.zip", "w") as output:
    link = member("link.lua", ZIP_STORED)
    link.create_system = 3
    link.external_attr = 0o120777 << 16
    output.writestr(link, b"outside")


# A tiny ZIP64 archive with all three central fields and both end records in
# their 64-bit forms. The wire format is made explicitly to avoid a multi-GB
# fixture while testing the large-offset and large-count parser branches.
name = b"Fixture.modinfo"
payload = b'<Mod id="civis.zip64.fixture" version="4294967297"><Files/></Mod>'
compressor = zlib.compressobj(wbits=-15)
packed = compressor.compress(payload) + compressor.flush()
checksum = zlib.crc32(payload)
local_extra = pack("<HHQQ", 1, 16, len(payload), len(packed))
local = (
    pack(
        "<IHHHHHIIIHH", 0x04034B50, 45, 0x800, 8, 0, 0, checksum,
        0xFFFFFFFF, 0xFFFFFFFF, len(name), len(local_extra),
    )
    + name + local_extra + packed
)
central_extra = pack("<HHQQQ", 1, 24, len(payload), len(packed), 0)
central = (
    pack(
        "<IHHHHHHIIIHHHHHII", 0x02014B50, (3 << 8) | 45, 45, 0x800,
        8, 0, 0, checksum, 0xFFFFFFFF, 0xFFFFFFFF, len(name),
        len(central_extra), 0, 0, 0, 0o100644 << 16, 0xFFFFFFFF,
    )
    + name + central_extra
)
zip64_end = pack(
    "<IQHHIIQQQQ", 0x06064B50, 44, 45, 45, 0, 0, 1, 1,
    len(central), len(local),
)
locator = pack("<IIQI", 0x07064B50, 0, len(local) + len(central), 1)
end = pack(
    "<IHHHHIIH", 0x06054B50, 0, 0, 0xFFFF, 0xFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0,
)
(ROOT / "mod-archive-zip64.zip").write_bytes(local + central + zip64_end + locator + end)
