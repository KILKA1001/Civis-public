// Inside civ5::ui AFTER complete InitialLuaGcReturn::Impl and child Frame.
struct detail::NotificationContextCursorRestoreReturn::Impl {
    struct DirectLoadTag final {};
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::shared_ptr<const NotificationContextCursorLoadPrefixReturn> prefix;
    const std::shared_ptr<const NotificationContextInitialLuaGcReturn> gc;
    const std::shared_ptr<NotificationContextGuiPort> manager;
    const std::uint64_t serial,topology,effects,sequence,parser_serial;
    const std::uint64_t before_manager_revision,after_manager_revision;
    const std::int32_t before_cursor,restored_cursor;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,
        const runtime::BnwWorldViewSourceOperationLoan& operation,
        const std::shared_ptr<const NotificationContextInitialLuaGcReturn>& returned,
        const NotificationContextGuiPort::CursorChange& change)
        :core(actual),prefix(actual->context_cursor_load_prefix),gc(returned),manager(actual->context_gui_port),
         serial(operation.operation_serial()),topology(actual->topology_revision),effects(actual->effect_revision),
         sequence(actual->context_layout_baseline->sequence_),parser_serial(*actual->context_layout_baseline->parsed_source_serial_),
         before_manager_revision(change.before_revision),after_manager_revision(change.after_revision),
         before_cursor(change.previous),restored_cursor(change.current){}
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,
        const runtime::BnwWorldViewSourceOperationLoan& operation,
        const std::shared_ptr<const NotificationContextInitialLuaGcReturn>& returned,
        const NotificationContextGuiPort::CursorChange& change,DirectLoadTag)
        :core(actual),prefix(actual->context_cursor_load_prefix),gc(returned),manager(actual->context_gui_port),
         serial(operation.operation_serial()),topology(actual->topology_revision),effects(actual->effect_revision),
         sequence(actual->context_layout_baseline->sequence_),parser_serial(0),
         before_manager_revision(change.before_revision),after_manager_revision(change.after_revision),
         before_cursor(change.previous),restored_cursor(change.current){
        require(actual->context_direct_load_frame.lock()&&!actual->context_layout_baseline->parsed_source_serial_,
            "Direct cursor restore requires authentic nonparser caller");
    }
};
detail::NotificationContextCursorRestoreReturn::NotificationContextCursorRestoreReturn(std::shared_ptr<const Impl> p)
    :impl_(std::move(p)){}
detail::NotificationContextCursorRestoreReturn::~NotificationContextCursorRestoreReturn()=default;

