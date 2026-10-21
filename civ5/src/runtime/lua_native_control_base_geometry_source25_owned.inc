//Inside the actual Host::Impl after Source10's genuine current Base supplier.
//All allocating argument extraction/ref destruction has finished before entry.
//Read the permanent Controls owner; RootRead's local named_ref was genuinely
//released by8F8820 and cannot grant callback-time geometry or currentness.
static bool native_control_base_setoffset_result_physical_without_callbacks(
 Impl&p,lua_State*L,const NativeRootOrdinaryControlsResult&r){
 if(!L||!p.state||G(L)!=G(p.state)||L->ci->top-L->top<24||
  r.slot>=p.child_contexts.size()||!r.concrete||r.cdata!=r.concrete.get()||
  r.keys_ref<0||r.table_ref<0||r.metatable_ref<0||r.types_ref<0||!r.keys||!r.table||!r.metatable||!r.types||
  !r.metatype_owner||!r.cdata_creator||
  r.stage!=NativeRootOrdinaryControlsResult::Stage::returned)return false;
 const auto& current=p.child_contexts[r.slot];const auto& type=*r.metatype_owner;
 if(current.retired||!current.state_prepared||current.environment!=r.environment||
  current.execution_thread!=r.thread||current.source!=r.parent_source||
  current.controls_table_ref<0||current.controls_table_pointer!=r.parent_table||
  type.host!=&p||type.creator!=std::this_thread::get_id()||!type.alive.load()||
  type.stage!=NativeOrdinaryMetatypeResult::Stage::returned||type.table!=r.metatable||
  type.table_ref<0||!type.master||type.master->cache!=r.types||!type.descriptor||
  type.descriptor->native_creator!=r.cdata_creator||type.name!=r.cdata_creator->type_name||
  type.name_operand!=NativeOrdinaryMetatypeResult::Name::descriptor0)return false;
 const auto index=static_cast<std::size_t>(r.cdata_creator->subtype);
 if(index>=6||p.native_ordinary_type_descriptors[index]!=type.descriptor||
  type.descriptor->host!=&p||!type.descriptor->alive.load())return false;
 for(std::size_t i=0;i<3;++i){const auto&fn=type.functions[i];
  if(!fn||fn!=p.native_ordinary_517c_functions[i]||fn->host!=&p||!fn->alive.load()||
   fn->creator!=std::this_thread::get_id()||static_cast<std::size_t>(fn->original)!=i||!fn->body)return false;}
 const int top=lua_gettop(L);
 const auto stop=[&](bool value){lua_settop(L,top);return value;};
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.thread);
 if(lua_type(L,-1)!=LUA_TTHREAD||lua_tothread(L,-1)!=r.actual_thread)return stop(false);
 lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,r.environment);const int environment=lua_gettop(L);
 if(lua_type(L,environment)!=LUA_TTABLE||lua_topointer(L,environment)!=r.actual_environment||
  !lua_rawequal(L,-1,-2))return stop(false);
 //Existing pure field scanner never interns strings or calls __index. A
 //running genuine518 thread need not have the old pre-execution empty stack.
 if(!native_entered_push_field_without_callbacks(L,environment,"Controls",8)||
  lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.parent_table)return stop(false);
 lua_rawgeti(L,LUA_REGISTRYINDEX,current.controls_table_ref);
 if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.parent_table||!lua_rawequal(L,-1,-2))return stop(false);
 const int controls=lua_gettop(L);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.keys_ref);const int keys=lua_gettop(L);
 if(lua_type(L,keys)!=LUA_TTABLE||lua_topointer(L,keys)!=r.keys||lua_objlen(L,keys)!=4)return stop(false);
 const char*const expected[]={r.id.data(),"CData","ExposedTypes",r.cdata_creator->type_name};
 for(int i=0;i<4;++i){lua_rawgeti(L,keys,i+1);
  if(lua_type(L,-1)!=LUA_TSTRING)return stop(false);
  std::size_t length=0;const char*const bytes=lua_tolstring(L,-1,&length);
  if(!bytes||length!=std::strlen(expected[i])||std::memcmp(bytes,expected[i],length))return stop(false);
  lua_pop(L,1);}
 lua_rawgeti(L,keys,1);lua_rawget(L,controls);lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);
 if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.table||!lua_rawequal(L,-1,-2))return stop(false);
 const int actual=lua_gettop(L);
 lua_rawgeti(L,keys,2);lua_rawget(L,actual);
 if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA||lua_touserdata(L,-1)!=r.cdata)return stop(false);lua_pop(L,1);
 if(!lua_getmetatable(L,actual))return stop(false);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatable_ref);
 if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.metatable||!lua_rawequal(L,-1,-2))return stop(false);lua_pop(L,2);
 //Borrow the actual retained metatype/master instances. This is not a claim
 //to whole library/global readiness;89E050 consumed current BaseType first.
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.types_ref);const int types=lua_gettop(L);
 if(!native_ordinary_master_physical_without_callbacks(p,L,*type.master,types))return stop(false);
 lua_rawgeti(L,keys,4);lua_rawget(L,types);lua_rawgeti(L,LUA_REGISTRYINDEX,r.metatable_ref);
 if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.metatable||!lua_rawequal(L,-1,-2))return stop(false);
 return stop(true);
}

