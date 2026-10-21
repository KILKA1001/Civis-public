// Include INSIDE canonical Tree::Core after actual revision/storage members.
// Issued only by genuine original initialLua invocation; this is no public DTO.
enum class InitialLuaWriter { ordinary_command, layout_command, scroll_command, receiver_store };
struct InitialLuaMutationJournal final {
    struct Store {
        InitialLuaWriter writer;
        std::uint64_t before_effects,after_effects,before_sequence,after_sequence;
        std::uint64_t node,receiver_kind;
    };
    const std::thread::id creator=std::this_thread::get_id();
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,topology,before_effects,before_sequence;
    std::uint64_t effects,sequence;
    std::vector<Store> stores;
    bool sealed=false;
    InitialLuaMutationJournal(const runtime::BnwWorldViewSourceOperationLoan& actual,
        std::uint64_t t,std::uint64_t e,std::uint64_t s)
        :operation(&actual),serial(actual.operation_serial()),topology(t),before_effects(e),
         before_sequence(s),effects(e),sequence(s){}
};
std::shared_ptr<InitialLuaMutationJournal> initial_lua_mutation_journal;
void initial_lua_preflight_store_owned()const{
    const auto& journal=initial_lua_mutation_journal;if(!journal)return;
    require(journal->creator==creator&&creator==std::this_thread::get_id()&&facade_alive.load()&&!failed.load()&&
        !journal->sealed&&journal->operation==&source->current_operation_loan()&&
        journal->serial==journal->operation->operation_serial()&&journal->topology==topology_revision&&
        journal->effects==effect_revision&&journal->sequence==layout_sequence,
        "InitialLua writer differs from genuine current Core/Source/pre-store history");
    journal->operation->require_origin_without_callbacks();
    require(journal->stores.size()<journal->stores.max_size(),"InitialLua mutation history exhausted");
    // Reserve BEFORE the physical store; commit below never allocates.
    journal->stores.reserve(journal->stores.size()+1);
}
void initial_lua_record_store_owned(InitialLuaWriter writer,std::uint64_t node=0,std::uint64_t kind=0){
    const auto& journal=initial_lua_mutation_journal;if(!journal)return;
    require(!journal->sealed&&journal->creator==std::this_thread::get_id()&&facade_alive.load()&&!failed.load()&&
        journal->operation==&source->current_operation_loan()&&journal->serial==journal->operation->operation_serial()&&
        journal->topology==topology_revision&&journal->effects<UINT64_MAX&&effect_revision==journal->effects+1&&
        journal->stores.capacity()>journal->stores.size(),"InitialLua actual writer commit is untracked/reentered");
    const auto sequence_step=writer==InitialLuaWriter::receiver_store?1u:0u;
    require(journal->sequence<=UINT64_MAX-sequence_step&&layout_sequence==journal->sequence+sequence_step,
        "InitialLua writer actual layout-store sequence differs");
    journal->stores.push_back({writer,journal->effects,effect_revision,journal->sequence,layout_sequence,node,kind});
    journal->effects=effect_revision;journal->sequence=layout_sequence;
}
void initial_lua_require_existing_topology_owned()const{
    // Capturing a cloned template is not the native dynamic ctor/+10 issuer.
    // Keep that next concrete producer boundary explicit BEFORE any store.
    require(!initial_lua_mutation_journal,
        "InitialLua topology change requires genuine dynamic constructor/parser/+10 execution and typed topology admission");
}
