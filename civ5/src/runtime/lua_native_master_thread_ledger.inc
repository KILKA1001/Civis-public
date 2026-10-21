// Real477B20+81904..81920 active-thread ledger initial stores and the actual
//7458E0 first matching node+8=NULL mutation. No default-thread loan is issued.
struct NativeMasterThreadLedgerCtorFrame final {
    Impl& host;const ScriptSourceProvider*const source;const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    explicit NativeMasterThreadLedgerCtorFrame(Impl&p):host(p),source(&p.sources),creator(std::this_thread::get_id()),
        epoch(p.failure_epoch),writes(p.failed.writes_),uncertainty(p.execution_uncertainty){
        if(p.native_master_thread_ledger||p.native_master_thread_ledger_ctor_frame||p.state||p.calling||creator!=p.ui_thread||p.failed.exhausted_||
           !p.native_master_reference_bank||p.native_master_reference_bank_ctor_frame||
           p.native_master_reference_bank->phase!=NativeMasterReferenceBankOwner::Phase::subscribed)
            throw ScriptError("Native Lua ledger requires actual returned5C2790 bank before VM creation");
        p.native_master_thread_ledger_ctor_frame=this;
    }
    ~NativeMasterThreadLedgerCtorFrame(){if(host.native_master_thread_ledger_ctor_frame==this)host.native_master_thread_ledger_ctor_frame=nullptr;}
    NativeMasterThreadLedgerCtorFrame(const NativeMasterThreadLedgerCtorFrame&)=delete;
    NativeMasterThreadLedgerCtorFrame& operator=(const NativeMasterThreadLedgerCtorFrame&)=delete;
    void current()const {
        if(host.native_master_thread_ledger_ctor_frame!=this||source!=&host.sources||host.state||host.calling||creator!=std::this_thread::get_id()||
           creator!=host.ui_thread||epoch!=host.failure_epoch||writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_||
           !host.native_master_reference_bank||host.native_master_reference_bank->phase!=NativeMasterReferenceBankOwner::Phase::subscribed)
            throw ScriptError("Native Lua ledger constructor actual Source/bank/frame changed");
    }
};
class NativeMasterThreadLedgerOwner final {
    friend struct Impl;
    struct Node final {std::uint32_t next,previous_and_live;lua_State* actual_vm;};
    Impl& host;const ScriptSourceProvider*const source;const std::thread::id creator;
    const std::uint64_t epoch,writes,uncertainty;
    const NativeMasterThreadLedgerCtorFrame* constructor;
    enum class Phase {created,initialized,vm_bound,retired};Phase phase=Phase::created;
    std::uint32_t free_head=0,free_count=0;std::unique_ptr<Node[]> nodes;
    std::uint32_t storage_count=0,storage_capacity=0,live_count=0,head=0,tail=0;
    lua_State* master=nullptr;global_State* actual_global=nullptr;const Table* canonical_lua_registry=nullptr;
    NativeMasterReferenceBankOwner* bank=nullptr;
    NativeMasterThreadLedgerOwner(Impl&p,const NativeMasterThreadLedgerCtorFrame&f):host(p),source(&p.sources),creator(f.creator),
        epoch(f.epoch),writes(f.writes),uncertainty(f.uncertainty),constructor(&f){}
    NativeMasterThreadLedgerOwner(const NativeMasterThreadLedgerOwner&)=delete;
    NativeMasterThreadLedgerOwner& operator=(const NativeMasterThreadLedgerOwner&)=delete;
    void initialize_original_fields(){
        if(host.native_master_thread_ledger.get()!=this||!constructor||host.native_master_thread_ledger_ctor_frame!=constructor||phase!=Phase::created)
            throw ScriptError("Native Lua actual ledger constructor owner differs");
        constructor->current();
        // Exact original477B3D..477B6D order. No default policy creates an
        // empty ledger; these actual constructor writes establish its state.
        free_head=0x0fffffff;free_count=0;storage_count=0;storage_capacity=0;nodes.reset();live_count=0;
        head=0x0fffffff;tail=0x0fffffff;phase=Phase::initialized;constructor->current();
    }
    void physical_current_without_callbacks()const {
        if(host.native_master_thread_ledger.get()!=this||constructor||host.native_master_thread_ledger_ctor_frame||phase!=Phase::vm_bound||
           source!=&host.sources||creator!=std::this_thread::get_id()||creator!=host.ui_thread||epoch!=host.failure_epoch||
           writes!=host.failed.writes_||uncertainty!=host.execution_uncertainty||host.failed.exhausted_||!master||host.state!=master||
           !actual_global||G(master)!=actual_global||!ttistable(&actual_global->l_registry)||hvalue(&actual_global->l_registry)!=canonical_lua_registry||
           !bank||host.native_master_reference_bank.get()!=bank||bank->master!=master||bank->actual_global!=actual_global||
           bank->canonical_lua_registry!=canonical_lua_registry||storage_count>storage_capacity||live_count>storage_count||
           ((storage_capacity!=0)!=static_cast<bool>(nodes)))
            throw ScriptError("Native Lua actual active ledger/master/registry/owner retired");
        bank->physical_current_without_callbacks();bank->selected_registry->require_five_owned(native_master_debug_bank_entries,*bank);
    }
    void clear_matching_state_callback_free(lua_State* actual_borrowed_thread){
        physical_current_without_callbacks();
        // Private physical primitive only. The actual7458E0 caller must first
        // earn its closed scoped-thread loan and bank reset. This primitive
        // cannot create/accept a caller readiness flag or issue a scope return.
        std::uint32_t current=head,visited=0;
        while(current!=0x0fffffff){
            if(!nodes||current>=storage_count||visited++>=live_count||(nodes[current].previous_and_live&0x80000000u)==0)
                throw ScriptError("Native Lua actual active ledger chain is incomplete");
            if(nodes[current].actual_vm==actual_borrowed_thread){nodes[current].actual_vm=nullptr;return;}
            current=nodes[current].next;
        }
        // Original7458E0 leaves the list/count unchanged when no node matches.
        // A real constructor-empty list is data, never a completed destructor.
    }
    void retire_prefix()noexcept {
        if(phase==Phase::retired)return;
        // No4A31F0 publisher is issued by this constructor prefix. A future
        // Context registration must port actual list allocation/publication
        // and master cleanup before it may populate these owned nodes.
        if(live_count||storage_count||storage_capacity||nodes)host.record_failure();
        constructor=nullptr;master=nullptr;actual_global=nullptr;canonical_lua_registry=nullptr;bank=nullptr;
        nodes.reset();phase=Phase::retired;
    }
public:~NativeMasterThreadLedgerOwner(){retire_prefix();}
};
void initialize_native_master_thread_ledger_before_vm_owned(){
    {
        NativeMasterThreadLedgerCtorFrame frame(*this);frame.current();
        native_master_thread_ledger.reset(new NativeMasterThreadLedgerOwner(*this,frame));frame.current();
        native_master_thread_ledger->initialize_original_fields();frame.current();native_master_thread_ledger->constructor=nullptr;
    }
    if(!native_master_thread_ledger||native_master_thread_ledger_ctor_frame||native_master_thread_ledger->constructor||
       native_master_thread_ledger->phase!=NativeMasterThreadLedgerOwner::Phase::initialized)
        throw ScriptError("Native Lua actual ledger constructor did not return");
}
void bind_native_master_thread_ledger_actual_vm_owned(){
    auto*const ledger=native_master_thread_ledger.get();auto*const bank=native_master_reference_bank.get();
    if(!state||calling||!ledger||ledger->constructor||native_master_thread_ledger_ctor_frame||
       ledger->phase!=NativeMasterThreadLedgerOwner::Phase::initialized||!bank)
        throw ScriptError("Native Lua ledger binding requires actual newly created VM/returned constructor");
    bank->physical_current_without_callbacks();bank->selected_registry->require_five_owned(native_master_debug_bank_entries,*bank);
    ledger->bank=bank;ledger->master=state;ledger->actual_global=G(state);ledger->canonical_lua_registry=hvalue(&G(state)->l_registry);
    ledger->phase=NativeMasterThreadLedgerOwner::Phase::vm_bound;ledger->physical_current_without_callbacks();
}
static NativeMasterThreadLedgerOwner* native_master_thread_ledger_current_owned(lua_State*L){
    auto& p=owner(L);(void)native_master_reference_bank_current_owned(L);
    auto*const ledger=p.native_master_thread_ledger.get();p.source_error=nullptr;p.source_exception={};
    try{
        if(!ledger||!p.state||G(L)!=G(p.state))throw ScriptError("Native Lua actual ledger offered VM differs");
        ledger->physical_current_without_callbacks();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Native Lua actual active ledger physical FIRST refused";}
    if(p.source_error)return (native_master_thread_refuse(L,p.source_error),nullptr);
    return ledger;
}
void native_master_thread_ledger_retire()noexcept {
    native_master_thread_ledger_ctor_frame=nullptr;
    if(native_master_thread_ledger)native_master_thread_ledger->retire_prefix();
}
