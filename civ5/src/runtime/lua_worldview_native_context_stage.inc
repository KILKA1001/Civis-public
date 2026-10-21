std::weak_ptr<ui::CivisInputPolicyOwner> worldview_input_policy;bool worldview_input_policy_bound=false;
// Inside the actual retained LuaScriptHost::Impl. Native scopes are C++ callers
// outside protected Lua boundaries; no allocating Lua API can skip retirement.
std::weak_ptr<BnwSelectedWorldViewContextOwner> worldview_sources;
bool worldview_sources_bound=false;
std::vector<std::int64_t> worldview_mouse_events;
static int worldview_grid_to_world(lua_State* L){
 auto& p=owner(L);
 if(lua_gettop(L)!=2||lua_type(L,1)!=LUA_TNUMBER||lua_type(L,2)!=LUA_TNUMBER)
  return luaL_error(L,"GridToWorld requires actual two numeric grid coordinates");
 const double x=lua_tonumber(L,1),y=lua_tonumber(L,2);
 if(!std::isfinite(x)||!std::isfinite(y)||std::trunc(x)!=x||std::trunc(y)!=y||
  x<std::numeric_limits<std::int32_t>::min()||x>std::numeric_limits<std::int32_t>::max()||
  y<std::numeric_limits<std::int32_t>::min()||y>std::numeric_limits<std::int32_t>::max())
  return luaL_error(L,"GridToWorld original grid operands are outside int32");
 lua_Debug caller{};std::size_t compiled=graph_no_owner;int closure=0;
 for(int level=1;lua_getstack(L,level,&caller);++level){
  if(!lua_getinfo(L,"f",&caller))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=p.graph_compiled_owner(L,-1);closure=lua_gettop(L);break;
 }
 if(compiled==graph_no_owner){p.record_failure();return luaL_error(L,"GridToWorld caller lacks actual selected compiled source");}
 std::size_t child=0;
 for(;child<p.child_contexts.size();++child)if(p.child_contexts[child].name==p.graph_compiled_sources[compiled].context)break;
 if(child==p.child_contexts.size()||p.child_contexts[child].environment==LUA_NOREF){p.record_failure();return luaL_error(L,"GridToWorld caller has no actual retained child");}
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[child].environment);
 const bool same_env=lua_rawequal(L,-1,-2);lua_pop(L,3);
 if(!same_env){p.record_failure();return luaL_error(L,"GridToWorld compiled closure environment changed");}
 std::array<double,3> result{};p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();
  if(!p.worldview_native_child_controls_current(child)||!p.selected_loading_context_matches(p.graph_compiled_sources[compiled].context))
   throw ScriptError("GridToWorld actual selected native child lineage changed");
  const auto source=p.worldview_sources.lock();
  if(!source)throw ScriptError("GridToWorld actual source retired");
  const auto& loan=source->current_operation_loan();
  if(&loan!=p.worldview_operation)throw ScriptError("GridToWorld genuine native issuer changed");
  result=loan.grid_to_world_owned(static_cast<std::int32_t>(x),static_cast<std::int32_t>(y));
  p.worldview_require_callback_origin();
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="GridToWorld actual native producer refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 // FLuaVector.Vector3/Vector4 consume Original's multiple numeric returns.
 lua_pushnumber(L,result[0]);lua_pushnumber(L,result[1]);lua_pushnumber(L,result[2]);return 3;
}
static int worldview_input_policy_call(lua_State* L){
 auto& p=owner(L);const auto op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(1)));
 if(lua_gettop(L)!=(op==2?1:0))return luaL_error(L,"WorldView UI input policy argument count differs");
 std::int32_t requested=0;
 if(op==2){
  if(lua_type(L,1)!=LUA_TNUMBER)return luaL_error(L,"WorldView interface mode requires exact numeric operand");
  const auto number=lua_tonumber(L,1);
  if(!std::isfinite(number)||std::trunc(number)!=number||number<std::numeric_limits<std::int32_t>::min()||number>std::numeric_limits<std::int32_t>::max())
   return luaL_error(L,"WorldView interface mode operand outside int32");
  requested=static_cast<std::int32_t>(number);
 }
 lua_Debug caller{};std::size_t compiled=graph_no_owner;int closure=0;
 for(int level=1;lua_getstack(L,level,&caller);++level){
  if(!lua_getinfo(L,"f",&caller))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=p.graph_compiled_owner(L,-1);closure=lua_gettop(L);break;
 }
 if(compiled==graph_no_owner){p.record_failure();return luaL_error(L,"WorldView input policy first Lua caller has no authentic compiled source");}
 std::size_t child=0;
 for(;child<p.child_contexts.size();++child)if(p.child_contexts[child].name==p.graph_compiled_sources[compiled].context)break;
 if(child==p.child_contexts.size()||p.child_contexts[child].environment==LUA_NOREF){p.record_failure();return luaL_error(L,"WorldView input policy child environment unavailable");}
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[child].environment);
 const bool environment_matches=lua_rawequal(L,-1,-2);lua_pop(L,3);
 if(!environment_matches){p.record_failure();return luaL_error(L,"WorldView input policy actual closure environment changed");}
 bool touch=false;std::int32_t mode=0;p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();
  if(!p.worldview_native_child_controls_current(child)||!p.selected_loading_context_matches(p.graph_compiled_sources[compiled].context))
   throw ScriptError("WorldView input policy selected child source changed");
  const auto source=p.worldview_sources.lock();
  if(!source)throw ScriptError("WorldView input policy actual source retired");
  {
  const auto policy=p.worldview_input_policy.lock();
  if(!policy)throw ScriptError("WorldView input policy actual owner retired");
  const auto& active=source->current_operation_loan();
  if(&active!=p.worldview_operation)throw ScriptError("WorldView input policy active native loan changed");
  active.require_same_vm(*p.worldview_host_owner);active.require_same_source(*p.child_contexts[child].source);
  if(op==0)touch=policy->is_touch_enabled(active);
  else if(op==1)mode=policy->interface_mode(active);
  else if(op==2){const auto change=policy->set_interface_mode_owned(active,requested);mode=change.current;}
  else throw ScriptError("WorldView input policy method unsupported");
  p.worldview_require_callback_origin();policy->require_for_source(active);p.worldview_require_callback_origin();
  } // Drop the operation policy borrower before the final outside-lifetime fence.
  p.worldview_require_callback_origin();
  if(p.worldview_input_policy.expired())throw ScriptError("WorldView input controller retired before Lua return");
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual input policy failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(op==0){lua_pushboolean(L,touch);return 1;}if(op==1){lua_pushinteger(L,mode);return 1;}return 0;
}
// Persistent returns are issued by actual namespace publications, never by
// an installer accepting a table because its values happen to match.
class NativeWorldViewNamespaceReturn final {
 friend struct Impl;
 enum class Kind {ui_enum,locale,configured_enum,interface_enum,readonly_api,content_enum,info_types};
 enum class Stage {captured,returned};Stage stage=Stage::captured;
 const Impl*const host;const ScriptSourceProvider*const host_source;
 const std::thread::id creator;const std::uint64_t epoch,writes,uncertainty;
 lua_State*const vm;global_State*const global;const Table*const registry_table;
 const std::size_t child;const int environment_ref;const void*const environment;
 const std::weak_ptr<const ScriptUiChildSource>source;const ScriptUiChildSource*const source_get;
 const Kind kind;const std::string name;
 const int master_globals_ref;const void*const master_globals;
 const int master_ref;const void*const master;const void*const metadata;
 int namespace_ref=LUA_NOREF;const void*name_space=nullptr;const void*master_metadata=nullptr;
 std::size_t configured_ordinal=0;
 NativeWorldViewNamespaceReturn(Impl&p,Kind k,const char*key,std::size_t slot,const void*actual_environment,int parent_ref,
  const void*parent,const void*meta):host(&p),host_source(&p.sources),creator(std::this_thread::get_id()),
  epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),vm(p.state),global(G(p.state)),
  registry_table(hvalue(&G(p.state)->l_registry)),child(slot),environment_ref(p.child_contexts.at(slot).environment),
  environment(actual_environment),source(p.child_contexts.at(slot).source),source_get(p.child_contexts.at(slot).source.get()),
  kind(k),name(key),master_globals_ref(p.native_master_thread_bootstrap->globals_ref),
  master_globals(p.native_master_thread_bootstrap->globals),master_ref(parent_ref),master(parent),metadata(meta){}
};
std::vector<std::unique_ptr<NativeWorldViewNamespaceReturn>> worldview_namespace_returns;
static bool worldview_namespace_header_physical_owned(Impl&p,lua_State*L,const NativeWorldViewNamespaceReturn&r,bool selected){
 if(r.host!=&p||r.host_source!=&p.sources||r.creator!=std::this_thread::get_id()||r.creator!=p.ui_thread||
  r.epoch!=p.failure_epoch||r.writes!=p.failed.writes_||r.uncertainty!=p.execution_uncertainty||p.failed.exhausted_||
  r.stage!=NativeWorldViewNamespaceReturn::Stage::returned||!p.state||r.vm!=p.state||r.global!=G(L)||G(p.state)!=r.global||
  r.master_globals_ref<0||!r.master_globals||!ttistable(gt(p.state))||hvalue(gt(p.state))!=r.master_globals||
  !ttistable(&r.global->l_registry)||hvalue(&r.global->l_registry)!=r.registry_table||r.namespace_ref<0||!r.name_space)return false;
 if(!selected)return true; // Global ctor output stays owned across child retirement.
 if(r.child>=p.child_contexts.size()||r.source.expired())return false;
 const auto&child=p.child_contexts[r.child];
 return child.state_prepared&&!child.retired&&child.source&&child.source.get()==r.source_get&&
  !r.source.owner_before(child.source)&&!child.source.owner_before(r.source)&&child.environment==r.environment_ref;
}
static NativeWorldViewNamespaceReturn*worldview_namespace_begin_owned(lua_State*L,NativeWorldViewNamespaceReturn::Kind kind,
 const char*name,Job&job,int master_ref,const void*master,const void*meta){
 auto&p=owner(L);native_db_query_current(L);
 if(!p.worldview_native_host||p.worldview_native_host->child!=job.worldview_child||job.worldview_child>=p.child_contexts.size()||
  !p.worldview_native_child_controls_current(job.worldview_child)||!p.child_contexts[job.worldview_child].state_prepared||!job.context_state_environment)
  return (native_master_thread_refuse(L,"namespace real publisher lost its selected child/Source"),nullptr);
 p.worldview_namespace_returns.push_back(std::unique_ptr<NativeWorldViewNamespaceReturn>(
  new NativeWorldViewNamespaceReturn(p,kind,name,job.worldview_child,job.context_state_environment,master_ref,master,meta)));
 return p.worldview_namespace_returns.back().get();
}
static void worldview_namespace_retain_actual_value_owned(lua_State*L,NativeWorldViewNamespaceReturn&r,int offered){
 const int top=lua_gettop(L);if(offered<0)offered=top+offered+1;
 if(lua_type(L,offered)!=LUA_TTABLE)return (void)native_master_thread_refuse(L,"namespace real publisher output is not a table");
 r.name_space=lua_topointer(L,offered);lua_pushvalue(L,offered);r.namespace_ref=luaL_ref(L,LUA_REGISTRYINDEX);
 if(r.namespace_ref<0)return (void)native_master_thread_refuse(L,"namespace actual output retention failed");
}
static bool worldview_namespace_master_was_issued_owned(Impl&p,lua_State*L,NativeWorldViewNamespaceReturn::Kind kind,const void*master){
 const int top=lua_gettop(L);bool issued=false;
 for(const auto&r:p.worldview_namespace_returns)if(r&&r->kind==kind&&r->master==master&&
  worldview_namespace_header_physical_owned(p,L,*r,false)&&r->master_ref>=0){
  lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_ref);issued=lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==master;
  lua_settop(L,top);if(issued)break;
 }
 return issued;
}
static void worldview_capture_ui_enum_publications_owned(lua_State*L,Job&job){
 auto&p=owner(L);auto*f=p.ui_enums_frame;native_ui_enums_full(L,job);
 if(!f||p.ui_enums_frame!=f||f->actual_job!=&job||f->master_ref<0||!f->master||f->copied.empty())
  return (void)native_master_thread_refuse(L,"UIEnums persistent return requires its actual completed publisher");
 if((f->constructed_groups!=20||f->constructed_members!=387)&&
  !worldview_namespace_master_was_issued_owned(p,L,NativeWorldViewNamespaceReturn::Kind::ui_enum,f->master))
  return (void)native_master_thread_refuse(L,"UIEnums cache HIT requires its own previously completed constructor return");
 const int top=lua_gettop(L);
 for(const char*name:{"Mouse","KeyEvents","MouseEvents","Keys"}){
  native_ui_enums_full(L,job);f=p.ui_enums_frame;
  auto*r=worldview_namespace_begin_owned(L,NativeWorldViewNamespaceReturn::Kind::ui_enum,name,job,f->master_ref,f->master,nullptr);
  lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);lua_getfield(L,-1,name);
  worldview_namespace_retain_actual_value_owned(L,*r,-1);lua_settop(L,top);native_ui_enums_full(L,job);
  r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
 }
}
static void worldview_capture_locale_publication_owned(lua_State*L,Job&job){
 auto&p=owner(L);auto*f=p.locale_frame;native_locale_registration_full(L,job);
 if(!f||p.locale_frame!=f||f->actual_job!=&job||f->stores!=1||f->master_ref<0||f->proxy_ref<0||!f->master||!f->proxy||!f->meta)
  return (void)native_master_thread_refuse(L,"Locale persistent return requires its actual completed selected store");
 if(f->fields_created!=32&&!worldview_namespace_master_was_issued_owned(p,L,NativeWorldViewNamespaceReturn::Kind::locale,f->master))
  return (void)native_master_thread_refuse(L,"Locale cache HIT requires its own genuine completed constructor return");
 const int top=lua_gettop(L);
 auto*r=worldview_namespace_begin_owned(L,NativeWorldViewNamespaceReturn::Kind::locale,"Locale",job,f->master_ref,f->master,f->meta);
 lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);worldview_namespace_retain_actual_value_owned(L,*r,-1);lua_settop(L,top);
 native_locale_registration_full(L,job);r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
}
static bool worldview_namespace_physical_owned(lua_State*L,int environment,const char*name,NativeWorldViewNamespaceReturn::Kind kind){
 auto&p=owner(L);const int top=lua_gettop(L);if(environment<0)environment=top+environment+1;
 if(environment<1||environment>top||lua_type(L,environment)!=LUA_TTABLE)return false;
 const auto stop=[&](bool value){lua_settop(L,top);return value;};
 const NativeWorldViewNamespaceReturn*r=nullptr;
 // A latest actual publisher wins; historical receipts never authorize a
 // changed current environment or namespace table.
 for(auto i=p.worldview_namespace_returns.rbegin();i!=p.worldview_namespace_returns.rend();++i)
  if(*i&&(*i)->kind==kind&&(*i)->name==name&&worldview_namespace_header_physical_owned(p,L,**i,true)){
   lua_rawgeti(L,LUA_REGISTRYINDEX,(*i)->environment_ref);
   const bool matches=lua_type(L,-1)==LUA_TTABLE&&lua_topointer(L,-1)==(*i)->environment&&lua_rawequal(L,-1,environment);lua_pop(L,1);
   if(matches){r=i->get();break;}
  }
 if(!r||!p.worldview_native_child_controls_current(r->child))return stop(false);
 lua_getfield(L,environment,name);lua_rawgeti(L,LUA_REGISTRYINDEX,r->namespace_ref);
 if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r->name_space||!lua_rawequal(L,-1,-2))return stop(false);
 const int offered=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,r->environment_ref);
 if(lua_getmetatable(L,-1))return stop(false);lua_pop(L,1);
 if(kind==NativeWorldViewNamespaceReturn::Kind::locale||kind==NativeWorldViewNamespaceReturn::Kind::readonly_api){
  lua_pushnil(L);const bool empty=lua_next(L,offered)==0;lua_settop(L,offered);
  if(!empty)return stop(false);
  if(!lua_getmetatable(L,offered)||lua_topointer(L,-1)!=r->metadata)return stop(false);
  const int meta=lua_gettop(L);lua_getfield(L,meta,"__index");lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_ref);
  if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=r->master)return stop(false);lua_pop(L,2);
  lua_getfield(L,meta,"__metatable");if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return stop(false);lua_pop(L,1);
  lua_getfield(L,meta,"__newindex");
  if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,1);
  lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_ref);const int master=lua_gettop(L);
  lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_globals_ref);
  if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r->master_globals)return stop(false);
  lua_getfield(L,-1,r->name.c_str());lua_remove(L,-2);
  if(!lua_rawequal(L,-1,master))return stop(false);lua_pop(L,1);
  if(kind==NativeWorldViewNamespaceReturn::Kind::locale||r->name=="Modding"||r->name=="DB"||r->name=="ContentManager"){
   if(lua_getmetatable(L,master))return stop(false);
  }else {
   const bool has=lua_getmetatable(L,master)!=0;
   if(has?(lua_topointer(L,-1)!=r->master_metadata):(r->master_metadata!=nullptr))return stop(false);
   if(has)lua_pop(L,1);
  }
  if(kind==NativeWorldViewNamespaceReturn::Kind::locale){
   for(const auto&entry:native_locale_entries){lua_getfield(L,master,entry.name);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=entry.function||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,1);}
  }else if(r->name=="Modding"){
   for(const auto&entry:native_modding_entries){lua_getfield(L,master,entry.name);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=entry.function||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,1);}
  }else if(r->name=="ContentManager"){
   for(const auto&entry:native_content_entries){lua_getfield(L,master,entry.name);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=entry.function||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,1);}
  }else if(r->name=="DB"){
   const lua_CFunction functions[]={native_db_memory_usage,native_db_collect_memory_usage,native_db_statement_count,native_db_statement_sql,native_db_query,native_db_create_query};
   for(std::size_t i=0;i<6;++i){lua_getfield(L,master,native_db_keys[i+1]);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=functions[i]||lua_getupvalue(L,-1,1))return stop(false);lua_pop(L,1);}
  }
 }else {
  if(lua_getmetatable(L,offered))return stop(false);
  if(kind==NativeWorldViewNamespaceReturn::Kind::ui_enum){
   lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_ref);lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_globals_ref);
   if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r->master_globals)return stop(false);
   lua_getfield(L,-1,"UIEnumsLibrary");lua_remove(L,-2);
   if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=r->master)return stop(false);lua_pop(L,1);
   lua_getfield(L,-1,name);if(!lua_rawequal(L,-1,offered))return stop(false);lua_pop(L,2);
   for(const auto&group:native_ui_enum_groups)if(!std::strcmp(group.name,name))
    for(std::size_t i=0;i<group.count;++i){const auto&entry=native_ui_enum_entries[group.begin+i];lua_getfield(L,offered,entry.name);
     if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=static_cast<lua_Number>(entry.value))return stop(false);lua_pop(L,1);}
  }else if(kind==NativeWorldViewNamespaceReturn::Kind::configured_enum){
   if(r->configured_ordinal>=p.enums.tables.size()||p.enums.tables[r->configured_ordinal].name!=name)return stop(false);
   for(const auto&entry:p.enums.tables[r->configured_ordinal].members){lua_getfield(L,offered,entry.first.c_str());
    if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=static_cast<lua_Number>(entry.second))return stop(false);lua_pop(L,1);}
  }else if(kind==NativeWorldViewNamespaceReturn::Kind::content_enum){
   lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_ref);lua_rawgeti(L,LUA_REGISTRYINDEX,r->master_globals_ref);
   if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r->master_globals)return stop(false);
   lua_getfield(L,-1,"ContentManagerEnums");lua_remove(L,-2);
   if(!lua_rawequal(L,-1,-2)||lua_topointer(L,-1)!=r->master)return stop(false);lua_pop(L,1);
   lua_getfield(L,-1,"ContentType");if(!lua_rawequal(L,-1,offered))return stop(false);lua_pop(L,2);
   for(std::size_t i=0;i<3;++i){lua_getfield(L,offered,native_content_retained_keys[i+9]);
    if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=native_content_enum_values[i])return stop(false);lua_pop(L,1);}
  }else if(kind==NativeWorldViewNamespaceReturn::Kind::info_types){
   // Actual DB type-provider output is mutable Lua data. Its real publication
   // identity is retained; no constant guessed data is treated as a receipt.
  }else {
   for(const auto&entry:detail::bnw_worldview_original_input_enum_entries)if(!std::strcmp(entry.table,name)){
    lua_getfield(L,offered,entry.name);if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=static_cast<lua_Number>(entry.value))return stop(false);lua_pop(L,1);}
  }
 }
 return stop(true);
}
static bool worldview_namespace_current_owned(lua_State*L,int environment,const char*name,NativeWorldViewNamespaceReturn::Kind kind){
 if(!lua_checkstack(L,24))return false;native_db_query_current(L);worldview_require_lua_origin(L);
 if(!worldview_namespace_physical_owned(L,environment,name,kind))return false;
 native_db_query_current(L);worldview_require_lua_origin(L);
 return worldview_namespace_physical_owned(L,environment,name,kind);
}
static bool worldview_configured_enum_preserve_owned(lua_State*L,int environment,const char*name){
 auto&p=owner(L);if(!p.worldview_native_host)return false;
 lua_getfield(L,environment,name);const bool present=!lua_isnil(L,-1);lua_pop(L,1);
 if(!present)return false;
 if(worldview_namespace_current_owned(L,environment,name,NativeWorldViewNamespaceReturn::Kind::ui_enum)||
  worldview_namespace_current_owned(L,environment,name,NativeWorldViewNamespaceReturn::Kind::configured_enum))return true;
 return (native_master_thread_refuse(L,"GameCore enum configuration cannot replace a foreign current namespace"),false);
}
static void worldview_capture_configured_enum_publication_owned(lua_State*L,int environment,std::size_t ordinal){
 auto&p=owner(L);if(!p.worldview_native_host)return;
 native_db_query_current(L);const int top=lua_gettop(L);if(environment<0)environment=top+environment+1;
 const auto slot=p.worldview_native_host->child;
 if(slot>=p.child_contexts.size()||ordinal>=p.enums.tables.size()||!p.worldview_native_child_controls_current(slot))
  return (void)native_master_thread_refuse(L,"GameCore actual enum configuration publisher lost its selected Source");
 lua_rawgeti(L,LUA_REGISTRYINDEX,p.child_contexts[slot].environment);
 if(!lua_rawequal(L,-1,environment))return (void)native_master_thread_refuse(L,"GameCore actual enum publisher has foreign destination");lua_pop(L,1);
 p.worldview_namespace_returns.push_back(std::unique_ptr<NativeWorldViewNamespaceReturn>(new NativeWorldViewNamespaceReturn(
  p,NativeWorldViewNamespaceReturn::Kind::configured_enum,p.enums.tables[ordinal].name.c_str(),slot,lua_topointer(L,environment),LUA_NOREF,nullptr,nullptr)));
 auto*r=p.worldview_namespace_returns.back().get();r->configured_ordinal=ordinal;
 lua_getfield(L,environment,r->name.c_str());worldview_namespace_retain_actual_value_owned(L,*r,-1);lua_settop(L,top);
 native_db_query_current(L);r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
 if(!worldview_namespace_current_owned(L,environment,r->name.c_str(),r->kind))
  return (void)native_master_thread_refuse(L,"GameCore actual enum publication persistent POST failed");
}
static int worldview_configure_locale_coexisting_owned(lua_State*L){
 auto&p=owner(L);luaL_checktype(L,1,LUA_TTABLE);
 if(p.worldview_native_host){
  if(!worldview_namespace_current_owned(L,1,"Locale",NativeWorldViewNamespaceReturn::Kind::locale))
   return native_master_thread_refuse(L,"selected native Locale requires its genuine completed registrar publication");
  return 0; // The real32-member readonly selected namespace already exists.
 }
 // The exact genuine legacy configure closure is a real upvalue retained by
 // the initializer. Preserve its ordinary context behavior outside this flow.
 if(!lua_isfunction(L,lua_upvalueindex(1))||lua_iscfunction(L,lua_upvalueindex(1)))
  return luaL_error(L,"Locale configure adapter lost its actual Lua factory return");
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,1);lua_call(L,1,0);return 0;
}

