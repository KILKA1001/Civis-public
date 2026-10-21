// Actual ChildContext-owned partial refs survive Lua protected error/OOM.
// Attempt/publication fields are not caller-set readiness or fullsetup receipts.
bool setup_state_name_attempted=false;
int setup_state_name_table_ref=LUA_NOREF,setup_state_name_key_ref=LUA_NOREF;
std::weak_ptr<const ui::detail::NotificationContextLuaStateNamePrefixReturn> setup_state_name_return;
#include "lua_native_context_debug_config_fields.inc"
