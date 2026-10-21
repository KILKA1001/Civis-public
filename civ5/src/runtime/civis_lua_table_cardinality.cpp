#include "civ5/runtime/civis_lua_table_cardinality.hpp"
#include <limits>
#include <stdexcept>
extern "C" {
#include "lua.h"
}

namespace civ5::runtime {
std::int64_t count_civis_raw_table_keys(lua_State *state,int stack_index) {
    if(!state)throw std::invalid_argument("Civis raw table count requires a Lua state");
    const auto top=lua_gettop(state);
    const auto index=stack_index>0?stack_index:top+static_cast<std::int64_t>(stack_index)+1;
    if(stack_index==0||index<1||index>top)
        throw std::invalid_argument("Civis raw table count requires an ordinary stack index");
    if(lua_type(state,static_cast<int>(index))!=LUA_TTABLE)
        throw std::invalid_argument("Civis raw table count requires a table");
    if(!lua_checkstack(state,2))
        throw std::runtime_error("Civis raw table count cannot reserve traversal stack");
    std::int64_t count=0;
    lua_pushnil(state);
    while(lua_next(state,static_cast<int>(index))) {
        lua_pop(state,1);
        if(count==std::numeric_limits<std::int64_t>::max()) {
            lua_settop(state,top);
            throw std::overflow_error("Civis raw table count exceeds signed cardinality");
        }
        ++count;
    }
    return count;
}
}
