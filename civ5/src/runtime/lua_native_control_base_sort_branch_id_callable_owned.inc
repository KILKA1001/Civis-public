//Inside actual Host::Impl after Source18 geometry and frozen Source2 ref kernels.
//Sort24 executes its genuine argument/member/ref prefix. Required const-child
//v80/descriptor4 supplier and positive-comparison introsort remain unissued.
static const NativeContextDefinition::Record*native_control_base_sort_branch_id_record_current_owned(
 lua_State*L,std::size_t ordinal){
 native_control_base_setoffset_record_current_owned(L);auto&p=owner(L);
 struct Recipe{std::uint32_t writer,key,primary,secondary;int flag;const char* name;};
 static constexpr Recipe exact[]={
  {0x9be700u,0xa3f8c4u,0x5bf0b0u,0x6c2210u,0,"SortChildren"},
  {0x9be760u,0xa3f910u,0x75eb20u,0x5404b0u,0,"BranchResetAnimation"},
  {0x9be7c0u,0xa3f944u,0x4dd890u,0x685430u,1,"GetID"},
 };
 if(ordinal<24||ordinal>26)return (luaL_error(L,"ControlBase actual source18 ordinal differs"),nullptr);
 const auto& proof=exact[ordinal-24];const auto& record=p.native_control_base_constructor->definition->records[ordinal];
 if(!record.recipe||!record.key||record.key!=record.recipe->key||record.recipe->writer!=proof.writer||
  record.recipe->key_address!=proof.key||std::strcmp(record.key,proof.name)||record.primary!=proof.primary||
  record.secondary!=proof.secondary||record.primary!=record.recipe->primary||record.secondary!=record.recipe->secondary||
  record.flag!=proof.flag||record.flag!=record.recipe->flag||record.reference!=-1)
  return (luaL_error(L,"ControlBase source18 actual recipe/flag/reference changed"),nullptr);
 return &record;
}