struct NativeControlBaseOffsetX final {float value;};
struct NativeControlBaseOffsetY final {float value;};
struct NativeControlBaseOffsetPair69 final {std::array<float,2> value;};
struct NativeControlBaseAnchor final {const char* value;};
struct NativeControlBaseBooleanFlag final {ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite site;bool value;};
struct NativeControlBaseUintRead final {ui::BnwWorldViewControlTree::NativeOrdinaryUintRead read;};
struct NativeControlBasePodRead final {
 std::optional<ui::BnwWorldViewControlTree::NativeOrdinaryIDRead> id58;std::optional<double> fp64;std::optional<float> fp32;std::optional<std::uint32_t> uint32;std::optional<std::array<float,2>> vector2;};
static_assert(std::is_trivially_destructible_v<NativeControlBasePodRead>);
struct NativeControlBaseOffsetRead final {ui::BnwWorldViewControlTree::NativeOrdinaryOffsetRead read;};
struct NativeControlBaseReprocessAnchoring final {};
struct NativeControlBaseSizeVectorRead final {};
struct NativeControlBaseSizeWrite final {std::array<float,2> value;};
struct NativeControlBaseSizeX final {float value;};
struct NativeControlBaseSizeY final {float value;};
struct NativeControlBaseColorWrite final {std::array<float,4> value;};
struct NativeControlBaseAlphaWrite final {float value;};
struct NativeControlBaseAlphaRead final {};
struct NativeControlBaseBranchReset final {};
struct NativeControlBaseIDRead final {};
struct NativeControlBaseDisabledWrite final {bool value;};
struct NativeControlBaseVisibilityCalculate final {};
struct NativeControlBaseTooltipWrite final {ui::BnwWorldViewControlTree::NativeOrdinaryTooltipWrite member;const char* input;int argument_top;bool rooted_argument;};
struct NativeControlBaseTooltipRead final {std::optional<std::string>* output;};
struct NativeControlBaseChildrenCommand final {ui::BnwWorldViewControlTree::NativeOrdinaryChildrenMember member;const void* argument;};
struct NativeControlBaseSizeAxisRead final {ui::BnwWorldViewControlTree::NativeOrdinarySizeRead read;};
using NativeControlBaseOffsetCommand=std::variant<std::array<float,2>,NativeControlBaseOffsetX,
 NativeControlBaseOffsetY,NativeControlBaseOffsetPair69,NativeControlBaseOffsetRead,NativeControlBaseAnchor,NativeControlBaseBooleanFlag,NativeControlBaseUintRead,NativeControlBaseReprocessAnchoring,NativeControlBaseSizeVectorRead,NativeControlBaseSizeAxisRead,NativeControlBaseSizeWrite,NativeControlBaseSizeX,NativeControlBaseSizeY,NativeControlBaseColorWrite,NativeControlBaseAlphaWrite,NativeControlBaseAlphaRead,NativeControlBaseBranchReset,NativeControlBaseIDRead,NativeControlBaseDisabledWrite,NativeControlBaseVisibilityCalculate,NativeControlBaseTooltipWrite,NativeControlBaseTooltipRead,NativeControlBaseChildrenCommand>;
