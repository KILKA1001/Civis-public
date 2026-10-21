// Inside civ5::ui after Caption5 and original Material definitions. This
// additive embedded CURRENT preparation executes real records/descriptor/
// arena/scalar/append actors. It refuses at the still-unissued actual Label
// Common56 tail, before any public CPU-child-return or whole-Root seal.
void BnwWorldViewControlTree::require_normal_caption_material_current_without_callbacks(
    const detail::BnwNormalLabelMaterialLoan::Impl& material){
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(material.creator==std::this_thread::get_id()&&material.alive.load()&&material.caption&&
        material.embedded_run&&!material.run&&!material.sealed&&!material.cpu_child_returned,
        "Embedded Caption Material lacks original distinct CURRENT owners or earned future return");
    const auto caption=*material.caption;
    require_normal_button_caption_call_current_without_callbacks(caption);
    material.frame.require_current_without_callbacks();
    material.root.require_physical_current_without_callbacks();material.item.require_physical_current_without_callbacks();
    material.item.require_same_root_without_callbacks(material.root);
    const auto root=material.root.impl_;const auto item=material.item.impl_;const auto call=caption.impl_;
    const auto prefix=root->prefix;const auto tree=root->tree;
    require(material.tree&&same(material.tree,tree)&&same(call->root.impl_,root)&&item&&
        same(item->root.impl_,root)&&item->caption_history&&same(item->caption_history->impl_,call)&&
        same(material.embedded_run,call->run)&&same(material.paint.impl_,item->embedded_paint_storage)&&
        !material.paint.standalone_&&!item->paint_storage&&
        material.publication==item->publication.get()&&material.index==item->index&&
        material.operation==root->operation&&material.serial==root->serial&&
        same(material.data.impl_,item->data.impl_)&&
        same(item->embedded_paint_storage->frame.impl_,material.frame.impl_),
        "Embedded Caption Material not SAME actual Root/Item/Run/Frame/Paint/DATA/Publication occurrence");
    material.frame.require_actual_paint_item_without_callbacks(*item->publication,item->index);
    material.frame.require_actual_control_without_callbacks(*tree,item->control);
    material.frame.require_actual_renderer_without_callbacks(*root->renderer);
    material.data.require_current_without_callbacks();material.paint.require_current_without_callbacks();
    material.data.require_same_paint_guard_without_callbacks(material.paint);
    material.data.require_same_normal_draw_records_without_callbacks(material.binding);
    require_normal_payload_pending_caption_without_callbacks(*prefix,material.data,material.paint,caption,material.item);
    material.matrix_physical_first();
    tree->core_->layout_origin(true);
    const auto& fresh=tree->core_->source->current_operation_loan();fresh.require_origin_without_callbacks();
    require(&fresh==material.operation&&fresh.operation_serial()==material.serial,
        "Embedded Caption Material canonical current Source changed before raw operand");
    material.data.require_same_normal_draw_records_without_callbacks(material.binding);
    material.frame.require_current_without_callbacks();
    require_normal_button_caption_call_current_without_callbacks(caption);
}

