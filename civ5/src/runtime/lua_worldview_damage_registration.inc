// Persistent original listener storage precedes Lua allocations/longjmp.
struct WorldViewDamageRegistration {
 std::size_t created=graph_no_owner;
 int function=LUA_NOREF,thread=LUA_NOREF,update_health=LUA_NOREF;
 const void*closure=nullptr,*health_closure=nullptr;
 lua_State*registered=nullptr;
 std::uint64_t epoch=0,uncertainty=0;
 bool retired=false;
};
std::deque<WorldViewDamageRegistration>worldview_damage_registrations;
bool worldview_damage_dispatching=false;