static void worldview_capture_readonly_return_owned(lua_State*L,Job&job,const char*name,int master_ref,int proxy_ref,
 const void*master,const void*proxy,const void*meta,bool constructor_returned){
 auto&p=owner(L);const int top=lua_gettop(L);
 if(master_ref<0||proxy_ref<0||!master||!proxy||!meta||
  (!constructor_returned&&!worldview_namespace_master_was_issued_owned(p,L,NativeWorldViewNamespaceReturn::Kind::readonly_api,master)))
  return (void)native_master_thread_refuse(L,"readonly namespace requires its own actual constructor/publisher return");
 auto*r=worldview_namespace_begin_owned(L,NativeWorldViewNamespaceReturn::Kind::readonly_api,name,job,master_ref,master,meta);
 lua_rawgeti(L,LUA_REGISTRYINDEX,master_ref);
 if(lua_getmetatable(L,-1)){r->master_metadata=lua_topointer(L,-1);lua_pop(L,1);}lua_pop(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,proxy_ref);worldview_namespace_retain_actual_value_owned(L,*r,-1);lua_settop(L,top);
 native_db_query_current(L);r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
}
static void worldview_capture_modding_publication_owned(lua_State*L,Job&job){
 auto&p=owner(L);native_modding_registration_full(L,job);const auto*f=p.modding_frame;
 if(!f||f->actual_job!=&job||f->stores!=1)return (void)native_master_thread_refuse(L,"Modding persistent publisher order changed");
 worldview_capture_readonly_return_owned(L,job,"Modding",f->master_ref,f->proxy_ref,f->master,f->proxy,f->meta,f->fields_created==63);
 native_modding_registration_full(L,job);
}
static void worldview_capture_db_publication_owned(lua_State*L,Job&job){
 auto&p=owner(L);native_db_registration_full(L,job);const auto*f=p.db_registration_frame;
 if(!f||f->actual_job!=&job||f->stores!=1)return (void)native_master_thread_refuse(L,"DB persistent publisher order changed");
 worldview_capture_readonly_return_owned(L,job,"DB",f->master_ref,f->proxy_ref,f->master,f->proxy,f->meta,f->fields_created==6);
 native_db_registration_full(L,job);
}
static void worldview_capture_database_publications_owned(lua_State*L,Job&job){
 auto&p=owner(L);native_database_namespaces_full(L,job);auto*f=p.database_namespaces_frame;
 if(!f||f->actual_job!=&job||f->assigned!=2)return (void)native_master_thread_refuse(L,"Database persistent publisher order changed");
 for(std::size_t i=0;i<2;++i){native_database_namespaces_full(L,job);f=p.database_namespaces_frame;const auto&s=f->stores[i];
  worldview_capture_readonly_return_owned(L,job,native_database_namespace_keys[i],s.master_ref,s.proxy_ref,s.master,s.proxy,s.meta,s.constructor_returned);
  native_database_namespaces_full(L,job);
 }
}
static void worldview_capture_content_publications_owned(lua_State*L,Job&job){
 auto&p=owner(L);native_content_registration_full(L,job);const auto*f=p.content_frame;
 if(!f||f->actual_job!=&job||f->stores!=1||f->enums_ref<0||!f->enums||f->enums_copied.empty())
  return (void)native_master_thread_refuse(L,"Content persistent publisher order changed");
 worldview_capture_readonly_return_owned(L,job,"ContentManager",f->master_ref,f->proxy_ref,f->master,f->proxy,f->meta,f->fields_created==6);
 native_content_registration_full(L,job);f=p.content_frame;
 if(f->enums_constructed_members!=3&&!worldview_namespace_master_was_issued_owned(p,L,NativeWorldViewNamespaceReturn::Kind::content_enum,f->enums))
  return (void)native_master_thread_refuse(L,"ContentType requires its own actual constructor return");
 const int top=lua_gettop(L);auto*r=worldview_namespace_begin_owned(L,NativeWorldViewNamespaceReturn::Kind::content_enum,"ContentType",job,f->enums_ref,f->enums,nullptr);
 lua_rawgeti(L,LUA_REGISTRYINDEX,f->enums_ref);lua_getfield(L,-1,"ContentType");worldview_namespace_retain_actual_value_owned(L,*r,-1);lua_settop(L,top);
 native_content_registration_full(L,job);r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
}
static bool worldview_gameplay_namespace_preserve_owned(lua_State*L,int environment,const char*name){
 auto&p=owner(L);if(!p.worldview_native_host)return false;
 if(!worldview_namespace_current_owned(L,environment,name,NativeWorldViewNamespaceReturn::Kind::readonly_api))
  return (native_master_thread_refuse(L,"native gameplay configuration requires its genuine existing namespace"),false);
 return true;
}
static int worldview_gameplay_namespace_preserve_callback_owned(lua_State*L){
 luaL_checktype(L,1,LUA_TTABLE);const char*name=luaL_checkstring(L,2);
 if(std::strcmp(name,"DB")&&std::strcmp(name,"GameDefines"))return luaL_error(L,"unsupported gameplay preservation namespace");
 const bool value=worldview_gameplay_namespace_preserve_owned(L,1,name);lua_pushboolean(L,value);return 1;
}
template<int Which>static int worldview_configure_api_coexisting_owned(lua_State*L){
 auto&p=owner(L);luaL_checktype(L,1,LUA_TTABLE);
 if(p.worldview_native_host){
  constexpr const char*name=Which==0?"Modding":Which==1?"GameInfo":"ContentManager";
  if(!worldview_namespace_current_owned(L,1,name,NativeWorldViewNamespaceReturn::Kind::readonly_api))
   return native_master_thread_refuse(L,"native configuration requires its genuine completed readonly namespace");
  if constexpr(Which==2){
   if(!worldview_namespace_current_owned(L,1,"ContentType",NativeWorldViewNamespaceReturn::Kind::content_enum))
    return native_master_thread_refuse(L,"Content configuration requires its genuine completed ContentType publisher");
  }
  if constexpr(Which==1){
   lua_getfield(L,1,"GameInfoTypes");const bool present=!lua_isnil(L,-1);lua_pop(L,1);
   if(present){
    if(!worldview_namespace_current_owned(L,1,"GameInfoTypes",NativeWorldViewNamespaceReturn::Kind::info_types))
     return native_master_thread_refuse(L,"GameInfoTypes configuration cannot replace a foreign namespace");
   }else {
    // No earlier original5C7880 GameInfoTypes writer exists in this Root.
    // Execute the genuine existing gameplay DB type provider, not an empty
    // table or fabricated cache, and retain its actual produced output.
    native_db_query_current(L);lua_pushcfunction(L,gameplay_info_types);lua_call(L,0,1);
    const auto slot=p.worldview_native_host->child;
    p.worldview_namespace_returns.push_back(std::unique_ptr<NativeWorldViewNamespaceReturn>(new NativeWorldViewNamespaceReturn(
     p,NativeWorldViewNamespaceReturn::Kind::info_types,"GameInfoTypes",slot,lua_topointer(L,1),LUA_NOREF,nullptr,nullptr)));
    auto*r=p.worldview_namespace_returns.back().get();worldview_namespace_retain_actual_value_owned(L,*r,-1);
    lua_setfield(L,1,"GameInfoTypes");native_db_query_current(L);r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
    if(!worldview_namespace_current_owned(L,1,"GameInfoTypes",r->kind))return native_master_thread_refuse(L,"GameInfoTypes actual producer POST failed");
   }
  }
  return 0;
 }
 if(!lua_isfunction(L,lua_upvalueindex(1))||lua_iscfunction(L,lua_upvalueindex(1)))return luaL_error(L,"namespace adapter lost its actual legacy factory return");
 const int arguments=lua_gettop(L);lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,1);lua_call(L,arguments,0);return 0;
}