static_assert(std::is_trivially_destructible_v<NativeControlBaseOffsetCommand>);
static_assert(std::is_trivially_destructible_v<std::optional<float>>);
static NativeControlBasePodRead native_control_base_offset_command_member_owned(
 lua_State*L,const void*resolved_cdata,const NativeControlBaseOffsetCommand& command){
 //This FULL converts its own exceptions/longjmp before C++ borrowers exist.
 native_ordinary_metatype_full(L);auto&p=owner(L);p.source_exception={};p.source_error=nullptr;
 NativeControlBasePodRead observed; //OWN disengaged results, never invented native fields.
 try{
  const auto self=p.weak_from_this().lock();const auto source=p.worldview_sources.lock();
  const auto life=p.worldview_lifetime.lock();p.worldview_require_callback_origin();
  if(!self||self.get()!=&p||!source||!life||life!=p.end_turn_lifetime||!life->alive||life->failed||
   life->vm!=p.state||life->host!=p.worldview_host_owner||!L||G(L)!=G(p.state)||!resolved_cdata)
   throw ScriptError("SetOffset current genuine Host/VM/source/physical CData borrower unavailable");
  //Fresh canonical publication FIRST. Never dereference r.operation, whose
  //historical export scope need not remain the active operation after v64.
  const auto&operation=source->current_operation_loan();p.worldview_require_callback_origin();
  if(&operation!=p.worldview_operation||!p.worldview_native_host||p.worldview_native_host->loan!=&operation)
   throw ScriptError("SetOffset current Source operation changed before physical receiver borrow");
  std::shared_ptr<NativeRootOrdinaryControlsResult> result;
  for(const auto&actual:p.root_ordinary_controls_export_results)
   if(actual&&actual->cdata==resolved_cdata&&actual->stage==NativeRootOrdinaryControlsResult::Stage::returned){
    if(result&&result!=actual)throw ScriptError("SetOffset ambiguous actual CData owning return");result=actual;}
  if(!result)throw ScriptError("SetOffset resolved pointer has no genuine retained ordinary Controls return");
  const auto tree=result->tree.lock();
  if(!tree)throw ScriptError("SetOffset real canonical Tree retired before mutable receiver borrow");
  const auto pure=[&]{
   p.worldview_require_callback_origin();
   if(p.weak_from_this().lock()!=self||p.worldview_sources.lock()!=source||p.worldview_lifetime.lock()!=life||
    !life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner||
    result->host.lock()!=self||result->creator!=std::this_thread::get_id()||result->creator!=p.ui_thread||
    result->stage!=NativeRootOrdinaryControlsResult::Stage::returned||result->epoch!=p.failure_epoch||
    result->writes!=p.failed.writes_||result->uncertainty!=p.execution_uncertainty||
    result->cdata!=resolved_cdata||result->concrete.get()!=resolved_cdata||result->tree.lock()!=tree||
    !result->receiver||!result->parent_source||!result->cdata_creator||
    !p.worldview_native_host||p.worldview_native_host->loan!=&operation||
    p.worldview_native_host->child!=result->slot||!p.worldview_native_child_controls_current(result->slot)||
    p.worldview_native_host->child_source.lock()!=result->parent_source||
    std::none_of(p.root_ordinary_controls_export_results.begin(),p.root_ordinary_controls_export_results.end(),
     [&](const auto&kept){return kept==result;}))
    throw ScriptError("SetOffset SAME active Source/returned CData/receiver custody changed");
   const auto index=static_cast<std::size_t>(result->cdata_creator->subtype);
   if(index>=6||p.native_ordinary_cdata_creators[index]!=result->cdata_creator||
    result->cdata_creator->host!=&p||!result->cdata_creator->alive.load()||
    result->cdata_creator->creator!=p.ui_thread||!result->cdata_creator->push_metatype||
    !native_control_base_setoffset_result_physical_without_callbacks(p,L,*result))
    throw ScriptError("SetOffset current permanent Controls/CData/518/metatype physical borrower failed");
   tree->require_native_ordinary_geometry_receiver_without_callbacks(operation,result->receiver,result->concrete);
   p.worldview_require_callback_origin();
  };
  pure();operation.require_same_vm(*p.worldview_host_owner);pure();
  operation.require_same_source(*result->parent_source);pure();
  //Only the Tree member opens busy/View. No allocating Lua API or luaL_error
  //is called while these C++ physical/source owners are alive.
  using Axis=ui::BnwWorldViewControlTree::NativeOrdinaryOffsetAxis;
  if(const auto*vector=std::get_if<std::array<float,2>>(&command))
   tree->set_native_ordinary_offset_4c8fe0_owned(operation,result->receiver,result->concrete,*vector);
  else if(const auto*x=std::get_if<NativeControlBaseOffsetX>(&command))
   tree->set_native_ordinary_offset_axis_owned(operation,result->receiver,result->concrete,Axis::x_4a8380,x->value);
  else if(const auto*y=std::get_if<NativeControlBaseOffsetY>(&command))
   tree->set_native_ordinary_offset_axis_owned(operation,result->receiver,result->concrete,Axis::y_6158f0,y->value);
  else if(const auto*pair=std::get_if<NativeControlBaseOffsetPair69>(&command))
   tree->set_native_ordinary_offset_69cd90_owned(operation,result->receiver,result->concrete,pair->value);
  else if(const auto*read=std::get_if<NativeControlBaseOffsetRead>(&command))
   observed.fp32=tree->read_native_ordinary_offset_owned(operation,result->receiver,result->concrete,read->read);
  else if(const auto*anchor=std::get_if<NativeControlBaseAnchor>(&command)){
   //Actual lua_tolstring result is rooted in slot2. Check its physical
   //identity after EACH Tree Source callback before dereferencing it; a
   //pre-decoded filename/string cache does not substitute this Lua input.
   const auto rooted=[&]{
    p.worldview_require_callback_origin();
    if(!L||!p.state||G(L)!=G(p.state)||lua_gettop(L)!=2)
     throw ScriptError("SetAnchor actual SAME Lua argument frame retired");
    const int kind=lua_type(L,2);
    if(anchor->value){
     //lua_tolstring of an already STRING does not convert/allocate/longjmp.
     if(kind!=LUA_TSTRING||lua_tolstring(L,2,nullptr)!=anchor->value)
      throw ScriptError("SetAnchor actual rooted TString result changed");
    }else if(kind==LUA_TSTRING||kind==LUA_TNUMBER)
     throw ScriptError("SetAnchor genuine NULL tolstring result replaced");
    p.worldview_require_callback_origin();
   };
   rooted();tree->set_native_ordinary_anchor_51e6a0_owned(operation,result->receiver,result->concrete,anchor->value,rooted);rooted();
  }
  else if(const auto*flag=std::get_if<NativeControlBaseBooleanFlag>(&command))
   tree->set_native_ordinary_boolean_flag_owned(operation,result->receiver,result->concrete,flag->site,flag->value);
  else if(const auto*read=std::get_if<NativeControlBaseUintRead>(&command)){
   if(read->read==ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::mouse_over_42_bit0_669850&&
    p.worldview_native_input_entered)
    throw ScriptError("HasMouseOver after actual input dispatch requires SAME canonical hover enter/exit lifecycle writer; retained pre-input flags40 cannot replace that route");
   observed.uint32=tree->read_native_ordinary_uint_owned(operation,result->receiver,result->concrete,read->read);
  }
  else if(std::holds_alternative<NativeControlBaseReprocessAnchoring>(command))
   tree->reprocess_native_ordinary_anchoring_4a7f10_owned(operation,result->receiver,result->concrete);
  else if(std::holds_alternative<NativeControlBaseSizeVectorRead>(command))
   observed.vector2=tree->read_native_ordinary_size_a4_owned(operation,result->receiver,result->concrete);
  else if(const auto* read=std::get_if<NativeControlBaseSizeAxisRead>(&command))
   observed.fp32=tree->read_native_ordinary_size_axis_owned(operation,result->receiver,result->concrete,read->read);
  else if(const auto* write=std::get_if<NativeControlBaseSizeWrite>(&command))
   tree->set_native_ordinary_size98_owned(operation,result->receiver,result->concrete,write->value);
  else if(const auto* x=std::get_if<NativeControlBaseSizeX>(&command))
   tree->set_native_ordinary_size_axis_owned(operation,result->receiver,result->concrete,
    ui::BnwWorldViewControlTree::NativeOrdinarySizeAxis::x_4e99c0,x->value);
  else if(const auto* y=std::get_if<NativeControlBaseSizeY>(&command))
   tree->set_native_ordinary_size_axis_owned(operation,result->receiver,result->concrete,
    ui::BnwWorldViewControlTree::NativeOrdinarySizeAxis::y_68a240,y->value);
  else if(const auto* write=std::get_if<NativeControlBaseColorWrite>(&command))
   tree->set_native_ordinary_color58_owned(operation,result->receiver,result->concrete,write->value);
  else if(const auto* write=std::get_if<NativeControlBaseAlphaWrite>(&command))
   tree->set_native_ordinary_alpha9c_owned(operation,result->receiver,result->concrete,write->value);
  else if(std::holds_alternative<NativeControlBaseAlphaRead>(command))
   observed.fp64=tree->read_native_ordinary_alpha_a0_owned(operation,result->receiver,result->concrete);
  else if(std::holds_alternative<NativeControlBaseBranchReset>(command))
   tree->branch_reset_native_ordinary_animation54_owned(operation,result->receiver,result->concrete);
  else if(std::holds_alternative<NativeControlBaseIDRead>(command))
   observed.id58=tree->read_native_ordinary_id58_owned(operation,result->receiver,result->concrete);
  else if(const auto* write=std::get_if<NativeControlBaseDisabledWrite>(&command))
   tree->set_native_ordinary_disabled88_owned(operation,result->receiver,result->concrete,write->value);
  else if(std::holds_alternative<NativeControlBaseVisibilityCalculate>(command))
   tree->calculate_native_ordinary_visibility94_owned(operation,result->receiver,result->concrete);
  else if(const auto* write=std::get_if<NativeControlBaseTooltipWrite>(&command)){
   const auto rooted=[&]{
    p.worldview_require_callback_origin();
    if(!L||!p.state||G(L)!=G(p.state)||lua_gettop(L)!=write->argument_top)
     throw ScriptError("Tooltip SAME actual Lua argument frame retired");
    if(write->rooted_argument){
     const int kind=lua_type(L,2);
     if(write->input){
      if(kind!=LUA_TSTRING||lua_tolstring(L,2,nullptr)!=write->input)
       throw ScriptError("Tooltip actual rooted TString result changed");
     }else if(kind==LUA_TSTRING||kind==LUA_TNUMBER)
      throw ScriptError("Tooltip genuine NULL tolstring argument changed");
    }
    p.worldview_require_callback_origin();
   };
   rooted();tree->write_native_ordinary_tooltip_owned(operation,result->receiver,result->concrete,write->member,write->input,rooted);rooted();
  }else if(const auto* read=std::get_if<NativeControlBaseTooltipRead>(&command)){
   if(!read->output)throw ScriptError("GetToolTipString persistent actual output owner absent");
   tree->read_native_ordinary_tooltip54_owned(operation,result->receiver,result->concrete,*read->output);
  }else if(const auto* children=std::get_if<NativeControlBaseChildrenCommand>(&command)){
   tree->execute_native_ordinary_children_owned(operation,result->receiver,result->concrete,children->member,children->argument);
  }else throw ScriptError("Required real executing native ControlBase command");
  pure();operation.require_same_vm(*p.worldview_host_owner);pure();
  operation.require_same_source(*result->parent_source);pure();
 }catch(...){p.source_exception=std::current_exception();p.source_error="SetOffset genuine canonical geometry/required virtual return refused";}
 //All borrowers above have ended before converting a failure to Lua longjmp.
 if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
 return observed; //Only successful actual current getter fills it; writers return no numeric value.
}

