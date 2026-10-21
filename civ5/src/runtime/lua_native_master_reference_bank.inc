// Original5C2790 bank constructor and its five native debug subscriptions.
// The canonical registry below belongs only to this actual master domain.
// It is not Events112, an emitter, a completed master or a default-scope grant.
class NativeMasterReferenceBankOwner;
struct NativeMasterReferenceBankCtorFrame final {
    Impl& host;const ScriptSourceProvider*const source;
    const std::thread::id creator;const std::uint64_t epoch,writes,uncertainty;
    explicit NativeMasterReferenceBankCtorFrame(Impl&p):host(p),source(&p.sources),creator(std::this_thread::get_id()),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(p.native_master_reference_bank_ctor_frame||p.native_master_reference_bank||p.native_master_debug_subscriptions||
           p.state||p.calling||creator!=p.ui_thread||p.failed.exhausted_)
            throw ScriptError("Native Lua bank requires actual first pre-VM master constructor");
        p.native_master_reference_bank_ctor_frame=this;
    }
    ~NativeMasterReferenceBankCtorFrame(){if(host.native_master_reference_bank_ctor_frame==this)host.native_master_reference_bank_ctor_frame=nullptr;}
    NativeMasterReferenceBankCtorFrame(const NativeMasterReferenceBankCtorFrame&)=delete;
    NativeMasterReferenceBankCtorFrame& operator=(const NativeMasterReferenceBankCtorFrame&)=delete;
    void current()const {
        if(host.native_master_reference_bank_ctor_frame!=this||source!=&host.sources||host.state||host.calling||
           creator!=std::this_thread::get_id()||creator!=host.ui_thread||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("Native Lua bank constructor actual Source/frame/failure changed");
    }
};
class NativeMasterDebugSubscriptionRegistry final {
    friend struct Impl;
    using Consumer=void(*)(Impl&,NativeMasterReferenceBankOwner&);
    struct Entry final {
        friend struct Impl;friend class NativeMasterDebugSubscriptionRegistry;
        const std::uint32_t add,remove,original_callback,original_selected_slot;const Consumer consumer;
        constexpr Entry(std::uint32_t a,std::uint32_t r,std::uint32_t c,std::uint32_t s,Consumer f)
            :add(a),remove(r),original_callback(c),original_selected_slot(s),consumer(f){}
    };
    struct Subscription final {
        const Entry*const entry;NativeMasterReferenceBankOwner*const receiver;
        std::uint64_t serial;bool retired=false;
        Subscription(const Entry&e,NativeMasterReferenceBankOwner&r,std::uint64_t s):entry(&e),receiver(&r),serial(s){}
    };
    Impl& host;const ScriptSourceProvider*const source;const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    volatile LONG mutation_lock=0;std::uint64_t serial=0;
    std::array<std::vector<std::unique_ptr<Subscription>>,5> selected_channels;
    enum class Phase {constructing,registered,vm_bound,retired};Phase phase=Phase::constructing;
    lua_State* master=nullptr;global_State* actual_global=nullptr;const Table* canonical_lua_registry=nullptr;
    NativeMasterDebugSubscriptionRegistry(Impl&p,const NativeMasterReferenceBankCtorFrame&f):host(p),source(&p.sources),
        creator(f.creator),epoch(f.epoch),writes(f.writes),uncertainty(f.uncertainty){f.current();}
    NativeMasterDebugSubscriptionRegistry(const NativeMasterDebugSubscriptionRegistry&)=delete;
    NativeMasterDebugSubscriptionRegistry& operator=(const NativeMasterDebugSubscriptionRegistry&)=delete;
    struct Mutation final {
        NativeMasterDebugSubscriptionRegistry& subscription_registry;
        explicit Mutation(NativeMasterDebugSubscriptionRegistry&r):subscription_registry(r){
            // Original registrar/remover5 uses one actual Interlocked lock for
            // these selected channel lists. No Lua API crosses this C++ lock.
            while(::InterlockedCompareExchange(&subscription_registry.mutation_lock,1,0)!=0){}
        }
        ~Mutation(){::InterlockedExchange(&subscription_registry.mutation_lock,0);}
        Mutation(const Mutation&)=delete;Mutation& operator=(const Mutation&)=delete;
    };
    void source_current()const {
        if(host.native_master_debug_subscriptions.get()!=this||source!=&host.sources||phase==Phase::retired||
           creator!=std::this_thread::get_id()||creator!=host.ui_thread||epoch!=host.failure_epoch||writes!=host.failed.writes_||
           uncertainty!=host.execution_uncertainty||host.failed.exhausted_)
            throw ScriptError("Native Lua selected debug registry owner/Source retired");
    }
    void add_owned(std::size_t channel,const Entry&e,NativeMasterReferenceBankOwner&receiver,const NativeMasterReferenceBankCtorFrame&f){
        f.current();source_current();
        if(phase!=Phase::constructing||channel>=selected_channels.size()||serial!=channel||
           !selected_channels[channel].empty()||!e.consumer||host.native_master_reference_bank.get()!=&receiver)
            throw ScriptError("Native Lua bank native subscription original order/receiver differs");
        {
            Mutation locked(*this);
            // Real8A7040/8A7B40 copies receiver and callback into current list
            // storage and appends after the old tail. No callback is invoked.
            selected_channels[channel].emplace_back(new Subscription(e,receiver,++serial));
        }
        f.current();source_current();
    }
    void require_five_owned(const Entry* entries,const NativeMasterReferenceBankOwner&receiver)const {
        source_current();
        if(!entries||serial!=5||phase==Phase::retired)throw ScriptError("Native Lua actual five bank subscriptions incomplete");
        for(std::size_t i=0;i<5;++i){
            const auto& channel=selected_channels[i];
            if(channel.size()!=1||!channel[0]||channel[0]->retired||channel[0]->serial!=i+1||
               channel[0]->entry!=entries+i||channel[0]->receiver!=&receiver||!channel[0]->entry->consumer)
                throw ScriptError("Native Lua actual debug subscription receiver/callback custody changed");
        }
    }
    void remove_owned(std::size_t channel,const Entry&e,NativeMasterReferenceBankOwner&receiver)noexcept {
        if(channel>=selected_channels.size())return;
        Mutation locked(*this);auto& nodes=selected_channels[channel];
        // Original five removers stop at the first matching receiver/callback
        // pair. A distinct selected channel is retained even for4E8430 alias.
        for(auto it=nodes.begin();it!=nodes.end();++it)if(*it&&(*it)->receiver==&receiver&&
            (*it)->entry->original_callback==e.original_callback){(*it)->retired=true;nodes.erase(it);return;}
    }
    void retire()noexcept {
        if(phase==Phase::retired)return;
        for(auto& channel:selected_channels)for(auto& sub:channel)if(sub)sub->retired=true;
        for(auto& channel:selected_channels)channel.clear();
        master=nullptr;actual_global=nullptr;canonical_lua_registry=nullptr;phase=Phase::retired;
    }
public:~NativeMasterDebugSubscriptionRegistry(){retire();}
};
class NativeMasterReferenceBankOwner final {
    friend struct Impl;
    struct OriginalRecord final {std::array<unsigned char,0x104> original_payload;};
    struct OwnedLuaRef final {lua_State* actual_vm;int actual_reference;};
    struct SelfLink final {const std::unique_ptr<SelfLink>*const actual_slot;explicit SelfLink(const std::unique_ptr<SelfLink>*s):actual_slot(s){}};
    struct OriginalEmptyVector final {void* storage=nullptr;std::uint32_t count=0,capacity=0;};
    Impl& host;const ScriptSourceProvider*const source;const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    const NativeMasterReferenceBankCtorFrame* constructor;
    enum class Phase {created,storage_reserved,socket_created,fields_initialized,options_completed,subscribed,vm_bound,retired};
    Phase phase=Phase::created;
    std::unique_ptr<OriginalRecord[]> records;std::uint32_t record_count=0,record_capacity=0;
    std::unique_ptr<OwnedLuaRef[]> references;std::uint32_t reference_count=0,reference_capacity=0;
    void* collector_owner=nullptr;void* collector_callback=nullptr;void* debugger_client=nullptr;
    NativeMasterDebugSocketOwner* actual_socket_owner=nullptr;std::uint32_t native_socket_bank_mode=0;
    std::unique_ptr<SelfLink> self_link;
    std::array<void*,4> original_debug_pointers{};float original_elapsed=0;std::uint32_t original_update_counter=0;
    std::array<unsigned char,4> original_debug_flags{};std::array<OriginalEmptyVector,3> original_debug_vectors{};
    std::uint32_t original_free_head=0x0fffffff,original_free_count=0;
    OriginalEmptyVector original_breakpoints{};std::uint32_t original_breakpoint_auxiliary=0;
    std::uint32_t original_breakpoint_head=0x0fffffff,original_breakpoint_tail=0x0fffffff;
    lua_State* master=nullptr;global_State* actual_global=nullptr;const Table* canonical_lua_registry=nullptr;
    NativeMasterDebugSubscriptionRegistry* selected_registry=nullptr;
    NativeMasterReferenceBankOwner(Impl&p,const NativeMasterReferenceBankCtorFrame&f):host(p),source(&p.sources),creator(f.creator),
        epoch(f.epoch),writes(f.writes),uncertainty(f.uncertainty),constructor(&f){}
    NativeMasterReferenceBankOwner(const NativeMasterReferenceBankOwner&)=delete;
    NativeMasterReferenceBankOwner& operator=(const NativeMasterReferenceBankOwner&)=delete;
    void constructor_current()const {
        if(host.native_master_reference_bank.get()!=this||!constructor||host.native_master_reference_bank_ctor_frame!=constructor||
           phase==Phase::retired||source!=&host.sources||creator!=std::this_thread::get_id())
            throw ScriptError("Native Lua bank real constructor owner/frame changed");
        constructor->current();
    }
    void reserve_original_storage(){
        constructor_current();if(phase!=Phase::created)throw ScriptError("Native Lua bank storage constructor replay");
        // Real allocations in original order. Unused capacity is never read
        // as eight initialized LuaRefs; count remains the actual current zero.
        records.reset(new OriginalRecord[8]);record_capacity=8;constructor_current();
        references.reset(new OwnedLuaRef[8]);reference_capacity=8;constructor_current();
        collector_owner=nullptr;collector_callback=nullptr;debugger_client=nullptr;phase=Phase::storage_reserved;
    }
    void initialize_original_fields_after_socket(){
        constructor_current();if(phase!=Phase::socket_created)throw ScriptError("Native Lua bank fields require actual socket prefix");
        self_link.reset(new SelfLink(&self_link));constructor_current();
        const std::uint32_t original_float_bits=0x42c7cccd;static_assert(sizeof(original_elapsed)==sizeof(original_float_bits));
        std::memcpy(&original_elapsed,&original_float_bits,sizeof(original_elapsed));
        original_debug_pointers.fill(nullptr);original_update_counter=0;original_debug_flags.fill(0);
        for(auto& vector:original_debug_vectors){vector.storage=nullptr;vector.count=0;vector.capacity=0;}
        original_free_head=0x0fffffff;original_free_count=0;
        original_breakpoints.storage=nullptr;original_breakpoints.count=0;original_breakpoints.capacity=0;
        original_breakpoint_auxiliary=0;original_breakpoint_head=0x0fffffff;original_breakpoint_tail=0x0fffffff;
        native_socket_bank_mode=2;phase=Phase::fields_initialized;constructor_current();
    }
    void physical_current_without_callbacks()const {
        if(host.native_master_reference_bank.get()!=this||constructor||host.native_master_reference_bank_ctor_frame||phase!=Phase::vm_bound||
           source!=&host.sources||creator!=std::this_thread::get_id()||creator!=host.ui_thread||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_||!master||host.state!=master||
           !actual_global||G(master)!=actual_global||!ttistable(&actual_global->l_registry)||hvalue(&actual_global->l_registry)!=canonical_lua_registry||
           !records||!references||record_capacity!=8||reference_capacity!=8||record_count>record_capacity||reference_count>reference_capacity||
           !self_link||self_link->actual_slot!=&self_link||!actual_socket_owner||host.native_master_debug_socket.get()!=actual_socket_owner||
           !selected_registry||host.native_master_debug_subscriptions.get()!=selected_registry||
           selected_registry->phase!=NativeMasterDebugSubscriptionRegistry::Phase::vm_bound||selected_registry->master!=master||
           selected_registry->actual_global!=actual_global||selected_registry->canonical_lua_registry!=canonical_lua_registry)
            throw ScriptError("Native Lua bank actual current master/VM/registry/storage retired");
        actual_socket_owner->require_current_without_callbacks();
    }
    void retire_prefix()noexcept {
        if(phase==Phase::retired)return;
        // No LuaRef publication/collector issuer exists in this prefix. If a
        // future writer is added it must also earn the real protected cleanup;
        // nonempty storage is never treated as successfully reset here.
        if(reference_count!=0||record_count!=0||collector_owner||collector_callback||debugger_client)host.record_failure();
        constructor=nullptr;master=nullptr;actual_global=nullptr;canonical_lua_registry=nullptr;selected_registry=nullptr;actual_socket_owner=nullptr;
        self_link.reset();references.reset();records.reset();phase=Phase::retired;
    }
public:~NativeMasterReferenceBankOwner(){retire_prefix();}
};
// Original ctor registers these consumers without invoking their required
// payload/backends. Each reached invocation is an explicit mandatory STOP.
// No empty event, debugger-disabled policy or generic success is issued here.
static void native_master_debug_required_465b00(Impl&p,NativeMasterReferenceBankOwner&bank){bank.physical_current_without_callbacks();p.record_failure();throw ScriptError("Native Lua debug465B00 requires actual current debug event payload/transport consumer; provider unissued");}
static void native_master_debug_required_6559b0(Impl&p,NativeMasterReferenceBankOwner&bank){bank.physical_current_without_callbacks();p.record_failure();throw ScriptError("Native Lua debug6559B0 requires its actual current event/backend consumer; provider unissued");}
static void native_master_debug_required_73a000(Impl&p,NativeMasterReferenceBankOwner&bank){bank.physical_current_without_callbacks();p.record_failure();throw ScriptError("Native Lua debug73A000 requires actual current debug-domain input/client consumer; provider unissued");}
static void native_master_debug_required_4e8430(Impl&p,NativeMasterReferenceBankOwner&bank){bank.physical_current_without_callbacks();p.record_failure();throw ScriptError("Native Lua debug4E8430 requires actual504A50 cleanup consumer; provider unissued");}
inline static const NativeMasterDebugSubscriptionRegistry::Entry native_master_debug_bank_entries[]={
    {0x8878c0,0x884cc0,0x465b00,0x01c,native_master_debug_required_465b00},
    {0x8b14a0,0x8ac0c0,0x6559b0,0x464,native_master_debug_required_6559b0},
    {0x8973a0,0x8965e0,0x73a000,0x514,native_master_debug_required_73a000},
    {0x86e220,0x86de00,0x4e8430,0x454,native_master_debug_required_4e8430},
    {0x86e280,0x86df00,0x4e8430,0x45c,native_master_debug_required_4e8430}
};
void initialize_native_master_reference_bank_before_vm_owned(){
    {
        NativeMasterReferenceBankCtorFrame frame(*this);frame.current();
        native_master_reference_bank.reset(new NativeMasterReferenceBankOwner(*this,frame));frame.current();
        auto*const bank=native_master_reference_bank.get();
        native_master_debug_subscriptions.reset(new NativeMasterDebugSubscriptionRegistry(*this,frame));frame.current();
        try{
            bank->reserve_original_storage();frame.current();
            {
                NativeMasterDebugSocketCtorFrame socket_frame(*this);socket_frame.current();
                native_master_debug_socket.reset(new NativeMasterDebugSocketOwner(*this,socket_frame));socket_frame.current();
                native_master_debug_socket->construct_socket_prefix();socket_frame.current();frame.current();
                bank->actual_socket_owner=native_master_debug_socket.get();bank->phase=NativeMasterReferenceBankOwner::Phase::socket_created;
                bank->initialize_original_fields_after_socket();frame.current();socket_frame.current();
                native_master_debug_socket->construct_options_suffix();socket_frame.current();frame.current();
                native_master_debug_socket->constructor=nullptr;
            }
            native_master_debug_socket->require_current_without_callbacks();frame.current();bank->phase=NativeMasterReferenceBankOwner::Phase::options_completed;
            for(std::size_t i=0;i<5;++i){native_master_debug_subscriptions->add_owned(i,native_master_debug_bank_entries[i],*bank,frame);frame.current();}
            native_master_debug_subscriptions->require_five_owned(native_master_debug_bank_entries,*bank);frame.current();
            native_master_debug_subscriptions->phase=NativeMasterDebugSubscriptionRegistry::Phase::registered;
            bank->selected_registry=native_master_debug_subscriptions.get();bank->phase=NativeMasterReferenceBankOwner::Phase::subscribed;bank->constructor=nullptr;
        }catch(...){native_master_reference_bank_retire();native_master_debug_socket_retire();throw;}
    }
    if(!native_master_reference_bank||native_master_reference_bank->constructor||native_master_reference_bank_ctor_frame||
       native_master_reference_bank->phase!=NativeMasterReferenceBankOwner::Phase::subscribed)
        throw ScriptError("Native Lua bank constructor actual return incomplete");
}
void bind_native_master_reference_bank_actual_vm_owned(){
    auto*const bank=native_master_reference_bank.get();auto*const registry=native_master_debug_subscriptions.get();
    if(!state||calling||native_master_reference_bank_ctor_frame||!bank||bank->constructor||!registry||
       bank->phase!=NativeMasterReferenceBankOwner::Phase::subscribed||registry->phase!=NativeMasterDebugSubscriptionRegistry::Phase::registered||
       !ttistable(&G(state)->l_registry))throw ScriptError("Native Lua bank binding requires actual newly created master/registry");
    registry->require_five_owned(native_master_debug_bank_entries,*bank);bank->actual_socket_owner->require_current_without_callbacks();
    bank->master=state;bank->actual_global=G(state);bank->canonical_lua_registry=hvalue(&G(state)->l_registry);
    registry->master=state;registry->actual_global=bank->actual_global;registry->canonical_lua_registry=bank->canonical_lua_registry;
    registry->phase=NativeMasterDebugSubscriptionRegistry::Phase::vm_bound;bank->phase=NativeMasterReferenceBankOwner::Phase::vm_bound;
    bank->physical_current_without_callbacks();
    // This empty state is earned solely from the completed live constructor.
    // Master catalog/collector/active ledger/defaultScope remain mandatory.
    if(bank->record_count||bank->reference_count||bank->collector_owner||bank->collector_callback||bank->debugger_client)
        throw ScriptError("Native Lua bank actual initial current counts/links differ");
}
static NativeMasterReferenceBankOwner* native_master_reference_bank_current_owned(lua_State*L){
    auto& p=owner(L);const auto saved=native_master_thread_callback_first(L);
    auto*const bank=p.native_master_reference_bank.get();p.source_error=nullptr;p.source_exception={};
    try{
        if(!bank||!p.state||G(L)!=G(p.state))throw ScriptError("Native Lua bank offered thread is not in the actual master VM");
        bank->physical_current_without_callbacks();bank->selected_registry->require_five_owned(native_master_debug_bank_entries,*bank);
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native Lua bank genuine current VM/registry FIRST refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    native_master_thread_callback_post(L,saved);
    // Source POST may itself run a provider callback. Recover the owning slot
    // again before any physical raw pointer read or returning the bank loan.
    if(p.native_master_reference_bank.get()!=bank)
        return (native_master_thread_refuse(L,"Native Lua bank owner changed during Source POST"),nullptr);
    p.source_error=nullptr;p.source_exception={};
    try{bank->physical_current_without_callbacks();bank->selected_registry->require_five_owned(native_master_debug_bank_entries,*bank);}
    catch(...){p.source_exception=std::current_exception();p.source_error="Native Lua bank callback-free physical POST refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    return bank;
}
void native_master_reference_bank_retire()noexcept {
    native_master_reference_bank_ctor_frame=nullptr;
    auto*const bank=native_master_reference_bank.get();auto*const registry=native_master_debug_subscriptions.get();
    // Original4561F0 removes these same five receiver/callback pairs first.
    if(bank&&registry)for(std::size_t i=0;i<5;++i)registry->remove_owned(i,native_master_debug_bank_entries[i],*bank);
    if(bank)bank->retire_prefix();if(registry)registry->retire();
}
