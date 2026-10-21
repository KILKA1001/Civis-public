// SAME original Root execution: bootstrap the first ACTIVE child in its
// already-open original paint, then continue only after that paint RETURN.
// No Publication/Frame clone, recursive paint or ACTIVE Matrix renewal.
void BnwWorldViewControlTree::execute_normal_visible_label_child_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<detail::BnwNormalRootBatchExecution::Impl>& execution,
    const std::shared_ptr<const NormalVisibleLabelConsumer>& consumer,
    const detail::BnwLabelActualPaintCallFrame::Guard* actual_bootstrap_frame)const{
    using Execution=detail::BnwNormalRootBatchExecution;using Call=detail::BnwNormalVisibleLabelCall;
    using Prefix=detail::BnwNormalGuiBatchPrefix::Impl;
    // Guard copy and all original controls precede any FULL or allocator.
    std::optional<detail::BnwLabelActualPaintCallFrame::Guard> bootstrap;
    if(actual_bootstrap_frame)bootstrap.emplace(*actual_bootstrap_frame);
    execution->first();const auto root=execution->root;const auto actual=root.impl_;
    const auto prefix=actual->prefix;const auto matrix=execution->original_matrix;
    const auto common=prefix->common_root;
    const auto first=[&]{execution->first();root.require_physical_current_without_callbacks();
        require(actual->tree.get()==this&&execution->original_prefix&&
            execution->original_prefix->impl_==prefix&&actual->matrix_facade==matrix&&
            actual->label_traversal_active&&!actual->label_traversal_completed&&
            (execution->label_phase==Execution::Impl::LabelPhase::bootstrap_active||
             execution->label_phase==Execution::Impl::LabelPhase::continuation_active),
            "CURRENT Label child lacks genuine active Root traversal phase");
        if(bootstrap){
            bootstrap->require_current_without_callbacks();
            require(prefix->index&&prefix->indexed_control&&prefix->control&&!prefix->context_entry,
                "Indexed Label bootstrap cannot consume a Context root prefix");
            bootstrap->require_actual_paint_item_without_callbacks(*actual->publication,*prefix->index);
            bootstrap->require_actual_control_without_callbacks(*this,*prefix->control);
            require(execution->label_phase==Execution::Impl::LabelPhase::bootstrap_active,
                "Bootstrap Frame cannot grant later child staging");
        }
    };
    const auto full=[&]{first();execution->full(operation);first();
        if(bootstrap){bootstrap->require_source_owned(operation);first();}};
    first();full();first();
    require(common&&!common->continuing&&common->stage==Prefix::CommonRoot::Stage::child18_required&&
        common->cursor<common->count&&!common->label_tail&&actual->current_visible_label.expired(),
        "Root traversal is not at genuine pending child18 call");
    const auto ordinal=common->cursor;const auto receiver=common->ordered[ordinal];
    require(receiver&&receiver->impl_&&receiver->impl_->id,
        "Visible Context child requires its actual recursive dispatcher");
    const auto receiver_storage=receiver->impl_;const auto control=receiver_storage->id;
    const auto label=receiver_storage->standalone_label.lock();
    require(label&&receiver_storage->core.lock().get()==core_.get(),
        "Visible non-Label/domain receiver requires its actual dispatcher");
    if(bootstrap){
        bootstrap->require_actual_control_without_callbacks(*this,control);
        require(control==prefix->control&&execution->original_materials.empty()&&common->label_returns.empty(),
            "First actual native visible child differs from SAME original bootstrap paint");
    }
    const auto pending=[&]{first();require(common==prefix->common_root&&!common->continuing&&
        common->stage==Prefix::CommonRoot::Stage::child18_required&&common->cursor==ordinal&&
        !common->whole_return&&!common->label_tail&&actual->current_visible_label.expired()&&
        receiver==common->ordered[ordinal]&&receiver->impl_==receiver_storage&&
        receiver_storage->id==control&&receiver_storage->standalone_label.lock()==label,
        "Visible Label issuer allocation POST changed real callsite");};
    pending();full();pending();
    auto owned=std::make_shared<Call::Impl>(root,receiver,label,control,actual->serial,ordinal);
    pending();full();pending();
    actual->current_visible_label=owned;owned->canonical_self=owned;owned->sealed=true;
    const Call::IssuerKey key;const Call call(owned,key);
    call.require_current_without_callbacks();call.require_source_owned(operation);first();
    const auto before=execution->original_materials.size();
    require(before<execution->original_materials.max_size()&&
        common->label_returns.size()<common->label_returns.max_size(),
        "Actual completed Label journal capacity exhausted");
    execution->original_materials.reserve(before+1);
    call.require_current_without_callbacks();full();call.require_current_without_callbacks();
    // Bootstrap closure stages SAME already-open original item directly.
    // Continuation closure enters its distinct original Publication once.
    // Neither callback bool nor copied scalar can earn a genuine child return.
    auto material=(*consumer)(operation,root,matrix,call);
    call.require_current_without_callbacks();full();call.require_current_without_callbacks();
    require(material&&material->impl_,"Visible Label consumer did not return original Material");
    const auto returned=material->impl_;returned->child_return_first();
    require(!returned->sealed&&returned->cpu_child_returned&&returned->selected_call.lock()==owned&&
        returned->root.impl_==actual&&returned->batch==prefix&&returned->item.impl_->control==control&&
        returned->child_tail&&common->label_tail==returned->child_tail&&
        returned->child_tail->ordinal==ordinal&&execution->original_materials.size()==before,
        "Visible Label consumer returned another Item/control/tail/material or wholeRoot shortcut");
    returned->child_return_full(operation);call.require_current_without_callbacks();first();
    const auto tail=common->label_tail;
    common->label_returns.reserve(common->label_returns.size()+1);
    returned->child_return_first();full();call.require_current_without_callbacks();
    require(common->label_tail==tail&&common->cursor==ordinal&&
        execution->original_materials.size()==before,
        "Visible Label returned journal allocation POST changed actual child");
    // Reserved effect commit archives original Material and tail, retires
    // CURRENT token, then executes the authentic caller56F727 cursor advance.
    execution->original_materials.push_back(std::move(material));
    common->label_returns.push_back(tail);common->label_tail.reset();
    owned->alive.store(false);actual->current_visible_label.reset();
    ++common->cursor;common->stage=Prefix::CommonRoot::Stage::arithmetic;
    continue_normal_context_common56_owned(prefix,nullptr);
    first();full();first();
}

