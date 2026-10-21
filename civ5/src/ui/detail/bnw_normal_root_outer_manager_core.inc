// Inside civ5::ui after complete Root/Common/Material/normal GUI queue bodies.
// The two storage arrays are distinct: outer command0x90 and inner entry0x20.
void BnwWorldViewControlTree::open_normal_root_outer_window_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchHistory::Guard& actual_root)const{
    using Gui=detail::NotificationContextGuiPort;
    const auto root=actual_root;root.require_physical_current_without_callbacks();
    const auto saved=root.impl_;require(saved&&saved->tree.get()==this,"Outer open another actual Root/Tree");
    const auto prefix=saved->prefix;const auto manager=saved->gui.lock();
    require(bool(manager),"Outer open actual manager retired");
    auto& queue=prefix->batch->payload_queue;
    const auto first=[&]{root.require_physical_current_without_callbacks();
        require(!prefix->common_root&&!saved->outer_window&&!saved->outer_registering&&
            !manager->normal_queue_dispatching&&!manager->normal_queue_registering&&
            queue.alive.load()&&!queue.allocating&&!queue.appending&&
            queue.entry_count_c==queue.entries.size()&&queue.append_sequence==queue.entry_count_c&&
            queue.outer_index_4==queue.outer_windows.size()&&!queue.outer_open_18,
            "Outer open requires genuine fresh pre-Common/current outer Queue slot");};
    const auto full=[&]{first();root.require_source_owned(operation);first();};
    first();require(!saved->outer_opening,"Actual outer open once-only factory reentered");
    full();first();require(!saved->outer_opening&&queue.outer_index_4<UINT32_MAX,"Actual outer Queue slot space exhausted/reentered");
    const auto index=queue.outer_index_4,begin=queue.entry_count_c;
    saved->outer_opening=true;
    struct End{std::shared_ptr<const detail::BnwNormalRootBatchHistory::Impl> r;~End(){r->outer_opening=false;}}end{saved};
    try{
        const auto pending=[&]{first();require(saved->outer_opening&&queue.outer_index_4==index&&
            queue.entry_count_c==begin,"Outer open allocation POST changed actual Queue counters");};
        pending();full();pending();queue.outer_windows.reserve(queue.outer_windows.size()+1);pending();full();pending();
        auto outer=std::make_shared<Gui::Impl::NormalGuiOuterWindow>(prefix,index);pending();full();pending();
        // Actual Root wrapper arg4/header=0, ctor +58=inner count, +5C=0.
        // Own constructor return is the CURRENT outer index, never selector1.
        queue.outer_windows.push_back(outer);queue.outer_open_18=true;
        manager->normal_queue_drained=false;manager->normal_queue_dispatch_count=0;
        outer->header_0.emplace(0);outer->begin_58.emplace(begin);outer->end_5c.emplace(0);outer->opened=true;
        saved->outer_window=outer;saved->outer_opening=false;
        root.require_physical_current_without_callbacks();root.require_source_owned(operation);
        root.require_physical_current_without_callbacks();
        require(outer->index==index&&queue.outer_index_4==index&&queue.outer_open_18&&
            outer->begin_58==begin&&outer->end_5c==0&&outer->header_0==0,
            "Actual open outer stores/returned index changed");
    }catch(...){manager->failed.store(true);throw;}
}

void BnwWorldViewControlTree::require_normal_root_queue_registration_metadata_without_callbacks(
    const detail::BnwNormalRootBatchHistory::Guard& root){
    root.require_physical_current_without_callbacks();const auto saved=root.impl_;
    const auto manager=saved->gui.lock();const auto registration=saved->outer_registration.lock();
    const auto outer=saved->outer_window;const auto prefix=saved->prefix;
    require(manager&&registration&&outer&&registration->creator==saved->creator&&registration->sealed&&
        registration->root.lock()==saved&&registration->prefix.lock()==prefix&&registration->outer==outer&&
        registration->outer_index==outer->index&&registration->table_index<manager->normal_queue_registrations.size()&&
        manager->normal_queue_registrations[registration->table_index]==registration&&
        registration->serial==saved->serial&&registration->batch_revision==saved->batch_revision&&
        !saved->outer_opening&&!saved->outer_registering&&!manager->normal_queue_registering,
        "Root queue registration lacks SAME actual manager table/outer index/control");
    const auto& queue=prefix->batch->payload_queue;
    require(queue.alive.load()&&!queue.allocating&&!queue.appending&&!queue.outer_open_18&&
        outer->creator==saved->creator&&outer->opened&&outer->closed&&outer->prefix.lock()==prefix&&
        outer->header_0==0&&outer->begin_58&&outer->end_5c&&*outer->begin_58<=*outer->end_5c&&
        *outer->end_5c==queue.entry_count_c&&queue.entry_count_c==queue.entries.size()&&
        outer->index<queue.outer_windows.size()&&queue.outer_windows[outer->index]==outer&&
        queue.outer_index_4==outer->index+1,
        "Root outer command half-open inner32 window/counter stores changed");
    const auto common=prefix->common_root;
    require(common&&common->whole_return&&!common->continuing&&common->cursor==common->count&&
        (common->stage==decltype(common)::element_type::Stage::returned_root||
         common->stage==decltype(common)::element_type::Stage::returned_empty),
        "Root manager registry lacks actual sealed complete Root18 metadata");
    const auto returned=common->whole_return;
    require(returned->creator==saved->creator&&returned->root.lock()==common&&returned->batch.lock()==prefix&&
        returned->serial==saved->serial&&returned->count==common->count&&returned->cursor==common->count,
        "Root manager registry earned WholeReturn original control/serial/count changed");
    root.require_physical_current_without_callbacks();
}

