// Inside civ5::ui AFTER RuntimeDimensions registration and actual ParentCore.
void BnwWorldViewControlTree::run_layout_resize_98_owned(
    const detail::NotificationLayoutResizeRegistration& receipt,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
    const auto saved=receipt.core.lock();
    require(saved&&receipt.creator==std::this_thread::get_id()&&!receipt.failed.load(),
        "Resize98 real registered receiver retired/foreign/failed");
    std::shared_ptr<View::Binding> binding;
    const auto pure=[&](bool busy=false){
        saved->layout_origin(busy);operation.require_origin_without_callbacks();
        const auto manager=receipt.manager.lock();const auto occurrence=receipt.occurrence.lock();
        require(manager&&occurrence&&saved->context_gui_port&&
            manager.get()==saved->context_gui_port.get()&&!manager.owner_before(saved->context_gui_port)&&
            !saved->context_gui_port.owner_before(manager)&&occurrence->id==receipt.id&&
            occurrence->constructor_serial==receipt.constructor_serial&&(receipt.registered_flags&0x300000u)&&
            &saved->source->current_operation_loan()==&operation,
            "Resize98 SAME registration/manager/source occurrence changed");
        // Membership in the actual callback list, not a separately constructed
        // receipt or numerical flags, authenticates this retained registration.
        require(std::any_of(manager->impl_->resize_registrations.begin(),manager->impl_->resize_registrations.end(),
            [&](const auto& x){return x&&x.get()==&receipt;}),"Resize98 registration is outside actual GUI callback list");
        const detail::NotificationLayoutBaseline* state=nullptr;
        std::shared_ptr<const Core::LayoutReceiverOccurrence> current;
        if(receipt.id==0){require(saved->context_construction_return&&saved->context_layout_baseline,
            "Resize98 Context genuine construction retired");current=saved->context_receiver_occurrence;state=&*saved->context_layout_baseline;
        }else{const auto& n=Core::node(saved->nodes,receipt.id);
            require(n.alive&&n.layout_baseline,"Resize98 canonical control retired");
            current=n.layout_receiver_occurrence;state=&*n.layout_baseline;}
        require(current&&current.get()==occurrence.get()&&!current.owner_before(occurrence)&&!occurrence.owner_before(current)&&
            state->parsed_source_serial_&&*state->parsed_source_serial_==receipt.constructor_serial,
            "Resize98 actual constructor/parser allocation changed");
        const bool context=receipt.id==0&&state->kind_==detail::NotificationLayoutBaseline::Kind::context&&state->actual_type_=="Context";
        const bool container=receipt.id!=0&&state->kind_==detail::NotificationLayoutBaseline::Kind::container&&state->actual_type_=="Container";
        const bool box=receipt.id!=0&&state->kind_==detail::NotificationLayoutBaseline::Kind::box&&
            state->actual_type_=="Box"&&state->box_&&!state->box_->failed&&state->box_->common_children_return_&&
            state->box_->resize_registration_.get()==&receipt;
        if(box)require_box_native_common_return_owned(saved,receipt.id,operation,busy);
        require(context||container||box,"Resize98 subtype lacks genuine670F10 constructor/vtable mapping; generic base refusal");
        operation.require_origin_without_callbacks();saved->layout_origin(busy);
    };
    try{
        pure();saved->layout_current_owned();pure();operation.require_current();pure();
        saved->busy=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{saved->busy};pure(true);
        // Genuine668750 operands are derived inside the actual registered
        // event0 receiver. Only Full axes read canonical signed644/648.
        std::array<float,2> next;
        {const auto manager=receipt.manager.lock();require(bool(manager),"Resize98 GUI retired before operands");
            const auto pixels=manager->dimensions_without_callbacks(operation);
            const auto& current=receipt.id==0?*saved->context_layout_baseline:*Core::node(saved->nodes,receipt.id).layout_baseline;
            next=current.base_.size_30_34;
            if(current.base_.flags_40&0x100000u)next[0]=static_cast<float>(pixels[0]);
            if(current.base_.flags_40&0x200000u)next[1]=static_cast<float>(pixels[1]);}
        pure(true);for(float x:next)require(std::isfinite(x),"Resize98 native size outside finite supported profile");
        {
            auto& state=receipt.id==0?*saved->context_layout_baseline:*saved->nodes.at(receipt.id-1).layout_baseline;
            require(saved->effect_revision<=UINT64_MAX-4&&saved->layout_sequence<=UINT64_MAX-4&&
                state.sequence_<=UINT64_MAX-4,"Resize98 physical prefix history exhausted");
            for(float x:state.base_.margin_70_74)require(std::isfinite(x),"Resize98 actual margin unsupported");
            state.history_.reserve(state.history_.size()+4); // Admission before first physical store.
            //Box becomes reachable only with its actual opaque return. Fence
            //allocator callbacks before reading the borrowed physical state.
            pure(true);saved->layout_current_owned();pure(true);operation.require_current();pure(true);
            // Exact670F10 order, even same-value setters. These four real stores
            // remain journalled if the mandatory4A7F10 graph fails afterward.
            state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
            state.base_.size_30_34[0]=next[0];++saved->effect_revision;++saved->layout_sequence;
            state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
            state.base_.size_30_34[1]=next[1];++saved->effect_revision;++saved->layout_sequence;
            volatile float extent_x=std::fabs(state.base_.margin_70_74[0])+state.base_.size_30_34[0];
            require(std::isfinite(extent_x),"Resize98 FABS70/FADD30/FSTP20 result unsupported");
            state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
            state.base_.extent_20_24[0]=extent_x;++saved->effect_revision;++saved->layout_sequence;
            volatile float extent_y=std::fabs(state.base_.margin_70_74[1])+state.base_.size_30_34[1];
            require(std::isfinite(extent_y),"Resize98 FABS74/FADD34/FSTP24 result unsupported");
            state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
            state.base_.extent_20_24[1]=extent_y;++saved->effect_revision;++saved->layout_sequence;
        }
        pure(true);
        // Only AFTER physical30/34/20/24 does native670F3F enter4A7F10.
        // Reuse the original-order lazy canonical parent/child receiver owner.
        binding=std::make_shared<View::Binding>(saved,operation);
        std::vector<Parent::Store> stores;std::vector<Parent::FourCReturn> returns;
        {
            View view(binding,binding->captured);
            Parent::reprocess_anchoring(view,Parent::NodeKey{0,receipt.id});
            view.require_pure();Parent::slot94(view,Parent::NodeKey{0,receipt.id}); // Distinct670F4E suffix call.
            view.require_full();view.require_pure();stores=view.stores_;returns=view.four_c_returns_;view.require_pure();
        }
        binding->pure(false);require(binding->destruction_passed,"Resize98 actual4A7F10/94 scope destruction incomplete");
        // Derived quote/cache mirrors describe the real stored canonical state;
        // they do not grant subtype+10 or global Context initialization.
        const auto mirror=[&](const std::shared_ptr<Core>& core,std::uint64_t id,std::uint64_t topology,std::uint64_t effects){
            if(id==0)return;(void)Core::node(core->nodes,id);auto& node=core->nodes.at(id-1);
            const auto& state=*node.layout_baseline;auto value=node.layout.value_or(LayoutState{});
            value.native_offset=state.base_.offset_28_2c;value.calculated_size=state.base_.size_30_34;
            value.native_extent=state.base_.extent_20_24;value.native_bounds=state.base_.bounds_10_1c;
            // This setter/reflow never executes a Stack calculate command;
            // do not promote an older calculator cache to the new epoch.
            value.calculate_returned=false;
            value.source_serial=binding->serial;value.topology_revision=topology;value.effect_revision=effects;
            value.command_sequence=core->layout_sequence;node.layout=std::move(value);
        };
        mirror(saved,receipt.id,binding->domains.front().topology,binding->domains.front().effects);
        for(const auto& store:stores){const auto& d=binding->domains.at(store.key.domain);
            mirror(d.core,store.key.id,d.topology,d.effects);
            if(store.key.id&&store.kind==Parent::Store::Kind::stack_anchor){auto& n=d.core->nodes.at(store.key.id-1);
                require(n.layout&&n.layout_baseline&&n.layout_baseline->stack_,"Resize98 actual Stack anchor mirror missing");
                n.layout->anchor_origin={n.layout_baseline->stack_->field_90,n.layout_baseline->stack_->field_94};
                n.layout->anchor_prepass_stored=true;n.layout->anchoring_returned=true;}
        }
        for(const auto& returned:returns){const auto& d=binding->domains.at(returned.key.domain);
            Core::LayoutFourCReturn stamp{returned.actual_offset,returned.baseline_sequence,binding->serial,d.topology,returned.offset_store_sequence};
            if(returned.key.id==0)d.core->context_layout_four_c_return=stamp;
            else d.core->nodes.at(returned.key.id-1).layout_four_c_return=stamp;
        }
        binding->pure(false);binding->finished=true;binding.reset();
        // Actual binding/scope destruction precedes the final Source POST.
        pure(true);operation.require_current();pure(true);
    }catch(...){if(binding)binding->poison();saved->failed.store(true);receipt.failed.store(true);throw;}
}