void BnwWorldViewControlTree::bootstrap_normal_root_visible_label_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchExecution& original_execution,
    const detail::BnwLabelActualPaintCallFrame& original_frame,
    std::shared_ptr<const NormalVisibleLabelConsumer> actual_consumer)const{
    using Execution=detail::BnwNormalRootBatchExecution;
    using Prefix=detail::BnwNormalGuiBatchPrefix::Impl;
    const auto frame=original_frame.capture_guard_without_callbacks();
    const auto execution=original_execution.impl_;const auto consumer=std::move(actual_consumer);
    require(execution&&consumer&&bool(*consumer),"Root bootstrap needs original execution and owned genuine first consumer");
    execution->first();const auto root=execution->root;const auto actual=root.impl_;
    const auto prefix=actual->prefix;
    const auto first=[&]{
        frame.require_current_without_callbacks();execution->first();root.require_physical_current_without_callbacks();
        require(prefix->index&&prefix->indexed_control&&prefix->control&&!prefix->context_entry,
            "Indexed first Label entry requires its genuine Control and paint index");
        frame.require_actual_paint_item_without_callbacks(*actual->publication,*prefix->index);
        frame.require_actual_control_without_callbacks(*this,*prefix->control);
        require(actual->tree.get()==this&&core_->busy&&actual->publication->adjusted_frame_painting_&&
            execution->original_prefix&&execution->original_prefix->impl_==prefix&&
            actual->matrix_facade==execution->original_matrix,
            "Root bootstrap is not inside SAME first original active paint");
    };
    const auto full=[&]{first();execution->full(operation);first();frame.require_source_owned(operation);first();};
    first();full();first();
    require(execution->label_phase==Execution::Impl::LabelPhase::unstarted&&
        !actual->label_traversal_active&&!actual->label_traversal_completed&&!prefix->common_root&&
        actual->current_visible_label.expired()&&execution->original_materials.empty(),
        "Root bootstrap replay/reentry or Common already entered");
    execution->label_phase=Execution::Impl::LabelPhase::bootstrap_active;
    actual->label_traversal_active=true;
    struct End{std::shared_ptr<const detail::BnwNormalRootBatchHistory::Impl> root;
        ~End(){root->label_traversal_active=false;}}end{actual};
    try{
        // ONE actual Common entry pauses at its first native visible child.
        execute_normal_context_selected_label_entry_owned(prefix,nullptr);
        first();full();first();const auto common=prefix->common_root;
        require(common&&!common->whole_return&&
            common->stage==Prefix::CommonRoot::Stage::child18_required,
            "Root bootstrap has no genuine first visible native Label call");
        execute_normal_visible_label_child_owned(operation,execution,consumer,&frame);
        first();full();first();
        require(execution->original_materials.size()==1&&common->label_returns.size()==1&&
            !common->label_tail&&actual->current_visible_label.expired()&&
            execution->label_phase==Execution::Impl::LabelPhase::bootstrap_active,
            "Root bootstrap did not complete exactly SAME first child and actual cursor");
        // This phase is earned by authentic child return; it grants no Root
        // seal, registration or drain. The caller still must return its paint.
        actual->label_traversal_active=false;
        execution->label_phase=Execution::Impl::LabelPhase::bootstrap_returned;
        first();full();first();
    }catch(...){
        if(const auto current=actual->current_visible_label.lock())current->alive.store(false);
        actual->current_visible_label.reset();prefix->alive.store(false);prefix->batch->alive.store(false);
        if(const auto manager=actual->gui.lock())manager->failed.store(true);
        core_->failed.store(true);throw;
    }
}

