// Inside civ5::ui after complete Core, canonical Button state and opaque+10 return.
BnwWorldViewControlTree::ButtonVisualOwned BnwWorldViewControlTree::button_visual_owned(
    const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=core_;saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
    const auto id=saved->checked(control);
    const auto pure=[&]{saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Button visual SAME Source changed");
        saved->require_layout_resources_owned();saved->layout_origin();};
    try{
        pure();const auto& n=Core::node(saved->nodes,id);
        require(n.layout_baseline&&n.layout_receiver_occurrence&&n.layout_baseline->parsed_source_serial_&&
            n.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button&&
            n.layout_baseline->actual_type_=="Button"&&n.layout_baseline->button_&&n.button_slot10_return,
            "Button visual requires actual canonical constructor/parser/subtype+10 return");
        require(bool(n.button_slot10_return->impl_),"Button visual subtype+10 storage absent");
        const auto& done=*n.button_slot10_return->impl_;const auto parent=done.parent.lock();
        require(parent&&parent==saved&&!parent.owner_before(saved)&&!saved.owner_before(parent)&&
            done.source_serial==operation.operation_serial()&&done.lifetime&&done.lifetime->creator==saved->creator&&
            !done.lifetime->failed.load()&&done.lifetime->destroyed&&done.lifetime->destruction_passed,
            "Button visual requires SAME actual post-DTOR subtype occurrence");
        const auto member=std::find_if(done.members.begin(),done.members.end(),[&](const auto& value){return value.id==id;});
        require(member!=done.members.end()&&*n.layout_baseline->parsed_source_serial_==member->parser_serial&&
            member->initialized_sequence&&n.layout_baseline->sequence_>=member->initialized_sequence,
            "Button visual subtype return does not contain this actual receiver");
        const auto b=n.layout_baseline->button_;
        require(b->texture_binding_earned&&b->child_178_observation,
            "Button visual real resource/Label178 outcome unearned");
        using Run=detail::BnwPositionedLabelRunLoan;
        std::shared_ptr<const Run> run;
        std::optional<Run::Guard> label_guard;
        if(*b->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null){
            //Preserve the earned original NULL178 path and text-command restriction.
            require(!n.layout_baseline->actual_text_command_,
                "Button visual constructorNULL178 has an actual text command");
        }else{
            require(*b->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::factory_child,
                "Button visual actual178 observation unknown");
            //Only the actual checked Control's genuine post-DTOR published Run.
            //This callback-capable getter validates SAME child/Factory/ticket,
            //CC/170/Realm/10..34 and complete Source POST before it returns.
            run=positioned_label_run_owned(control,operation);
            require(bool(run),"Button visual factory178 has no actual published Run");
            label_guard.emplace(run->capture_guard_without_callbacks(operation));
            label_guard->require_source_owned(operation);label_guard->require_current_without_callbacks();
        }
        const auto checked_visual=[&]{
            if(label_guard)label_guard->require_current_without_callbacks();
            pure();
            if(label_guard){
                label_guard->require_current_without_callbacks();
                label_guard->require_source_owned(operation);label_guard->require_current_without_callbacks();
                //Read actual current initialized geometry DATA, not an init/paint grant.
                (void)label_guard->actual_geometry_bits_without_callbacks();
            }
            const auto& current=Core::node(saved->nodes,id);
            require(saved->checked(control)==id&&current.layout_baseline&&
                current.layout_baseline->button_.get()==b.get()&&
                !current.layout_baseline->button_.owner_before(b)&&!b.owner_before(current.layout_baseline->button_)&&
                (!run||(b->label_178_&&b->label_178_->impl_->completed_run.get()==run.get()&&
                    !b->label_178_->impl_->completed_run.owner_before(run)&&
                    !run.owner_before(b->label_178_->impl_->completed_run))),
                "Button visual actual Control/field owner/published Run changed");
            if(label_guard)label_guard->require_current_without_callbacks();
        };
        checked_visual();
        if(b->texture_extent_15c){
            require(b->texture_keeper&&(*b->texture_extent_15c)[0]&&(*b->texture_extent_15c)[1],
                "Button visual actual texture extent/keeper absent");
            detail::NotificationLayoutTextureKeeper::require_canonical(b->texture_keeper);
        }else require(!b->texture_keeper&&b->texture_key.empty(),"Button visual Texture missing cannot be nativeNULL");
        ButtonVisualOwned result{b->color_12c_138,b->image_13c_140,b->image_144_148,b->image_14c_150,
            b->uv_168_16c,b->uv_170_174,b->texture_extent_15c,b->texture_keeper,b->state_104,true};
        //Retained DATA does not renew Run/child/Source authority. The guard
        //remains live through all resource/result work and the final FULL POST.
        checked_visual();return result;
    }catch(...){saved->failed.store(true);throw;}
}
