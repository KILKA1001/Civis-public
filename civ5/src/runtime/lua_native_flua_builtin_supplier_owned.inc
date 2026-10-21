// Executing x64 supplier for original fresh-session31 descriptor writers,
// 517C20/4E35C0 constructors, 47C500/663140 and the live761D30 registry.
// All Lua roots have Host custody before allocating APIs. Original reference
// slot+4 is untouched; private retained registry references are separate.
#include "lua_native_flua_builtin_writer_recipes.inc"
class NativeFLuaBuiltinRegistryOwner final {
 friend struct Impl;
 struct Descriptor final {
  const NativeBuiltin31Recipe*recipe=nullptr;bool constructed=false,named=false;
  const char*name0=nullptr;std::array<char,64>name4{};
  std::uint32_t current548=0,current54c=0,count544=0;
  std::array<const NativeBuiltin31MethodRecipe*,49>records{};
 };
 Impl*const host;const ScriptSourceProvider*const source;const std::thread::id creator;
 const std::uint64_t epoch,writes,uncertainty;
 lua_State*master=nullptr;global_State*actual_global=nullptr;const Table*lua_registry=nullptr;
 enum class Phase {fresh_image,vm_bound,retired};Phase phase=Phase::fresh_image;
 std::array<Descriptor,31>descriptors{};
 const std::shared_ptr<NativeDefinitionDeferredQueue>queue;
 std::array<const NativeBuiltin31Recipe*,1024>callback_recipes{};
 std::uint64_t actual_readers=0,actual_writers=0,lazy_initializers=0;
 explicit NativeFLuaBuiltinRegistryOwner(Impl&p):host(&p),source(&p.sources),creator(std::this_thread::get_id()),
  epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty),queue(p.native_definition_deferred_queue){}
 void physical_current_without_callbacks()const {
  if(!host||host->native_flua_builtin_registry.get()!=this||source!=&host->sources||phase!=Phase::vm_bound||
   creator!=std::this_thread::get_id()||creator!=host->ui_thread||epoch!=host->failure_epoch||
   writes!=host->failed.writes_||uncertainty!=host->execution_uncertainty||host->failed.exhausted_||
   !master||master!=host->state||G(master)!=actual_global||!ttistable(&actual_global->l_registry)||
   hvalue(&actual_global->l_registry)!=lua_registry||queue!=host->native_definition_deferred_queue||!queue)
   throw ScriptError("FLua builtin registry actual Source/VM/queue retired");
  queue->require_origin();
 }
};
struct NativeFLuaMetatypeReturn final {
 const Impl*const host;const std::thread::id creator=std::this_thread::get_id();bool alive=true;
 const NativeBuiltin31Recipe*const recipe;const bool is_const;
 const void*table=nullptr;int table_ref=LUA_NOREF;bool returned=false;
 std::shared_ptr<NativeOrdinaryMasterTypesResult>master;
 const void*parent=nullptr;
 NativeFLuaMetatypeReturn(const Impl*p,const NativeBuiltin31Recipe*r,bool c):host(p),recipe(r),is_const(c){}
};
struct NativeFLuaMethodLibraryReturn final {
 struct Row final {const NativeBuiltin31MethodRecipe*recipe=nullptr;lua_CFunction primary=nullptr,secondary=nullptr;
  const void*primary_value=nullptr,*secondary_value=nullptr;int primary_ref=LUA_NOREF,secondary_ref=LUA_NOREF,key_ref=LUA_NOREF;
  std::shared_ptr<const NativeOrdinaryCodeBuddyTableReturn>codebuddy;};
 const Impl*const host;const std::thread::id creator=std::this_thread::get_id();bool alive=true;
 const NativeBuiltin31Recipe*const recipe;const bool is_const;const void*const metatype;
 const void*table=nullptr;int metatype_ref=LUA_NOREF,table_ref=LUA_NOREF;std::array<Row,49>rows{};
 std::size_t assigned=0;bool returned=false;
 NativeFLuaMethodLibraryReturn(const Impl*p,const NativeBuiltin31Recipe*r,bool c,const void*t):host(p),recipe(r),is_const(c),metatype(t){}
};
static const NativeBuiltin31Recipe*native_flua_builtin_recipe_owned(std::uint32_t descriptor){
 for(const auto&r:native_builtin31_recipes)if(r.descriptor==descriptor)return &r;
 return nullptr;
}
template<std::uint32_t Descriptor>static void native_flua_builtin_queue_callback_owned(lua_State*L){
 const auto*r=native_flua_builtin_recipe_owned(Descriptor);
 const auto body=r?native_builtin31_callback_body_owned(r->original_callback):nullptr;
 if(!body)return (void)native_master_thread_refuse(L,"FLua live builtin callback identity lost its genuine body");
 body(L,-1);
}
static void(*native_flua_builtin_queue_body_owned(std::uint32_t descriptor))(lua_State*){
 // Generated direct bindings are emitted below; no Original pointer is cast.
#include "lua_native_flua_builtin_queue_bindings.inc"
 return nullptr;
}
void initialize_native_flua_builtin_registry_before_vm_owned(){
 if(native_flua_builtin_registry||state||calling||failed.exhausted_||std::this_thread::get_id()!=ui_thread||
  !native_definition_deferred_queue||!native_control_base_constructor)
  throw ScriptError("FLua builtin writers require the actual fresh pre-VM Host");
 native_control_base_constructor->require_returned();
 native_flua_builtin_registry.reset(new NativeFLuaBuiltinRegistryOwner(*this));
 auto&actual=*native_flua_builtin_registry;
 for(std::size_t i=0;i<std::size(native_builtin31_recipes);++i)actual.descriptors[i].recipe=&native_builtin31_recipes[i];
 // Preserve the genuine earlier ControlBase registration object and actor;
 // publish the proven selected CRT callback order into this SAME live queue.
 // Source28's one-row constructor proof is historical, not current count31.
 auto base_entry=std::move(actual.queue->entries[0]);
 if(actual.queue->count!=1||!base_entry||base_entry.get()!=native_control_base_constructor->registration||
  base_entry->original_callback!=0x7a4df0u)throw ScriptError("FLua fresh queue adoption lost genuine Source28 ControlBase registration");
 actual.queue->flags=0;actual.queue->count=0;
 for(const auto&writer:native_flua_builtin_writers){
  const auto*recipe=native_flua_builtin_recipe_owned(writer.descriptor);
  if(!recipe)throw ScriptError("FLua qualified selected writer descriptor missing");
  auto&d=actual.descriptors[static_cast<std::size_t>(recipe-native_builtin31_recipes)];
  if(!d.constructed){
   d.count544=0;d.current548=0;d.current54c=0;d.name0="Unkown";d.constructed=true;
   if(!(actual.queue->flags&1u)){actual.queue->flags|=1u;actual.queue->count=0;++actual.lazy_initializers;}
   if(actual.queue->count<actual.queue->entries.size()){
    const auto ordinal=actual.queue->count;
    if(recipe->descriptor==0xcb7c68u)actual.queue->entries[ordinal]=std::move(base_entry);
    else actual.queue->entries[ordinal].reset(new NativeDefinitionDeferredQueue::Registration{
     recipe->original_callback,&d,native_flua_builtin_queue_body_owned(recipe->descriptor)});
    if(!actual.queue->entries[ordinal]||!actual.queue->entries[ordinal]->body)
     throw ScriptError("FLua genuine callback body missing at actual append");
    actual.callback_recipes[ordinal]=recipe;++actual.queue->count;
   }
  }
  if(writer.kind==0){
   d.name0=recipe->name;(void)::sprintf_s(d.name4.data(),d.name4.size(),"const %s",d.name0);d.named=true;
  }else if(writer.kind==1){d.current54c=recipe->callback54c;d.current548=recipe->callback548;}
  else {
   if(writer.ordinal!=d.count544||writer.ordinal>=recipe->method_count||d.count544>=d.records.size())
    throw ScriptError("FLua actual method writer stream/ordinal changed");
   const auto&record=native_builtin31_method_recipes[recipe->method_begin+writer.ordinal];
   ++d.count544;d.records[writer.ordinal]=&record;
  }
  ++actual.actual_writers;
 }
 if(base_entry||actual.queue->count!=31)throw ScriptError("FLua selected31 live queue constructor stream incomplete");
 for(const auto&d:actual.descriptors)if(!d.constructed||!d.named||d.count544!=d.recipe->method_count||
  d.current548!=d.recipe->callback548||d.current54c!=d.recipe->callback54c||
  std::strcmp(d.name0,d.recipe->name)||std::strcmp(d.name4.data(),d.recipe->const_name))
  throw ScriptError("FLua actual name/prefix/method writers did not finish");
 native_control_base_constructor->require_returned();
 const auto*r=native_flua_builtin_recipe_owned(0xcb35d8u);
 if(!r)throw ScriptError("FLua actual ButtonBase constructor missing");
 native_ordinary_nested_descriptors[1]=std::shared_ptr<const NativeOrdinaryNestedDescriptorReturn>(
  new NativeOrdinaryNestedDescriptorReturn(this,NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb35d8,
   r->descriptor,r->constructor,r->name));
}
void bind_native_flua_builtin_registry_actual_vm_owned(){
 auto*actual=native_flua_builtin_registry.get();
 if(!actual||actual->phase!=NativeFLuaBuiltinRegistryOwner::Phase::fresh_image||!state||calling||
  actual->host!=this||actual->source!=&sources||actual->creator!=std::this_thread::get_id()||
  actual->epoch!=failure_epoch||actual->writes!=failed.writes_||actual->uncertainty!=execution_uncertainty||
  failed.exhausted_||!ttistable(&G(state)->l_registry))throw ScriptError("FLua builtin registry actual VM binding refused");
 actual->master=state;actual->actual_global=G(state);actual->lua_registry=hvalue(&G(state)->l_registry);
 actual->phase=NativeFLuaBuiltinRegistryOwner::Phase::vm_bound;actual->physical_current_without_callbacks();
}
static NativeFLuaBuiltinRegistryOwner*native_flua_builtin_registry_current_owned(lua_State*L){
 auto&p=owner(L);auto*const actual=p.native_flua_builtin_registry.get();
 if(!native_flua_exposure_metadata_current_if_entered(L)||p.native_flua_builtin_registry.get()!=actual)
  return (native_master_thread_refuse(L,"FLua builtin supplier requires its genuine current registrar"),nullptr);
 auto*const scope=p.native_flua_exposure_registration.get();
 if(!scope||scope->stage!=NativeFLuaExposureRegistrationOwner::Stage::builtin)
  return (native_master_thread_refuse(L,"FLua builtin supplier entered outside761D30"),nullptr);
 p.source_error=nullptr;p.source_exception={};
 try{if(!actual||G(L)!=G(p.state))throw ScriptError("FLua actual builtin registry absent");actual->physical_current_without_callbacks();}
 catch(...){p.source_exception=std::current_exception();p.source_error="FLua builtin registry FIRST refused";}
 if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
 return actual;
}
static const char*native_flua_exposure_name_cell_owned(lua_State*L,std::uint32_t cell){
 // Invoked signature needs real live descriptor name storage; no registrar
 // callback is entered and no fabricated Original-process cell is read.
 auto&p=owner(L);native_db_query_current(L);p.source_error=nullptr;p.source_exception={};
 const char*result=nullptr;
 try{auto*actual=p.native_flua_builtin_registry.get();if(!actual)throw ScriptError("FLua actual name storage absent");
  actual->physical_current_without_callbacks();
  for(const auto&r:native_flua_name_cells)if(r.cell==cell){
   const auto*recipe=native_flua_builtin_recipe_owned(r.descriptor);
   if(!recipe)throw ScriptError("FLua actual name-cell descriptor absent");
   const auto&d=actual->descriptors[static_cast<std::size_t>(recipe-native_builtin31_recipes)];
   if(!d.named||d.recipe!=recipe)throw ScriptError("FLua name writer owner changed");
   result=r.is_const?d.name4.data():d.name0;break;
  }
  if(!result)throw ScriptError("FLua invoked signature requires its genuine named-cell writer");
 }catch(...){p.source_exception=std::current_exception();p.source_error="FLua live signature type-name FIRST refused";}
 if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
 return result;
}
static void native_flua_exposure_select_descriptor_owned(lua_State*L,std::uint32_t descriptor){
 auto&p=owner(L);auto*actual=native_flua_builtin_registry_current_owned(L);auto*scope=p.native_flua_exposure_registration.get();
 if(scope->actual_ordinal>=actual->queue->count||scope->actual_ordinal>=actual->callback_recipes.size())
  return (void)native_master_thread_refuse(L,"FLua builtin callback has no current live registry ordinal");
 const auto*callback=actual->callback_recipes[scope->actual_ordinal];
 const auto expected=scope->descriptor_depth?scope->descriptor_stack[scope->descriptor_depth-1]:callback?callback->descriptor:0;
 if(!callback||scope->actual_callback!=callback->original_callback||descriptor!=expected||!native_flua_builtin_recipe_owned(descriptor))
  return (void)native_master_thread_refuse(L,"FLua actual descriptor does not belong to entered callback/parent getter");
 scope->actual_descriptor=descriptor;(void)native_flua_exposure_metadata_current_if_entered(L,descriptor);
}
static const NativeFLuaBuiltinRegistryOwner::Descriptor*native_flua_builtin_descriptor_current_owned(lua_State*L,std::uint32_t descriptor){
 native_flua_exposure_select_descriptor_owned(L,descriptor);auto*actual=native_flua_builtin_registry_current_owned(L);
 const auto*r=native_flua_builtin_recipe_owned(descriptor);const auto*d=&actual->descriptors[static_cast<std::size_t>(r-native_builtin31_recipes)];
 if(!d->constructed||!d->named||d->recipe!=r||d->count544!=r->method_count||
  d->current548!=r->callback548||d->current54c!=r->callback54c||std::strcmp(d->name0,r->name)||std::strcmp(d->name4.data(),r->const_name))
  return (native_master_thread_refuse(L,"FLua builtin current constructor/name/count/parent writes changed"),nullptr);
 auto&p=owner(L);
 if(descriptor==0xcb7c68u){native_control_base_current_definition_read_owned(L,*p.native_context_definitions[1]);
  const auto&base=*p.native_context_definitions[1];
  if(base.count544!=d->count544||base.name0!=d->name0||base.name4!=d->name4.data())
   return (native_master_thread_refuse(L,"FLua shared genuine ControlBase storage differs from selected writers"),nullptr);
 }
 return d;
}
static bool native_flua_metatype_physical_owned(Impl&p,lua_State*L,const NativeFLuaMetatypeReturn&r,int offered){
 if(!r.alive||!r.returned||r.host!=&p||r.creator!=std::this_thread::get_id()||!r.recipe||!r.master||r.table_ref<0||!r.table)
  return false;
 const int top=lua_gettop(L);if(offered<0)offered=top+offered+1;
 const auto stop=[&](bool ok){lua_settop(L,top);return ok;};
 if(offered<1||lua_type(L,offered)!=LUA_TTABLE||lua_topointer(L,offered)!=r.table)return false;
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.table_ref);if(!lua_rawequal(L,-1,offered))return stop(false);lua_pop(L,1);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.master->cache_ref);
 if(!native_ordinary_master_physical_without_callbacks(p,L,*r.master,-1))return stop(false);
 lua_getfield(L,-1,r.is_const?r.recipe->const_name:r.recipe->name);
 if(!lua_rawequal(L,-1,offered))return stop(false);lua_pop(L,2);
 if(!lua_getmetatable(L,offered))return stop(false);
 lua_rawgeti(L,LUA_REGISTRYINDEX,r.master->metadata_ref);if(!lua_rawequal(L,-1,-2))return stop(false);lua_pop(L,2);
 lua_getfield(L,offered,"CTypeName");const char*name=lua_tostring(L,-1);
 if(!name||std::strcmp(name,r.is_const?r.recipe->const_name:r.recipe->name))return stop(false);lua_pop(L,1);
 for(std::size_t i=0;i<3;++i){const auto&f=p.native_ordinary_517c_functions[i];
  if(!f||!f->alive.load()||f->host!=&p||!f->body)return stop(false);
  lua_getfield(L,offered,f->field.c_str());if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=f->body)return stop(false);lua_pop(L,1);
 }
 lua_getfield(L,offered,"BaseType");
 if(r.parent?(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r.parent):!lua_isnil(L,-1))return stop(false);
 return stop(true);
}
static NativeFLuaMetatypeReturn*native_flua_offered_metatype_owned(lua_State*L,std::uint32_t descriptor,bool is_const,int offered){
 auto&p=owner(L);native_flua_builtin_descriptor_current_owned(L,descriptor);
 for(const auto&r:p.native_flua_metatype_results)if(r&&r->recipe->descriptor==descriptor&&r->is_const==is_const&&
  native_flua_metatype_physical_owned(p,L,*r,offered))return r.get();
 return (native_master_thread_refuse(L,"FLua offered metatype lacks its genuine completed ctor/cache keeper"),nullptr);
}
static void native_flua_exposure_push_parent_owned(lua_State*L,std::uint32_t parent,bool is_const){
 auto&p=owner(L);auto*scope=p.native_flua_exposure_registration.get();const auto prior=scope->actual_descriptor;
 if(scope->descriptor_depth>=scope->descriptor_stack.size())return (void)native_master_thread_refuse(L,"FLua genuine parent descriptor depth exhausted");
 scope->descriptor_stack[scope->descriptor_depth++]=parent;native_flua_exposure_select_descriptor_owned(L,parent);
 const auto*d=native_flua_builtin_descriptor_current_owned(L,parent);
 native_ordinary_push_master_types_615d70_owned(L);native_flua_exposure_select_descriptor_owned(L,parent);
 lua_getfield(L,-1,is_const?d->name4.data():d->name0);native_flua_exposure_select_descriptor_owned(L,parent);
 if(lua_type(L,-1)!=LUA_TTABLE){lua_pop(L,2);native_flua_exposure_construct_metatype_owned(L,parent,is_const);}
 else {lua_remove(L,-2);(void)native_flua_offered_metatype_owned(L,parent,is_const,-1);}
 --scope->descriptor_depth;native_flua_exposure_select_descriptor_owned(L,prior);
}
static void native_flua_exposure_construct_metatype_owned(lua_State*L,std::uint32_t descriptor,bool is_const){
 auto&p=owner(L);const auto*d=native_flua_builtin_descriptor_current_owned(L,descriptor);const auto*recipe=d->recipe;
 p.native_flua_metatype_results.push_back(std::make_shared<NativeFLuaMetatypeReturn>(&p,recipe,is_const));
 auto*r=p.native_flua_metatype_results.back().get();
 lua_createtable(L,0,0);native_flua_builtin_descriptor_current_owned(L,descriptor);r->table=lua_topointer(L,-1);
 lua_pushvalue(L,-1);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_builtin_descriptor_current_owned(L,descriptor);
 if(r->table_ref<0)return (void)native_master_thread_refuse(L,"FLua metatype actual table retention failed");
 d=native_flua_builtin_descriptor_current_owned(L,descriptor);
 lua_pushstring(L,is_const?d->name4.data():d->name0);lua_setfield(L,-2,"CTypeName");native_flua_builtin_descriptor_current_owned(L,descriptor);
 for(std::size_t i=0;i<3;++i){const auto*f=native_ordinary_required_517c_function_owned(L,i);
  lua_pushcclosure(L,f->body,0);native_flua_builtin_descriptor_current_owned(L,descriptor);
  lua_setfield(L,-2,f->field.c_str());native_flua_builtin_descriptor_current_owned(L,descriptor);
 }
 d=native_flua_builtin_descriptor_current_owned(L,descriptor);const auto parent=is_const?recipe->parent54c:recipe->parent548;
 if(parent){native_flua_exposure_push_parent_owned(L,parent,is_const);native_flua_builtin_descriptor_current_owned(L,descriptor);
  r->parent=lua_topointer(L,-1);lua_setfield(L,-2,"BaseType");native_flua_builtin_descriptor_current_owned(L,descriptor);}
 native_ordinary_push_master_types_615d70_owned(L);native_flua_builtin_descriptor_current_owned(L,descriptor);
 for(const auto&held:p.native_ordinary_master_types_results)if(held&&held->cache==lua_topointer(L,-1)&&
  native_ordinary_master_physical_without_callbacks(p,L,*held,-1)){r->master=held;break;}
 if(!r->master)return (void)native_master_thread_refuse(L,"FLua metatype actual615D70 cache keeper absent");
 lua_rawgeti(L,-1,1);native_flua_builtin_descriptor_current_owned(L,descriptor);
 if(lua_type(L,-1)==LUA_TTABLE)lua_setmetatable(L,-3);else lua_pop(L,1);
 native_flua_builtin_descriptor_current_owned(L,descriptor);
 lua_pushvalue(L,-2);lua_setfield(L,-2,is_const?recipe->const_name:recipe->name);lua_pop(L,1);
 native_flua_builtin_descriptor_current_owned(L,descriptor);r->returned=true;
 if(!native_flua_metatype_physical_owned(p,L,*r,-1))return (void)native_master_thread_refuse(L,"FLua genuine metatype physical POST failed");
 // Adopt these actual executed outputs into the same established Root
 // keepers. Later CData cache HIT sees its own real typed constructor return.
 if(!is_const&&(descriptor==0xcaaeb0u||descriptor==0xcb7c68u)){
  auto adopted=std::make_shared<NativeContextMetatypeResult>(&p,p.native_context_definitions[descriptor==0xcaaeb0u?0:1],p.native_ordinary_517c_functions);
  adopted->table=r->table;adopted->table_ref=r->table_ref;adopted->master=r->master;
  adopted->stage=NativeContextMetatypeResult::Stage::returned;p.native_context_metatype_results.push_back(std::move(adopted));
 }
 for(std::size_t i=0;i<p.native_ordinary_type_descriptors.size();++i){const auto&original=p.native_ordinary_type_descriptors[i];
  if(original&&original->original_descriptor==descriptor){
   auto adopted=std::make_shared<NativeOrdinaryMetatypeResult>(&p,original,p.native_ordinary_517c_functions,r->master,
    is_const?NativeOrdinaryMetatypeResult::Name::descriptor4:NativeOrdinaryMetatypeResult::Name::descriptor0,
    is_const?recipe->const_name:recipe->name);
   adopted->table=r->table;adopted->table_ref=r->table_ref;adopted->stage=NativeOrdinaryMetatypeResult::Stage::returned;
   p.native_ordinary_metatype_results.push_back(std::move(adopted));break;
  }
 }
}
static void native_flua_exposure_collect_methods_owned(lua_State*L,std::uint32_t descriptor,bool is_const){
 auto&p=owner(L);const auto*d=native_flua_builtin_descriptor_current_owned(L,descriptor);const auto*recipe=d->recipe;
 auto*metatype=native_flua_offered_metatype_owned(L,descriptor,is_const,-1);const int before=lua_gettop(L);
 if(descriptor==0xcb7c68u&&!is_const){native_control_base_mutable_library_47c500_owned(L);return;}
 p.native_flua_method_libraries.push_back(std::make_shared<NativeFLuaMethodLibraryReturn>(&p,recipe,is_const,metatype->table));
 auto*r=p.native_flua_method_libraries.back().get();
 lua_createtable(L,0,0);native_flua_builtin_descriptor_current_owned(L,descriptor);r->table=lua_topointer(L,-1);
 lua_pushvalue(L,-2);r->metatype_ref=luaL_ref(L,LUA_REGISTRYINDEX);
 lua_pushvalue(L,-1);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_builtin_descriptor_current_owned(L,descriptor);
 if(r->metatype_ref<0||r->table_ref<0)return (void)native_master_thread_refuse(L,"FLua actual library table retention failed");
 std::size_t ordinal=0;
 while(ordinal<native_flua_builtin_descriptor_current_owned(L,descriptor)->count544){
  d=native_flua_builtin_descriptor_current_owned(L,descriptor);const auto*record=d->records.at(ordinal);
  if(!record||record->descriptor!=descriptor||record->ordinal!=ordinal||record->constructor_reference!=-1)
   return (void)native_master_thread_refuse(L,"FLua actual method row/ref sentinel changed");
  if(!is_const||record->flag){auto&row=r->rows.at(ordinal);row.recipe=record;
   row.primary=descriptor==0xcb7c68u?native_control_base_lifecycle49_primary_owned(L,ordinal):native_flua_exposure_primary_body_owned(L,record->original_primary);
   row.secondary=descriptor==0xcb7c68u?native_control_base_lifecycle49_secondary_owned(L,ordinal):native_flua_builtin_secondary_body_owned(L,record->original_secondary);
   lua_pushcclosure(L,row.primary,0);native_flua_builtin_descriptor_current_owned(L,descriptor);row.primary_value=lua_topointer(L,-1);
   lua_pushvalue(L,-1);row.primary_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_builtin_descriptor_current_owned(L,descriptor);
   native_flua_associate_codebuddy_8f7e20_owned(L,row.secondary);native_flua_builtin_descriptor_current_owned(L,descriptor);
   // Capture the actual CodeBuddy return/association, never invoke secondary.
   native_ordinary_push_codebuddy_table_8f7d40_owned(L);native_flua_builtin_descriptor_current_owned(L,descriptor);
   for(const auto&held:p.native_ordinary_codebuddy_tables)if(held&&native_ordinary_codebuddy_table_physical_without_callbacks(p,L,*held,-1)){row.codebuddy=held;break;}
   if(!row.codebuddy)return (void)native_master_thread_refuse(L,"FLua actual library CodeBuddy keeper absent");
   lua_pushvalue(L,-2);lua_rawget(L,-2);row.secondary_value=lua_topointer(L,-1);
   if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=row.secondary)return (void)native_master_thread_refuse(L,"FLua actual secondary association changed");
   row.secondary_ref=luaL_ref(L,LUA_REGISTRYINDEX);lua_pop(L,1);native_flua_builtin_descriptor_current_owned(L,descriptor);
   lua_setfield(L,-2,record->key);native_flua_builtin_descriptor_current_owned(L,descriptor);
   lua_pushstring(L,record->key);row.key_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_flua_builtin_descriptor_current_owned(L,descriptor);
   if(row.primary_ref<0||row.secondary_ref<0||row.key_ref<0||lua_gettop(L)!=before+1)
    return (void)native_master_thread_refuse(L,"FLua actual method closure/key retention POST failed");
   ++r->assigned;
  }
  ++ordinal;
 }
 if(r->assigned!=(is_const?recipe->const_method_count:recipe->method_count)||lua_gettop(L)!=before+1)
  return (void)native_master_thread_refuse(L,"FLua actual collector current-count return incomplete");
 r->returned=true;
}
static void native_flua_builtin_registry_761d30_owned(lua_State*L,int operand){
 auto&p=owner(L);auto*actual=native_flua_builtin_registry_current_owned(L);
 if(actual->actual_readers==UINT64_MAX)return (void)native_master_thread_refuse(L,"FLua builtin actual reader history exhausted");
 ++actual->actual_readers;const int before=lua_gettop(L);
 native_ordinary_push_master_types_615d70_owned(L);(void)native_flua_builtin_registry_current_owned(L);
 native_ordinary_push_reflection_5f36d0_owned(L);(void)native_flua_builtin_registry_current_owned(L);
 if(!(actual->queue->flags&1u)){actual->queue->flags|=1u;actual->queue->count=0;++actual->lazy_initializers;}
 std::size_t ordinal=0;
 while(ordinal<actual->queue->count){
  actual=native_flua_builtin_registry_current_owned(L);
  if(ordinal>=actual->queue->entries.size()||!actual->queue->entries[ordinal]||!actual->callback_recipes[ordinal])
   return (void)native_master_thread_refuse(L,"FLua current builtin queue row lacks its genuine constructor producer");
  const auto*r=actual->callback_recipes[ordinal];const auto*entry=actual->queue->entries[ordinal].get();
  if(entry->original_callback!=r->original_callback||!entry->body)
   return (void)native_master_thread_refuse(L,"FLua current callback registration identity changed");
  auto*scope=p.native_flua_exposure_registration.get();scope->actual_ordinal=ordinal;scope->actual_callback=r->original_callback;scope->actual_descriptor=0;
  p.source_error=nullptr;p.source_exception={};
  try{const auto body=native_builtin31_callback_body_owned(entry->original_callback);
   if(!body)throw ScriptError("FLua actual live queue callback body unissued");body(L,operand);}
  catch(...){p.source_exception=std::current_exception();p.source_error="FLua actual builtin registrar threw";}
  if(p.source_error)return (void)native_master_thread_refuse(L,p.source_error);
  actual=native_flua_builtin_registry_current_owned(L);
  if(lua_gettop(L)!=before+2||scope->descriptor_depth)return (void)native_master_thread_refuse(L,"FLua actual builtin callback stack/parent POST failed");
  ++ordinal; // genuine live count reread, not a hardcoded31 runtime loop
 }
 lua_pop(L,2);(void)native_flua_builtin_registry_current_owned(L);
 if(lua_gettop(L)!=before)return (void)native_master_thread_refuse(L,"FLua761D30 actual table pop POST failed");
}
static void native_flua_exposure_bootstrap_owned(lua_State*L,Job&job){
 auto&p=owner(L);native_flua_exposure_begin_owned(L,job);
 native_flua_typed_registry_5fc250_owned(L);
 p.native_flua_exposure_registration->stage=NativeFLuaExposureRegistrationOwner::Stage::named;
 native_flua_named_registry_8f9300_owned(L);
 p.native_flua_exposure_registration->stage=NativeFLuaExposureRegistrationOwner::Stage::builtin;
 native_flua_builtin_registry_761d30_owned(L,-1);native_flua_exposure_returned_owned(L);
}
static lua_CFunction native_flua_exposure_existing_primary_callback_owned(lua_State*L,std::uint32_t original){
 native_db_query_current(L);
 if(const auto body=native_context_direct_field_primary_owned(original))return body;
 if(original==0x4aa3a0u)return &Impl::worldview_grid_to_world;
 if(original==0x48e920u)return &Impl::native_postread_ui_manager_screen_size_owned;
 return native_flua_exposure_known_control_primary_body(original);
}
static bool native_flua_exposure_grid_endpoint_current_owned(lua_State*L,int offered){
 auto&p=owner(L);native_db_query_current(L);const int top=lua_gettop(L);
 if(offered<0)offered=top+offered+1;
 if(!lua_iscfunction(L,offered)||lua_tocfunction(L,offered)!=&Impl::native_flua_exposure_primary_endpoint_owned<0x4aa3a0u>)return false;
 if(lua_getupvalue(L,offered,1)){lua_settop(L,top);return false;}
 p.source_error=nullptr;p.source_exception={};bool issued=false;
 try{auto*actual=p.native_flua_named_registry.get();if(!actual)throw ScriptError("FLua genuine named GridToWorld writer absent");
  actual->physical_current_without_callbacks();
  for(std::size_t i=0;i<actual->count;++i){const auto&r=actual->records[i];
   if(r.primary_va==0x4aa3a0u&&r.primary==&Impl::native_flua_exposure_primary_endpoint_owned<0x4aa3a0u>&&
    r.name&&!std::strcmp(r.name,"GridToWorld")){issued=true;break;}
  }
 }catch(...){p.source_exception=std::current_exception();p.source_error="FLua genuine GridToWorld registration Source FIRST refused";}
 if(p.source_error)return (native_master_thread_refuse(L,p.source_error),false);
 return issued;
}
static NativeContextMetatypeResult*native_flua_exposure_existing_context_metatype_owned(lua_State*L,bool context){
 // Called only after the established postread borrower. Preserve current
 // ExposedMembers[metatype] association by borrowing the genuine cache HIT.
 auto&p=owner(L);const int top=lua_gettop(L);const auto descriptor=context?0xcaaeb0u:0xcb7c68u;
 lua_getfield(L,LUA_GLOBALSINDEX,"ExposedTypes");
 if(lua_type(L,-1)!=LUA_TTABLE){lua_settop(L,top);return nullptr;}
 lua_getfield(L,-1,context?"LuaContext":"ControlBase");lua_remove(L,-2);
 if(lua_type(L,-1)!=LUA_TTABLE){lua_settop(L,top);return nullptr;}
 for(const auto&native:p.native_flua_metatype_results)if(native&&native->recipe->descriptor==descriptor&&!native->is_const&&
  native_flua_metatype_physical_owned(p,L,*native,-1)){
  for(const auto&adopted:p.native_context_metatype_results)if(adopted&&adopted->host==&p&&adopted->alive.load()&&
   adopted->creator==std::this_thread::get_id()&&adopted->descriptor==p.native_context_definitions[context?0:1]&&
   adopted->stage==NativeContextMetatypeResult::Stage::returned&&adopted->master==native->master&&
   adopted->table==native->table&&adopted->table_ref==native->table_ref)return adopted.get();
 }
 return (native_master_thread_refuse(L,"Context metatype cache HIT lacks its authentic completed bootstrap/adopted keeper"),nullptr);
}
void native_flua_builtin_registry_retire()noexcept{
 if(native_flua_builtin_registry)native_flua_builtin_registry->phase=NativeFLuaBuiltinRegistryOwner::Phase::retired;
 for(const auto&r:native_flua_metatype_results)if(r)r->alive=false;
 for(const auto&r:native_flua_method_libraries)if(r)r->alive=false;
}
