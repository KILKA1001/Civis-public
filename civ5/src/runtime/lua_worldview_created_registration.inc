// SAME Impl storage exists before any registry allocation can longjmp.
struct WorldViewCreatedRegistration {
 std::shared_ptr<const ScriptUiChildSource>source;
 std::weak_ptr<ScriptUiControlGraph>graph;
 std::size_t graph_slot=graph_no_owner,child=graph_no_owner,compiled=graph_no_owner;
 const Proto*root=nullptr;const void*closure=nullptr;const void*environment_identity=nullptr;
 const void*class_identity=nullptr,*new_identity=nullptr,*initialize_identity=nullptr,*master_identity=nullptr;
 int function=LUA_NOREF,thread=LUA_NOREF,environment=LUA_NOREF;
 int klass=LUA_NOREF,new_function=LUA_NOREF,initialize=LUA_NOREF,master=LUA_NOREF;
 lua_State*registered=nullptr;std::uint64_t epoch=0,uncertainty=0;
 std::array<int,3>constructor_managers{LUA_NOREF,LUA_NOREF,LUA_NOREF};
 std::array<const void*,3>constructor_manager_identities{};
 int constructor_manager_class=LUA_NOREF;const void*constructor_manager_class_identity=nullptr;
 std::array<int,2>constructor_manager_methods{LUA_NOREF,LUA_NOREF};
 std::array<const void*,2>constructor_manager_method_identities{};
 bool retired=false;
};
struct WorldViewCreatedReturnedInstance {
 std::size_t registration=graph_no_owner;std::int32_t owner=-1,full_id=-1;
 int instance=LUA_NOREF,controls=LUA_NOREF,cargo=LUA_NOREF;
 const void*instance_identity=nullptr,*controls_identity=nullptr,*cargo_identity=nullptr;
 std::array<std::uint64_t,6>health_ids{};
 std::optional<std::uint64_t>cargo_pull_down;
 std::size_t graph_instance=graph_no_owner,cargo_graph_instance=graph_no_owner;
 std::uint64_t instance_root=0,cargo_instance_root=0;
 bool original_returned=false,retired=false;
};
std::deque<WorldViewCreatedRegistration>worldview_created_registrations;
std::deque<WorldViewCreatedReturnedInstance>worldview_created_instances;
bool worldview_created_dispatching=false;
struct WorldViewCreatedJob;
WorldViewCreatedJob*worldview_unitflag_constructor_job=nullptr;
const graphics::BnwUnitSetupGraphicalQueue::CreatedOccurrenceLoan*worldview_created_pending_occurrence=nullptr;
