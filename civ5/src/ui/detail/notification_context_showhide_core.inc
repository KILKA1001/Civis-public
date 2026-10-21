// Inside civ5::ui, after complete canonical Core and ChildSlot10Frame.
void BnwWorldViewControlTree::register_context_showhide_flag_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto actual=core_;actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation&&actual->context_layout_baseline&&
        actual->context_layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::context&&
        actual->context_lua_preparation&&actual->context_initial_lua_started&&!actual->context_initial_lua_gc_return&&
        actual->initial_lua_mutation_journal&&actual->effect_revision<UINT64_MAX,
        "SetShowHideHandler flag store needs SAME actual initial Context/source/writer owner");
    auto& state=*actual->context_layout_baseline;
    actual->initial_lua_preflight_store_owned();state.record_owned(detail::NotificationLayoutBaseline::Mutation::context_showhide_handler);
    state.base_.flags_40|=0x8000000u;actual->layout_sequence=state.sequence_;++actual->effect_revision;
    actual->initial_lua_record_store_owned(Core::InitialLuaWriter::receiver_store,0,0);
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
}

void BnwWorldViewControlTree::invoke_context_post_lua_showhide_owned(ContextChildSlot10Frame& frame,
    const std::shared_ptr<Core>& actual,
    const std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>& prefix,
    const std::shared_ptr<const detail::NotificationContextCursorRestoreReturn>& restored){
    frame.first();frame.full();
    require(actual->context_cursor_restore_return.get()==restored.get()&&restored&&
        !actual->context_cursor_restore_return.owner_before(restored)&&!restored.owner_before(actual->context_cursor_restore_return)&&
        actual->context_cursor_load_prefix.get()==prefix.get()&&actual->context_layout_baseline&&
        (actual->context_layout_baseline->base_.flags_40&0x8000000u)&&!actual->context_loader_cleanup_return,
        "Initial showhide SAME actual restored Context/handler suffix absent");
    const auto hidden=(actual->context_layout_baseline->base_.flags_40&2u)!=0;
    const auto& host=frame.operation->context_lua_host_owned();frame.first();frame.full();
    // Capture actual writer preimages BEFORE callback; no observed after-state
    // is used to manufacture permission for a new domain or unknown store.
    std::vector<std::shared_ptr<Core::InitialLuaMutationJournal>> history;
    history.reserve(frame.domains.size());frame.full();
    for(const auto& d:frame.domains){
        d.core->layout_origin();frame.first();require(!d.core->initial_lua_mutation_journal,"Initial showhide journal reentry");
        auto journal=std::make_shared<Core::InitialLuaMutationJournal>(*frame.operation,d.topology,d.effects,d.core->layout_sequence);
        d.core->initial_lua_mutation_journal=journal;history.push_back(std::move(journal));
    }
    host.invoke_context_initial_showhide_owned(*frame.operation,actual->context_lua_preparation,actual,prefix,restored,hidden);
    frame.first();
    require(history.size()==frame.domains.size(),"Initial showhide domain insertion requires real topology issuer");
    std::vector<std::uint64_t> earned;earned.reserve(history.size());
    for(std::size_t i=0;i<history.size();++i){
        const auto& d=frame.domains[i];const auto& j=history[i];d.core->layout_origin();frame.first();
        require(j&&d.core->initial_lua_mutation_journal.get()==j.get()&&!j->sealed&&j->creator==std::this_thread::get_id()&&
            j->operation==frame.operation&&j->serial==frame.serial&&j->topology==d.topology&&d.core->topology_revision==d.topology&&
            j->before_effects==d.effects&&j->effects==d.core->effect_revision&&j->sequence==d.core->layout_sequence,
            "Initial showhide real writer domain/preimage changed");
        auto e=j->before_effects,s=j->before_sequence;
        for(const auto& store:j->stores){
            const auto step=store.writer==Core::InitialLuaWriter::receiver_store?1u:0u;
            require(store.before_effects==e&&e<UINT64_MAX&&store.after_effects==e+1&&store.before_sequence==s&&
                s<=UINT64_MAX-step&&store.after_sequence==s+step,"Initial showhide unknown/gapped physical writer effect");
            e=store.after_effects;s=store.after_sequence;
        }
        require(e==j->effects&&s==j->sequence,"Initial showhide journal omits a store");earned.push_back(e);
    }
    // All domains verified before any frame after-row advances. No callbacks
    // intervene in this exact adoption of recorded physical writer effects.
    for(std::size_t i=0;i<history.size();++i){history[i]->sealed=true;frame.domains[i].effects=earned[i];
        frame.domains[i].core->initial_lua_mutation_journal.reset();}
    frame.pure();frame.full();
}
