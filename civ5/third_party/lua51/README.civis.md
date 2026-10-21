# Lua 5.1.5, x64 source build

C sources and headers from https://www.lua.org/ftp/lua-5.1.5.tar.gz .
Archive SHA-256: `2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333`.
`upstream.sha256.json` pins each original file. Upstream MIT notice: COPYRIGHT.
Interpreter/compiler CLI files are omitted.

`civis-patches.json` pins the single patched source and its reviewable upstream
diff. `ltable.c` checks numeric key ranges before using the small array-index
optimization and subtracts unsigned values in `luaH_getnum`, avoiding both
out-of-range float-to-int conversion and INT_MIN - 1 overflow. Sparse keys above
UINT32_MAX remain full Lua numbers in the hash part, including during rehash
and traversal. This does not widen the VM's dense array allocation limit.

The installed Civ V SDK declares Lua 5.1.4. Civis uses the 5.1.5 maintenance
release, built into its own x64 executable, without lua51_Win32.lib or a game
DLL. Compile-time assertions require 64-bit pointers, lua_Integer and GC memory
accounting. `LUA_USELONGLONG` also selects 64-bit integer formatting on Windows,
where `long` remains 32-bit. Lua 5.1 binary64 arithmetic is retained for source compatibility.
The bridge accepts integers above UINT32_MAX and rejects values outside the
contiguous exact binary64 range instead of silently rounding world IDs.
Fully exact int64 scripting values remain future work. Lua's bytecode/compiler,
individual-table and C-stack limits are not claimed removed; they do not impose
an x86 process address-space limit or game roster limit.

The host initializes base/coroutine, table, string, math, debug and OS libraries
in its master VM. Its `os.clock` binding uses the project's wide Windows
process-clock implementation. Binding exposure and content loading are owned
by `src/runtime/lua_script_host.cpp`; compiling a standard library does not
establish complete game API or saved-session compatibility.
