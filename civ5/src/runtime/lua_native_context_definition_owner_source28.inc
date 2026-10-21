//Inside the SAME LuaScriptHost::Impl after Source7's root CData body.
//The separate actual collected-library4E35C0 and its C functions are NOT
//constructed by these records. A native callback address is a recipe tag,
//never a cast to an executable x86 function or an error-only Lua closure.
struct NativeContextMethodRecipe final {
    std::uint32_t writer,key_address,primary,secondary;
    std::uint8_t flag;
    const char*key;
};
struct NativeContextDefinition final {
    enum class Callback {unissued,none,fn6a6920,fn4acbf0};
    enum class Stage {birth,name,prefix,records,metadata_returned,retired};
    struct Record final {
        const NativeContextMethodRecipe*recipe=nullptr;
        const char*key=nullptr;
        std::uint32_t primary=0,secondary=0;
        std::uint8_t flag=0;
        std::int32_t reference=0;
    };
    const Impl*const host;
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool>alive{true};
    const bool context;
    const std::shared_ptr<const NativeContextCDataCreator>context_creator;
    const std::uint32_t descriptor,constructor,name_writer,deferred_callback;
    //Only these three fields represent qualified logical birth stores. The
    //metadata object is not a token for the whole Original ctor/registration.
    const std::uint32_t birth_count544=0;
    const Callback birth548=Callback::none,birth54c=Callback::none;
    //Current values are separate. In particular ControlBase birthNULL does
    //not fill either current operand. A reached genuine writer must do that.
    Callback current548=Callback::unissued,current54c=Callback::unissued;
    std::uint32_t count544=0;
    std::array<Record,49>records;
    std::string name0,name4;
    Stage stage=Stage::birth;
    NativeContextDefinition(const Impl*p,bool is_context,
        std::shared_ptr<const NativeContextCDataCreator>issuer)
        :host(p),context(is_context),context_creator(std::move(issuer)),
        descriptor(is_context?0xcaaeb0u:0xcb7c68u),
        constructor(is_context?0x476380u:0x5210f0u),
        name_writer(is_context?0x9bae20u:0x9bdde0u),
        deferred_callback(is_context?0x6394a0u:0x7a4df0u){}
};
struct NativeContextMetatypeResult final {
    enum class Stage {entered,table,name,callbacks,parent,metadata,cache_assigned,returned,retired};
    const Impl*const host;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool>alive{true};
    const std::shared_ptr<const NativeContextDefinition>descriptor;
    const std::array<std::shared_ptr<const NativeOrdinary517CFunction>,3>functions;
    std::shared_ptr<NativeOrdinaryMasterTypesResult>master;
    const void*table=nullptr;
    int table_ref=LUA_NOREF;
    Stage stage=Stage::entered;
    NativeContextMetatypeResult(const Impl*p,std::shared_ptr<const NativeContextDefinition>d,
        std::array<std::shared_ptr<const NativeOrdinary517CFunction>,3>f)
        :host(p),descriptor(std::move(d)),functions(std::move(f)){}
};