static void worldview_install_native_enums(lua_State* L,int environment,std::string_view context){
 auto& p=owner(L);
 if(!p.worldview_native_host)return;
 p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}catch(...){p.source_exception=std::current_exception();p.source_error="WorldView native enum origin refused";}
 if(p.source_error){p.record_failure();return (void)luaL_error(L,"%s",p.source_error);}
 // Actual UIEnums/Locale/UI publishers precede selected source execution.
 // Preserve only their genuine persistent returns, including the full native
 // enum groups. A table with matching values is not publication provenance.
 if(!worldview_namespace_current_owned(L,environment,"KeyEvents",NativeWorldViewNamespaceReturn::Kind::ui_enum)||
    !worldview_namespace_current_owned(L,environment,"MouseEvents",NativeWorldViewNamespaceReturn::Kind::ui_enum)||
    !worldview_namespace_current_owned(L,environment,"Keys",NativeWorldViewNamespaceReturn::Kind::ui_enum))
  return (void)native_master_thread_refuse(L,"WorldView required native input enums lack their completed original publisher returns");
 lua_getfield(L,environment,"InterfaceModeTypes");
 if(!lua_isnil(L,-1)){
  lua_pop(L,1);
  if(!worldview_namespace_current_owned(L,environment,"InterfaceModeTypes",NativeWorldViewNamespaceReturn::Kind::configured_enum)&&
     !worldview_namespace_current_owned(L,environment,"InterfaceModeTypes",NativeWorldViewNamespaceReturn::Kind::interface_enum))
   return (void)native_master_thread_refuse(L,"WorldView cannot replace foreign InterfaceModeTypes namespace");
 }else {
  lua_pop(L,1);const auto slot=p.worldview_native_host->child;
  p.worldview_namespace_returns.push_back(std::unique_ptr<NativeWorldViewNamespaceReturn>(new NativeWorldViewNamespaceReturn(
   p,NativeWorldViewNamespaceReturn::Kind::interface_enum,"InterfaceModeTypes",slot,lua_topointer(L,environment),LUA_NOREF,nullptr,nullptr)));
  auto*r=p.worldview_namespace_returns.back().get();
  lua_newtable(L);worldview_namespace_retain_actual_value_owned(L,*r,-1);
  for(const auto&item:detail::bnw_worldview_original_input_enum_entries)if(!std::strcmp(item.table,"InterfaceModeTypes")){
   lua_pushinteger(L,static_cast<lua_Integer>(item.value));lua_setfield(L,-2,item.name);
  }
  lua_setfield(L,environment,"InterfaceModeTypes");native_db_query_current(L);r->stage=NativeWorldViewNamespaceReturn::Stage::returned;
  if(!worldview_namespace_current_owned(L,environment,"InterfaceModeTypes",r->kind))
   return (void)native_master_thread_refuse(L,"WorldView actual InterfaceModeTypes publication POST failed");
 }
 worldview_require_lua_origin(L);
 if(p.worldview_mouse_events.size()!=4||p.worldview_mouse_events[0]!=0||p.worldview_mouse_events[1]!=2||p.worldview_mouse_events[2]!=5||p.worldview_mouse_events[3]!=6)return (void)luaL_error(L,"WorldView actual native Mouse enum source missing");
 if(!worldview_namespace_current_owned(L,environment,"Mouse",NativeWorldViewNamespaceReturn::Kind::ui_enum))
  return (void)native_master_thread_refuse(L,"WorldView cannot replace foreign or missing native Mouse enum source");
 // The native seven-member Mouse group is retained. Cross-check only the
 // four current input-owner operands needed by actual Control callbacks.
 lua_getfield(L,environment,"Mouse");const int mouse=lua_gettop(L);
 constexpr const char*mouse_names[]={"eLClick","eRClick","eMouseEnter","eMouseExit"};
 for(std::size_t i=0;i<4;++i){lua_getfield(L,mouse,mouse_names[i]);
  if(lua_type(L,-1)!=LUA_TNUMBER||lua_tonumber(L,-1)!=static_cast<lua_Number>(p.worldview_mouse_events[i]))
   return (void)native_master_thread_refuse(L,"WorldView native Mouse publication and actual input owner differ");lua_pop(L,1);
 }lua_pop(L,1);
 lua_getfield(L,environment,"GridToWorld");
 if(!lua_isnil(L,-1)){
  if(!native_flua_exposure_grid_endpoint_current_owned(L,-1))
   return (void)luaL_error(L,"WorldView cannot replace existing GridToWorld source");
  lua_pop(L,1); // SAME genuine FLua endpoint already resolves the real body.
 }else {
  lua_pop(L,1);lua_pushcfunction(L,worldview_grid_to_world);lua_setfield(L,environment,"GridToWorld");
 }

 if(!p.locale.current||!p.locale.select||!p.locale.convert_text_key)return (void)luaL_error(L,"WorldView selected Locale issuer missing");
 if(!worldview_namespace_current_owned(L,environment,"Locale",NativeWorldViewNamespaceReturn::Kind::locale))
  return (void)native_master_thread_refuse(L,"WorldView cannot replace foreign or missing native Locale namespace");
 // The real Locale32 callbacks already include exact ConvertTextKey/Lookup
 // and current-language providers. Uncalled backend consumers stay lazy.
 lua_getfield(L,environment,"UI");
 if(lua_isnil(L,-1)||!native_context_ui_selected_namespace_current_owned(L,environment))
  return (void)native_master_thread_refuse(L,"WorldView requires genuine completed selected UI namespace");
 lua_pop(L,1);
 // UI's original0-upvalue policy consumers own real op0/1/2 one-upvalue
 // adapters to worldview_input_policy_call. No foreign table is overwritten.



}
const BnwWorldViewSourceOperationLoan* worldview_operation=nullptr;
const LuaScriptHost* worldview_host_owner=nullptr;
std::weak_ptr<Impl> worldview_self;
std::weak_ptr<EndTurnQueue::VmLifetime> worldview_lifetime;
lua_State* worldview_control_entry=nullptr;
lua_State* worldview_control_validation=nullptr;
struct WorldViewNativeHostScope;
WorldViewNativeHostScope* worldview_native_host=nullptr;
struct WorldViewNativeHostScope final {
 Impl& p;const LuaScriptHost* const host;const BnwWorldViewSourceOperationLoan* const loan;
 const std::thread::id creator=std::this_thread::get_id();
 const std::uint64_t serial,epoch,writes,uncertainty;
 const std::weak_ptr<BnwSelectedWorldViewContextOwner> sources;
 const std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;
 std::size_t child=graph_no_owner;
 std::weak_ptr<const ScriptUiChildSource> child_source;
 bool returned=false;
 WorldViewNativeHostScope(Impl& actual,const LuaScriptHost& facade,const BnwWorldViewSourceOperationLoan& active)
  :p(actual),host(&facade),loan(&active),serial(active.operation_serial()),epoch(p.failure_epoch),
   writes(p.failed.writes_),uncertainty(p.execution_uncertainty),sources(p.worldview_sources),lifetime(p.end_turn_lifetime){
  const char* refused=nullptr;
  if(creator!=p.ui_thread)refused="creator/ui_thread";
  else if(p.calling)refused="calling";
  else if(p.failed)refused="failed";
  else if(p.worldview_native_host)refused="worldview_native_host";
  else if(!p.worldview_sources_bound)refused="worldview_sources_bound";
  else if(!p.end_turn_lifetime)refused="end_turn_lifetime";
  else if(!p.end_turn_lifetime->alive)refused="lifetime.alive";
  else if(p.end_turn_lifetime->failed)refused="lifetime.failed";
  else if(p.end_turn_lifetime->host!=host)refused="lifetime.host";
  else if(p.end_turn_lifetime->vm!=p.state)refused="lifetime.vm/main_state";
  if(refused)throw ScriptError(std::string("WorldView native Host scope origin refused: first=")+refused);
  p.worldview_native_host=this;p.worldview_operation=loan;p.worldview_host_owner=host;
  p.worldview_self=p.weak_from_this();p.worldview_lifetime=lifetime;
 }
 void pure()const {
  if(p.worldview_control_validation){p.record_failure();throw ScriptError("WorldView control validation did not return");}
  if(creator!=std::this_thread::get_id())
   throw ScriptError("WorldView actual native scope/failure writer changed: first=creator/current_thread");
  else if(creator!=p.ui_thread)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=creator/ui_thread");
  else if(p.worldview_native_host!=this)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=worldview_native_host");
  else if(p.worldview_operation!=loan)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=worldview_operation");
  else if(p.worldview_host_owner!=host)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=worldview_host_owner");
  else if(p.failure_epoch!=epoch)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=failure_epoch expected="+
    std::to_string(epoch)+" actual="+std::to_string(p.failure_epoch));
  else if(p.execution_uncertainty!=uncertainty)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=execution_uncertainty expected="+
    std::to_string(uncertainty)+" actual="+std::to_string(p.execution_uncertainty));
  else if(p.failed.exhausted_)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=failed.exhausted expected=false actual=true");
  else if(p.failed.writes_!=writes)
   throw ScriptError("WorldView actual native scope/failure writer changed: first=failed.writes expected="+
    std::to_string(writes)+" actual="+std::to_string(p.failed.writes_));
  const auto life=lifetime.lock();
  if(!life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
     p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->host!=host||life->vm!=p.state)
   throw ScriptError("WorldView native Host facade/allocation retired");
  if(sources.expired()||sources.owner_before(p.worldview_sources)||p.worldview_sources.owner_before(sources))
   throw ScriptError("WorldView actual source control changed");
 }
 ~WorldViewNativeHostScope(){
  if(creator!=std::this_thread::get_id())std::terminate();
  if(p.worldview_native_host!=this||!returned){p.record_failure();if(p.end_turn_lifetime)p.end_turn_lifetime->failed=true;}
  p.worldview_operation=nullptr;p.worldview_host_owner=nullptr;p.worldview_native_host=nullptr;
  p.worldview_self.reset();p.worldview_lifetime.reset();
 }
 WorldViewNativeHostScope(const WorldViewNativeHostScope&)=delete;
};
void worldview_require_callback_origin(){
 const auto* const scope=worldview_native_host;
 if(!scope)throw ScriptError("WorldView has no actual active native Host scope");
 scope->pure();
 const auto source=worldview_sources.lock();if(!source)throw ScriptError("WorldView canonical source facade retired");
 // Fetch the currently published scope before touching any saved stack loan.
 const auto& actual=source->current_operation_loan();
 if(&actual!=worldview_operation||&actual!=scope->loan)throw ScriptError("WorldView native loan replaced or retired");
 if(actual.operation_serial()!=scope->serial)throw ScriptError("WorldView native source operation changed");
 scope->pure();
}
bool worldview_native_child_controls_current(std::size_t slot)const{
 const auto* scope=worldview_native_host;
 if(!scope||scope->child!=slot||slot>=child_contexts.size())return false;
 scope->pure();
 const auto selected=scope->child_source.lock();const auto owner_sources=scope->sources.lock();
 return selected&&owner_sources&&owner_sources.get()==context_sources.get()&&
  !owner_sources.owner_before(context_sources)&&!context_sources.owner_before(owner_sources)&&
  selected.get()==child_contexts[slot].source.get()&&
  !selected.owner_before(child_contexts[slot].source)&&!child_contexts[slot].source.owner_before(selected)&&
  !child_contexts[slot].retired;
}
bool selected_loading_context_matches(std::string_view context)const{
 if(worldview_native_host){
  const auto slot=worldview_native_host->child;
  return worldview_native_child_controls_current(slot)&&child_contexts[slot].name==context;
 }
 return context==end_turn_context;
}
static void worldview_require_lua_origin(lua_State* L){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual native Lua origin refused";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
struct WorldViewControlScope final {
 Impl& p;lua_State* const entry;const std::uint64_t epoch,writes;const bool prior_failed,prior_ready;
 bool returned=false;
 WorldViewControlScope(Impl& actual,lua_State* L):p(actual),entry(L),epoch(p.failure_epoch),writes(p.failed.writes_),
  prior_failed(p.failed),prior_ready(p.ready){
  p.worldview_require_callback_origin();
  if(p.worldview_control_entry)throw ScriptError("WorldView control lender reentered");
  p.worldview_control_entry=L;
 }
 void current()const{
  if(p.worldview_control_entry!=entry||p.failure_epoch!=epoch||p.failed.writes_!=writes||p.failed.exhausted_)
   throw ScriptError("WorldView control lender/failure changed");
  p.worldview_require_callback_origin();
 }
 ~WorldViewControlScope(){
  if(!returned||p.worldview_control_entry!=entry)p.record_failure();
  p.worldview_control_entry=nullptr;
 }
};
static int worldview_control_body(lua_State* L){
 auto& p=owner(L);const auto slot=graph_slot(L);graph_require_context(L,slot);
 const int op=static_cast<int>(lua_tointeger(L,lua_upvalueindex(2)));
 bool health=false;p.source_exception={};p.source_error=nullptr;
 try{
  p.worldview_require_callback_origin();
  if(!p.worldview_operation)throw ScriptError("Health control actual Source absent");
  health=p.worldview_operation->unitflag_health_operation_without_callbacks();
  p.worldview_require_callback_origin();
  if(health){
   if(op!=0&&op!=14&&op!=18&&op!=21)throw ScriptError("Health Source cannot grant another control operation");
   if(!p.worldview_native_host||p.worldview_native_host->child>=p.child_contexts.size()||
    p.child_contexts[p.worldview_native_host->child].name!="UnitFlagManager"||
    p.control_graphs[slot].graph->context()!="UnitFlagManager"||
    !p.worldview_native_child_controls_current(p.worldview_native_host->child))
    throw ScriptError("Health control lacks genuine UnitFlag native child/graph");
  }
 }catch(const std::exception&error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="Actual control purpose classification failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 const auto id=graph_checked(L,1,slot)->id;
 const int argc=op==3||op==8||op==19||op==20?1:op==9||op==13?3:op==11?4:2;
 if(lua_gettop(L)!=argc)return luaL_error(L,"WorldView control arguments differ");
 if(!p.worldview_control_entry||p.worldview_control_entry!=L)return luaL_error(L,"WorldView control lender absent");
 if(op==0||op==5)luaL_checktype(L,2,LUA_TBOOLEAN);
 if(op==4||op==18)luaL_checktype(L,2,LUA_TNUMBER);
 if(op==6||op==7||op==16)luaL_checktype(L,2,LUA_TSTRING);
 if(op==9){luaL_checktype(L,2,LUA_TNUMBER);luaL_checktype(L,3,LUA_TFUNCTION);}
 std::int32_t native_void=0;
 if(op==22||op==23){
  luaL_checktype(L,2,LUA_TNUMBER);const auto value=lua_tonumber(L,2);
  if(!std::isfinite(value)||std::trunc(value)!=value||value<INT32_MIN||value>INT32_MAX)
   return luaL_error(L,"UnitFlag Void operand outside exact int32");
  native_void=static_cast<std::int32_t>(value);
 }
 std::array<double,4> color{};
 std::array<double,3> position{};std::array<double,2> offset{};double offset_x=0;
 if(op==10||op==12||op==15||op==17||op==21){
  luaL_checktype(L,2,LUA_TTABLE);
  for(int i=0;i<(op==15||op==21?4:op==10?3:2);++i){
   constexpr const char* names[]={"x","y","z","w"};lua_getfield(L,2,names[i]);
   if(lua_type(L,-1)!=LUA_TNUMBER)return luaL_error(L,"WorldView actual vector component absent/non-numeric");
   const auto value=lua_tonumber(L,-1);lua_pop(L,1);
   if(!std::isfinite(value))return luaL_error(L,"WorldView actual vector component nonfinite");
   if(op==15||op==21)color[static_cast<std::size_t>(i)]=value;else if(op==10)position[static_cast<std::size_t>(i)]=value;else offset[static_cast<std::size_t>(i)]=value;
  }
 }
 if(op==11||op==13||op==14){
  for(int i=2;i<=argc;++i){luaL_checktype(L,i,LUA_TNUMBER);const auto value=lua_tonumber(L,i);
   if(!std::isfinite(value))return luaL_error(L,"WorldView actual value coordinate nonfinite");
   if(op==11)position[static_cast<std::size_t>(i-2)]=value;
   else if(op==13)offset[static_cast<std::size_t>(i-2)]=value;else offset_x=value;
  }
 }
 p.failed.set_loader_sentinel();p.ready=false;p.control_graphs[slot].touched=true;
 bool hidden=false;ScriptUiControlGraph::Id button=0;
 p.source_exception={};p.source_error=nullptr;
 try {
  p.worldview_require_callback_origin();auto& graph=*p.control_graphs[slot].graph;
  if(op==0){if(health)graph.set_unitflag_health_hidden_owned(id,lua_toboolean(L,2)!=0);else graph.set_hidden(id,lua_toboolean(L,2)!=0);}
  else if(op==1)graph.change_parent(id,graph_checked(L,2,slot)->id);
  else if(op==2)graph.destroy_child(id,graph_checked(L,2,slot)->id);
  else if(op==3)hidden=graph.inspect(id).hidden;
  else if(op==4)graph.set_alpha(id,lua_tonumber(L,2));
  else if(op==5)graph.set_disabled(id,lua_toboolean(L,2)!=0);
  else if(op==6||op==7){
   std::size_t length=0;const auto* value=lua_tolstring(L,2,&length);
   if(op==6)graph.set_text(id,std::string(value,length));else graph.set_tooltip(id,std::string(value,length));
  }else if(op==8)button=graph.get_button(id);
  else if(op==22||op==23){
   if(!graph.backend_)throw ScriptError("UnitFlag Void actual backend missing");
   if(op==22)graph.backend_->set_void1(id,native_void);else graph.backend_->set_void2(id,native_void);
  }
  else if(op==9)worldview_register_owned_callback(L,slot,id,source_integer(L,2),3);
  else if(op==10||op==11)graph.set_world_position(id,position);
  else if(op==12||op==13)graph.set_offset(id,offset);
  else if(op==14){if(health)graph.set_unitflag_health_offset_x_owned(id,offset_x);else graph.set_offset_x(id,offset_x);}
  else if(op==15)graph.set_color(id,color);
  else if(op==16){std::size_t n=0;const auto* v=lua_tolstring(L,2,&n);graph.set_texture(id,std::string(v,n));}
  else if(op==17)graph.set_texture_offset(id,offset);
  else if(op==18){if(health)graph.set_unitflag_health_percent_owned(id,lua_tonumber(L,2));else graph.set_percent(id,lua_tonumber(L,2));}
  else if(op==19)graph.animation_set_to_beginning(id);
  else if(op==20)graph.animation_play(id);
  else if(op==21){if(health)graph.set_unitflag_health_fg_color_owned(id,color);else graph.set_fg_color(id,color);}
  else throw ScriptError("WorldView control method unsupported");
  p.worldview_require_callback_origin();
  if(health){
   const auto command=op==0?BnwUnitFlagHealthCommand::hide:op==14?BnwUnitFlagHealthCommand::offset_x:
    op==18?BnwUnitFlagHealthCommand::percent:BnwUnitFlagHealthCommand::fg_color;
   if(!p.worldview_operation->unitflag_health_operation_without_callbacks())
    throw ScriptError("Actual Health operation retired after retained control store");
   p.worldview_require_callback_origin();graph.require_unitflag_health_current_owned(id,command);
   p.worldview_require_callback_origin();
  }else{graph.require_current();p.worldview_require_callback_origin();}
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView control failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(op==3){lua_pushboolean(L,hidden);return 1;}
 if(op==8){graph_push_control(L,slot,button);return 1;}return 0;
}
static int worldview_control_call(lua_State* L){return worldview_graph_protected_call(L,false);}
static int worldview_context_call(lua_State* L){return worldview_graph_protected_call(L,true);}
static int worldview_graph_protected_call(lua_State* L,bool context){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView control validation source retired";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 p.worldview_control_validation=L;
 const auto slot=graph_slot(L);
 graph_require_context(L,slot);
 if(!p.control_graphs[slot].graph->backend_)return luaL_error(L,"WorldView expected actual canonical backend");
 if(!context)(void)graph_checked(L,1,slot);
 else if(!lua_rawequal(L,1,lua_upvalueindex(3)))return luaL_error(L,"WorldView ContextPtr receiver differs");
 const int args=lua_gettop(L);
 lua_pushvalue(L,lua_upvalueindex(1));lua_pushvalue(L,lua_upvalueindex(2));
 if(context)lua_pushvalue(L,lua_upvalueindex(3));
 lua_pushcclosure(L,context?graph_context_call:worldview_control_body,context?3:2);lua_insert(L,1);
 p.worldview_control_validation=nullptr; // all genuine receiver/caller checks and closure publication returned
 int status=0;p.source_exception={};p.source_error=nullptr;
 try{
  {WorldViewControlScope lender(p,L);lender.current();
   status=lua_pcall(L,args,LUA_MULTRET,0);
   if(!status){lender.current();p.ready=lender.prior_ready;
    if(lender.prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
    lender.returned=true;}
  }
 }catch(const std::exception& error){p.source_exception=std::current_exception();p.source_error=error.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView protected control failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);return lua_gettop(L);
}
static void perform_worldview_context_load(lua_State* L,Job& job){
 auto& p=owner(L);
 p.worldview_require_callback_origin();
 if(!p.end_turn_child_loader||p.end_turn_child_loader->entry!=L||
    p.end_turn_child_loader->child!=job.worldview_child||job.worldview_child>=p.child_contexts.size())
  luaL_error(L,"WorldView actual native loader lender absent");
 const auto attempt=p.child_attempt_succeeded.size();
 if(attempt!=p.end_turn_child_loader->attempt)luaL_error(L,"WorldView native load occurrence changed");
 p.child_attempt_succeeded.push_back(false);
 if(!p.end_turn_child_loader->current())luaL_error(L,"WorldView genuine native loader origin changed");
 p.failed.set_loader_sentinel();p.ready=false;
 p.executed_contexts.emplace(p.child_contexts[job.worldview_child].name);
 lua_pushinteger(L,static_cast<lua_Integer>(job.worldview_child));lua_pushcclosure(L,run_child_context,1);
 const auto previous=p.graph_execution_scope;
 ++p.child_load_depth;const int status=lua_pcall(L,0,0,0);--p.child_load_depth;p.graph_execution_scope=previous;
 if(status)return (void)lua_error(L);
 p.worldview_require_callback_origin();p.child_require_current(job.worldview_child);p.worldview_require_callback_origin();
 auto& child=p.child_contexts[job.worldview_child];
 child.source->require_captured_bytes(child.bytes);p.worldview_require_callback_origin();
 for(std::size_t i=attempt+1;i<p.child_attempt_succeeded.size();++i)
  if(!p.child_attempt_succeeded[i])luaL_error(L,"WorldView nested actual context load failed");
 child.loading=false;child.loaded=true;p.child_attempt_succeeded[attempt]=true;
 p.failed=p.end_turn_child_loader->prior_failed;p.ready=p.end_turn_child_loader->prior_ready;
}
