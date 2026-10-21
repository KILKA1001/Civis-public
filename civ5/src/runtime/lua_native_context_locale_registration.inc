// Native5DD5E0 installs real zero-upvalue consumers without invoking them.
// An issued callback must come from the corresponding genuine backend owner.
// This prefix has no such issuer; the closed binding cannot be constructed.
// Presence checks below never synthesize a Locale result or success.
class NativeLocaleBackendBinding final {
    NativeLocaleBackendBinding()=delete;
    NativeLocaleBackendBinding(const NativeLocaleBackendBinding&)=delete;
    NativeLocaleBackendBinding& operator=(const NativeLocaleBackendBinding&)=delete;
    Impl* actual_host=nullptr;lua_State* actual_vm=nullptr;
    std::uint32_t original_function=0;lua_CFunction actual_callback=nullptr;
    std::weak_ptr<EndTurnQueue::VmLifetime> lifetime;bool retired=false;
    friend struct Impl;
public:~NativeLocaleBackendBinding()=default;
};
template<std::uint32_t NativeFunction> static int native_locale_method(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    NativeLocaleBackendBinding* binding=nullptr;
    for(const auto& issued:p.native_locale_backend_bindings)
        if(issued&&issued->original_function==NativeFunction){if(binding)return luaL_error(L,"Locale actual backend binding duplicated");binding=issued.get();}
    if(!binding){p.record_failure();return luaL_error(L,"Locale native consumer %08X requires its genuine backend; provider unissued",static_cast<unsigned>(NativeFunction));}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    p.source_exception={};p.source_error=nullptr;
    lua_CFunction callback=nullptr;lua_State* actual_vm=nullptr;const EndTurnQueue::VmLifetime* actual_lifetime=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_locale_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Locale binding retired during source FIRST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||binding->actual_vm!=p.state||!binding->actual_callback||
           !life||life.get()!=p.end_turn_lifetime.get()||life.owner_before(p.end_turn_lifetime)||
           p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||life->vm!=p.state||life->host!=p.worldview_host_owner)
            throw ScriptError("Locale actual backend/facade custody retired");
        callback=binding->actual_callback;actual_vm=binding->actual_vm;actual_lifetime=life.get();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Locale genuine backend FIRST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    // No owning C++ local or speculative argument/result conversion crosses
    // this real Lua CFunction invocation. A future genuine issuer must supply
    // the complete original method body and its real backend custody.
    const int count=callback(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        p.worldview_require_callback_origin();
        bool owned=false;for(const auto& issued:p.native_locale_backend_bindings)if(issued.get()==binding){owned=true;break;}
        if(!owned)throw ScriptError("Locale binding retired during callback/source POST");
        const auto life=binding->lifetime.lock();
        if(binding->retired||binding->original_function!=NativeFunction||binding->actual_host!=&p||
           binding->actual_vm!=actual_vm||actual_vm!=p.state||binding->actual_callback!=callback||
           !life||life.get()!=actual_lifetime||life.get()!=p.end_turn_lifetime.get()||
           life.owner_before(p.end_turn_lifetime)||p.end_turn_lifetime.owner_before(life)||!life->alive||life->failed||
           life->vm!=actual_vm||life->host!=p.worldview_host_owner||epoch!=p.failure_epoch||
           writes!=p.failed.writes_||uncertainty!=p.execution_uncertainty)
            throw ScriptError("Locale genuine callback/VM/weak lifetime/source changed");
    }catch(...){p.source_exception=std::current_exception();p.source_error="Locale genuine callback/source POST refused";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    if(count<0||count>lua_gettop(L)){p.record_failure();return luaL_error(L,"Locale actual callback returned invalid result count");}
    return count;
}
// These consumers use the actual selected Locale provider. A plain-text
// provider may explicitly refuse missing keys, grammar or parameter formatting;
// registration never turns that refusal into a successful string.
static void native_locale_callback_post_cpp(Impl& p,const void* scope,std::size_t child,
    const void* source,lua_State* vm,const void* entry,std::uint32_t function,
    std::uint64_t epoch,std::uint64_t writes,std::uint64_t uncertainty){
    p.worldview_require_callback_origin();
    if(p.state!=vm||p.worldview_native_host!=scope||!p.worldview_native_host||
       p.worldview_native_host->child!=child||child>=p.child_contexts.size()||
       p.native_locale_operation_entry!=entry||p.native_locale_operation_function!=function)
        throw ScriptError("Locale SAME actual invocation/source/VM retired");
    const auto retained_source=p.child_contexts[child].source;
    if(!retained_source||retained_source.get()!=source||p.child_contexts[child].retired)
        throw ScriptError("Locale selected source retired before callback POST");
    p.child_require_current(child);p.worldview_require_callback_origin();
    if(p.state!=vm||p.worldview_native_host!=scope||!p.worldview_native_host||
       p.worldview_native_host->child!=child||child>=p.child_contexts.size()||
       retained_source.get()!=p.child_contexts[child].source.get()||
       retained_source.owner_before(p.child_contexts[child].source)||
       p.child_contexts[child].source.owner_before(retained_source)||p.child_contexts[child].retired||
       p.native_locale_operation_entry!=entry||p.native_locale_operation_function!=function||
       p.failed||p.failed.exhausted_||epoch!=p.failure_epoch||writes!=p.failed.writes_||
       uncertainty!=p.execution_uncertainty)
        throw ScriptError("Locale source/failure/actual invocation changed during callback POST");
}
static void native_locale_operation_post(lua_State* L,const void* scope,std::size_t child,
    const void* source,lua_State* vm,const void* entry,std::uint32_t function,
    std::uint64_t epoch,std::uint64_t writes,std::uint64_t uncertainty){
    auto& p=owner(L);native_db_query_current(L);
    p.source_exception={};p.source_error=nullptr;
    try{native_locale_callback_post_cpp(p,scope,child,source,vm,entry,function,epoch,writes,uncertainty);}
    catch(...){p.source_exception=std::current_exception();p.source_error="Locale SAME actual source/callback POST refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
template<std::uint32_t NativeFunction> static int native_locale_convert_exact(lua_State* L){
    static_assert(NativeFunction==0x75b6a0||NativeFunction==0x6f9b20);
    auto& p=owner(L);native_db_query_current(L);
    // Native75B6A0 and6F9B20 both check the key with luaL_checklstring, then
    // pass a C-string to Localization::Lookup. Embedded NUL truncation belongs
    // to this native call rather than our length-preserving generic API.
    const char*const key=luaL_checkstring(L,1);native_db_query_current(L);
    const int top=lua_gettop(L);
    for(int i=2;i<=top;++i){const int type=lua_type(L,i);
        if(type!=LUA_TBOOLEAN&&type!=LUA_TNUMBER&&type!=LUA_TSTRING){
            p.record_failure();return luaL_argerror(L,i,"Argument must be [boolean, number, or string].");
        }
    }
    if(!p.locale.convert_text_key){p.record_failure();return luaL_error(L,"Locale native text consumer requires the genuine selected localization provider; provider unissued");}
    if(p.native_locale_operation_entry){p.record_failure();return luaL_error(L,"Locale text callback reentry would replace actual invocation custody");}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    const void*const scope=p.worldview_native_host;const auto child=p.worldview_native_host->child;
    const void*const source=p.child_contexts[child].source.get();lua_State*const vm=p.state;
    const unsigned char actual_entry=0;p.native_locale_operation_entry=&actual_entry;p.native_locale_operation_function=NativeFunction;
    p.source_exception={};p.source_error=nullptr;
    try{p.native_locale_key_text=key;p.locale_arguments.clear();p.locale_text.clear();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Locale actual key/argument custody allocation failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,NativeFunction,epoch,writes,uncertainty);
    for(int i=2;i<=top;++i){
        const int type=lua_type(L,i);const char*const text=type==LUA_TSTRING?lua_tostring(L,i):nullptr;
        const bool flag=type==LUA_TBOOLEAN&&lua_toboolean(L,i);const double number=type==LUA_TNUMBER?lua_tonumber(L,i):0;
        p.source_exception={};p.source_error=nullptr;
        try{
            if(type==LUA_TSTRING)p.locale_arguments.emplace_back(std::string(text));
            else if(type==LUA_TNUMBER)p.locale_arguments.emplace_back(number);
            else if(type==LUA_TBOOLEAN)p.locale_arguments.emplace_back(flag);
            else throw ScriptError("Locale actual argument type replaced during source callbacks");
        }catch(...){p.source_exception=std::current_exception();p.source_error="Locale actual argument conversion failed";}
        if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
        native_locale_operation_post(L,scope,child,source,vm,&actual_entry,NativeFunction,epoch,writes,uncertainty);
    }
    p.source_exception={};p.source_error=nullptr;
    try{
        native_locale_callback_post_cpp(p,scope,child,source,vm,&actual_entry,NativeFunction,epoch,writes,uncertainty);
        if(!p.locale.convert_text_key)throw ScriptError("Locale selected text provider retired");
        p.locale_text=p.locale.convert_text_key(p.native_locale_key_text,p.locale_arguments);
        native_locale_callback_post_cpp(p,scope,child,source,vm,&actual_entry,NativeFunction,epoch,writes,uncertainty);
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Locale actual selected text lookup failed";}
    // Owning provider result, exception and argument strings live on the Host.
    // The C++ helper's retained source retires before allocating/error Lua APIs.
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,NativeFunction,epoch,writes,uncertainty);
    lua_pushlstring(L,p.locale_text.data(),p.locale_text.size());
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,NativeFunction,epoch,writes,uncertainty);
    std::size_t bytes=0;const char*const output=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&bytes):nullptr;
    if(!output||bytes!=p.locale_text.size()||std::memcmp(output,p.locale_text.data(),bytes)){
        p.record_failure();return luaL_error(L,"Locale actual UTF8 result replaced during source POST");}
    p.locale_arguments.clear();p.locale_text.clear();p.native_locale_key_text.clear();
    p.native_locale_operation_entry=nullptr;p.native_locale_operation_function=0;return 1;
}
static int native_locale_current_exact(lua_State* L){
    auto& p=owner(L);native_db_query_current(L);
    if(!p.locale.current){p.record_failure();return luaL_error(L,"Locale GetCurrentLanguage requires the genuine selected language provider; provider unissued");}
    if(p.native_locale_operation_entry){p.record_failure();return luaL_error(L,"Locale language callback reentry would replace actual invocation custody");}
    const auto epoch=p.failure_epoch,writes=p.failed.writes_,uncertainty=p.execution_uncertainty;
    const void*const scope=p.worldview_native_host;const auto child=p.worldview_native_host->child;
    const void*const source=p.child_contexts[child].source.get();lua_State*const vm=p.state;
    const unsigned char actual_entry=0;p.native_locale_operation_entry=&actual_entry;p.native_locale_operation_function=0x73ca40;
    p.source_exception={};p.source_error=nullptr;
    try{
        native_locale_callback_post_cpp(p,scope,child,source,vm,&actual_entry,0x73ca40,epoch,writes,uncertainty);
        if(!p.locale.current)throw ScriptError("Locale selected current-language provider retired");
        p.locale_language=p.locale.current();
        native_locale_callback_post_cpp(p,scope,child,source,vm,&actual_entry,0x73ca40,epoch,writes,uncertainty);
    }catch(const std::exception& e){p.source_exception=std::current_exception();p.source_error=e.what();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Locale actual current-language lookup failed";}
    if(p.source_error){p.record_failure();return luaL_error(L,"%s",p.source_error);}
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,0x73ca40,epoch,writes,uncertainty);
    // Raw PE73CA40 builds table(0,2), then native C-string Type/DisplayName.
    lua_createtable(L,0,2);const int table=lua_gettop(L);const void*const result=lua_topointer(L,table);
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,0x73ca40,epoch,writes,uncertainty);
    native_locale_key(L,34);lua_pushstring(L,p.locale_language.type.c_str());lua_rawset(L,table);
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,0x73ca40,epoch,writes,uncertainty);
    native_locale_key(L,35);lua_pushstring(L,p.locale_language.display_name.c_str());lua_rawset(L,table);
    native_locale_operation_post(L,scope,child,source,vm,&actual_entry,0x73ca40,epoch,writes,uncertainty);
    if(!lua_istable(L,table)||lua_topointer(L,table)!=result||lua_getmetatable(L,table)){
        p.record_failure();return luaL_error(L,"Locale current-language actual result table replaced");}
    for(int i=0;i<2;++i){native_locale_key(L,i+34);lua_rawget(L,table);
        std::size_t n=0;const char*const text=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        const char*const expected=i==0?p.locale_language.type.c_str():p.locale_language.display_name.c_str();
        if(!text||n!=std::strlen(expected)||std::memcmp(text,expected,n)){
            p.record_failure();return luaL_error(L,"Locale current-language native Type/DisplayName result changed");}lua_pop(L,1);
    }
    p.locale_language={};p.native_locale_operation_entry=nullptr;p.native_locale_operation_function=0;return 1;
}
struct NativeLocaleEntry {const char* name;std::uint32_t original_function;lua_CFunction function;};
static constexpr NativeLocaleEntry native_locale_entries[]={
    {"ConvertTextKey",0x75b6a0,native_locale_convert_exact<0x75b6a0>}, // 0x5dd604
    {"HasTextKey",0x500920,native_locale_method<0x500920>}, // 0x5dd61e
    {"GetHotKeyDescription",0x6d3230,native_locale_method<0x6d3230>}, // 0x5dd63b
    {"Lookup",0x6f9b20,native_locale_convert_exact<0x6f9b20>}, // 0x5dd655
    {"LookupLanguage",0x48d4e0,native_locale_method<0x48d4e0>}, // 0x5dd66f
    {"StartsWith",0x6052c0,native_locale_method<0x6052c0>}, // 0x5dd68c
    {"EndsWith",0x47bd60,native_locale_method<0x47bd60>}, // 0x5dd6a6
    {"IsNilOrEmpty",0x760770,native_locale_method<0x760770>}, // 0x5dd6c0
    {"IsNilOrWhitespace",0x75a4f0,native_locale_method<0x75a4f0>}, // 0x5dd6dd
    {"Length",0x7030c0,native_locale_method<0x7030c0>}, // 0x5dd6f7
    {"ToUpper",0x6fb7f0,native_locale_method<0x6fb7f0>}, // 0x5dd711
    {"ToLower",0x6689f0,native_locale_method<0x6689f0>}, // 0x5dd72e
    {"ToCurrency",0x5f1110,native_locale_method<0x5f1110>}, // 0x5dd748
    {"ToNumber",0x641b50,native_locale_method<0x641b50>}, // 0x5dd762
    {"ToPercent",0x58ae80,native_locale_method<0x58ae80>}, // 0x5dd77f
    {"ToRomanNumeral",0x73c700,native_locale_method<0x73c700>}, // 0x5dd799
    {"ToSpellout",0x75cf70,native_locale_method<0x75cf70>}, // 0x5dd7b3
    {"ToOrdinal",0x710df0,native_locale_method<0x710df0>}, // 0x5dd7d0
    {"TruncateString",0x401fa0,native_locale_method<0x401fa0>}, // 0x5dd7ea
    {"Substring",0x4623d0,native_locale_method<0x4623d0>}, // 0x5dd804
    {"Compare",0x4f5570,native_locale_method<0x4f5570>}, // 0x5dd821
    {"SetCurrentLanguage",0x6b3e60,native_locale_method<0x6b3e60>}, // 0x5dd83b
    {"GetCurrentLanguage",0x73ca40,native_locale_current_exact}, // 0x5dd855
    {"GetSupportedLanguages",0x484f70,native_locale_method<0x484f70>}, // 0x5dd872
    {"GetCurrentSpokenLanguage",0x5d9890,native_locale_method<0x5d9890>}, // 0x5dd88c
    {"SetCurrentSpokenLanguage",0x581fd0,native_locale_method<0x581fd0>}, // 0x5dd8a6
    {"GetSupportedSpokenLanguages",0x4a2db0,native_locale_method<0x4a2db0>}, // 0x5dd8c3
    {"IsASCII",0x476c50,native_locale_method<0x476c50>}, // 0x5dd8dd
    {"IsUTF8",0x5e2460,native_locale_method<0x5e2460>}, // 0x5dd8f7
    {"LocalSystemCPToUTF8",0x6ce540,native_locale_method<0x6ce540>}, // 0x5dd914
    {"UTF8ToLocalSystemCP",0x4757d0,native_locale_method<0x4757d0>}, // 0x5dd92e
    {"ConvertPosixTimeToString",0x68b230,native_locale_method<0x68b230>}, // 0x5dd948
};
static constexpr const char* native_locale_retained_key(int i){return i==0?"Locale":i==33?"Type":i==34?"DisplayName":native_locale_entries[i-1].name;}
struct NativeContextLocaleFrame final {
    Impl& host;const NativeContextSteamFrame*const predecessor;
    const std::thread::id creator=std::this_thread::get_id();
    const std::uint64_t epoch,writes,uncertainty;
    int keys_ref=LUA_NOREF;const void* keys=nullptr;
    int master_ref=LUA_NOREF,proxy_ref=LUA_NOREF;const void* master=nullptr;const void* proxy=nullptr;const void* meta=nullptr;
    std::size_t fields_created=0,stores=0;Job* actual_job=nullptr;
    explicit NativeContextLocaleFrame(Impl& p):host(p),predecessor(p.steam_frame),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(host.locale_frame||!predecessor)throw ScriptError("Locale registrar requires actual Steam predecessor");
        predecessor->first();if(predecessor->stores!=1||predecessor->master_ref<0||!predecessor->master)throw ScriptError("Locale original predecessor did not execute original master/copies");
        host.locale_frame=this;
    }
    ~NativeContextLocaleFrame(){if(host.locale_frame==this)host.locale_frame=nullptr;}
    void first()const {
        if(host.locale_frame!=this||creator!=std::this_thread::get_id()||!predecessor||host.steam_frame!=predecessor||
           epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty)
            throw ScriptError("Locale registrar actual source/frame changed");
        predecessor->first();
    }
    void full()const {first();predecessor->full();first();}
};
static void native_locale_key(lua_State* L,int i){
    const auto& p=owner(L);lua_rawgeti(L,LUA_REGISTRYINDEX,p.native_locale_registration_keys_ref);lua_rawgeti(L,-1,i);lua_remove(L,-2);
}
static void native_locale_registration_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.locale_frame;p.source_exception={};p.source_error=nullptr;
    try{if(!f)throw ScriptError("Locale registrar has no genuine entered frame");f->full();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Locale registrar SAME source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(f->actual_job&&f->actual_job!=&job)return (void)luaL_error(L,"Locale registrar actual entered Job replaced");
    if(!f->predecessor->actual_job)return (void)luaL_error(L,"Locale genuine Steam predecessor Job retired");
    native_steam_registration_full(L,*f->predecessor->actual_job);
    p.source_exception={};p.source_error=nullptr;
    try{if(p.locale_frame!=f)throw ScriptError("Locale frame replaced after source callbacks");f->first();}
    catch(...){p.source_exception=std::current_exception();p.source_error="Locale registrar FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    const int top=lua_gettop(L);
    if(f->keys_ref!=LUA_NOREF){
        if(f->keys_ref!=p.native_locale_registration_keys_ref)return (void)luaL_error(L,"Locale registrar keys custody changed");
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->keys||lua_objlen(L,-1)!=35||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Locale registrar retained key table changed");
        for(int i=0;i<35;++i){
            lua_rawgeti(L,-1,i+1);std::size_t n=0;const char*const key=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!key||n!=std::strlen(native_locale_retained_key(i))||std::memcmp(key,native_locale_retained_key(i),n))
                return (void)luaL_error(L,"Locale registrar retained key TString changed");lua_pop(L,1);
        }lua_pop(L,1);
    }
    if(f->master_ref>=0){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->master)return (void)luaL_error(L,"Locale actual master table changed");
        native_locale_key(L,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Locale original master cache changed");lua_pop(L,2);
    }
    if(f->stores){
        if(f->stores!=1||f->proxy_ref<0||!f->proxy||!f->meta||f->master_ref<0)
            return (void)luaL_error(L,"Locale selected store lacks actual return");
        lua_rawgeti(L,LUA_REGISTRYINDEX,p.lua_events_frame->environment);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=p.lua_events_frame->actual_environment||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Locale actual selected fenv changed");
        native_locale_key(L,1);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->proxy_ref);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f->proxy||!lua_rawequal(L,-1,-2)||!lua_getmetatable(L,-1)||lua_topointer(L,-1)!=f->meta)
            return (void)luaL_error(L,"Locale selected proxy/ref/metatable changed");
        native_api_prefix_key(L,2);lua_rawget(L,-2);lua_rawgeti(L,LUA_REGISTRYINDEX,f->master_ref);
        if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Locale proxy actual master __index changed");lua_pop(L,2);
        native_api_prefix_key(L,3);lua_rawget(L,-2);
        if(lua_type(L,-1)!=LUA_TBOOLEAN||lua_toboolean(L,-1))return (void)luaL_error(L,"Locale proxy original false __metatable changed");lua_pop(L,1);
        native_api_prefix_key(L,4);lua_rawget(L,-2);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_api_readonly_write||lua_getupvalue(L,-1,1))
            return (void)luaL_error(L,"Locale proxy original write-error consumer changed");
    }
    lua_settop(L,top);
}
static int native_locale_factory(lua_State* L){
    auto& p=owner(L);auto*const f=p.locale_frame;
    if(!f||!f->actual_job)return luaL_error(L,"Native Locale constructor lacks its actual entered Job");
    auto& job=*f->actual_job;
    native_locale_registration_full(L,job);
    if(!f||f->fields_created||f->master_ref!=LUA_NOREF)return luaL_error(L,"Native Locale5DD5E0 constructor replay refused");
    lua_createtable(L,0,0);const int table=lua_gettop(L);const void*const value=lua_topointer(L,table);native_locale_registration_full(L,job);
    for(int i=0;i<32;++i){
        native_locale_key(L,i+2);lua_pushcfunction(L,native_locale_entries[i].function);lua_rawset(L,table);++f->fields_created;native_locale_registration_full(L,job);
    }
    if(f->fields_created!=32||!lua_istable(L,table)||lua_topointer(L,table)!=value||lua_getmetatable(L,table))
        return luaL_error(L,"Native Locale5DD5E0 actual namespace constructor changed");
    for(int i=0;i<32;++i){native_locale_key(L,i+2);lua_rawget(L,table);
        if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=native_locale_entries[i].function||lua_getupvalue(L,-1,1))return luaL_error(L,"Locale original zero-upvalue consumer changed");lua_pop(L,1);}
    return 1;
}
static void perform_native_context_locale_registration(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.locale_frame;native_locale_registration_full(L,job);
    if(!f||f->stores||f->keys_ref!=LUA_NOREF||f->actual_job||!lua_checkstack(L,32))
        return (void)luaL_error(L,"Locale original registrar replay/key custody/stack refused");
    f->actual_job=&job;
    if(p.native_locale_registration_keys_ref==LUA_NOREF){
        lua_createtable(L,35,0);const int keys=lua_gettop(L);const void*const key_value=lua_topointer(L,keys);native_locale_registration_full(L,job);
        for(int i=0;i<35;++i){lua_pushstring(L,native_locale_retained_key(i));lua_rawseti(L,keys,i+1);native_locale_registration_full(L,job);}
        lua_pushvalue(L,keys);const int key_ref=luaL_ref(L,LUA_REGISTRYINDEX);
        if(key_ref<0)return (void)luaL_error(L,"Locale actual retained key allocation failed");
        f->keys_ref=key_ref;f->keys=key_value;p.native_locale_registration_keys_ref=key_ref;p.native_locale_registration_keys_pointer=key_value;
        native_locale_registration_full(L,job);lua_pop(L,1);
    }else{
        f->keys_ref=p.native_locale_registration_keys_ref;f->keys=p.native_locale_registration_keys_pointer;native_locale_registration_full(L,job);
    }
    // Original6DFB60/5605C0 selects actual518 before inspecting
    // GLOBALS.Locale; a cache miss runs5DD5E0 zero args/one result.
    perform_native_context_state_query(L,job);native_locale_registration_full(L,job);const int environment=lua_gettop(L);
    if(lua_topointer(L,environment)!=p.lua_events_frame->actual_environment)
        return (void)luaL_error(L,"Locale registrar actual prepared518 environment changed");
    native_locale_key(L,1);lua_gettable(L,LUA_GLOBALSINDEX);native_locale_registration_full(L,job);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);lua_pushcfunction(L,native_locale_factory);lua_call(L,0,1);native_locale_registration_full(L,job);
        native_locale_key(L,1);lua_pushvalue(L,-2);lua_settable(L,LUA_GLOBALSINDEX);native_locale_registration_full(L,job);
    }
    if(!lua_istable(L,-1))return (void)luaL_error(L,"Locale original namespace master is not a table");
    f->master=lua_topointer(L,-1);lua_pushvalue(L,-1);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_locale_registration_full(L,job);
    native_api_readonly_proxy(L,job);native_locale_registration_full(L,job);const int proxy=lua_gettop(L);f->proxy=lua_topointer(L,proxy);
    if(!lua_getmetatable(L,proxy))return (void)luaL_error(L,"Locale original49B190 proxy has no metatable");f->meta=lua_topointer(L,-1);lua_pop(L,1);
    native_locale_key(L,1);lua_pushvalue(L,proxy);lua_settable(L,environment);native_locale_registration_full(L,job);
    lua_pushvalue(L,proxy);f->proxy_ref=luaL_ref(L,LUA_REGISTRYINDEX);++f->stores;native_locale_registration_full(L,job);
    job.context_locale_master_reference=f->master_ref;job.context_locale_proxy_reference=f->proxy_ref;
    job.context_locale_table=f->master;job.context_locale_proxy=f->proxy;lua_pop(L,2);
    native_locale_registration_full(L,job);worldview_capture_locale_publication_owned(L,job);native_locale_registration_full(L,job);
}