struct NativeControlBaseSortCall final {
 using Reference=NativeControlBaseSetOffsetDecodeCall::Reference;
 enum class Stage{entered,function_retained,member_entered,comparator_built,algorithm_entered,required_const_child,returned,retired};
 const Impl*const host;const std::thread::id creator=std::this_thread::get_id();
 NativeControlBaseSetOffsetDecodeCall*const borrower;
 bool alive=true;Reference incoming,temporary,comparator,sort_argument,algorithm_argument;
 lua_State* comparison_state=nullptr;Stage stage=Stage::entered;
 NativeControlBaseSortCall(Impl*p,NativeControlBaseSetOffsetDecodeCall*b):host(p),borrower(b){
  if(!host||!borrower)throw ScriptError("SortChildren actual reference custody absent");
 }
};
static void native_control_base_sort_current_owned(lua_State*L,const NativeControlBaseSortCall&r){
 native_control_base_sort_branch_id_record_current_owned(L,24);auto&p=owner(L);
 if(r.host!=&p||r.creator!=std::this_thread::get_id()||!r.alive||!r.borrower||
  r.stage==NativeControlBaseSortCall::Stage::retired||
  std::none_of(p.native_control_base_sort_calls.begin(),p.native_control_base_sort_calls.end(),
   [&](const auto& owned){return owned.get()==&r;})||
  std::none_of(p.native_control_base_setoffset_decode_calls.begin(),p.native_control_base_setoffset_decode_calls.end(),
   [&](const auto& owned){return owned.get()==r.borrower;}))
  return (void)luaL_error(L,"SortChildren SAME Host/ref custody retired");
 native_control_base_setoffset_decode_full_owned(L,*r.borrower);
 native_control_base_sort_branch_id_record_current_owned(L,24);
}
static NativeControlBaseSortCall*native_control_base_sort_begin_owned(lua_State*L,const void*cdata){
 auto*const borrower=native_control_base_setoffset_begin_decode_owned(L,cdata);auto&p=owner(L);
 p.source_exception={};p.source_error=nullptr;
 try{p.native_control_base_sort_calls.push_back(std::make_unique<NativeControlBaseSortCall>(&p,borrower));}
 catch(...){p.source_exception=std::current_exception();p.source_error="SortChildren actual ref custody allocation failed";}
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);return nullptr;}
 auto*r=p.native_control_base_sort_calls.back().get();native_control_base_sort_current_owned(L,*r);return r;
}
static void native_control_base_sort_release_owned(lua_State*L,NativeControlBaseSortCall&r,
 NativeControlBaseSortCall::Reference& value){
 native_control_base_sort_current_owned(L,r);
 native_control_base_setoffset_reference_release_8f8820_owned(L,*r.borrower,value);
 native_control_base_sort_current_owned(L,r);
}
static void native_control_base_sort_copy_8f84a0_owned(lua_State*L,NativeControlBaseSortCall&r,
 NativeControlBaseSortCall::Reference& destination,const NativeControlBaseSortCall::Reference& source){
 native_control_base_sort_current_owned(L,r);
 if(destination.state||destination.reference!=LUA_NOREF)
  return (void)luaL_error(L,"SortChildren8F84A0 needs fresh destination");
 destination.state=source.state;destination.reference=LUA_NOREF; //Actual first two constructor stores.
 if(source.state&&source.reference!=LUA_NOREF){
  if(source.state!=L)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"SortChildren reference foreign state");
  (void)lua_checkstack(L,1);native_control_base_sort_current_owned(L,r);
  lua_rawgeti(L,LUA_REGISTRYINDEX,source.reference);native_control_base_sort_current_owned(L,r);
  destination.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_sort_current_owned(L,r);
 }
}
static bool native_control_base_sort_isfunction_8f89f0_owned(lua_State*L,NativeControlBaseSortCall&r){
 native_control_base_sort_current_owned(L,r);
 if(!r.incoming.state||r.incoming.reference==LUA_NOREF)return false;
 if(!native_control_base_setoffset_reference_push_8f86d0_owned(L,*r.borrower,r.incoming))return false;
 const bool function=lua_type(L,-1)==LUA_TFUNCTION;native_control_base_sort_current_owned(L,r);
 lua_settop(L,-2);native_control_base_sort_current_owned(L,r);return function;
}
static void native_control_base_sort_member_431410_prefix_owned(lua_State*L,NativeControlBaseSortCall&r){
 native_control_base_sort_current_owned(L,r);r.stage=NativeControlBaseSortCall::Stage::member_entered;
 if(native_control_base_sort_isfunction_8f89f0_owned(L,r)){
  //43142A copy temporary;431433 default ctor;431442 assignment8F8A90.
  //Assignment's destination is genuinely unbound, so its unref branch skips.
  native_control_base_sort_copy_8f84a0_owned(L,r,r.temporary,r.incoming);
  native_control_base_sort_release_owned(L,r,r.comparator);
  native_control_base_sort_copy_8f84a0_owned(L,r,r.comparator,r.temporary);
  r.comparison_state=r.temporary.state; //Actual third comparator word43144D.
  native_control_base_sort_release_owned(L,r,r.temporary);
  r.stage=NativeControlBaseSortCall::Stage::comparator_built;
  const auto count=native_control_base_numchildren_78_08_owned(L,r.borrower->cdata);
  native_control_base_sort_current_owned(L,r);
  if(count){
   //43147B actual byVALUE12 argument, followed by a fresh native78/04/08 read.
   native_control_base_sort_copy_8f84a0_owned(L,r,r.sort_argument,r.comparator);
   const auto fresh_count=native_control_base_numchildren_78_08_owned(L,r.borrower->cdata);
   native_control_base_sort_current_owned(L,r);
   //491023 copies byVALUE comparator into4944C0. Count<=1 really does
   //no comparison/store, but BOTH argument copies and destructors execute.
   native_control_base_sort_copy_8f84a0_owned(L,r,r.algorithm_argument,r.sort_argument);
   r.stage=NativeControlBaseSortCall::Stage::algorithm_entered;
   if(fresh_count>1){
    r.stage=NativeControlBaseSortCall::Stage::required_const_child;
    //4944C0 requires insertion/partition/heap comparisons.5759E0 exports
    //fresh const tables via childv80, NOT permanent mutable Controls/v64.
    //All six actual callback cells are nonNULL in Original construction.
    //The distinct descriptor4 callback and complete positive-comparison
    //algorithm must be supplied before any successful sort return.
    luaL_error(L,"required genuine SortChildren4944C0/5759E0 positive-comparison algorithm and fresh child const-v80 callbacks CB564C/CB5650/CB81E0/CB1A50/CB4600/CAADBC; mutable Controls/v64 is not that supplier");
    std::terminate();
   }
   native_control_base_sort_release_owned(L,r,r.algorithm_argument); //Actual4945EB.
   native_control_base_sort_release_owned(L,r,r.sort_argument); //Actual491048 tail8F8820.
  }
  native_control_base_sort_release_owned(L,r,r.comparator); //Actual43149C.
 }
 native_control_base_sort_release_owned(L,r,r.incoming); //Actual4314A5 even false IsFunction.
 r.stage=NativeControlBaseSortCall::Stage::returned;
}
static int native_control_base_sortchildren_primary_5bf0b0_owned(lua_State*L){
 native_control_base_sort_branch_id_record_current_owned(L,24);
 const int count=lua_gettop(L);native_control_base_sort_branch_id_record_current_owned(L,24);
 if(count!=2){(void)lua_gettop(L);native_control_base_sort_branch_id_record_current_owned(L,24);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E3FD ControlBase::SortChildren receiver-plus-one-argument count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_sort_branch_id_record_current_owned(L,24);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E3E0 ControlBase::SortChildren NULL receiver");
 //8F77F0 retains the actual L/index2 values, not a registry ref yet.
 const bool valid=lua_type(L,2)==LUA_TFUNCTION;native_control_base_sort_branch_id_record_current_owned(L,24);
 if(!valid){
  const int type=lua_type(L,2);native_control_base_sort_branch_id_record_current_owned(L,24);
  (void)lua_typename(L,type);native_control_base_sort_branch_id_record_current_owned(L,24);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"41C8A4 SortChildren function argument type");
 }
 auto*r=native_control_base_sort_begin_owned(L,cdata);
 r->incoming.state=L;r->incoming.reference=LUA_NOREF;
 (void)lua_checkstack(L,1);native_control_base_sort_current_owned(L,*r);
 lua_pushvalue(L,2);native_control_base_sort_current_owned(L,*r);
 r->incoming.reference=luaL_ref(L,LUA_REGISTRYINDEX);native_control_base_sort_current_owned(L,*r); //Actual8F8500.
 r->stage=NativeControlBaseSortCall::Stage::function_retained;
 if(!lua_checkstack(L,32))return luaL_error(L,"SortChildren current receiver borrower needs actual reserve32");
 native_control_base_sort_current_owned(L,*r);native_control_base_sort_member_431410_prefix_owned(L,*r);
 native_control_base_sort_current_owned(L,*r);
 if(r->stage!=NativeControlBaseSortCall::Stage::returned||r->incoming.state||r->temporary.state||
  r->comparator.state||r->sort_argument.state||r->algorithm_argument.state||lua_gettop(L)!=2)
  return luaL_error(L,"SortChildren actual prefix/reference return did not complete");
 return 0; //Only actual NULL/empty78 or<=1 algorithm branch can reach this return.
}

