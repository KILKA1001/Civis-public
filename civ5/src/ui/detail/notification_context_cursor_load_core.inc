// Canonical supported CLIENT manager cursor continuation; inside civ5::ui.
detail::NotificationContextGuiPort::CursorChange detail::NotificationContextGuiPort::set_cursor_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::int32_t requested){
    const auto state=impl_;
    try{
        state->first(operation);require_current_owned(operation);state->first(operation);
        require(state->cursor_constructor_returned&&state->no_custom_cursor_graph&&requested>=0&&requested<64,
            "Context cursor requires actual supported constructor/table/no-custom-graph owner");
        const auto previous=state->cursor_6dc;const auto before=state->revision;
        if(previous==requested){state->first(operation);return {previous,previous,before,before};}
        require(state->revision<=UINT64_MAX-4,"Context cursor partial history exhausted");
        state->history.reserve(state->history.size()+4);
        const auto serial=operation.operation_serial();const auto selected=state->cursor_a80[static_cast<std::size_t>(requested)];
        const auto hwnd=reinterpret_cast<HWND>(state->hwnd);
        state->busy=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{state->busy};
        const auto record=[&](Impl::Store kind){
            // Capacity and serial were sealed before the physical call. No
            // allocating/callback supplier stands between return and journal.
            state->history.push_back({state->revision+1,serial,kind});++state->revision;
        };
        state->first(operation,true);SetLastError(0);
        const auto prior_class=SetClassLongPtrW(hwnd,GCLP_HCURSOR,reinterpret_cast<LONG_PTR>(selected));
        const auto class_error=GetLastError();record(Impl::Store::cursor_class_return);
        state->first(operation,true);operation.require_current();state->first(operation,true);
        require(prior_class!=0||class_error==0,"Context cursor actual SetClassLongPtrW failed");
        // Exact native9C9A08 is DefWindowProcW, not a synthetic message ACK.
        (void)DefWindowProcW(hwnd,WM_SETCURSOR,reinterpret_cast<WPARAM>(hwnd),0x10200);
        record(Impl::Store::cursor_defwindow_return);
        state->first(operation,true);operation.require_current();state->first(operation,true);
        state->cursor_6dc=requested;record(Impl::Store::cursor_current_store);
        (void)SetCursor(selected);record(Impl::Store::cursor_set_return);
        state->first(operation,true);operation.require_current();state->first(operation,true);
        // Actual port ctor owns the absence of a custom cursor graph. This
        // corresponds only to657C30's real NULL9680 early return; Original's
        // unrelated pointer and other configured profiles remain unsupported.
        require(state->no_custom_cursor_graph,"Context cursor graphical suffix owner changed");
        return {previous,requested,before,state->revision};
    }catch(...){state->failed.store(true);throw;}
}

