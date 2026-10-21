// Inside civ5::ui AFTER complete Indexed ContextChildSlot10Frame and Root origins.
struct detail::NotificationContextInitialLuaGcReturn::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
    const std::shared_ptr<const NotificationContextCursorLoadPrefixReturn> cursor;
    const std::uint64_t serial,before_topology,after_topology,before_effects,after_effects;
    const std::uint64_t before_sequence,after_sequence;
    struct Domain {
        const std::weak_ptr<BnwWorldViewControlTree::Core> core;
        const std::shared_ptr<const BnwWorldViewControlTree::Core::InitialLuaMutationJournal> journal;
    };
    const std::vector<Domain> domains;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,
        const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t topology,std::uint64_t effects,std::uint64_t sequence,
        std::vector<Domain> history)
        :core(actual),preparation(actual->context_lua_preparation),cursor(actual->context_cursor_load_prefix),
         serial(op.operation_serial()),before_topology(topology),after_topology(actual->topology_revision),
         before_effects(effects),after_effects(actual->effect_revision),before_sequence(sequence),
         after_sequence(actual->context_layout_baseline->sequence_),domains(std::move(history)){}
};

// Only the closed actual original-compiler/coroutine/manager/GC return can
// advance this active Frame. The stores were recorded by real Core writers,
// with their preimages captured BEFORE invoking original initialLua.
void BnwWorldViewControlTree::ContextChildSlot10Frame::adopt_context_initial_lua_mutations_owned(
    const std::shared_ptr<Core>& actual,
    const std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>& returned){
    first();require(returned&&returned->impl_,"InitialLua adoption lacks genuine body/GC return");
    const auto& done=*returned->impl_;const auto owner=done.core.lock();
    require(owner&&owner.get()==actual.get()&&!owner.owner_before(actual)&&!actual.owner_before(owner)&&
        done.creator==creator&&done.serial==serial&&actual->context_initial_lua_gc_return.get()==returned.get()&&
        done.domains.size()==domains.size(),"InitialLua adoption current return/domain identity differs");
    std::vector<std::uint64_t> earned;earned.reserve(domains.size());
    for(std::size_t i=0;i<domains.size();++i){
        const auto core=done.domains[i].core.lock();const auto& tracked=domains[i];
        const auto& journal=done.domains[i].journal;first();
        require(core&&core.get()==tracked.core.get()&&!core.owner_before(tracked.core)&&!tracked.core.owner_before(core)&&
            journal&&journal.get()==core->initial_lua_mutation_journal.get()&&journal->sealed&&
            journal->creator==creator&&journal->operation==operation&&journal->serial==serial&&
            journal->topology==tracked.topology&&journal->topology==core->topology_revision&&
            journal->before_effects==tracked.effects&&journal->effects==core->effect_revision&&
            journal->sequence==core->layout_sequence&&!core->failed.load()&&!core->busy&&core->facade_alive.load(),
            "InitialLua adoption not SAME canonical preimage/closed writer history");
        auto effects=journal->before_effects,sequence=journal->before_sequence;
        for(const auto& store:journal->stores){
            require(store.before_effects==effects&&effects<UINT64_MAX&&store.after_effects==effects+1&&
                store.before_sequence==sequence,"InitialLua actual writer history has gap/replay");
            const auto step=store.writer==Core::InitialLuaWriter::receiver_store?1u:0u;
            require(sequence<=UINT64_MAX-step&&store.after_sequence==sequence+step,
                "InitialLua actual writer sequence cannot be adopted");
            effects=store.after_effects;sequence=store.after_sequence;
        }
        require(effects==journal->effects&&sequence==journal->sequence,
            "InitialLua after-state contains unrecorded physical stores");earned.push_back(effects);
    }
    // All domains validate before any Frame row is advanced. No callback or
    // user supplied epoch intervenes. Original stores remain if validation fails.
    for(std::size_t i=0;i<domains.size();++i){domains[i].effects=earned[i];domains[i].core->initial_lua_mutation_journal.reset();}
    pure();full();
}
detail::NotificationContextInitialLuaGcReturn::NotificationContextInitialLuaGcReturn(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
detail::NotificationContextInitialLuaGcReturn::~NotificationContextInitialLuaGcReturn()=default;
void BnwWorldViewControlTree::require_context_initial_lua_gc_return_owned(const std::shared_ptr<Core>& actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>& returned){
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    require(returned&&returned->impl_&&actual->context_initial_lua_gc_return.get()==returned.get()&&
        !actual->context_initial_lua_gc_return.owner_before(returned)&&!returned.owner_before(actual->context_initial_lua_gc_return),
        "Context genuine originalLua/manager/GC return absent or replaced");
    const auto& done=*returned->impl_;const auto core=done.core.lock();
    require(done.creator==std::this_thread::get_id()&&core&&core.get()==actual.get()&&
        !core.owner_before(actual)&&!actual.owner_before(core)&&done.serial==operation.operation_serial()&&
        &actual->source->current_operation_loan()==&operation&&done.after_topology==actual->topology_revision&&
        done.after_effects==actual->effect_revision&&actual->context_layout_baseline&&
        done.after_sequence==actual->context_layout_baseline->sequence_&&
        done.preparation.get()==actual->context_lua_preparation.get()&&done.cursor.get()==actual->context_cursor_load_prefix.get(),
        "Context originalLua return differs from SAME physical receiver/source/state");
    require(!done.domains.empty(),"Context GC return actual writer domains absent");
    for(const auto& entry:done.domains){
        const auto live=entry.core.lock();actual->layout_origin();
        require(live&&entry.journal&&entry.journal->sealed&&entry.journal->creator==std::this_thread::get_id()&&
            entry.journal->operation==&operation&&entry.journal->serial==done.serial&&
            live->source.get()==actual->source.get()&&!live->source.owner_before(actual->source)&&
            !actual->source.owner_before(live->source)&&live->topology_revision==entry.journal->topology&&
            live->effect_revision==entry.journal->effects&&live->layout_sequence==entry.journal->sequence,
            "Context GC return genuine writer domain changed before use");live->layout_origin();actual->layout_origin();
    }
    const auto& host=operation.context_lua_host_owned();actual->layout_origin();
    host.require_context_initial_lua_state_owned(operation,done.preparation,actual,done.cursor);actual->layout_origin();
    operation.require_origin_without_callbacks();actual->layout_origin();
}
void BnwWorldViewControlTree::execute_context_slot10_initial_lua_owned(ContextChildSlot10Frame& frame,
    const std::shared_ptr<const detail::NotificationContextSlot10LoadOrigin>& origin,
    const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& initialized){
    frame.full();require(origin&&origin->impl_,"Context original initialLua real load origin absent");
    const auto actual=origin->impl_->core.lock();frame.first();require(bool(actual),"Context original initialLua Core retired");
    const auto domain=frame.domain_of(actual);
    // The immutable prefix is historical now: genuine5761 has already made
    // further owned stores. Never demand/renew its former after-effects here.
    const auto& load=*origin->impl_;
    require(actual->context_load_origin.get()==origin.get()&&!actual->context_load_origin.owner_before(origin)&&
        !origin.owner_before(actual->context_load_origin)&&!(load.flags&0x10000000U)&&
        load.source_serial==frame.serial&&load.topology==frame.domains.at(domain).topology&&
        load.manager.get()==actual->context_gui_port.get()&&!load.manager.owner_before(actual->context_gui_port)&&
        !actual->context_gui_port.owner_before(load.manager),
        "Context original initialLua historical nondeferred load/Core/manager/source differs");frame.first();frame.pure();
    const auto checked=require_context_initialization_return_owned(actual,initialized);frame.first();frame.pure();
    require(checked.get()==actual.get()&&!checked.owner_before(actual)&&!actual.owner_before(checked)&&
        !actual->context_initial_lua_started&&!actual->context_initial_lua_gc_return,
        "Context original initialLua genuine5761/new occurrence absent");
    require_context_lua_preparation_owned(actual,*frame.operation,actual->context_lua_preparation);frame.first();frame.full();
    // Original515120 named map588 inserts only when the existing value isNULL.
    // Parent3C is a real previously captured parent-kind/identity. No uncaptured
    // weak_ptr expiry is converted to the native NULL/THIS branch.
    require(actual->context_parent_kind.has_value(),"Context initialLua Parent3C origin unissued");
    Core::ContextNamedReceiver receiver;
    if(*actual->context_parent_kind==Core::ContextParentKind::native_null){
        require(actual->context_parent_core.expired()&&!actual->context_parent_control_id,
            "Context initialLua captured nativeNULL differs");receiver={actual,0};
    }else{
        const auto parent=actual->context_parent_core.lock();require(bool(parent),"Context initialLua real Parent3C retired");
        parent->layout_origin();frame.first();
        require(parent->source.get()==actual->source.get()&&!parent->source.owner_before(actual->source)&&
            !actual->source.owner_before(parent->source),"Context initialLua Parent3C belongs to another Source");
        if(*actual->context_parent_kind==Core::ContextParentKind::canonical_context)
            require(!actual->context_parent_control_id,"Context initialLua canonical Context parent is another control");
        else{require(*actual->context_parent_kind==Core::ContextParentKind::canonical_control&&actual->context_parent_control_id,
                "Context initialLua Parent3C kind unsupported");
            require(Core::node(parent->nodes,actual->context_parent_control_id).alive,"Context initialLua native control parent dead");}
        receiver={parent,actual->context_parent_control_id};
    }
    frame.full();actual->context_initial_lua_started=true;
    const auto prior=actual->context_named_receivers.find("..");
    if(prior==actual->context_named_receivers.end())actual->context_named_receivers.emplace("..",receiver);
    else require(!prior->second.core.expired(),"Context initialLua native nonNULL alias unexpectedly retired");
    const auto before_topology=actual->topology_revision,before_effects=actual->effect_revision;
    const auto before_sequence=actual->context_layout_baseline->sequence_;
    const auto& host=frame.operation->context_lua_host_owned();frame.first();frame.full();
    // Genuine retained mutable Tree is the SAME object from constructor/parser
    // resource capture. The earlier dimension receipt is historical after5761;
    // do not reissue it by calling the old prefix-afterstate getter here.
    const auto retained=actual->layout_resource_tree.lock();
    require(retained&&retained.get()==actual->layout_resource_actual_tree&&
        retained->core_.get()==actual.get()&&!retained->core_.owner_before(actual)&&!actual.owner_before(retained->core_)&&
        actual->layout_resource_operation==frame.operation&&actual->layout_resource_serial==frame.serial,
        "Context initialLua actual retained constructor/resource Tree absent or substituted");
    auto tree=std::const_pointer_cast<BnwWorldViewControlTree>(retained);
    frame.full();host.register_context_initial_lua_graph_owned(*frame.operation,actual->context_lua_preparation,tree);
    frame.first();frame.full();
    // Genuine active Frame preimages precede EVERY original Lua store. Each
    // existing canonical domain owns its own journal; foreign/new domains are
    // not admitted by a numeric observation after a callback.
    std::vector<detail::NotificationContextInitialLuaGcReturn::Impl::Domain> history;
    history.reserve(frame.domains.size());frame.full();
    for(const auto& entry:frame.domains){
        entry.core->layout_origin();frame.first();
        require(!entry.core->initial_lua_mutation_journal,"InitialLua nested journal owner is unsupported");
        auto journal=std::make_shared<Core::InitialLuaMutationJournal>(*frame.operation,
            entry.topology,entry.effects,entry.core->layout_sequence);
        entry.core->initial_lua_mutation_journal=journal;
        history.push_back({entry.core,std::move(journal)});
    }
    host.invoke_context_initial_lua_owned(*frame.operation,actual->context_lua_preparation,actual,actual->context_cursor_load_prefix);
    frame.first(); // FIRST before borrowing any post-Lua receiver fields
    host.require_context_initial_lua_state_owned(*frame.operation,actual->context_lua_preparation,actual,actual->context_cursor_load_prefix);
    frame.first();
    // This closed return is produced by the actual original-source compiler,
    // identical retained coroutine/env, strict native backend and GC callback
    // path, not by a caller-supplied arbitrary callback/snapshot grant.
    for(const auto& entry:history){
        const auto core=entry.core.lock();frame.first();require(bool(core),"InitialLua writer owner retired on actual GC return");
        core->layout_origin();frame.first();const auto journal=core->initial_lua_mutation_journal;
        require(journal&&journal.get()==entry.journal.get()&&!journal->sealed&&journal->operation==frame.operation&&
            journal->serial==frame.serial&&journal->topology==core->topology_revision&&
            journal->effects==core->effect_revision&&journal->sequence==core->layout_sequence,
            "InitialLua original-source return contains an untracked writer/topology effect");journal->sealed=true;
    }
    const auto body=std::make_shared<const detail::NotificationContextInitialLuaGcReturn::Impl>(actual,*frame.operation,
        before_topology,before_effects,before_sequence,std::move(history));
    actual->context_initial_lua_gc_return=std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>(
        new detail::NotificationContextInitialLuaGcReturn(body));frame.first();
    require_context_initial_lua_gc_return_owned(actual,*frame.operation,actual->context_initial_lua_gc_return);frame.first();
    frame.adopt_context_initial_lua_mutations_owned(actual,actual->context_initial_lua_gc_return);frame.first();frame.full();
    (void)domain;
    finish_context_post_initial_lua_owned(frame,actual,actual->context_initial_lua_gc_return);
    frame.first();frame.full();
}
