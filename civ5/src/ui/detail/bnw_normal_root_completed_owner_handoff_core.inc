// Inside civ5::ui AFTER actual Root traversal, manager dispatch and Material.
// This is owner retirement, not native Queue.byte50/pool-reset emulation.
std::array<std::uint64_t,4> BnwWorldViewControlTree::retire_completed_normal_root_batch_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchExecution& original_execution)const{
    using Execution=detail::BnwNormalRootBatchExecution;
    // Capture the SAME original strong controls before any callback/FULL.
    const auto execution=original_execution.impl_;
    require(bool(execution),"Completed Root handoff lacks original execution control");
    execution->first();const auto root=execution->root;const auto saved=root.impl_;
    const auto manager=saved->gui.lock();const auto prefix=saved->prefix;
    const auto batch=prefix->batch;const auto source=manager?manager->source.lock():nullptr;
    require(manager&&batch&&source&&saved->tree.get()==this,
        "Completed Root handoff lacks SAME actual Tree/Gui/Source/Batch");
    const auto same=[](const auto&a,const auto&b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    };
    const auto first=[&]{
        // CURRENT authority FIRST before all borrowed owner/queue/lifetime data.
        execution->first();root.require_physical_current_without_callbacks();
        require(saved->tree.get()==this&&execution->original_prefix&&
            same(execution->original_prefix->impl_,prefix)&&same(execution->original_matrix,saved->matrix_facade)&&
            !core_->busy&&!saved->publication->adjusted_frame_painting_&&
            execution->label_phase==Execution::Impl::LabelPhase::completed&&
            saved->label_traversal_completed&&!saved->label_traversal_active&&!saved->item_capturing&&
            saved->current_visible_label.expired()&&!saved->outer_opening&&!saved->outer_registering,
            "Completed Root handoff requires genuine whole traversal and all original paints RETURN");
        require(same(manager->source.lock(),source)&&!manager->normal_root_history_capturing&&
            !manager->normal_gui_batch_beginning&&same(manager->normal_root_history.lock(),saved)&&
            same(manager->normal_gui_batch_prefix.lock(),prefix)&&same(manager->normal_gui_batch.lock(),batch)&&
            manager->normal_gui_batch_revision==saved->batch_revision&&
            manager->normal_queue_drained&&!manager->normal_queue_dispatching&&!manager->normal_queue_registering&&
            manager->normal_queue_registrations.empty()&&manager->normal_queue_dispatch_count==0&&
            manager->normal_queue_current_permission.expired()&&!manager->normal_queue_permission_capturing&&
            !manager->normal_queue_selected_registration,
            "Completed Root handoff requires actual completed manager table/count/permission retirement");
        require_normal_root18_return_without_callbacks(*prefix);
        const auto common=prefix->common_root;
        require(common&&common->whole_return&&execution->original_materials.size()==common->label_returns.size()&&
            saved->queue_drained&&!saved->completed_registrations.empty(),
            "Completed Root handoff lacks genuine whole return/retained consumed registrations");
        const auto& queue=batch->payload_queue;
        require(queue.alive.load()&&queue.creator==saved->creator&&!queue.allocating&&!queue.appending&&
            queue.current_loan.expired()&&!queue.outer_open_18&&queue.entries.size()==queue.entry_count_c&&
            queue.append_sequence==queue.entry_count_c&&queue.outer_windows.size()==queue.outer_index_4,
            "Completed Root handoff original CPU queue is still active or its physical counters changed");
        std::uint64_t consumed=0;
        for(std::size_t table=0;table<saved->completed_registrations.size();++table){
            const auto registration=saved->completed_registrations[table];
            require(registration&&registration->creator==saved->creator&&registration->sealed&&registration->drained&&
                registration->table_index==table&&same(registration->root.lock(),saved)&&
                same(registration->prefix.lock(),prefix)&&registration->serial==saved->serial&&
                registration->batch_revision==saved->batch_revision&&
                same(registration,saved->outer_registration.lock())&&same(registration->outer,saved->outer_window),
                "Completed Root handoff consumed registration is not SAME actual owner/outer/table occurrence");
            const auto outer=registration->outer;
            require(outer&&outer->creator==saved->creator&&same(outer->prefix.lock(),prefix)&&
                outer->opened&&outer->closed&&outer->header_0==0&&outer->begin_58&&outer->end_5c&&
                *outer->begin_58<=*outer->end_5c&&*outer->end_5c==queue.entry_count_c&&
                registration->current_inner==outer->end_5c&&registration->outer_index==outer->index&&
                outer->index<queue.outer_windows.size()&&same(queue.outer_windows[outer->index],outer)&&
                queue.outer_index_4==outer->index+1,
                "Completed Root handoff original half-open window/current inner cursor did not finish");
            const auto length=std::uint64_t{*outer->end_5c-*outer->begin_58};
            require(consumed<=UINT64_MAX-length,"Completed Root handoff consumed cursor count exhausted");
            consumed+=length;
        }
        const auto observations=saved->queue_observations;
        require(observations[0]==consumed&&observations[1]<=observations[0]&&
            observations[2]<=observations[0]-observations[1]&&
            observations[3]==observations[0]-observations[1]-observations[2],
            "Completed Root handoff observations differ from actual returned original inner cursor");
        execution->first();root.require_physical_current_without_callbacks();
    };
    const auto full=[&]{first();root.require_source_owned(operation);first();};
    try{
        first();full();first();
        const auto observations=saved->queue_observations;
        const auto registrations=saved->completed_registrations.size();
        const auto materials=execution->original_materials.size();
        const auto revision=manager->normal_gui_batch_revision;
        const auto matrix_epoch=saved->matrix_epoch;
        const auto& queue=batch->payload_queue;
        const auto inner_count=queue.entry_count_c,outer_count=queue.outer_index_4;
        first();full();first();
        require(saved->queue_observations==observations&&saved->completed_registrations.size()==registrations&&
            execution->original_materials.size()==materials&&manager->normal_gui_batch_revision==revision&&
            saved->matrix_epoch==matrix_epoch&&queue.entry_count_c==inner_count&&queue.outer_index_4==outer_count,
            "Completed Root handoff immutable observations/custody changed across final FULL");
        // One irreversible retirement. No allocator, arithmetic, Source FULL
        // or callback occurs between these stores. No epoch/revision/cursor,
        // queue bytes, arena, CPU entries or original owner is reset/reissued.
        execution->alive.store(false);
        prefix->alive.store(false);batch->alive.store(false);
        manager->normal_root_history.reset();
        manager->normal_gui_batch_prefix.reset();manager->normal_gui_batch.reset();

        // This distinct postretirement cut validates OWN retained physical
        // custody and fresh Source origin, never a retired Root/Item/Matrix
        // CURRENT guard. It grants no next stage, Draw or renewed permission.
        require(execution->creator==std::this_thread::get_id()&&execution->sealed&&!execution->alive.load()&&
            execution->original_prefix&&same(execution->original_prefix->impl_,prefix)&&
            same(execution->original_matrix,saved->matrix_facade)&&
            !prefix->alive.load()&&!batch->alive.load()&&same(prefix->batch,batch)&&
            execution->label_phase==Execution::Impl::LabelPhase::completed&&
            execution->original_materials.size()==materials&&saved->queue_drained&&
            saved->completed_registrations.size()==registrations&&saved->queue_observations==observations&&
            queue.alive.load()&&queue.entry_count_c==inner_count&&queue.entries.size()==inner_count&&
            queue.outer_index_4==outer_count&&queue.outer_windows.size()==outer_count,
            "Completed Root handoff original physical owner/cursor custody changed during retirement");
        saved->root_scope.require_physical_current_without_callbacks();
        saved->root_scope.require_actual_renderer_without_callbacks(*saved->renderer);
        const auto& fresh=source->current_operation_loan();
        require(&fresh==&operation&& &fresh==saved->operation,
            "Completed Root handoff fresh Source differs before saved operand");
        fresh.require_origin_without_callbacks();
        require(fresh.operation_serial()==saved->serial&&same(manager->source.lock(),source),
            "Completed Root handoff immutable Source origin/serial changed");
        manager->first(fresh);core_->layout_origin(true);
        require(&core_->source->current_operation_loan()==&fresh&&same(saved->gui.lock(),manager)&&
            manager->normal_root_history.expired()&&manager->normal_gui_batch_prefix.expired()&&
            manager->normal_gui_batch.expired()&&!manager->normal_root_history_capturing&&
            !manager->normal_gui_batch_beginning&&manager->normal_gui_batch_revision==revision&&
            saved->matrix_epoch==matrix_epoch&&saved->matrix_storage->update==matrix_epoch&&
            manager->normal_queue_drained&&!manager->normal_queue_dispatching&&!manager->normal_queue_registering&&
            manager->normal_queue_registrations.empty()&&manager->normal_queue_dispatch_count==0&&
            manager->normal_queue_current_permission.expired()&&!manager->normal_queue_permission_capturing&&
            !manager->normal_queue_selected_registration,
            "Completed Root handoff failed to detach only SAME old authority or renewed an epoch");
        saved->root_scope.require_physical_current_without_callbacks();
        fresh.require_origin_without_callbacks();
        require(&source->current_operation_loan()==&fresh&&fresh.operation_serial()==saved->serial,
            "Completed Root handoff terminal Source physical cut changed");
        return observations;
    }catch(...){manager->failed.store(true);core_->failed.store(true);throw;}
}