struct detail::NotificationContextCursorLoadPrefixReturn::Impl {
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::shared_ptr<NotificationContextGuiPort> manager;
    const std::uint64_t topology,parser_serial,source_serial,before_effects,after_effects,before_sequence,after_sequence;
    const std::uint64_t before_manager_revision,after_manager_revision;
    const std::uint32_t flags;
    const bool touch;
    const std::int32_t previous_cursor,current_cursor;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,
        const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint64_t parsed,
        std::uint64_t before_seq,std::uint64_t before_fx,bool actual_touch,
        const NotificationContextGuiPort::CursorChange& cursor)
        :core(actual),manager(actual->context_gui_port),topology(actual->topology_revision),parser_serial(parsed),
         source_serial(operation.operation_serial()),before_effects(before_fx),after_effects(actual->effect_revision),
         before_sequence(before_seq),after_sequence(actual->context_layout_baseline->sequence_),
         before_manager_revision(cursor.before_revision),after_manager_revision(cursor.after_revision),
         flags(actual->context_layout_baseline->base_.flags_40),touch(actual_touch),
         previous_cursor(cursor.previous),current_cursor(cursor.current){}
};
void BnwWorldViewControlTree::prepare_context_cursor_load_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto actual=core_;actual->layout_origin();
    try{
        require(actual->context_construction_return&&actual->context_gui_port&&actual->context_layout_baseline&&
            actual->context_layout_baseline->parsed_source_serial_&&!actual->context_cursor_load_prefix&&
            !(actual->context_layout_baseline->base_.flags_40&0x10000000U),
            "Context cursor prefix needs authentic nondeferred ctor/parser and cannot replay");
        const auto parser=*actual->context_layout_baseline->parsed_source_serial_;
        const auto before_fx=actual->effect_revision;const auto before_seq=actual->context_layout_baseline->sequence_;
        const auto manager=actual->context_gui_port;
        operation.require_current();actual->layout_origin();
        const bool touch=operation.context_touch_bit8_owned();actual->layout_origin();
        require(actual->context_gui_port.get()==manager.get()&&!actual->context_gui_port.owner_before(manager)&&
            !manager.owner_before(actual->context_gui_port)&&actual->effect_revision==before_fx&&
            actual->context_layout_baseline->sequence_==before_seq&&
            actual->context_layout_baseline->parsed_source_serial_==parser,
            "Context physical policy callback changed actual parser/receiver");
        if(touch){
            require(actual->effect_revision<UINT64_MAX&&actual->layout_sequence<UINT64_MAX,
                "Context physical conditional flag history exhausted");
            actual->context_layout_baseline->record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
            actual->context_layout_baseline->base_.flags_40|=0x400000U;
            ++actual->effect_revision;++actual->layout_sequence;
        }
        const auto after_fx=actual->effect_revision;const auto after_seq=actual->context_layout_baseline->sequence_;
        const auto flags=actual->context_layout_baseline->base_.flags_40;
        const auto cursor=manager->set_cursor_owned(operation,1);actual->layout_origin();
        require(actual->context_gui_port.get()==manager.get()&&!actual->context_gui_port.owner_before(manager)&&
            !manager.owner_before(actual->context_gui_port)&&actual->effect_revision==after_fx&&
            actual->context_layout_baseline->sequence_==after_seq&&actual->context_layout_baseline->base_.flags_40==flags&&
            actual->context_layout_baseline->parsed_source_serial_==parser&&!actual->context_cursor_load_prefix,
            "Context cursor callback changed canonical partial prefix");
        const auto body=std::make_shared<const detail::NotificationContextCursorLoadPrefixReturn::Impl>(
            actual,operation,parser,before_seq,before_fx,touch,cursor);
        actual->layout_origin();actual->context_cursor_load_prefix=std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>(
            new detail::NotificationContextCursorLoadPrefixReturn(body));actual->layout_origin();
    }catch(...){actual->failed.store(true);throw;}
}
std::shared_ptr<const detail::NotificationContextCursorLoadPrefixReturn>
BnwWorldViewControlTree::require_context_cursor_load_prefix_owned(const std::shared_ptr<Core>& actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    const auto receipt=actual->context_cursor_load_prefix;require(receipt&&receipt->impl_,"Context actual cursor prefix absent");
    const auto& saved=*receipt->impl_;const auto receiver=saved.core.lock();const auto manager=actual->context_gui_port;
    require(receiver&&receiver.get()==actual.get()&&!receiver.owner_before(actual)&&!actual.owner_before(receiver)&&
        manager&&saved.manager.get()==manager.get()&&!saved.manager.owner_before(manager)&&!manager.owner_before(saved.manager)&&
        saved.source_serial==operation.operation_serial()&&&actual->source->current_operation_loan()==&operation&&
        saved.topology==actual->topology_revision&&actual->context_layout_baseline&&
        actual->context_layout_baseline->parsed_source_serial_==saved.parser_serial&&
        actual->context_layout_baseline->sequence_==saved.after_sequence&&actual->effect_revision==saved.after_effects&&
        actual->context_layout_baseline->base_.flags_40==saved.flags&&
        saved.after_effects==saved.before_effects+(saved.touch?1U:0U)&&
        saved.after_sequence==saved.before_sequence+(saved.touch?1U:0U),
        "Context typed physical/cursor prefix differs from SAME receiver after-state");
    manager->impl_->first(operation);
    require(manager->impl_->cursor_constructor_returned&&manager->impl_->no_custom_cursor_graph&&
        manager->impl_->cursor_6dc==saved.current_cursor&&saved.current_cursor==1&&
        manager->impl_->revision==saved.after_manager_revision&&
        saved.after_manager_revision==saved.before_manager_revision+(saved.previous_cursor==1?0U:4U),
        "Context actual canonical cursor journal differs");
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();return receipt;
}
