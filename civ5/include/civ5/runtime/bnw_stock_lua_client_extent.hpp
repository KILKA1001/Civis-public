#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
namespace civ5::runtime {
struct BnwStockLuaClientExtent {
    std::string surface_owner;
    std::uint64_t revision=0;
    std::int64_t width=0,height=0;
};
// Reads this Civis client's current pixel extent, never saved original UI sizes.
// Unavailable/minimized/unstable frames return nullopt and keep Lua suspended.
struct BnwStockLuaClientExtentProvider {
    std::function<std::optional<BnwStockLuaClientExtent>()> current;
};
// Reads GetClientRect only on a window owned by THIS process. Caller owns the
// HWND lifetime and extent revision. This does not attach to any other game.
BnwStockLuaClientExtentProvider bnw_stock_lua_win32_client_extent(
    std::uintptr_t owned_window,std::string surface_owner,
    std::function<std::uint64_t()> revision);
}