void BnwWorldViewControlTree::require_normal_root_queue_registration_without_callbacks(
    const detail::BnwNormalRootBatchHistory::Guard& root){
    root.require_physical_current_without_callbacks();const auto saved=root.impl_;
    const auto manager=saved->gui.lock();const auto registration=saved->outer_registration.lock();
    const auto outer=saved->outer_window;const auto prefix=saved->prefix;
    require(manager&&registration&&outer&&registration->creator==saved->creator&&registration->sealed&&
        registration->root.lock()==saved&&registration->prefix.lock()==prefix&&registration->outer==outer&&
        registration->outer_index==outer->index&&registration->table_index<manager->normal_queue_registrations.size()&&
        manager->normal_queue_registrations[registration->table_index]==registration&&
        registration->serial==saved->serial&&registration->batch_revision==saved->batch_revision&&
        !saved->outer_opening&&!saved->outer_registering&&!manager->normal_queue_registering,
        "Root queue registration lacks SAME actual manager table/outer index/control");
    const auto& queue=prefix->batch->payload_queue;
    require(queue.alive.load()&&!queue.allocating&&!queue.appending&&!queue.outer_open_18&&
        outer->creator==saved->creator&&outer->opened&&outer->closed&&outer->prefix.lock()==prefix&&
        outer->header_0==0&&outer->begin_58&&outer->end_5c&&*outer->begin_58<=*outer->end_5c&&
        *outer->end_5c==queue.entry_count_c&&queue.entry_count_c==queue.entries.size()&&
        outer->index<queue.outer_windows.size()&&queue.outer_windows[outer->index]==outer&&
        queue.outer_index_4==outer->index+1,
        "Root outer command half-open inner32 window/counter stores changed");
    require_normal_root18_return_without_callbacks(*prefix);
    root.require_physical_current_without_callbacks();
}

void BnwWorldViewControlTree::close_normal_root_outer_window_and_register_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchHistory::Guard& actual_root)const{
    using Gui=detail::NotificationContextGuiPort;
    const auto root=actual_root;root.require_physical_current_without_callbacks();const auto saved=root.impl_;
    require(saved&&saved->tree.get()==this,"Outer close/register another actual Root/Tree");
    const auto prefix=saved->prefix;const auto manager=saved->gui.lock();const auto outer=saved->outer_window;
    require(manager&&outer,"Outer close/register pre-Common original outer slot absent");
    auto& queue=prefix->batch->payload_queue;
    const auto first=[&]{root.require_physical_current_without_callbacks();
        require_normal_root18_return_without_callbacks(*prefix);
        require(!saved->outer_opening&&!saved->outer_registering&&saved->outer_registration.expired()&&
            !manager->normal_queue_registering&&!manager->normal_queue_dispatching&&
            queue.alive.load()&&!queue.allocating&&!queue.appending&&
            queue.entry_count_c==queue.entries.size()&&queue.append_sequence==queue.entry_count_c&&
            queue.outer_open_18&&queue.outer_index_4==outer->index&&
            outer->index<queue.outer_windows.size()&&queue.outer_windows[outer->index]==outer&&
            outer->opened&&!outer->closed&&outer->header_0==0&&outer->begin_58&&outer->end_5c==0&&
            *outer->begin_58<=queue.entry_count_c,
            "Outer close/register requires actual completed Root and SAME current original outer slot");};
    const auto full=[&]{first();root.require_source_owned(operation);first();};
    first();full();first();const auto end_inner=queue.entry_count_c,index=outer->index;
    const auto table=manager->normal_queue_registrations.size();
    saved->outer_registering=true;manager->normal_queue_registering=true;
    struct End{std::shared_ptr<const detail::BnwNormalRootBatchHistory::Impl> r;std::shared_ptr<Gui::Impl> m;
        ~End(){r->outer_registering=false;m->normal_queue_registering=false;}}end{saved,manager};
    try{
        const auto pending=[&]{root.require_physical_current_without_callbacks();
            require_normal_root18_return_without_callbacks(*prefix);
            require(saved->outer_registering&&manager->normal_queue_registering&&saved->outer_registration.expired()&&
                queue.outer_open_18&&queue.outer_index_4==index&&queue.entry_count_c==end_inner&&
                queue.entries.size()==end_inner&&queue.outer_windows[index]==outer&&
                !outer->closed&&outer->end_5c==0&&manager->normal_queue_registrations.size()==table,
                "Outer registration allocation/FULL POST changed original slot/window/table");};
        pending();root.require_source_owned(operation);pending();
        manager->normal_queue_registrations.reserve(table+1);pending();root.require_source_owned(operation);pending();
        auto registration=std::make_shared<Gui::Impl::NormalGuiManagerRegistration>(saved,prefix,outer,index,table,
            saved->serial,saved->batch_revision);pending();root.require_source_owned(operation);pending();
        // Close8ECED0 then registrar8EC9F0/8E2070. No Draw or drain is claimed.
        outer->end_5c.emplace(end_inner);++queue.outer_index_4;queue.outer_open_18=false;outer->closed=true;
        manager->normal_queue_registrations.push_back(registration);saved->outer_registration=registration;
        registration->sealed=true;saved->outer_registering=false;manager->normal_queue_registering=false;
        require_normal_root_queue_registration_without_callbacks(root);root.require_source_owned(operation);
        require_normal_root_queue_registration_without_callbacks(root);
    }catch(...){manager->failed.store(true);throw;}
}

