# zlib 1.3.2: unmodified inflate source subset

Upstream: https://www.zlib.net/ (Jean-loup Gailly and Mark Adler).
License: [LICENSE](LICENSE).

Downloaded from https://www.zlib.net/zlib132.zip on 2026-09-15.
Archive SHA-256, verified against the official release page:
`e8bf55f3017aa181690990cb58a994e77885da140609fc8f94abe9b65d2cae28`.

Only the source/header files needed for inflate and its checksums are included.
These upstream files are unchanged. Civis builds them as a private static C
library; no original-game DLL, Python, JVM, network or extraction is needed at
runtime. gzip file APIs and compression APIs are not linked.

The wrapper uses fixed-size input/output buffers and its own `uint64_t`
counters. zlib's Windows `uLong` totals are not used as file or state sizes.
