// Additive embedded178 CPU Item admission. Active capture below earns the
// separate historical SAME-original association; no retired Call/Frame API
// is renamed as a historical check. The old standalone Item body is unchanged.
detail::BnwNormalItemOccurrence::Impl::Impl(
    Root::Guard actual_root,BnwLabelActualPaintCallFrame::RootScopeHealthGuard actual_scope,
    std::shared_ptr<const BnwWorldViewControlRenderPublication> actual_publication,
    std::shared_ptr<const ComposedUiFrame> actual_frame,std::shared_ptr<BnwWorldViewControlTree> actual_tree,
    const UiScreenGlRenderer* actual_renderer,const runtime::BnwWorldViewSourceOperationLoan* actual_operation,
    std::uint64_t actual_serial,std::uint64_t actual_control,std::uint64_t actual_occurrence,std::size_t actual_index,
    Paint::Guard actual_paint,Data::Guard actual_data,Binding::Guard actual_binding,
    std::shared_ptr<Paint::Impl> actual_paint_storage,
    BnwNormalButtonCaptionCall actual_caption,std::shared_ptr<const BnwNormalButtonCaptionPaintEmission> actual_emission,
    std::shared_ptr<Paint::PositionedState> actual_data_storage,std::array<std::uint32_t,2> actual_numeric,
    std::vector<Data::RecordData> actual_records,std::vector<Data::GlyphData> actual_glyphs,
    std::vector<Data::PassData> actual_passes)
    :root(std::move(actual_root)),scope(std::move(actual_scope)),publication(std::move(actual_publication)),
     adjusted_frame(std::move(actual_frame)),tree(std::move(actual_tree)),renderer(actual_renderer),
     operation(actual_operation),serial(actual_serial),control(actual_control),occurrence(actual_occurrence),index(actual_index),
     paint(std::move(actual_paint)),data(std::move(actual_data)),binding(std::move(actual_binding)),paint_storage(),
     embedded_paint_storage(std::move(actual_paint_storage)),caption_history(std::move(actual_caption)),
     caption_emission(std::move(actual_emission)),data_storage(std::move(actual_data_storage)),
     numeric(actual_numeric),records(std::move(actual_records)),glyphs(std::move(actual_glyphs)),passes(std::move(actual_passes)){}

