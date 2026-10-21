// Inside namespace civ5::ui AFTER the real ContextChildSlot10Frame definition.
// Exact selected A3C9C0+10=6F6C10. No generic4C or readiness substitution.
void BnwWorldViewControlTree::initialize_button_slot10_owned(
    ContextChildSlot10Frame& frame,std::uint64_t domain,std::uint64_t id){
    using Parent=detail::NotificationLayoutParentReceiver;
    using Baseline=detail::NotificationLayoutBaseline;
    using Button=detail::NotificationLayoutButtonState;
    std::shared_ptr<detail::BnwPositionedLabelRunLoan> pending_run;
    try{
        frame.full();
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey key){
            view.require_pure();
            if(Parent::node(view,key).state->actual_type_=="Button"){
                // SAME selected-domain View/rawScope supplies parsed resource
                // custody beforefirst6F6C10 numericalprefix. Unknown/missing
                // texture stays refusal; Up/Down existing order is unchanged.
                require(key.domain==0,"Ordinary Button actual selected View domain changed");
                view.initialize_button_texture_owned(key.id);view.require_pure();
            }
            view.require_pure();auto& state=*Parent::node(view,key).state;
            require(state.kind_==Baseline::Kind::button&&state.button_&&
                (state.actual_type_=="UpButton"||state.actual_type_=="DownButton"||state.actual_type_=="Button"),
                "Button6F6C10 genuine selected subtype state absent");
            auto& button=*state.button_;
            require(button.texture_binding_earned,
                "Button6F6C10 actual embedded image resource parser/binding has not returned");
            if(button.texture_extent_15c)
                require(button.texture_keeper&&(*button.texture_extent_15c)[0]&&(*button.texture_extent_15c)[1],
                    "Button6F6C10 resource extent lacks actual retained keeper");
            else require(!button.texture_keeper&&button.texture_key.empty(),
                "Button6F6C10 missing Texture binding cannot substitute nativeNULL");
            // 6F6C13..30: use earned actual resourceNULL, preserve other flags;
            // native implicit state-height increment comes before image stores.
            view.preflight_store_owned(key);state.record_owned(Baseline::Mutation::receiver_prefix);
            if(state.actual_type_=="Button")
                detail::BnwNormalButtonSubobject118Backing::initialize_6f6c13_prefix_owned(button,state);
            else {if(!button.texture_extent_15c)state.base_.flags_40|=0x10000000U;
                if(!(state.base_.flags_40&0x20000000U))button.increment_110_114[1]=state.base_.size_30_34[1];}
            view.record_store_owned(key,Parent::Store::Kind::button);
            // 6F6C36..74: exact independently stored destination/source sizes
            // and pixel origin. No manufactured dimensions or normalized UV.
            view.preflight_store_owned(key);state.record_owned(Baseline::Mutation::receiver_prefix);
            if(state.actual_type_=="Button")
                detail::BnwNormalButtonSubobject118Backing::store_geometry6f6c42_owned(button,state);
            else {button.image_13c_140=state.base_.size_30_34;
                button.image_14c_150=button.authored_108_10c;
                button.image_144_148=state.base_.size_30_34;}
            view.record_store_owned(key,Parent::Store::Kind::button);
            //Fresh qualified actual9F3570+0=4535B0 and real E9->5C7B50.
            //Dispatch only from SAME executed constructor118 backing.
            if(state.actual_type_=="Button")
                detail::BnwNormalButtonSubobject118Backing::execute_slot0_4535b0_owned(button);
            else Button::coordinates_owned(button);view.require_pure();
            // 6F6C80: absence is a captured typed ctor/parser observation.
            // Unknown/nonNULL is refused AFTER preserving real prefix history.
            require(button.child_178_observation&&!state.actual_text_command_,
                "Button6F6C10 actual178 observation/text command unavailable");
            if(*button.child_178_observation==Button::Child178Observation::factory_child){
                using Child=detail::NotificationLayoutEmbeddedLabelState;
                require(button.label_178_&&button.label_178_factory_return_&&!button.pending_label_178_,
                    "Button6F6C80 actual published child missing");
                auto loan=std::shared_ptr<Child::InitializationLoan>(new Child::InitializationLoan(view,key));
                button.label_178_->initialize_from_parent_owned(loan);view.require_pure();
                pending_run=button.label_178_->make_positioned_run_pending_owned(loan);view.require_pure();
            }else require(!button.label_178_&&!button.label_178_factory_return_,
                "Button6F6C80 constructorNULL contradicted by live child");
            if(state.base_.flags_40&0x80000000U){
                view.preflight_store_owned(key);state.record_owned(Baseline::Mutation::receiver_prefix);
                if(state.actual_type_=="Button")
                    detail::BnwNormalButtonSubobject118Backing::restore_origin6f6cd1_owned(button,state);
                else button.image_14c_150=button.authored_108_10c;
                view.record_store_owned(key,Parent::Store::Kind::button);
                Button::coordinates_owned(button);view.require_pure();
            }
        });
        frame.first();frame.pure();frame.full();
        // Native6F6CE8 tail71A000 FIRST calls5761E0, then vslotC4 with
        // the actual current state104. Common dispatcher owns5761+children.
        initialize_control_5761_child_owned(frame,domain,id);
        frame.first();frame.pure();
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey key){
            view.require_pure();auto& state=*Parent::node(view,key).state;
            require(state.kind_==Baseline::Kind::button&&state.button_&&
                (state.actual_type_=="UpButton"||state.actual_type_=="DownButton"||state.actual_type_=="Button"),
                "Button71A000 actual C4 receiver changed");
            view.preflight_store_owned(key);state.record_owned(Baseline::Mutation::receiver_prefix);
            const auto actual_state=state.button_->state_104;
            Button::visual_state_owned(state,actual_state);
            view.record_store_owned(key,Parent::Store::Kind::button);view.require_pure();
        });
        frame.first();frame.pure();frame.full();
        if(pending_run){
            const auto data=pending_run->impl_;const auto core=frame.domains.at(domain).core;
            require(frame.receiver_destruction_return.has_value(),"Label Run actual final C4 DTOR return missing");
            const auto stamp=*frame.receiver_destruction_return; //POD VALUE before further Source callbacks.
            const auto check_return=[&]{frame.pure();require(stamp.domain==domain&&stamp.id==id&&stamp.source_serial==frame.serial&&
                stamp.topology==frame.domains.at(domain).topology&&stamp.effects==frame.domains.at(domain).effects&&
                stamp.actual_return_sequence==frame.receiver_return_sequence&&
                frame.receiver_destruction_return&&frame.receiver_destruction_return->actual_return_sequence==stamp.actual_return_sequence&&
                stamp.actual_return_sequence&&data->node_id==id,
                "Label Run lacks SAME final71A-C4 post-DTOR receiver return");
            };
            check_return();
            const auto actual_core=data->core.lock();
            require(actual_core&&actual_core.get()==core.get()&&!actual_core.owner_before(core)&&
                !core.owner_before(actual_core),"Label Run final canonical Core changed");
            data->validate_target_without_callbacks(*frame.operation);frame.pure();
            frame.full();data->validate_target_without_callbacks(*frame.operation);check_return();
            const auto child=data->child.lock();require(child&&!child->impl_->completed_run&&
                !data->published,"Label Run actual child already published/retired");
            //The physical pointer below is pending custody only: no typed
            //PublishedButtonReturn exists, so all Guard FIRST calls refuse.
            child->impl_->completed_run=pending_run;
            frame.full();data->validate_target_without_callbacks(*frame.operation);check_return();
            require(child->impl_->completed_run.get()==pending_run.get()&&
                !child->impl_->completed_run.owner_before(pending_run)&&
                !pending_run.owner_before(child->impl_->completed_run),"Label Run post-store custody changed");
            //All mandatory POST effects are complete. This final plain DATA
            //seal has no callback/allocation after it; only pure validation.
            data->published=detail::BnwPositionedLabelRunLoan::Impl::PublishedButtonReturn{
                stamp.source_serial,stamp.actual_return_sequence,stamp.topology,stamp.effects};
            data->first(*frame.operation);frame.pure();
        }
        //Receipt DATA is now post-common/71A return and actual receiver DTOR.
        //No591020 paint callframe, raster upload or native completion is granted.
    }catch(...){frame.poison();throw;}
}
