// Actual canonical GuiPort only: closed selected configuration refresh.
void detail::NotificationContextGuiPort::refresh_language_980a_owned(const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto saved=impl_;saved->first(operation);
    try{
        require(!saved->language_980a&&!saved->refreshing_language_980a,"Gui980A initial refresh replay/reentry");
        saved->refreshing_language_980a=true;struct End{bool& v;~End(){v=false;}}end{saved->refreshing_language_980a};
        //4630AC clears before Results/SQL. Uncompleted clear is never readable.
        const auto clear_revision=saved->revision,clear_history_size=saved->history.size();
        require(clear_revision<UINT64_MAX&&clear_history_size<saved->history.max_size(),"Gui980A clear history exhausted");
        saved->history.reserve(clear_history_size+1); //May allocate/call new_handler.
        saved->first(operation);operation.require_current();saved->first(operation);
        require(!saved->language_980a&&saved->refreshing_language_980a&&saved->revision==clear_revision&&
            saved->history.size()==clear_history_size&&saved->history.capacity()>saved->history.size(),
            "Gui980A clear owner/history changed across reserve POST");
        //Entry is trivial and capacity is proved: this record cannot allocate.
        saved->record(operation,Impl::Store::language_980a_clear);
        saved->language_980a.emplace(Impl::Language980ARefresh{0,{},saved->revision,0,operation.operation_serial(),false});
        saved->first(operation);operation.require_current();saved->first(operation);
        const auto actual=operation.selected_language_extended_font_owned();saved->first(operation);
        require(actual.second<=1&&!actual.first.empty(),"Gui980A actual selected GetBool result invalid");
        //46310E stores the actual post-Step boolean. Source query has already
        //completed Results destruction; failed producer retains only clear.
        //Prepare the owned string and reserve before any row semantic store.
        const auto row_revision=saved->revision,row_history_size=saved->history.size();
        const auto clear_sequence=saved->language_980a->clear_sequence;
        require(row_revision<UINT64_MAX&&row_history_size<saved->history.max_size(),"Gui980A row history exhausted");
        std::string prepared_language=actual.first; //May allocate/call new_handler.
        saved->first(operation);operation.require_current();saved->first(operation);
        require(saved->language_980a&&!saved->language_980a->completed&&saved->refreshing_language_980a&&
            saved->language_980a->clear_sequence==clear_sequence&&saved->language_980a->row_sequence==0&&
            saved->revision==row_revision&&saved->history.size()==row_history_size,
            "Gui980A row owner/history changed across string allocation POST");
        saved->history.reserve(row_history_size+1); //May allocate/call new_handler.
        saved->first(operation);operation.require_current();saved->first(operation);
        require(saved->language_980a&&!saved->language_980a->completed&&saved->refreshing_language_980a&&
            saved->language_980a->clear_sequence==clear_sequence&&saved->language_980a->row_sequence==0&&
            saved->revision==row_revision&&saved->history.size()==row_history_size&&
            saved->history.capacity()>saved->history.size(),"Gui980A row owner/history changed across allocation POST");
        const auto before_store=operation.selected_language_extended_font_owned();saved->first(operation);
        require(before_store==actual&&saved->revision==row_revision&&saved->history.size()==row_history_size&&
            saved->history.capacity()>saved->history.size()&&saved->language_980a&&!saved->language_980a->completed&&
            saved->language_980a->clear_sequence==clear_sequence&&saved->language_980a->row_sequence==0,
            "Gui980A selected row/owner changed before nonalloc publication");
        saved->record(operation,Impl::Store::language_980a_row); //capacity proved; no allocation.
        auto& field=*saved->language_980a;
        static_assert(noexcept(field.language.swap(prepared_language)));
        field.language.swap(prepared_language); //Nonalloc owned-string transfer.
        field.value=actual.second;field.row_sequence=saved->revision;
        saved->first(operation);operation.require_current();saved->first(operation);
        const auto after=operation.selected_language_extended_font_owned();saved->first(operation);
        require(after==actual&&field.source_serial==operation.operation_serial()&&field.clear_sequence<field.row_sequence,
            "Gui980A selected language/row changed across refresh POST");
        field.completed=true;saved->first(operation);
    }catch(...){saved->failed.store(true);throw;}
}
std::uint8_t detail::NotificationContextGuiPort::label_language_980a_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=impl_;saved->first(operation);
    try{
        require(!saved->refreshing_language_980a&&saved->language_980a&&saved->language_980a->completed,
            "Gui980A getter has no completed mandatory refresh");
        const auto field=*saved->language_980a; //Owned DATA copy may allocate.
        saved->first(operation);operation.require_current();saved->first(operation);
        require(field.source_serial&&field.clear_sequence&&field.row_sequence>field.clear_sequence&&field.value<=1,
            "Gui980A completed selected refresh history malformed");
        saved->first(operation);operation.require_current();saved->first(operation);
        const auto actual=operation.selected_language_extended_font_owned();saved->first(operation);
        require(actual.first==field.language&&actual.second==field.value,"Gui980A effective current language/DB row changed");
        operation.require_current();saved->first(operation);
        require(saved->language_980a&&saved->language_980a->completed&&saved->language_980a->source_serial==field.source_serial&&
            saved->language_980a->clear_sequence==field.clear_sequence&&saved->language_980a->row_sequence==field.row_sequence&&
            saved->language_980a->language==field.language&&saved->language_980a->value==field.value,
            "Gui980A canonical refresh replaced across getter POST");return field.value;
    }catch(...){saved->failed.store(true);throw;}
}