void BnwWorldViewControlTree::require_normal_caption_item_physical_without_callbacks(
    const detail::BnwNormalItemOccurrence::Impl& saved){
    using Item=detail::BnwNormalItemOccurrence;using History=detail::BnwNormalRootBatchHistory;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(saved.creator==std::this_thread::get_id()&&saved.sealed&&saved.embedded_paint_storage&&
        !saved.paint_storage&&saved.caption_history&&saved.caption_emission,
        "Embedded178 Item lacks earned distinct original storage FIRST");
    saved.root.require_physical_current_without_callbacks();saved.scope.require_physical_current_without_callbacks();
    const auto root=saved.root.impl_;
    require(root&&saved.publication&&saved.adjusted_frame&&saved.tree&&saved.renderer&&saved.data_storage&&
        root->operation==saved.operation&&root->serial==saved.serial&&root->renderer==saved.renderer&&
        same(root->tree,saved.tree)&&saved.occurrence&&saved.occurrence<=root->item_occurrences.size()&&
        !root->item_capturing&&root->item_occurrences[saved.occurrence-1].lock().get()==&saved,
        "Embedded178 Item is not genuine registered SAME Root occurrence");
    const auto* context=std::get_if<History::Impl::RootScope::Root>(&root->root_scope.value);
    require(context,"Embedded178 Item requires genuine Context Root health, no Label alias");
    const auto caption=saved.caption_history->impl_;const auto emission=saved.caption_emission->impl_;
    require(caption&&caption->creator==saved.creator&&caption->sealed&&
        same(caption->canonical_self.lock(),caption)&&emission&&emission->creator==saved.creator&&emission->sealed&&
        same(emission->canonical_self.lock(),emission)&&same(emission->canonical_facade.lock(),saved.caption_emission)&&
        same(emission->caption.impl_,caption)&&caption->original_emission_control&&
        same(caption->original_emission_control->lock(),emission)&&same(caption->run,emission->run)&&
        caption->paint_arguments&&!caption->projecting&&same(emission->origin,saved.adjusted_frame)&&
        emission->index==saved.index&&emission->count==saved.adjusted_frame->paints.size()&&
        saved.index<emission->count&&emission->item==&saved.adjusted_frame->paints[saved.index],
        "Embedded178 historical Item lost original Caption/Run/emission/carrier controls");
    const auto common=caption->enclosing_call.impl_;
    require(common&&common->creator==saved.creator&&common->sealed&&same(common->canonical_self.lock(),common)&&
        common->original_caption_call_control&&same(common->original_caption_call_control->lock(),caption)&&
        common->kind==detail::BnwNormalCommonChildCall::Kind::button&&common->serial==caption->serial&&
        common->base_sequence==caption->base_sequence&&common->ordinal==caption->ordinal&&
        same(common->original_button,caption->button)&&same(common->receiver,caption->receiver)&&
        saved.tree->core_->checked(common->control)==saved.control,
        "Embedded178 historical Item original Common controls changed; retirement is not reissuance");
    caption->root.require_same_root_without_callbacks(saved.root);
    // Genuine cross-type historical relation: same Context/entry/source/life,
    // never active stamp equality or a cast of a Generic Frame into Label.
    context->require_actual_label_child_scope_without_callbacks(saved.scope,caption->entry);
    saved.scope.require_actual_renderer_without_callbacks(*saved.renderer);
    saved.scope.require_actual_publication_without_callbacks(*saved.publication);
    saved.scope.require_actual_frame_owner_without_callbacks(saved.adjusted_frame);
    const auto original=saved.publication->original_owner_without_callbacks();
    require(same(original,saved.publication)&&same(saved.publication->tree_,saved.tree)&&
        same(saved.publication->normal_caption_emission_,saved.caption_emission)&&
        !saved.publication->normal_label_emission_&&!saved.publication->context_child_body_&&
        saved.publication->enclosing_context_entry_&&!saved.publication->root_context_entry_&&
        saved.publication->paint_indices_.size()==1&&saved.publication->paint_indices_[0]==saved.index,
        "Embedded178 historical Item does not retain SAME original caption Publication");
    caption->entry.require_same_entry_without_callbacks(*saved.publication->enclosing_context_entry_);
    saved.publication->require_adjusted_frame_owner_without_callbacks(saved.adjusted_frame);
    require(saved.publication->actual_checked_control_id_for_paint_without_callbacks()==saved.control&&
        saved.tree->core_->checked(caption->control)==saved.control,
        "Embedded178 historical Item actual parent Control changed");
    const auto fields=caption->label->impl_;const auto run=caption->run->impl_;
    const auto factory=caption->factory->impl_;
    const auto& node=Core::node(saved.tree->core_->nodes,saved.control);
    require(node.layout_baseline&&same(node.layout_baseline->button_,caption->button)&&
        same(node.layout_baseline->base_.receiver_owned,caption->receiver)&&
        same(caption->button->label_178_,caption->label)&&
        same(caption->button->label_178_factory_return_,caption->factory)&&
        factory&&factory->creator==saved.creator&&factory->alive.load()&&factory->published_sequence&&
        same(factory->child,caption->label)&&same(factory->parent.lock(),caption->button)&&
        factory->source_serial==caption->constructor_serial&&factory->layer==caption->parser_layer&&
        !caption->button->pending_label_178_&&fields&&same(fields->completed_run,caption->run)&&
        fields->sequence==caption->child_sequence&&fields->source_serial==caption->constructor_serial&&
        run&&same(run->child.lock(),caption->label)&&same(run->parent.lock(),caption->button)&&
        same(run->factory.lock(),caption->factory),
        "Embedded178 historical physical Button/Label/Factory/returned Run membership changed");
    const auto* vptr=std::get_if<detail::NotificationLayoutEmbeddedLabelState::Impl::Vptr>(&fields->fields[0]);
    require(vptr&&*vptr==detail::NotificationLayoutEmbeddedLabelState::Impl::Vptr::label_a48dd0,
        "Embedded178 historical actual Label vptr changed");
    const auto* parent=std::get_if<detail::NotificationLayoutEmbeddedLabelState::Impl::ParentBorrow>(
        &fields->fields[0x38/4]);
    const auto* realm=std::get_if<detail::NotificationLayoutEmbeddedLabelState::Impl::ParsedRealm>(
        &fields->fields[0x3c/4]);
    require(parent&&parent->baseline==&*node.layout_baseline&&same(parent->parent.lock(),caption->button)&&
        realm&&realm->value&&same(realm->value,node.layout_baseline->base_.context_realm_3c)&&
        same(realm->value,saved.tree->core_->context_realm_identity),
        "Embedded178 historical actual child38/Realm3C association changed");
    // Only actual initialized immutable Run/Label/module/Font health. This
    // check never invokes the retired Common CURRENT or Paint/Frame FIRST.
    run->require_published_data_without_callbacks();
    run->validate_child_without_callbacks(*saved.operation,caption->label);
    const auto paint=saved.embedded_paint_storage;const auto data=saved.data_storage;
    require_normal_caption_item_fp32_rect_physical_without_callbacks(saved);
    require(same(saved.paint.impl_,paint)&&!saved.paint.standalone_&&paint->creator==saved.creator&&
        same(paint->tree.lock(),saved.tree)&&paint->actual_control_id==saved.control&&
        same(paint->run.impl_,run)&&same(paint->system,run->system)&&paint->system&&
        saved.data.impl_&&same(saved.data.impl_->state,data)&&data->creator==saved.creator&&
        same(data->paint.impl_,paint)&&!data->paint.standalone_&&same(data->system,paint->system)&&
        data->numeric==saved.numeric&&saved.data.impl_->normal_draw_records_installed&&
        !saved.data.impl_->normal_draw_records_installing&&saved.data.impl_->normal_draw_records_revision==1&&
        bool(saved.data.impl_->normal_draw_records)&&saved.data.impl_->normal_item_occurrence_issued&&
        saved.data.impl_->normal_item_occurrence.lock().get()==&saved&&
        Item::Impl::same_records(data->records,saved.records)&&Item::Impl::same_glyphs(data->glyphs,saved.glyphs)&&
        Item::Impl::same_passes(data->passes,saved.passes),
        "Embedded178 historical SAME Paint/DATA controls or immutable CPU payload changed");
    require(bool(saved.historical_planes)==saved.planes_installed&&bool(saved.actual_atlas)==saved.planes_installed&&
        (!saved.planes_installing||(!saved.planes_installed&&!saved.historical_planes&&!saved.actual_atlas)),
        "Embedded178 historical once-only Atlas attachment journal differs");
    if(saved.planes_installed){saved.historical_planes->require_physical_current_without_callbacks();
        saved.historical_planes->require_actual_atlas_without_callbacks(*saved.actual_atlas);
        saved.historical_planes->require_actual_positioned_data_without_callbacks(saved.data);}
    const auto numeric=saved.scope.numeric_controls_without_callbacks();
    require(numeric[0]==saved.numeric[0]&&(numeric[1]&~std::uint32_t{0x3f})==(saved.numeric[1]&~std::uint32_t{0x3f}),
        "Embedded178 historical numeric control differs from earned CPU DATA");
    context->require_actual_label_child_scope_without_callbacks(saved.scope,caption->entry);
    saved.root.require_physical_current_without_callbacks();saved.scope.require_physical_current_without_callbacks();
}