static int native_control_base_branchresetanimation_primary_75eb20_owned(lua_State*L){
 native_control_base_sort_branch_id_record_current_owned(L,25);
 const int count=lua_gettop(L);native_control_base_sort_branch_id_record_current_owned(L,25);
 if(count!=1){(void)lua_gettop(L);native_control_base_sort_branch_id_record_current_owned(L,25);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"75EB70 ControlBase::BranchResetAnimation receiver-only count");}
 const void*const cdata=native_control_base_setoffset_cdata_89e050_owned(L,1);
 native_control_base_sort_branch_id_record_current_owned(L,25);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"75EB53 ControlBase::BranchResetAnimation NULL receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"BranchResetAnimation borrower needs actual reserve32");
 native_control_base_sort_branch_id_record_current_owned(L,25);
 native_control_base_branchreset_member_735aa0_owned(L,cdata);
 native_control_base_sort_branch_id_record_current_owned(L,25);return 0;
}
static const void*native_control_base_const_cdata_89e480_mutable_first_owned(lua_State*L,int offered){
 native_control_base_sort_branch_id_record_current_owned(L,26);
 const int type=lua_type(L,offered);native_control_base_sort_branch_id_record_current_owned(L,26);
 if(type==LUA_TTABLE){
  //Actual89DF00 FIRST calls89DE20. Successful mutable cast returns immediately.
  if(!native_control_base_setoffset_cast_89de20_owned(L,offered)){
   //Original CA8D24 was actually written4ACBF0, so absence of its executable
   //const descriptor4/library callback is not the original NULL branch.
   luaL_error(L,"GetID89DF00 mutable cast failed; required genuine const callback CA8D24/4ACBF0 before fallback traversal or expected const typename");
   std::terminate();
  }
  lua_pushvalue(L,offered);native_control_base_sort_branch_id_record_current_owned(L,26);
  lua_getfield(L,-1,"CData");native_control_base_sort_branch_id_record_current_owned(L,26);
  const void*const cdata=lua_touserdata(L,-1);native_control_base_sort_branch_id_record_current_owned(L,26);
  lua_settop(L,-3);native_control_base_sort_branch_id_record_current_owned(L,26);return cdata;
 }
 const int repeated=lua_type(L,offered);native_control_base_sort_branch_id_record_current_owned(L,26);
 if(repeated==LUA_TLIGHTUSERDATA){const void*const cdata=lua_touserdata(L,offered);
  native_control_base_sort_branch_id_record_current_owned(L,26);return cdata;}
 (void)lua_typename(L,repeated);native_control_base_sort_branch_id_record_current_owned(L,26);
 native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"89E513 const pointer argument type");
}
static int native_control_base_getid_primary_4dd890_owned(lua_State*L){
 native_control_base_sort_branch_id_record_current_owned(L,26);
 const int count=lua_gettop(L);native_control_base_sort_branch_id_record_current_owned(L,26);
 if(count!=1){(void)lua_gettop(L);native_control_base_sort_branch_id_record_current_owned(L,26);
  native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4DD906 ControlBase::GetID receiver-only count");}
 const void*const cdata=native_control_base_const_cdata_89e480_mutable_first_owned(L,1);
 native_control_base_sort_branch_id_record_current_owned(L,26);
 if(!cdata)native_control_base_setoffset_diagnostic_8f7960_required_owned(L,"4DD8E9 ControlBase::GetID NULL receiver");
 if(!lua_checkstack(L,32))return luaL_error(L,"GetID borrower needs actual reserve32");
 native_control_base_sort_branch_id_record_current_owned(L,26);
 const auto observed=native_control_base_getid_member58_owned(L,cdata);
 static_assert(std::is_trivially_destructible_v<decltype(observed)>);
 native_control_base_sort_branch_id_record_current_owned(L,26);
 lua_pushstring(L,observed.bytes.data());native_control_base_sort_branch_id_record_current_owned(L,26);return 1;
}

static int native_control_base_sortchildren_signature_6c2210_owned(lua_State*L){
 native_control_base_sort_branch_id_record_current_owned(L,24);char buffer[256]{};
 (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( %s%s )","void","","SortChildren","function","");
 native_control_base_sort_branch_id_record_current_owned(L,24);lua_pushstring(L,buffer);
 native_control_base_sort_branch_id_record_current_owned(L,24);return 1;
}
static int native_control_base_branchresetanimation_signature_5404b0_owned(lua_State*L){
 native_control_base_sort_branch_id_record_current_owned(L,25);char buffer[256]{};
 (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void )","void","","BranchResetAnimation");
 native_control_base_sort_branch_id_record_current_owned(L,25);lua_pushstring(L,buffer);
 native_control_base_sort_branch_id_record_current_owned(L,25);return 1;
}
static int native_control_base_getid_signature_685430_owned(lua_State*L){
 native_control_base_sort_branch_id_record_current_owned(L,26);char buffer[256]{};
 (void)::sprintf_s(buffer,sizeof(buffer),"%s%s %s( void ) const","const char","*","GetID");
 native_control_base_sort_branch_id_record_current_owned(L,26);lua_pushstring(L,buffer);
 native_control_base_sort_branch_id_record_current_owned(L,26);return 1;
}
void native_control_base_sort_retire()noexcept{
 for(const auto&r:native_control_base_sort_calls)if(r){
  if(r->host!=this||r->creator!=std::this_thread::get_id())std::terminate();
  r->alive=false;r->stage=NativeControlBaseSortCall::Stage::retired;
 }
 //Partial genuine registry refs remain in SAME Host custody until lua_close.
}
