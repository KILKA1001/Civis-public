#pragma once
#include <cstdint>

struct lua_State;

namespace civ5::runtime {
// Explicit Civis semantics: count immediate raw keys, including sparse,
// nonnumeric and false-valued entries. No native Civ5 ABI/parity claim.
// Accepts an ordinary positive or negative Lua stack index; leaves the stack
// unchanged. Throws for a null state, invalid index or non-table value.
// This helper installs no capability. Its caller owns VM/thread/lifetime guards
// and must translate exceptions at the protected Lua host boundary.
std::int64_t count_civis_raw_table_keys(lua_State *state,int stack_index);
}