detail::BnwNormalItemOccurrence::Guard BnwWorldViewControlTree::capture_normal_item_occurrence_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwNormalRootBatchHistory::Guard& actual_root,
    const BnwWorldViewControlRenderPublication& actual_publication,std::size_t index,
    const detail::BnwLabelActualPaintCallFrame& actual_frame,const detail::BnwLabelPaintTextLoan& actual_paint,
    const detail::BnwLabelPaintPositionedData& actual_data,const detail::BnwNormalButtonCaptionCall& actual_caption)const{
    using Item=detail::BnwNormalItemOccurrence;using History=detail::BnwNormalRootBatchHistory;
    const auto root=actual_root;const auto caption=actual_caption;
    root.require_physical_current_without_callbacks();require_normal_button_caption_call_current_without_callbacks(caption);
    const auto frame=actual_frame.capture_guard_without_callbacks();frame.require_current_without_callbacks();
    frame.require_actual_paint_item_without_callbacks(actual_publication,index);
    const auto publication=actual_publication.original_owner_without_callbacks();
    const auto tree=publication->tree_;const auto adjusted=publication->adjusted_frame_owner_;
    const auto paint=actual_paint.capture_guard_without_callbacks();paint.require_current_without_callbacks();
    const auto data=actual_data.capture_guard_without_callbacks();data.require_current_without_callbacks();
    data.require_same_paint_guard_without_callbacks(paint);
    const auto captured_root=root.impl_;const auto p=paint.impl_;const auto physical=data.impl_->state;
    const auto captured_caption=caption.impl_;const auto emission=publication->normal_caption_emission_;
    const auto rect=emission?emission->impl_->rect:std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan>{};
    require(captured_root&&captured_caption&&tree&&tree.get()==this&&p&&!paint.standalone_&&adjusted&&
        emission&&publication->renderer_,"Embedded178 Item capture lacks original actual owners");
    const auto renderer=publication->renderer_;const auto control=captured_caption->control;
    std::optional<graphics::detail::BnwNormalLabelDrawRecordBinding::Guard> captured_binding;
    const auto active_first=[&]{root.require_physical_current_without_callbacks();frame.require_current_without_callbacks();
        require_normal_button_caption_call_current_without_callbacks(caption);
        paint.require_current_without_callbacks();data.require_current_without_callbacks();
        data.require_same_paint_guard_without_callbacks(paint);
        frame.require_actual_paint_item_without_callbacks(*publication,index);
        frame.require_actual_control_without_callbacks(*tree,control.id_);frame.require_actual_renderer_without_callbacks(*renderer);
        require_normal_button_caption_publication_inputs_without_callbacks(*emission,tree,control,captured_caption->entry);
        captured_caption->root.require_same_root_without_callbacks(root);
        const auto& fresh=tree->core_->source->current_operation_loan();
        require(&fresh==&operation,"Embedded178 Item capture fresh Source differs before saved operand");
        fresh.require_origin_without_callbacks();
        require(captured_root->operation==&operation&&captured_root->serial==fresh.operation_serial()&&
            captured_root->renderer==renderer&&Item::Impl::same(captured_root->tree,tree)&&
            Item::Impl::same(paint.impl_,p)&&!paint.standalone_&&Item::Impl::same(p->tree.lock(),tree)&&
            p->actual_control_id==control.id_&&Item::Impl::same(p->run.impl_,captured_caption->run->impl_)&&
            Item::Impl::same(p->frame.impl_,frame.impl_)&&data.impl_&&Item::Impl::same(data.impl_->state,physical)&&
            Item::Impl::same(physical->paint.impl_,p)&&!physical->paint.standalone_&&
            Item::Impl::same(emission->impl_->caption.impl_,captured_caption)&&emission->impl_->index==index,
            "Embedded178 Item capture requires SAME ACTIVE Root/Caption/Frame/Paint/DATA/emission");
        require(bool(rect),"Embedded178 Item missing original FP32 geometry loan");
        const auto paint_rect=paint.actual_caption_fp32_rect_without_callbacks();
        const auto data_rect=data.actual_caption_fp32_rect_without_callbacks();
        require(Item::Impl::same(paint_rect,rect)&&Item::Impl::same(data_rect,rect)&&
            Item::Impl::same(emission->impl_->rect,rect),"Embedded178 ACTIVE Item lost SAME Paint/DATA/Emission loan");
        rect->require_same_caption_without_callbacks(caption);
        rect->require_same_entered_run_without_callbacks(p->run);
        if(captured_binding)data.require_same_normal_draw_records_without_callbacks(*captured_binding);
        publication->require_adjusted_frame_owner_without_callbacks(adjusted);
        root.require_physical_current_without_callbacks();frame.require_current_without_callbacks();};
    const auto active_full=[&]{active_first();root.require_source_owned(operation);active_first();
        require_normal_button_caption_call_source_owned(operation,caption);active_first();
        frame.require_source_owned(operation);active_first();paint.require_source_owned(operation);active_first();
        data.require_source_owned(operation);active_first();
        require_normal_button_caption_emission_publication_owned(*emission,*publication,operation);active_first();
        if(captured_binding){data.require_same_normal_draw_records_source_owned(operation,*captured_binding);active_first();}};
    active_first();active_full();active_first();
    const auto binding=data.capture_normal_draw_records_owned(operation);
    captured_binding.emplace(binding);active_first();active_full();active_first();
    data.require_same_normal_draw_records_source_owned(operation,binding);active_first();
    require(!captured_root->item_capturing&&captured_root->item_occurrences.size()<UINT64_MAX&&
        !data.impl_->normal_item_occurrence_issued&&data.impl_->normal_item_occurrence.expired(),
        "Embedded178 Item capture recursive/exhausted or DATA occurrence already issued");
    for(const auto& weak:captured_root->item_occurrences){const auto previous=weak.lock();
        require(!previous||!(previous->publication.get()==publication.get()&&previous->index==index&&
            previous->control==control.id_&&Item::Impl::same(previous->data.impl_,data.impl_)),
            "The exact embedded178 actual Root/DATA occurrence cannot be issued twice");}
    const auto serial=operation.operation_serial();const auto count=captured_root->item_occurrences.size();
    const auto numeric=physical->numeric;
    captured_root->item_capturing=true;
    struct End{std::shared_ptr<const History::Impl> root;~End(){root->item_capturing=false;}}end{captured_root};
    try{
        const auto scope=frame.capture_root_scope_health_owned(operation);active_first();active_full();active_first();
        const auto* context=std::get_if<History::Impl::RootScope::Root>(&captured_root->root_scope.value);
        require(context,"Embedded178 Item capture requires genuine Context Root health");
        context->require_actual_label_child_scope_without_callbacks(scope,captured_caption->entry);
        scope.require_actual_frame_owner_without_callbacks(adjusted);
        auto records=physical->records;active_first();active_full();active_first();
        auto glyphs=physical->glyphs;active_first();active_full();active_first();
        auto passes=physical->passes;active_first();active_full();active_first();
        const auto pending=[&]{active_first();scope.require_physical_current_without_callbacks();
            context->require_actual_label_child_scope_without_callbacks(scope,captured_caption->entry);
            require(captured_root->item_capturing&&captured_root->item_occurrences.size()==count&&
                !data.impl_->normal_item_occurrence_issued&&data.impl_->normal_item_occurrence.expired()&&
                operation.operation_serial()==serial&&physical->numeric==numeric,
                "Embedded178 Item capture allocator/FULL POST changed original occurrence");};
        pending();active_full();pending();
        captured_root->item_occurrences.reserve(count+1);pending();active_full();pending();
        auto owned=std::make_shared<Item::Impl>(root,scope,publication,adjusted,tree,renderer,&operation,serial,
            control.id_,count+1,index,paint,data,binding,p,caption,emission,physical,numeric,
            std::move(records),std::move(glyphs),std::move(passes));
        pending();active_full();pending();
        require(Item::Impl::same_records(owned->records,physical->records)&&
            Item::Impl::same_glyphs(owned->glyphs,physical->glyphs)&&Item::Impl::same_passes(owned->passes,physical->passes)&&
            Item::Impl::same(owned->embedded_paint_storage,p)&&owned->caption_history&&
            Item::Impl::same(owned->caption_history->impl_,captured_caption)&&Item::Impl::same(owned->caption_emission,emission),
            "Embedded178 Item original owner/CPU immutable allocation POST differs");
        // No callback or allocation between physical distinct-journal stores.
        captured_root->item_occurrences.emplace_back(owned);data.impl_->normal_item_occurrence=owned;
        data.impl_->normal_item_occurrence_issued=true;captured_root->item_capturing=false;owned->sealed=true;
        owned->first();owned->full(operation);owned->first();
        const Item::IssuerKey key;return Item::Guard(owned,key);
    }catch(...){const auto state=captured_root->gui.lock();if(state)state->failed.store(true);throw;}
}