void BnwWorldViewControlTree::stage_normal_root_visible_labels_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalRootBatchExecution& original_execution,
    std::shared_ptr<const NormalVisibleLabelConsumer> actual_consumer)const{
    using Execution=detail::BnwNormalRootBatchExecution;using Prefix=detail::BnwNormalGuiBatchPrefix::Impl;
    const auto execution=original_execution.impl_;const auto consumer=std::move(actual_consumer);
    require(execution&&consumer&&bool(*consumer),"Root continuation needs original execution and owned genuine consumer");
    execution->first();const auto root=execution->root;const auto actual=root.impl_;
    const auto prefix=actual->prefix;const auto matrix=execution->original_matrix;
    const auto first=[&]{execution->first();root.require_physical_current_without_callbacks();
        require(actual->tree.get()==this&&execution->original_prefix&&
            execution->original_prefix->impl_==prefix&&actual->matrix_facade==matrix&&
            !core_->busy&&!actual->publication->adjusted_frame_painting_,
            "Root continuation requires original first paint RETURN and no nested child paint");};
    const auto full=[&]{first();execution->full(operation);first();};
    first();full();first();const auto common=prefix->common_root;
    require(execution->label_phase==Execution::Impl::LabelPhase::bootstrap_returned&&
        !actual->label_traversal_active&&!actual->label_traversal_completed&&common&&
        actual->current_visible_label.expired()&&execution->original_materials.size()==1&&
        common->label_returns.size()==1&&!common->label_tail&&!common->continuing,
        "Root continuation lacks genuine first-child bootstrap phase or was replayed");
    execution->label_phase=Execution::Impl::LabelPhase::continuation_active;
    actual->label_traversal_active=true;
    struct End{std::shared_ptr<const detail::BnwNormalRootBatchHistory::Impl> root;
        ~End(){root->label_traversal_active=false;}}end{actual};
    try{
        while(!common->whole_return){
            first();full();first();
            execute_normal_visible_label_child_owned(operation,execution,consumer,nullptr);
            first();full();first();
        }
        require_normal_root18_return_without_callbacks(*prefix);first();full();first();
        close_normal_root_outer_window_and_register_owned(operation,root);first();full();first();
        require_normal_root_queue_registration_without_callbacks(root);
        require(execution->original_materials.size()==common->label_returns.size(),
            "Whole Root completed Label/material journal membership differs");
        for(const auto& material:execution->original_materials){
            const auto original=material->impl_;original->child_return_first();
            original->child_return_full(operation);first();
            require(!original->sealed,"Whole Root material sealed before real traversal/registration");
            original->sealed=true;original->first();original->full(operation);original->first();first();
        }
        require(actual->current_visible_label.expired()&&!actual->label_traversal_completed&&
            execution->label_phase==Execution::Impl::LabelPhase::continuation_active,
            "Whole Root completion current child/replay phase changed");
        actual->label_traversal_active=false;actual->label_traversal_completed=true;
        execution->label_phase=Execution::Impl::LabelPhase::completed;
        first();full();first();require_normal_root_queue_registration_without_callbacks(root);
    }catch(...){
        if(const auto current=actual->current_visible_label.lock())current->alive.store(false);
        actual->current_visible_label.reset();prefix->alive.store(false);prefix->batch->alive.store(false);
        if(const auto manager=actual->gui.lock())manager->failed.store(true);
        core_->failed.store(true);throw;
    }
}