static void native_control_base_setoffset_member_4c8fe0_owned(
 lua_State*L,const void*resolved_cdata,const std::array<float,2>& value){
 const NativeControlBaseOffsetCommand command=value;
 native_control_base_offset_command_member_owned(L,resolved_cdata,command);
}
static void native_control_base_offset_axis_member_owned(
 lua_State*L,const void*resolved_cdata,bool x_axis,float value){
 //The two actual CFunctions supply this identity. It is never readiness or
 //a substituted partner value; the Tree reads actual current74/70 itself.
 if(x_axis){const NativeControlBaseOffsetCommand command=NativeControlBaseOffsetX{value};
  native_control_base_offset_command_member_owned(L,resolved_cdata,command);}
 else {const NativeControlBaseOffsetCommand command=NativeControlBaseOffsetY{value};
  native_control_base_offset_command_member_owned(L,resolved_cdata,command);}
}
static void native_control_base_offset_pair_69cd90_owned(
 lua_State*L,const void*resolved_cdata,const std::array<float,2>& value){
 const NativeControlBaseOffsetCommand command=NativeControlBaseOffsetPair69{value};
 native_control_base_offset_command_member_owned(L,resolved_cdata,command);
}

static float native_control_base_offset_getter_member_owned(lua_State*L,const void*resolved_cdata,
 ui::BnwWorldViewControlTree::NativeOrdinaryOffsetRead read){
 const NativeControlBaseOffsetCommand command=NativeControlBaseOffsetRead{read};
 const auto observed=native_control_base_offset_command_member_owned(L,resolved_cdata,command);
 if(!observed.fp32){auto&p=owner(L);p.record_failure();luaL_error(L,"Required genuine observed current offset getter result");std::terminate();}
 return *observed.fp32;
}
static float native_control_base_getoffset_x_b0_owned(lua_State*L,const void*resolved_cdata){
 return native_control_base_offset_getter_member_owned(L,resolved_cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryOffsetRead::virtual_b0_5b5b50);
}
static float native_control_base_getoffset_y_b4_owned(lua_State*L,const void*resolved_cdata){
 return native_control_base_offset_getter_member_owned(L,resolved_cdata,
  ui::BnwWorldViewControlTree::NativeOrdinaryOffsetRead::virtual_b4_749a00);
}
static float native_control_base_getoffset_val_axis_owned(lua_State*L,const void*resolved_cdata,bool x_axis){
 //Actual6486F0 performs separate raw reads70 then74, with Lua pushX between.
 //No two-value snapshot is made before pushX; no raw native pointer escapes.
 return native_control_base_offset_getter_member_owned(L,resolved_cdata,x_axis?
  ui::BnwWorldViewControlTree::NativeOrdinaryOffsetRead::direct_70_6486f0:
  ui::BnwWorldViewControlTree::NativeOrdinaryOffsetRead::direct_74_6486f0);
}