void initialize_native_context_definition_owned(){
    //Real, single Host construction, before allocating/executing any Lua VM
    //API. This port's private actor performs actual logical writer stores; it
    //does not assert execution of the Original process's full CRT/init queue.
    if(native_context_definitions[0]||native_context_definitions[1])
        throw ScriptError("Context definition writer replay");
    if(!native_context_cdata_creator||native_context_cdata_creator->host!=this||
        !native_context_cdata_creator->alive.load()||
        native_context_cdata_creator->creator!=std::this_thread::get_id()||
        std::strcmp(native_context_cdata_creator->type_name,"LuaContext"))
        throw ScriptError("Context definition requires real SAME Host type/name issuer");
    //Exact chosen native rows, retained at static C++ lifetime. They are DATA
    //only until real primary/secondary bodies and4E35C0 have returned in Lua.
#include "lua_native_context_definition_recipes.inc"
    auto context=std::make_shared<NativeContextDefinition>(this,true,native_context_cdata_creator);
    auto base=std::make_shared<NativeContextDefinition>(this,false,nullptr);
    const auto publish_name=[](NativeContextDefinition&d,const char*name){
        //Exact positive ASCII %s output and64-byte limit of40A100/549B00.
        //Native errno/private formatter addresses are not a ported return.
        if(!name||!*name||std::strlen(name)+7>64)throw ScriptError("Context actual name writer input invalid");
        d.name0=name;d.name4="const ";d.name4+=name;d.stage=NativeContextDefinition::Stage::name;
    };
    publish_name(*context,native_context_cdata_creator->type_name);
    //9BAE4D writes54C BEFORE9BAE57 writes548, on this SAME definition.
    context->current54c=NativeContextDefinition::Callback::fn4acbf0;
    context->current548=NativeContextDefinition::Callback::fn6a6920;
    context->stage=NativeContextDefinition::Stage::prefix;
    publish_name(*base,"ControlBase");
    const auto append=[](NativeContextDefinition&d,const NativeContextMethodRecipe&r){
        if(d.count544>=d.records.size()||!r.key||!*r.key||!r.primary||!r.secondary)
            throw ScriptError("Context real native method record unavailable");
        auto&slot=d.records[d.count544];
        ++d.count544; //Actual count store precedes every field store.
        slot.key=r.key;slot.primary=r.primary;slot.secondary=r.secondary;
        slot.flag=r.flag;slot.reference=-1;slot.recipe=&r;
        d.stage=NativeContextDefinition::Stage::records;
    };
    for(const auto&r:context_rows)append(*context,r);
    for(const auto&r:base_rows)append(*base,r);
    if(context->count544!=15||base->count544!=49)
        throw ScriptError("Context selected native writer stream incomplete");
    context->stage=NativeContextDefinition::Stage::metadata_returned;
    base->stage=NativeContextDefinition::Stage::metadata_returned;
    //No library/listener completion and no current Base548/54C absence grant.
    native_context_definitions[0]=std::move(context);
    native_context_definitions[1]=std::move(base);
}