std::unique_ptr<detail::BnwNormalLabelMaterialLoan>
BnwWorldViewControlTree::normal_label_material_for_actual_item_owned(
    std::shared_ptr<const BnwWorldViewControlTree> supplied_tree,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlRenderPublication& actual_publication,std::size_t index,
    const detail::BnwLabelActualPaintCallFrame& actual_frame,
    const std::shared_ptr<const detail::BnwPositionedLabelRunLoan>& supplied_run,
    const detail::BnwLabelPaintTextLoan& actual_paint,const detail::BnwLabelPaintPositionedData& actual_data,
    const detail::BnwWindowedRendererMatrixDataLoan& actual_matrix,
    const detail::BnwNormalButtonCaptionCall& supplied_caption)const{
    using Loan=detail::BnwNormalLabelMaterialLoan;
    const auto tree=std::move(supplied_tree);const auto owner=core_;const auto run=supplied_run;
    const auto caption=supplied_caption;
    const auto frame_control=actual_frame.impl_;const auto paint_control=actual_paint.impl_;
    const auto standalone=actual_paint.standalone_;const auto data_control=actual_data.impl_;
    const auto matrix_control=actual_matrix.impl_;
    require_normal_button_caption_call_current_without_callbacks(caption);
    const auto root=caption.impl_->root;const auto actual_root=root.impl_;
    const auto actual_item=data_control?data_control->normal_item_occurrence.lock():nullptr;
    require(actual_item&&data_control->normal_item_occurrence_issued,
        "Embedded Caption Material requires already earned SAME original Item, no implicit capture");
    const detail::BnwNormalItemOccurrence::IssuerKey item_key;
    const detail::BnwNormalItemOccurrence::Guard item(actual_item,item_key);
    const auto frame=detail::BnwLabelActualPaintCallFrame::Guard(frame_control);
    const auto paint=detail::BnwLabelPaintTextLoan::Guard(paint_control);
    const auto data=detail::BnwLabelPaintPositionedData::Guard(data_control);
    root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
    frame.require_current_without_callbacks();paint.require_current_without_callbacks();data.require_current_without_callbacks();
    require(tree&&tree.get()==this&&tree->core_==owner&&run&&!standalone&&paint_control&&
        Loan::Impl::same(run,caption.impl_->run)&&actual_item->caption_history&&
        Loan::Impl::same(actual_item->caption_history->impl_,caption.impl_)&&actual_item->index==index,
        "Embedded Caption Material inputs not genuine SAME actual Caption/Run/Paint/Item");
    item.require_same_root_without_callbacks(root);item.require_actual_positioned_data_without_callbacks(data);
    item.require_actual_publication_without_callbacks(actual_publication);
    const auto publication=actual_item->publication;
    frame.require_actual_paint_item_without_callbacks(*publication,index);
    frame.require_actual_control_without_callbacks(*tree,actual_item->control);
    require_normal_payload_pending_caption_without_callbacks(*actual_root->prefix,data,paint,caption,item);
    const auto binding=data.capture_normal_draw_records_owned(operation);
    require_normal_payload_pending_caption_without_callbacks(*actual_root->prefix,data,paint,caption,item);
    data.require_same_normal_draw_records_source_owned(operation,binding);
    require_normal_payload_pending_caption_without_callbacks(*actual_root->prefix,data,paint,caption,item);
    const auto genuine_planes=item.historical_font_planes_without_callbacks();
    genuine_planes.require_physical_current_without_callbacks();
    root.require_source_owned(operation);item.require_source_owned(operation);
    require_normal_payload_pending_caption_without_callbacks(*actual_root->prefix,data,paint,caption,item);
    owner->layout_origin(true);const auto& fresh=owner->source->current_operation_loan();
    fresh.require_origin_without_callbacks();
    require(&fresh==&operation&&actual_root->operation==&operation&&actual_root->serial==fresh.operation_serial(),
        "Embedded Caption Material current Source differs before saved operand");
    const auto serial=actual_root->serial;
    // ONE original heap Material control exists before any queue entry;
    // pending state is never a public Material facade or a returned child.
    auto prepared_owner=[&]{try{return std::make_shared<Loan::Impl>(tree,publication.get(),index,
        &operation,serial,frame,run,paint,data,matrix_control,binding,root,item,caption);}
        catch(...){owner->failed.store(true);throw;}}();
    auto& prepared=*prepared_owner;prepared.canonical_self=prepared_owner;
    try{
        prepared.origin();prepared.full(operation);prepared.origin();
        prepared.capture_runtime_owned();prepared.full(operation);prepared.runtime();
        // Authentic same DATA/Binding positive170 and ascending0/1/2 records.
        // No synthetic pass or collapsed three-pass payload is admitted.
        prepared.capture_records_owned();prepared.full(operation);prepared.record_current();
        prepared.require_visible_child_owned();prepared.record_current();
        prepared.attach_shared_descriptor_owned();
        prepared.record_current();prepared.full(operation);prepared.record_current();
        auto& queue=prepared.batch->batch->payload_queue;
        require(queue.alive.load()&&!queue.allocating&&!queue.appending&&
            queue.entry_count_c==queue.entries.size()&&queue.append_sequence==queue.entry_count_c,
            "Embedded Caption entry sequence starts on another/reentered actual Queue");
        prepared.first_entry_slot=queue.entry_count_c;
        for(int p=2;p>=0;--p){
            const auto pass=static_cast<unsigned>(p);
            prepared.record_current();if(!prepared.pass_present[pass])continue;
            require(!prepared.payload,"Embedded Caption preceding current allocation not retired");
            prepared.payload=allocate_normal_font_payload_control_owned(prepared.batch,prepared.data,prepared.paint,operation,caption);
            prepared.record_current();prepared.full(operation);prepared.record_current();
            write_normal_font_scalar_control_owned(prepared.payload);
            prepared.record_current();prepared.full(operation);prepared.record_current();
            const auto value=prepared.actual_scalars_without_callbacks();
            if(prepared.scalars)require(*prepared.scalars==value,"Embedded Caption actual per-pass scalar input changed");
            else prepared.scalars=value;
            prepared.full(operation);prepared.record_current();
            require(*prepared.scalars==prepared.actual_scalars_without_callbacks(),
                "Embedded Caption scalar DATA changed after FULL");
            prepared.full(operation);prepared.record_current();
            auto descriptor=prepared.finalize_current_pass_owned(pass);
            prepared.record_current();prepared.full(operation);prepared.record_current();
            prepared.append_completed_pass_owned(pass,descriptor);
            prepared.record_current();prepared.full(operation);prepared.record_current();
        }
        require(!prepared.payload&&prepared.scalars&&
            *prepared.scalars==prepared.completed_scalars_without_callbacks(),
            "Embedded Caption historical entry sequence/scalars missing after real per-pass return");
        prepared.completed_selector=1;prepared.full(operation);prepared.record_current();
        // Label540A08/540B3E/540B54 suffix is NOT the outer Button Common
        // return. A zero children78 test does not waive its mandatory actual
        // manager prefix02DC1B1C/+6EC/+6E0/conditional55F690 effects.
        const auto embedded_common56=capture_normal_embedded_label_common56_owned(operation,prepared_owner);
        require_normal_embedded_label_common56_current_without_callbacks(embedded_common56);
        require_normal_embedded_label_common56_source_owned(operation,embedded_common56);
        require_normal_embedded_label_common56_current_without_callbacks(embedded_common56);
        throw runtime::ScriptError("Embedded Label540B54 receiver entered after real pass/spriteNULL; required current Common56 manager/arg1/prefix/tail supplier unissued");
    }catch(...){
        prepared.alive.store(false);
        if(prepared.payload){prepared.payload->alive.store(false);prepared.payload->payload->alive.store(false);}
        if(prepared.batch){prepared.batch->alive.store(false);prepared.batch->batch->alive.store(false);}
        owner->failed.store(true);throw;
    }
}
