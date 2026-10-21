// Included inside actual LuaScriptHost::Impl after native Scope definitions.
// Legacy unit references are unchanged outside actual WorldView native scopes.
struct WorldViewUnitReferenceScope;
WorldViewUnitReferenceScope* worldview_unit_reference_scope=nullptr;
struct WorldViewUnitReferenceScope final {
 Impl& p;lua_State* const entry;WorldViewNativeHostScope* const native;
 const BnwWorldViewSourceOperationLoan* const loan;
 const std::thread::id creator=std::this_thread::get_id();
 const std::uint64_t epoch,writes,uncertainty,serial;
 const std::size_t child,compiled;const int environment;
 const std::weak_ptr<const ScriptUiChildSource> source;
 const ScriptUiChildSource* const source_get;
 const std::shared_ptr<const ScriptUnitReadProvider> units;
 const bool lookup,prior_failed,prior_ready,calling,owned_callback;
 bool provisional=false,data_returned=false,returned=false,found=false;
 std::size_t reference=0;mutable bool checking=false;
 WorldViewUnitReferenceScope(Impl& actual,lua_State* L,bool find)
  :p(actual),entry(L),native(p.worldview_native_host),loan(p.worldview_operation),
   epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),
   serial(native?native->serial:0),child(native?native->child:graph_no_owner),compiled(p.graph_execution_scope),
   environment(child<p.child_contexts.size()?p.child_contexts[child].environment:LUA_NOREF),
   source(child<p.child_contexts.size()?p.child_contexts[child].source:std::shared_ptr<const ScriptUiChildSource>{}),
   source_get(child<p.child_contexts.size()?p.child_contexts[child].source.get():nullptr),
   units(p.gameplay.units),lookup(find),prior_failed(p.failed),prior_ready(p.ready),calling(p.calling),owned_callback(p.worldview_callback_dispatching){
  p.worldview_require_callback_origin();
  if(creator!=p.ui_thread||!native||!loan||!entry||(!calling&&!owned_callback)||p.worldview_unit_reference_scope||
     child>=p.child_contexts.size()||environment==LUA_NOREF||!source_get||!units||
     !p.worldview_native_child_controls_current(child)||compiled==graph_no_owner||
     compiled>=p.graph_compiled_sources.size()||
     p.graph_compiled_sources[compiled].context!=p.child_contexts[child].name)
   throw ScriptError("WorldView unit reference lacks actual pre-sentinel native child/source lender");
  p.worldview_unit_reference_scope=this;
 }
 void fail()const noexcept{p.record_failure();if(p.end_turn_lifetime)p.end_turn_lifetime->failed=true;}
 void pure()const{
  if(creator!=std::this_thread::get_id()){fail();throw ScriptError("WorldView unit reference foreign creator");}
  if(native!=p.worldview_native_host||!native){fail();throw ScriptError("WorldView unit reference native scope expired");}
  native->pure();
  if(p.worldview_unit_reference_scope!=this||p.worldview_operation!=loan||native->serial!=serial||native->child!=child||
     p.calling!=calling||p.worldview_callback_dispatching!=owned_callback||p.failure_epoch!=epoch||p.failed.writes_!=writes||p.failed.exhausted_||p.execution_uncertainty!=uncertainty||
     static_cast<bool>(p.failed)!=(provisional?true:prior_failed)||p.ready!=(provisional?false:prior_ready)||
     child>=p.child_contexts.size()||p.child_contexts[child].retired||p.child_contexts[child].environment!=environment||
     source.expired()||source_get!=p.child_contexts[child].source.get()||
     source.owner_before(p.child_contexts[child].source)||p.child_contexts[child].source.owner_before(source)||
     !units||units.get()!=p.gameplay.units.get()||units.owner_before(p.gameplay.units)||p.gameplay.units.owner_before(units)||
     compiled!=p.graph_execution_scope||!p.worldview_native_child_controls_current(child)){
   fail();throw ScriptError("WorldView unit reference actual lender/source/provider/failure changed");
  }
  native->pure();
 }
 void current()const{
  pure();if(checking){fail();throw ScriptError("WorldView unit reference lender reentered");}
  checking=true;struct End{bool&v;~End(){v=false;}}end{checking};
  try{p.worldview_require_callback_origin();pure();loan->require_operation(serial);pure();}
  catch(...){fail();throw;}
 }
 void begin(){current();p.failed.set_loader_sentinel();p.ready=false;provisional=true;pure();}
 void accept(lua_State* L){
  current();
  if(L!=entry||!data_returned||lua_gettop(L)!=1)throw ScriptError("WorldView unit reference publication shape differs");
  if(lookup){
   if(!found){if(!lua_isnil(L,1))throw ScriptError("WorldView native missing unit publication is not nil");}
   else{
    if(!lua_istable(L,1)||reference>=p.unit_references.size())throw ScriptError("WorldView native unit table publication differs");
    lua_pushliteral(L,"__instance");lua_rawget(L,1);
    const bool shape=lua_type(L,-1)==LUA_TUSERDATA&&lua_objlen(L,-1)==sizeof(UnitSlot);
    const auto* slot=shape?static_cast<const UnitSlot*>(lua_touserdata(L,-1)):nullptr;
    const bool same=slot&&slot->host==&p&&slot->slot==reference;lua_pop(L,1);
    if(!same)throw ScriptError("WorldView native unit userdata publication changed");
   }
  }else if(const auto* n=std::get_if<std::int64_t>(&p.unit_value)){
   if(lua_type(L,1)!=LUA_TNUMBER||lua_tonumber(L,1)!=static_cast<lua_Number>(*n))
    throw ScriptError("WorldView unit integer publication differs");
  }else if(const auto* b=std::get_if<bool>(&p.unit_value)){
   if(lua_type(L,1)!=LUA_TBOOLEAN||bool(lua_toboolean(L,1))!=*b)
    throw ScriptError("WorldView unit boolean publication differs");
  }else throw ScriptError("WorldView unit read publication requires earned scalar type");
  pure();p.ready=prior_ready;
  if(prior_failed)p.failed.set_loader_sentinel();else p.failed=false;
  provisional=false;pure();returned=true;
 }
 ~WorldViewUnitReferenceScope(){
  if(creator!=std::this_thread::get_id())std::terminate();
  if(p.worldview_unit_reference_scope!=this||!returned)fail();
  p.worldview_unit_reference_scope=nullptr;
 }
};
static int worldview_unit_reference_body(lua_State* L){
 auto& p=owner(L);auto* const scope=p.worldview_unit_reference_scope;
 if(!scope||scope->entry!=L){p.record_failure();return luaL_error(L,"WorldView unit reference protected lender absent");}
 // FIRST genuine Lua caller, never search through a foreign Lua frame.
 lua_Debug frame{};std::size_t compiled=graph_no_owner;int closure=0;
 for(int level=1;lua_getstack(L,level,&frame);++level){
  if(!lua_getinfo(L,"f",&frame))break;
  if(lua_iscfunction(L,-1)){lua_pop(L,1);continue;}
  compiled=p.graph_compiled_owner(L,-1);closure=lua_gettop(L);break;
 }
 if(compiled!=scope->compiled||closure==0){p.record_failure();return luaL_error(L,"WorldView unit caller lacks actual selected Proto");}
 lua_getfenv(L,closure);lua_rawgeti(L,LUA_REGISTRYINDEX,scope->environment);
 const bool environment=lua_rawequal(L,-1,-2);lua_pop(L,3);
 if(!environment){p.record_failure();return luaL_error(L,"WorldView unit caller environment changed");}
 p.source_exception={};p.source_error=nullptr;
 const char* method=nullptr;
 if(!scope->lookup){
  if(lua_gettop(L)!=1)return luaL_error(L,"WorldView unit scalar getter arguments differ");
  luaL_checktype(L,1,LUA_TTABLE);lua_pushliteral(L,"__instance");lua_rawget(L,1);
  if(lua_type(L,-1)!=LUA_TUSERDATA||lua_objlen(L,-1)!=sizeof(UnitSlot))
   return luaL_error(L,"WorldView unit userdata shape differs");
  const auto* slot=static_cast<const UnitSlot*>(luaL_checkudata(L,-1,"Civis.UnitReadReference"));
  if(!slot||slot->host!=&p||slot->slot>=p.unit_references.size())return luaL_error(L,"WorldView unit reference is foreign");
  scope->reference=slot->slot;lua_pop(L,1);
  if(lua_type(L,lua_upvalueindex(1))!=LUA_TSTRING)return luaL_error(L,"WorldView unit method selector differs");
  method=lua_tostring(L,lua_upvalueindex(1));
 }
 const auto actor=scope->lookup?object_id(L,false):0;
 const auto requested=scope->lookup?source_integer(L,2):0;
 try{
  scope->current();scope->units->require_current();scope->current();
  if(scope->lookup){
   if(lua_gettop(L)!=2)throw ScriptError("WorldView GetUnitByID exact argument count differs");
   if(p.worldview_health_membership_state){
    if(!p.worldview_damage_dispatching||p.worldview_created_dispatching||!p.worldview_native_host)
     throw ScriptError("Living Unit health reference escaped original damage occurrence");
    const auto member=p.worldview_health_membership_state;
    BnwUnitFlagHealthInstanceLoan::require_current_from_storage_owned(member);
    const auto ids=BnwUnitFlagHealthInstanceLoan::owner_full_id_from_storage_without_callbacks(member);
    scope->current();
    if(actor==ids.first&&requested==ids.second)
     p.unit_read_scratch=BnwUnitFlagHealthInstanceLoan::retain_living_unit_reference_from_storage_without_callbacks(member);
    else p.unit_read_scratch=scope->units->find(actor,requested);
    BnwUnitFlagHealthInstanceLoan::require_current_from_storage_owned(member);
   }else if(p.worldview_created_pending_occurrence){
    if(!p.worldview_created_dispatching||!p.worldview_callback_dispatching)
     throw ScriptError("Created pending Unit lookup escaped original callback");
    const auto&created=*p.worldview_created_pending_occurrence;created.require_same_source(*scope->loan);
    const auto ids=created.actual_owner_full_id();scope->current();
    if(actor==ids.first&&requested==ids.second)p.unit_read_scratch=created.retain_pending_unit_reference_owned();
    else p.unit_read_scratch=scope->units->find(actor,requested);
    created.require_origin_without_callbacks();
   }else p.unit_read_scratch=scope->units->find(actor,requested);
   scope->current();
   scope->found=bool(p.unit_read_scratch);
   if(scope->found){
    p.unit_read_scratch->require_current();scope->current();
    if(p.unit_references.size()>=unit_reference_limit)throw ScriptError("WorldView actual unit reference budget exhausted");
    p.unit_references.push_back(std::move(p.unit_read_scratch));scope->reference=p.unit_references.size()-1;
   }
  }else{
   p.unit_references[scope->reference]->require_current();scope->current();
   p.unit_value=p.unit_references[scope->reference]->read(method);scope->current();
   p.unit_references[scope->reference]->require_current();scope->current();
  }
  scope->units->require_current();scope->current();scope->data_returned=true;
 }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView actual unit provider failed";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!scope->lookup){push(L,p.unit_value);return 1;}
 if(!scope->found){lua_pushnil(L);return 1;}
 lua_newtable(L);auto* unit=static_cast<UnitSlot*>(lua_newuserdata(L,sizeof(UnitSlot)));*unit={&p,scope->reference};
 luaL_newmetatable(L,"Civis.UnitReadReference");lua_setmetatable(L,-2);lua_setfield(L,-2,"__instance");
 constexpr const char* methods[]={"GetID","GetOwner","GetX","GetY","MovesLeft","GetDamage","GetMaxHitPoints","IsDead","IsCargo",
  "GetCurrHitPoints","IsCombatUnit","IsEmbarked","CargoSpace","GetUnitType","GetDomainType","IsTrade","IsGarrisoned"};
 for(const auto* name:methods){lua_pushstring(L,name);lua_pushcclosure(L,unit_read,1);lua_setfield(L,-2,name);}
 return 1;
}
static int worldview_unit_reference_owned(lua_State* L,bool lookup){
 auto& p=owner(L);p.source_exception={};p.source_error=nullptr;
 try{p.worldview_require_callback_origin();}
 catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView unit native origin refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(!lua_checkstack(L,8)){p.record_failure();return luaL_error(L,"WorldView unit protected stack unavailable");}
 const int args=lua_gettop(L);
 if(lookup)lua_pushcfunction(L,worldview_unit_reference_body);
 else{lua_pushvalue(L,lua_upvalueindex(1));lua_pushcclosure(L,worldview_unit_reference_body,1);}
 lua_insert(L,1);int status=0;
 try{
  {WorldViewUnitReferenceScope scope(p,L,lookup);scope.begin();
   status=lua_pcall(L,args,1,0);
   if(!status)scope.accept(L);
  }
 }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
 catch(...){p.source_exception=std::current_exception();p.source_error="WorldView unit protected native lender refused";}
 if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
 if(status)return lua_error(L);
 return 1;
}