static void native_context_definition_source_full_owned(lua_State*L,bool context){
    if(native_flua_exposure_metadata_current_if_entered(L,context?0xcaaeb0u:0xcb7c68u))return;
    if(context)native_postread_root_cdata_full(L);
    else native_flua_exposure_registration_full(L); //Actual Base is shared by ordinary receivers.
}
static const NativeContextDefinition*native_context_current_definition_owned(lua_State*L,bool context){
    auto&p=owner(L);native_context_definition_source_full_owned(L,context);
    const auto&held=p.native_context_definitions[context?0:1];
    if(!held||held->host!=&p||!held->alive.load()||held->creator!=std::this_thread::get_id()||
        held->context!=context||held->stage!=NativeContextDefinition::Stage::metadata_returned||
        held->count544!=(context?15u:49u)||held->name0!=(context?"LuaContext":"ControlBase")){
        luaL_error(L,"required SAME current native Context/ControlBase definition writer owner");return nullptr;
    }
    if(context&&(!held->context_creator||held->context_creator!=p.native_context_cdata_creator||
        !held->context_creator->alive.load()||held->context_creator->host!=&p||
        held->current548!=NativeContextDefinition::Callback::fn6a6920||
        held->current54c!=NativeContextDefinition::Callback::fn4acbf0)){
        luaL_error(L,"Context actual9BAE30 prefix/creator changed");return nullptr;
    }
    for(std::size_t i=0;i<held->count544;++i){const auto&r=held->records[i];
        if(!r.recipe||r.key!=r.recipe->key||r.primary!=r.recipe->primary||
            r.secondary!=r.recipe->secondary||r.flag!=r.recipe->flag||r.reference!=-1){
            luaL_error(L,"Context actual method metadata changed before its library execution");return nullptr;
        }
    }
    if(!context)native_control_base_current_definition_read_owned(L,*held);
    return held.get();
}
static const NativeOrdinary517CFunction*native_context_actual_517c_function_owned(lua_State*L,std::size_t i,bool context){
    auto&p=owner(L);native_context_definition_source_full_owned(L,context);
    if(i>=3||!p.native_ordinary_517c_functions[i]){
        luaL_error(L,"Context required actual native517C closure writer missing");return nullptr;
    }
    const auto*f=p.native_ordinary_517c_functions[i].get();
    if(f->host!=&p||!f->alive.load()||f->creator!=std::this_thread::get_id()||
        static_cast<std::size_t>(f->original)!=i||!f->body||f->field.empty()){
        luaL_error(L,"Context SAME genuine517C callback/key owner changed");return nullptr;
    }
    return f;
}
static void native_context_base_metatype_6a6920_owned(lua_State*L){
    //Actual SAME parent, not an ordinary-six or child-completion substitute.
    //The port's known birth/name/49 metadata stores precede this call; a
    //separate actual whole-ctor loan is still mandatory. Missing current548
    //is consumed only if its own recursive517C reaches that field.
    auto&p=owner(L);native_flua_exposure_registration_full(L);
    //Do not turn this new metadata actor into the actual conditional5210F0
    //constructor return. Its deferred7A4DF0 registration belongs to the real
    //startup/catalog owner. The existing closed loan is the only borrower.
    const auto*constructor=p.native_ordinary_nested_descriptors[0].get();
    if(!constructor||constructor->host!=&p||!constructor->alive.load()||
        constructor->creator!=std::this_thread::get_id()||
        constructor->kind!=NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb7c68||
        constructor->actual_descriptor!=0xcb7c68u||constructor->actual_ctor!=0x5210f0u||
        constructor->actual_name0!="ControlBase")
        return (void)luaL_error(L,"required actual6A6938->5210F0 whole distinct ControlBase ctor/deferred7A4DF0 registration and current9BDDE0 name loan; this Source8 metadata actor cannot issue its constructor completion");
    const auto*d=native_context_current_definition_owned(L,false);
    native_ordinary_push_master_types_615d70_owned(L);native_flua_exposure_registration_full(L);
    d=native_context_current_definition_owned(L,false);
    lua_getfield(L,-1,d->name0.c_str());native_flua_exposure_registration_full(L);
    if(lua_type(L,-1)!=LUA_TTABLE){
        lua_settop(L,-3);native_flua_exposure_registration_full(L); //Actual6A6969
        native_context_construct_metatype_517c20_owned(L,false);
        native_flua_exposure_registration_full(L);
    }else {
        lua_remove(L,-2);native_flua_exposure_registration_full(L); //Actual6A6981
    }
}
static NativeContextMetatypeResult*native_context_construct_metatype_517c20_owned(lua_State*L,bool context){
    auto&p=owner(L);const auto*d=native_context_current_definition_owned(L,context);
    for(std::size_t i=0;i<3;++i)native_context_actual_517c_function_owned(L,i,context);
    //Persist partial OWN result outside all allocating Lua APIs; no temporary
    //shared_ptr/string/destructor crosses a possible Lua longjmp.
    p.native_context_metatype_results.push_back(std::make_shared<NativeContextMetatypeResult>(
        &p,p.native_context_definitions[context?0:1],p.native_ordinary_517c_functions));
    auto*r=p.native_context_metatype_results.back().get();
    lua_createtable(L,0,0);r->table=lua_topointer(L,-1);r->stage=NativeContextMetatypeResult::Stage::table;
    native_context_definition_source_full_owned(L,context);
    lua_pushvalue(L,-1);r->table_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_context_definition_source_full_owned(L,context);
    if(r->table_ref<0){luaL_error(L,"Context actual partial517C table retention failed");return nullptr;}
    d=native_context_current_definition_owned(L,context);
    lua_pushstring(L,d->name0.c_str());native_context_definition_source_full_owned(L,context);
    lua_setfield(L,-2,"CTypeName");r->stage=NativeContextMetatypeResult::Stage::name;native_context_definition_source_full_owned(L,context);
    for(std::size_t i=0;i<3;++i){
        const auto*f=native_context_actual_517c_function_owned(L,i,context);
        lua_pushcclosure(L,f->body,0);native_context_definition_source_full_owned(L,context);
        f=native_context_actual_517c_function_owned(L,i,context);
        lua_setfield(L,-2,f->field.c_str());native_context_definition_source_full_owned(L,context);
    }
    r->stage=NativeContextMetatypeResult::Stage::callbacks;
    d=native_context_current_definition_owned(L,context); //Actual517C99 fresh548
    if(d->current548==NativeContextDefinition::Callback::unissued){
        luaL_error(L,"required current CB7C68 ControlBase548 producer after selected complete definition stream at actual517C99; ctor5210FA NULL and49 metadata rows alone cannot issue current absence; partial table is NOT cache-published");return nullptr;
    }
    if(d->current548!=NativeContextDefinition::Callback::none){
        if(d->current548!=NativeContextDefinition::Callback::fn6a6920){
            luaL_error(L,"required genuine selected Context548 callback body");return nullptr;
        }
        native_context_base_metatype_6a6920_owned(L);native_context_definition_source_full_owned(L,context);
        //Only an actual returned6A6920 table may be consumed at517CB1.
        if(lua_type(L,-1)!=LUA_TTABLE){luaL_error(L,"Context actual parent548 returned non-table");return nullptr;}
        lua_setfield(L,-2,"BaseType");native_context_definition_source_full_owned(L,context);
    }
    r->stage=NativeContextMetatypeResult::Stage::parent;
    native_ordinary_push_master_types_615d70_owned(L);native_context_definition_source_full_owned(L,context);
    for(const auto&keeper:p.native_ordinary_master_types_results)
        if(keeper&&keeper->cache==lua_topointer(L,-1)&&native_ordinary_master_physical_without_callbacks(p,L,*keeper,-1)){
            r->master=keeper;break;
        }
    if(!r->master){luaL_error(L,"Context actual615D70 returned master owner missing");return nullptr;}
    lua_rawgeti(L,-1,1);native_context_definition_source_full_owned(L,context);
    if(lua_type(L,-1)==LUA_TTABLE){lua_setmetatable(L,-3);native_context_definition_source_full_owned(L,context);}
    else {lua_settop(L,-2);native_context_definition_source_full_owned(L,context);}
    r->stage=NativeContextMetatypeResult::Stage::metadata;
    lua_pushvalue(L,-2);native_context_definition_source_full_owned(L,context);
    d=native_context_current_definition_owned(L,context);
    lua_setfield(L,-2,d->name0.c_str());r->stage=NativeContextMetatypeResult::Stage::cache_assigned;
    native_context_definition_source_full_owned(L,context);
    lua_settop(L,-2);native_context_definition_source_full_owned(L,context);
    if(lua_type(L,-1)!=LUA_TTABLE||lua_topointer(L,-1)!=r->table){
        luaL_error(L,"Context actual517C returned table changed");return nullptr;
    }
    r->stage=NativeContextMetatypeResult::Stage::returned;return r;
}
static NativeOrdinaryMasterTypesResult*native_context_construct_root_metatype_and_borrow_master_owned(lua_State*L){
    auto&p=owner(L);native_postread_root_cdata_full(L);
    auto*r=native_flua_exposure_existing_context_metatype_owned(L,true);
    if(!r)r=native_context_construct_metatype_517c20_owned(L,true);
    native_postread_root_cdata_full(L);
    if(!r||r->host!=&p||!r->alive.load()||r->creator!=std::this_thread::get_id()||
        r->stage!=NativeContextMetatypeResult::Stage::returned||!r->master||
        !r->descriptor||r->descriptor!=p.native_context_definitions[0]){
        luaL_error(L,"postRead actual Context517C whole return missing");return nullptr;
    }
    return r->master.get();
}

static int native_context_definition_refs_cleanup(lua_State*L){
    auto*p=static_cast<Impl*>(lua_touserdata(L,1));
    if(!p||L!=p->state||p->ui_thread!=std::this_thread::get_id())
        return luaL_error(L,"Context definition cleanup requires actual Host VM/creator");
    for(auto&r:p->native_context_metatype_results)if(r&&r->alive.exchange(false)){
        r->stage=NativeContextMetatypeResult::Stage::retired;
        //Never retry an uncertain unref. VM close reclaims any retained value.
        const int ref=r->table_ref;r->table_ref=LUA_NOREF;
        if(ref!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,ref);
    }
    return 0;
}
void native_context_definition_close()noexcept{
    if(ui_thread!=std::this_thread::get_id())std::terminate();
    if(state){const int top=lua_gettop(state);
        (void)lua_cpcall(state,native_context_definition_refs_cleanup,this);lua_settop(state,top);
    }
    for(const auto&d:native_context_definitions)if(d)d->alive.store(false);
}