static void native_control_base_setanchor_member_51e6a0_owned(
 lua_State*L,const void*resolved_cdata,const char*borrowed_lua_string){
 const NativeControlBaseOffsetCommand command=NativeControlBaseAnchor{borrowed_lua_string};
 native_control_base_offset_command_member_owned(L,resolved_cdata,command);
}

static void native_control_base_sethide_member_6545c0_owned(lua_State*L,const void*cdata,bool value){
 const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::hide_v50_4ccec0,value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setnoclip_member_72c700_owned(lua_State*L,const void*cdata,bool value){
 const NativeControlBaseOffsetCommand command=NativeControlBaseBooleanFlag{
  ui::BnwWorldViewControlTree::NativeOrdinaryFlagWrite::no_clip_72c700,value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static std::uint32_t native_control_base_uint_member_owned(lua_State*L,const void*cdata,
 ui::BnwWorldViewControlTree::NativeOrdinaryUintRead read){
 const NativeControlBaseOffsetCommand command=NativeControlBaseUintRead{read};
 const auto observed=native_control_base_offset_command_member_owned(L,cdata,command);
 if(!observed.uint32){auto&p=owner(L);p.record_failure();luaL_error(L,"Required real observed unsigned getter result");std::terminate();}
 return *observed.uint32;
}
static bool native_control_base_ishidden_flag40_owned(lua_State*L,const void*cdata){
 return native_control_base_uint_member_owned(L,cdata,ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::hidden_40)!=0u;
}
static std::uint32_t native_control_base_numchildren_78_08_owned(lua_State*L,const void*cdata){
 return native_control_base_uint_member_owned(L,cdata,ui::BnwWorldViewControlTree::NativeOrdinaryUintRead::children_78_08);
}

static void native_control_base_reprocessanchoring_member_4a7f10_owned(lua_State*L,const void*cdata){
 const NativeControlBaseOffsetCommand command=NativeControlBaseReprocessAnchoring{};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static std::array<float,2> native_control_base_getsize_member_5f4190_owned(lua_State*L,const void*cdata){
 const NativeControlBaseOffsetCommand command=NativeControlBaseSizeVectorRead{};
 const auto observed=native_control_base_offset_command_member_owned(L,cdata,command);
 if(!observed.vector2){auto&p=owner(L);p.record_failure();luaL_error(L,"Required genuine GetSize current30/34 output-buffer return");std::terminate();}
 return *observed.vector2;
}
static float native_control_base_size_axis_member_owned(lua_State*L,const void*cdata,
 ui::BnwWorldViewControlTree::NativeOrdinarySizeRead read){
 const NativeControlBaseOffsetCommand command=NativeControlBaseSizeAxisRead{read};
 const auto observed=native_control_base_offset_command_member_owned(L,cdata,command);
 if(!observed.fp32){auto&p=owner(L);p.record_failure();luaL_error(L,"Required genuine observed current size axis");std::terminate();}
 return *observed.fp32;
}
static float native_control_base_getsize_x_member_65b6c0_owned(lua_State*L,const void*cdata){
 return native_control_base_size_axis_member_owned(L,cdata,ui::BnwWorldViewControlTree::NativeOrdinarySizeRead::virtual_a8_691d40);
}
static float native_control_base_getsize_y_member_5bf320_owned(lua_State*L,const void*cdata){
 return native_control_base_size_axis_member_owned(L,cdata,ui::BnwWorldViewControlTree::NativeOrdinarySizeRead::virtual_ac_428990);
}
static float native_control_base_getsize_val_axis_owned(lua_State*L,const void*cdata,bool x_axis){
 //Original467420 reads30 then pushesX before a FRESH34 read. Each call
 //borrows exactly one current physical cell; it never preloads both values.
 return native_control_base_size_axis_member_owned(L,cdata,x_axis?
  ui::BnwWorldViewControlTree::NativeOrdinarySizeRead::direct_30_467420:
  ui::BnwWorldViewControlTree::NativeOrdinarySizeRead::direct_34_467420);
}

static void native_control_base_setsize_member_7a5510_owned(lua_State*L,const void*cdata,
 const std::array<float,2>& value){
 //Actual89E920/7A5510 copies two FP32 arguments into an8byte byVALUE block.
 //Only this portable private interface lends the already copied POD.
 const NativeControlBaseOffsetCommand command=NativeControlBaseSizeWrite{value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setsize_x_member_4e99c0_owned(lua_State*L,const void*cdata,float value){
 const NativeControlBaseOffsetCommand command=NativeControlBaseSizeX{value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setsize_y_member_68a240_owned(lua_State*L,const void*cdata,float value){
 const NativeControlBaseOffsetCommand command=NativeControlBaseSizeY{value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setsize_val_member_4ab4f0_owned(lua_State*L,const void*cdata,
 const std::array<float,2>& value){
 //Actual4AB4F0 directly JMPs SAMEvptr98, with its two copied FP32 operands.
 const NativeControlBaseOffsetCommand command=NativeControlBaseSizeWrite{value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}

static void native_control_base_setcolor_member_6d19f0_owned(lua_State*L,const void*cdata,
 const std::array<float,4>& value){
 //Actual6D19F0 JMPv58; original ABI is four FP32s byVALUE16, not a pointer.
 const NativeControlBaseOffsetCommand command=NativeControlBaseColorWrite{value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static void native_control_base_setalpha_member_601c30_owned(lua_State*L,const void*cdata,float value){
 //Actual601C30 JMPv9C with one already-converted FP32 byVALUE4.
 const NativeControlBaseOffsetCommand command=NativeControlBaseAlphaWrite{value};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static double native_control_base_getalpha_member_6785a0_owned(lua_State*L,const void*cdata){
 const NativeControlBaseOffsetCommand command=NativeControlBaseAlphaRead{};
 const auto observed=native_control_base_offset_command_member_owned(L,cdata,command);
 if(!observed.fp64){auto&p=owner(L);p.record_failure();luaL_error(L,"Required actual GetAlpha729400/89E222 observed result");std::terminate();}
 return *observed.fp64;
}

static void native_control_base_branchreset_member_735aa0_owned(lua_State*L,const void*cdata){
 const NativeControlBaseOffsetCommand command=NativeControlBaseBranchReset{};
 native_control_base_offset_command_member_owned(L,cdata,command);
}
static ui::BnwWorldViewControlTree::NativeOrdinaryIDRead native_control_base_getid_member58_owned(lua_State*L,const void*cdata){
 const NativeControlBaseOffsetCommand command=NativeControlBaseIDRead{};
 const auto observed=native_control_base_offset_command_member_owned(L,cdata,command);
 if(!observed.id58){auto&p=owner(L);p.record_failure();luaL_error(L,"GetID requires real current58 observed storage");std::terminate();}
 return *observed.id58;
}
