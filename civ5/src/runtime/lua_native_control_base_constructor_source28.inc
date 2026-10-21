//Inside the SAME Host::Impl, after immutable Source8 definition/metatype core.
//Portable authoritative shared queue born in this real Host construction.
//It is NOT a reconstructed Original-process CB7168/flag snapshot, and pending
//registrations do not issue callback execution or method-library readiness.
struct NativeDefinitionDeferredQueue final {
    struct Registration final {
        const std::uint32_t original_callback;
        const void*const actual_constructor;
        void(*const body)(lua_State*);
    };
    const Impl*const host;
    const std::thread::id creator=std::this_thread::get_id();
    bool alive=true;
    std::uint32_t flags=0,count=0;
    std::array<std::unique_ptr<Registration>,1024>entries;
    explicit NativeDefinitionDeferredQueue(const Impl*p):host(p){}
    void require_origin()const {
        if(!host||!alive||creator!=std::this_thread::get_id()||count>entries.size())
            throw ScriptError("native deferred-definition queue owner changed");
    }
    const Registration*register_control_base_owned(const void*actual,void(*body)(lua_State*)){
        require_origin();if(!actual||!body)throw ScriptError("native deferred registration lacks real actor/body");
        //Actual521125 lazy shared bit/zero-count path. Initialization belongs
        //to this truly fresh port owner, never inferred from old process data.
        if(!(flags&1u)){flags|=1u;count=0;}
        if(count>=1024u)return nullptr; //Actual521157 JNC, normal no-append.
        auto entry=std::unique_ptr<Registration>(new Registration{0x7a4df0u,actual,body});
        const auto*out=entry.get();entries[count]=std::move(entry);++count;
        require_origin();return out;
    }
};
struct NativeControlBaseConstructor final {
    enum class Stage {entered,scalar_stores,callback_cells,registered,named,records,returned,retired};
    const Impl*const host;
    const std::thread::id creator=std::this_thread::get_id();
    bool alive=true;
    const std::shared_ptr<NativeDefinitionDeferredQueue>queue;
    const std::shared_ptr<NativeContextDefinition>definition;
    Stage stage=Stage::entered;
    //Real current writer identity/serial, distinct from Source8 birth facts.
    std::uint32_t current548_writer=0,current54c_writer=0;
    std::uint64_t current548_revision=0,current54c_revision=0;
    bool lazy_flag=false;
    void(*cdata_cell_ca8208)(lua_State*)=nullptr;
    std::uint32_t library_cell_ca8d24=0; //Qualified recipe, not callable x86 address.
    const NativeDefinitionDeferredQueue::Registration*registration=nullptr;
    std::uint32_t queue_count_at_return=0;
    const char*mutable_name_c809e0=nullptr,*const_name_c81ba4=nullptr;
    std::shared_ptr<const NativeOrdinaryNestedDescriptorReturn>canonical;
    NativeControlBaseConstructor(const Impl*p,std::shared_ptr<NativeDefinitionDeferredQueue>q,
        std::shared_ptr<NativeContextDefinition>d):host(p),queue(std::move(q)),definition(std::move(d)){}
    void require_origin()const {
        if(!host||!alive||creator!=std::this_thread::get_id()||!queue||queue->host!=host||
            !definition||definition->host!=host||!definition->alive.load()||definition->context)
            throw ScriptError("ControlBase real constructor/definition/queue owner changed");
        queue->require_origin();
    }
    void require_returned()const {
        require_origin();
        if(stage!=Stage::returned||!lazy_flag||definition->count544!=49||
            definition->name0!="ControlBase"||definition->name4!="const ControlBase"||
            definition->stage!=NativeContextDefinition::Stage::metadata_returned||
            current548_writer!=0x5210fau||current54c_writer!=0x521100u||
            current548_revision!=1||current54c_revision!=1||
            definition->current548!=NativeContextDefinition::Callback::none||
            definition->current54c!=NativeContextDefinition::Callback::none||
            cdata_cell_ca8208!=&Impl::native_context_base_metatype_6a6920_owned||library_cell_ca8d24!=0x4acbf0u||
            mutable_name_c809e0!=definition->name0.c_str()||const_name_c81ba4!=definition->name4.c_str())
            throw ScriptError("ControlBase current writer/name/whole constructor return changed");
        if(registration){
            bool present=false;
            for(std::size_t i=0;i<queue->count;++i)if(queue->entries[i].get()==registration){present=true;break;}
            if(!present||registration->actual_constructor!=this||registration->original_callback!=0x7a4df0u||
                registration->body!=&Impl::native_control_base_deferred_7a4df0_owned)
                throw ScriptError("ControlBase real deferred registration changed");
        }else if(queue_count_at_return<1024u){
            throw ScriptError("ControlBase deferred no-append lacks actual saturated queue outcome");
        }
    }
};
void initialize_native_definition_deferred_queue_owned(){
    if(state||native_definition_deferred_queue)
        throw ScriptError("native shared definition queue creation replay/after VM birth");
    native_definition_deferred_queue=std::make_shared<NativeDefinitionDeferredQueue>(this);
}
void initialize_native_control_base_constructor_owned(){
    if(state||native_control_base_constructor||native_ordinary_nested_descriptors[0]||
        !native_definition_deferred_queue||native_definition_deferred_queue->host!=this)
        throw ScriptError("ControlBase whole constructor replay/foreign owner/late creation");
    //Source8's prior parent is explicitly metadata-only and has NEVER issued
    //the whole ctor loan. Use fresh distinct storage; do not promote/replay it.
    const auto prior=native_context_definitions[1];
    if(!prior||prior->host!=this||!prior->alive.load()||prior->context)
        throw ScriptError("ControlBase predecessor metadata lineage missing");
    auto definition=std::make_shared<NativeContextDefinition>(this,false,nullptr);
    auto actual=std::make_shared<NativeControlBaseConstructor>(this,native_definition_deferred_queue,definition);
    actual->require_origin();
    actual->lazy_flag=true; //Real constructor occurrence entered once.
    //Actual5210F4/5210FA/521100 authoritative live storage writes. Current
    //548 is earned here, not copied from Source8::birth548 or an old save/cache.
    definition->count544=0;
    definition->current548=NativeContextDefinition::Callback::none;
    actual->current548_writer=0x5210fau;++actual->current548_revision;
    definition->current54c=NativeContextDefinition::Callback::none;
    actual->current54c_writer=0x521100u;++actual->current54c_revision;
    definition->name0="Unkown"; //Exact seven NUL-terminated bytes at9D3F40.
    actual->stage=NativeControlBaseConstructor::Stage::scalar_stores;
    //Actual521111 and52111B: two separate callback-cell stores precede queue.
    actual->cdata_cell_ca8208=&Impl::native_context_base_metatype_6a6920_owned;
    actual->library_cell_ca8d24=0x4acbf0u;
    actual->stage=NativeControlBaseConstructor::Stage::callback_cells;
    actual->registration=actual->queue->register_control_base_owned(actual.get(),&Impl::native_control_base_deferred_7a4df0_owned);
    actual->queue_count_at_return=actual->queue->count;
    actual->stage=NativeControlBaseConstructor::Stage::registered;
    //Actual9BDDE0->549B00 positive exact source; port observable format output.
    //Logical64-byte limit retained; padding/errno/native formatter bookkeeping
    //is not invented as an Original CRT-return marker.
    definition->name0="ControlBase";definition->name4="const ControlBase";
    if(definition->name4.size()+1>64)throw ScriptError("ControlBase real name64 overflow");
    actual->mutable_name_c809e0=definition->name0.c_str();
    actual->const_name_c81ba4=definition->name4.c_str();
    definition->stage=NativeContextDefinition::Stage::name;
    actual->stage=NativeControlBaseConstructor::Stage::named;
    //All49 exact known writers run on the SAME actually constructed storage.
    //Their complete bodies touch only count/record fields; latest548 remains
    //the actually executed5210FA writer. Flags1 are preserved, not zero-filled.
#include "lua_native_context_definition_recipes.inc"
    for(const auto&r:base_rows){
        if(definition->count544>=definition->records.size())throw ScriptError("ControlBase actual49 writer overflow");
        auto&slot=definition->records[definition->count544];++definition->count544;
        slot.key=r.key;slot.primary=r.primary;slot.secondary=r.secondary;
        slot.flag=r.flag;slot.reference=-1;slot.recipe=&r;
    }
    actual->stage=NativeControlBaseConstructor::Stage::records;
    definition->stage=NativeContextDefinition::Stage::metadata_returned;
    actual->stage=NativeControlBaseConstructor::Stage::returned;actual->require_returned();
    auto loan=std::shared_ptr<const NativeOrdinaryNestedDescriptorReturn>(new NativeOrdinaryNestedDescriptorReturn(
        this,NativeOrdinaryNestedDescriptorReturn::Kind::descriptor_cb7c68,0xcb7c68u,0x5210f0u,definition->name0));
    actual->canonical=loan;
    //All required effects and exact latest-writer checks precede publication.
    prior->alive.store(false);
    native_control_base_constructor=std::move(actual);
    native_context_definitions[1]=std::move(definition);
    native_ordinary_nested_descriptors[0]=std::move(loan);
    native_control_base_constructor->require_returned();
}
static void native_control_base_current_definition_read_owned(lua_State*L,const NativeContextDefinition&offered){
    auto&p=owner(L);
    if(!native_flua_exposure_metadata_current_if_entered(L,0xcb7c68u))native_ordinary_metatype_full(L);
    p.source_exception={};p.source_error=nullptr;
    try{
        const auto&actual=p.native_control_base_constructor;
        if(!actual||actual->host!=&p||actual->definition.get()!=&offered||
            p.native_context_definitions[1].get()!=&offered||
            actual->queue!=p.native_definition_deferred_queue||
            !actual->canonical||p.native_ordinary_nested_descriptors[0]!=actual->canonical)
            throw ScriptError("ControlBase current-field borrower is not the actual constructor owner");
        actual->require_returned(); //Fresh live548/54C + latest writer + queue/name.
    }catch(...){p.source_exception=std::current_exception();p.source_error="ControlBase actual current writer/constructor Source borrow refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
}
static void native_control_base_metadata_registration_full_owned(lua_State*L){
 if(native_flua_exposure_metadata_current_if_entered(L,0xcb7c68u))return;
 native_postread_root_cdata_full(L);
}
static void native_control_base_deferred_7a4df0_owned(lua_State*L){
    auto&p=owner(L);
    native_control_base_metadata_registration_full_owned(L);
    if(!p.native_control_base_constructor)
        return (void)luaL_error(L,"required actual ControlBase ctor before deferred7A4DF0");
    //Guarded C++ origin reads can throw; the actual Source7 protected caller
    //converts them before any Lua longjmp, as for existing615/std suppliers.
    p.native_control_base_constructor->require_returned();
    const auto*d=native_context_current_definition_owned(L,false);
    //Original7A4E1B uses the actual caller's ExposedTypes table at-2. No
    //replacement table/currentness/listener or VM-ready marker is emitted.
    if(lua_gettop(L)<2||lua_type(L,-2)!=LUA_TTABLE||lua_type(L,-1)!=LUA_TTABLE)
        return (void)luaL_error(L,"deferred7A4DF0 requires genuine Types then Members caller tables");
    lua_getfield(L,-2,d->name0.c_str());native_control_base_metadata_registration_full_owned(L);
    if(lua_type(L,-1)!=LUA_TTABLE){
        lua_settop(L,-2);native_control_base_metadata_registration_full_owned(L); //Actual7A4E33 pop1
        native_context_construct_metatype_517c20_owned(L,false);native_control_base_metadata_registration_full_owned(L);
    }
    //Actual7A4E4C invokes the executing mutable library. Its49 rows return
    //without invoking deferred methods; the following caller effect is strict.
    native_control_base_mutable_library_47c500_owned(L);
    return (void)luaL_error(L,"actual47C500 mutable49 table returned; required7A4E54 rawset(-3) and real const4E35C0/663140 metatype/library registration before class-cache return; no whole49 runtime readiness issued");
}
void native_control_base_constructor_retire()noexcept{
    if(native_control_base_constructor){auto&r=*native_control_base_constructor;
        if(r.creator!=std::this_thread::get_id())std::terminate();
        r.alive=false;r.stage=NativeControlBaseConstructor::Stage::retired;
        if(r.canonical)r.canonical->alive.store(false);
        if(r.definition)r.definition->alive.store(false);
    }
    if(native_definition_deferred_queue)native_definition_deferred_queue->alive=false;
}