void BnwWorldViewControlTree::finish_context_post_initial_lua_owned(ContextChildSlot10Frame& frame,
    const std::shared_ptr<Core>& actual,
    const std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>& returned){
    frame.full();const auto domain=frame.domain_of(actual);frame.first();
    const auto prefix=actual->context_cursor_load_prefix;const auto manager=actual->context_gui_port;
    require(prefix&&prefix->impl_&&manager&&actual->context_construction_return&&
        actual->context_load_origin&&actual->context_load_origin->impl_&&actual->context_layout_baseline&&
        !actual->context_cursor_restore_return,"Context restore genuine current constructor/load/cursor owner absent or replayed");
    const auto topology=actual->topology_revision;auto effects=actual->effect_revision;
    auto sequence=actual->context_layout_baseline->sequence_;
    const auto parsed=actual->context_layout_baseline->parsed_source_serial_;
    const auto constructor=actual->context_construction_return;const auto load=actual->context_load_origin;
    std::optional<std::uint64_t> expected_manager_revision;
    std::optional<std::int32_t> expected_manager_cursor;
    const auto pure=[&]{
        frame.pure();actual->layout_origin();
        require(actual->context_construction_return.get()==constructor.get()&&
            !actual->context_construction_return.owner_before(constructor)&&!constructor.owner_before(actual->context_construction_return)&&
            actual->context_load_origin.get()==load.get()&&!actual->context_load_origin.owner_before(load)&&!load.owner_before(actual->context_load_origin)&&
            actual->context_cursor_load_prefix.get()==prefix.get()&&!actual->context_cursor_load_prefix.owner_before(prefix)&&!prefix.owner_before(actual->context_cursor_load_prefix)&&
            actual->context_gui_port.get()==manager.get()&&!actual->context_gui_port.owner_before(manager)&&!manager.owner_before(actual->context_gui_port)&&
            actual->context_layout_baseline&&actual->context_layout_baseline->parsed_source_serial_==parsed&&parsed&&
            actual->topology_revision==topology&&actual->effect_revision==effects&&
            actual->context_layout_baseline->sequence_==sequence&&frame.domains.at(domain).effects==effects,
            "Context restore SAME receiver/constructor/parser/current Frame changed");
        const auto owner=prefix->impl_->core.lock();const auto loaded=load->impl_->core.lock();
        require(owner&&loaded&&owner.get()==actual.get()&&!owner.owner_before(actual)&&!actual.owner_before(owner)&&
            loaded.get()==actual.get()&&!loaded.owner_before(actual)&&!actual.owner_before(loaded)&&
            prefix->impl_->manager.get()==manager.get()&&!prefix->impl_->manager.owner_before(manager)&&!manager.owner_before(prefix->impl_->manager)&&
            prefix->impl_->source_serial==frame.serial&&prefix->impl_->parser_serial==*parsed&&
            load->impl_->source_serial==frame.serial&&load->impl_->parser_serial==*parsed&&
            prefix->impl_->topology==topology&&load->impl_->topology==topology&&!(load->impl_->flags&0x10000000u),
            "Context restore historical cursor/load allocation lineage differs");
        for(const auto& entry:frame.domains)require(!entry.core->initial_lua_mutation_journal,
            "Context restore requires genuine completed all-domain writer journal adoption");
        manager->impl_->first(*frame.operation);frame.first();
        if(expected_manager_revision)require(manager->impl_->revision==*expected_manager_revision&&
            manager->impl_->cursor_6dc==*expected_manager_cursor&&manager->impl_->history.size()==*expected_manager_revision,
            "Context restore actual GUI after-history changed across callback/publication");
    };
    try{
        pure();require_context_initial_lua_gc_return_owned(actual,*frame.operation,returned);frame.first();pure();
        manager->require_current_owned(*frame.operation);frame.first();pure();
        const auto state=manager->impl_;const auto& saved=*prefix->impl_;
        require(state->cursor_constructor_returned&&state->no_custom_cursor_graph&&saved.previous_cursor>=0&&saved.previous_cursor<64&&
            saved.current_cursor==1&&saved.before_manager_revision<=saved.after_manager_revision&&
            saved.after_manager_revision<=state->revision&&state->history.size()==state->revision,
            "Context restore actual savedprevious cursor/history domain absent");
        for(std::size_t i=0;i<state->history.size();++i)
            require(state->history[i].sequence==i+1,"Context restore GUI history contains gap/replay");
        const auto prefix_steps=saved.previous_cursor==1?0u:4u;
        require(saved.after_manager_revision==saved.before_manager_revision+prefix_steps,
            "Context restore original cursor prefix did not really return");
        if(prefix_steps){
            constexpr std::array<detail::NotificationContextGuiPort::Impl::Store,4> order{
                detail::NotificationContextGuiPort::Impl::Store::cursor_class_return,
                detail::NotificationContextGuiPort::Impl::Store::cursor_defwindow_return,
                detail::NotificationContextGuiPort::Impl::Store::cursor_current_store,
                detail::NotificationContextGuiPort::Impl::Store::cursor_set_return};
            for(std::size_t i=0;i<order.size();++i){const auto& store=state->history.at(saved.before_manager_revision+i);
                require(store.kind==order[i]&&store.source_serial==frame.serial,"Context restore earned prefix GUI order differs");}
        }
        const auto before=state->revision;const auto current_cursor=state->cursor_6dc;
        expected_manager_revision=before;expected_manager_cursor=current_cursor;
        pure();
        // Original45774C captures the saved prior cursor operand, then45775A
        // invokes the SAME6E08B0 setter AFTER childGC. This is not RAII cleanup:
        // exceptions before this point never synthesize a restoration return.
        const auto changed=manager->set_cursor_owned(*frame.operation,saved.previous_cursor);frame.first();
        require(changed.before_revision==before&&changed.previous==current_cursor&&changed.current==saved.previous_cursor&&
            state->revision==changed.after_revision&&state->cursor_6dc==changed.current&&
            changed.after_revision==before+(current_cursor==saved.previous_cursor?0u:4u),
            "Context restore real GUI setter return/history differs");
        // Only this actual setter return advances the GUI bookend. A later
        // callback observation cannot adopt an unknown manager epoch.
        expected_manager_revision=changed.after_revision;expected_manager_cursor=changed.current;pure();
        require_context_initial_lua_gc_return_owned(actual,*frame.operation,returned);frame.first();pure();frame.full();pure();
        const auto body=std::make_shared<const detail::NotificationContextCursorRestoreReturn::Impl>(actual,*frame.operation,returned,changed);
        pure();require(!actual->context_cursor_restore_return,"Context restore publication replayed");
        actual->context_cursor_restore_return=std::shared_ptr<const detail::NotificationContextCursorRestoreReturn>(
            new detail::NotificationContextCursorRestoreReturn(body));pure();
        // Native45775F reads CURRENT real flags after restore. It does not OR
        // 8000000 and absence of a +524 supplier is never a fake NULL result.
        const auto flags=actual->context_layout_baseline->base_.flags_40;
        if(flags&0x8000000u){
            invoke_context_post_lua_showhide_owned(frame,actual,prefix,actual->context_cursor_restore_return);
            frame.first();effects=actual->effect_revision;sequence=actual->context_layout_baseline->sequence_;pure();
        }
        pure();frame.full();pure();
        // This natural flag-clear branch now consumes real selected-Civis
        // BEFORE-Lua temporaries. The Host cannot borrow permanent VM refs.
        require(!actual->context_loader_cleanup_return,"Context loader cleanup publication replay");
        const auto restored=actual->context_cursor_restore_return;pure();
        const auto& vm=frame.operation->context_lua_host_owned();frame.first();pure();
        const auto cleaned=vm.cleanup_context_loader_temporaries_owned(*frame.operation,
            actual->context_lua_preparation,actual,prefix,restored);frame.first();pure();
        require(cleaned&&actual->context_cursor_restore_return.get()==restored.get()&&
            !actual->context_cursor_restore_return.owner_before(restored)&&!restored.owner_before(actual->context_cursor_restore_return),
            "Context loader real cleanup returned for another cursor lifetime");
        frame.full();pure();require(!actual->context_loader_cleanup_return,"Context cleanup publication reentered");
        actual->context_loader_cleanup_return=cleaned;pure();
        // Natural +10 dispatcher earns its own full return after this function
        // returns. This is no alpha/readiness or Original allocator ABI claim.
    }catch(...){manager->impl_->failed.store(true);frame.poison();throw;}
}
