// Complete ContextChildSlot10Frame/Parent first. This is actual Grid70F8E0,
// not a detached geometry DTO and not a Paint/Frame or generic subtype issuer.
void detail::NotificationLayoutGridState::initialize_geometry_70f8_owned(NotificationLayoutBaseline& owner){
    require(owner.kind_==NotificationLayoutBaseline::Kind::grid&&owner.actual_type_=="Grid"&&
        owner.grid_.get()==this&&parser_returned&&common_return_&&owner.base_.common_return_owned==common_return_&&
        !common_layer_&&!failed&&initialization_entered&&!initialization_returned&&
        (owner.base_.flags_40&1u)&&!(owner.base_.flags_40&0x400000u),
        "Grid70F8E0 requires SAME genuine5761 return; AutoSize563190 remains separate required suffix");
    require(texture_binding_earned,"Grid initializer cannot consume unresolved+278 resource");
    require((_mm_getcsr()&0xe040u)==0,"Grid selected SSE normalization mode unsupported");
    const auto add=[](float a,float b){volatile float result=a+b;
        require(std::isfinite(result),"Grid ADDSS result unsupported");return result;};
    const auto sub=[](float a,float b){volatile float result=a-b;
        require(std::isfinite(result),"Grid SUBSS result unsupported");return result;};
    const auto get=[&](std::size_t block,std::size_t i){return std::bit_cast<float>(words_c8_274[block][i]);};
    const auto set=[&](std::size_t block,std::size_t i,float value){words_c8_274[block][i]=std::bit_cast<std::uint32_t>(value);};
    for(std::size_t n=0;n<18;++n){
        const auto start=add(authored_b8_bc[n%2],get(2,n));set(2,n,start);set(3,n,add(start,get(1,n)));
    }
    const auto s=owner.base_.size_30_34;const auto k=kerning_a0_ac;
    //Raw70FD19..70FE35, retained SUB/ADD FP32 order; no algebraic regrouping.
    set(1,2,add(sub(add(sub(s[0],get(1,12)),k[0]),get(1,10)),k[1]));
    set(1,7,add(sub(add(sub(s[1],get(1,11)),k[2]),get(1,15)),k[3]));
    set(1,0,add(add(sub(sub(s[0],get(1,6)),get(1,8)),k[1]),k[0]));
    set(1,1,add(add(sub(sub(s[1],get(1,3)),get(1,5)),k[3]),k[2]));
    set(1,9,add(sub(add(sub(s[1],get(1,13)),k[2]),get(1,17)),k[3]));
    set(1,4,add(sub(add(sub(s[0],get(1,16)),k[0]),get(1,14)),k[1]));
    set(0,10,zero);set(0,11,zero);set(0,2,sub(get(1,10),k[0]));set(0,3,zero);
    set(0,12,sub(s[0],get(1,12)));set(0,13,zero);set(0,6,zero);set(0,7,sub(get(1,11),k[2]));
    set(0,0,sub(get(1,6),k[0]));set(0,1,sub(get(1,3),k[2]));
    set(0,8,sub(s[0],get(1,8)));set(0,9,sub(get(1,13),k[2]));
    set(0,14,zero);set(0,15,sub(s[1],get(1,15)));
    set(0,4,sub(get(1,14),k[0]));set(0,5,sub(s[1],get(1,5)));
    set(0,16,sub(s[0],get(1,16)));set(0,17,sub(s[1],get(1,17)));
    const auto rgba=color_90_9c;color_90_9c=rgba; //actual6BF870 four scalar stores, not guessed Color callback
    if(owner.base_.flags_40&0x800000u)current_c0_c4=authored_b8_bc;
    if(field_278==Field278::constructor_null){
        require(texture_key.empty()&&!texture_keeper&&!texture_extent_278,"Grid constructor NULL texture substituted");
        return; //genuine4B0E9A branch, retains native zero UV blocks
    }
    require(texture_keeper&&texture_extent_278&&*texture_extent_278==
        std::array<std::uint32_t,2>{texture_keeper->native_114_118_[1],texture_keeper->native_114_118_[0]},
        "Grid4B0E90 actual118/114 resource dimensions unavailable");
    const auto dims=*texture_extent_278;require(dims[0]&&dims[1],"Grid UV selected physical extent zero");
    volatile float invx=one/static_cast<float>(dims[0]),invy=one/static_cast<float>(dims[1]);
    for(std::size_t n=0;n<18;++n){
        const auto inverse=n%2?invy:invx;
        const auto start=add(get(2,n),current_c0_c4[n%2]);volatile float uv=start*inverse;set(4,n,uv);
        const auto negative=std::bit_cast<float>(std::bit_cast<std::uint32_t>(get(2,n))^0x80000000u);
        const auto span=add(negative,get(3,n));volatile float extent=span*inverse;set(5,n,extent);
    }
}
void BnwWorldViewControlTree::initialize_grid_slot10_owned(
    ContextChildSlot10Frame& frame,std::uint64_t domain,std::uint64_t id){
    using Baseline=detail::NotificationLayoutBaseline;using Grid=detail::NotificationLayoutGridState;
    using Parent=detail::NotificationLayoutParentReceiver;const ContextChildSlot10Frame::Key key{domain,id};
    try{
        frame.full();const auto grid=frame.baseline_owned(key).grid_;
        require(id&&grid&&frame.baseline_owned(key).actual_type_=="Grid"&&grid->parser_returned&&
            !grid->failed&&!grid->initialization_entered&&!grid->initialization_returned,
            "Grid+10 genuine constructor/parser occurrence missing or initialization repeated");
        //New cold/entered checker uses the actual Frame/View Core, never a
        //retired lexical Runtime or shadow Context argument. No allocation.
        const auto check_common=[&](const std::shared_ptr<Core>& core,Baseline& owner){
            require(core&&owner.kind_==Baseline::Kind::grid&&owner.actual_type_=="Grid"&&
                owner.grid_==grid&&grid->common_return_&&owner.base_.common_return_owned==grid->common_return_&&
                owner.base_.receiver_owned&&owner.base_.children_78&&!grid->common_layer_&&
                owner.parsed_source_serial_&&*owner.parsed_source_serial_==frame.serial&&
                grid->common_layer_returns_.size()==owner.parser_layers_.size()&&
                !grid->common_layer_returns_.empty()&&grid->common_layer_returns_.back().first==0,
                "Grid init requires actual complete selected layered common return");
            const auto& node=Core::node(core->nodes,id);const auto xml=node.full_xml_occurrence;
            require(node.alive&&node.layout_baseline&& &*node.layout_baseline==&owner&&xml&&!xml->dynamic&&
                xml->value.children.empty()&&xml->source.lock()==core->source&&node.layout_receiver_occurrence,
                "Grid init empty Source occurrence/actual canonical baseline changed");
            const auto receiver=owner.base_.receiver_owned;const auto fields=receiver->impl_;
            const auto actual_core=fields->core.lock();const auto actual_grid=fields->grid.lock();
            require(fields->creator==std::this_thread::get_id()&&actual_core==core&&
                !actual_core.owner_before(core)&&!core.owner_before(actual_core)&&
                actual_grid==grid&&!actual_grid.owner_before(grid)&&!grid.owner_before(actual_grid)&&
                fields->image.expired()&&fields->occurrence.lock()==node.layout_receiver_occurrence&&
                fields->id==id&&fields->serial==frame.serial&&fields->canonical.lock()==receiver,
                "Grid init actual receiver owner/control/constructor changed");
            const auto array=owner.base_.children_78;
            require(array->impl_&&array->impl_->creator==std::this_thread::get_id()&&
                array->impl_->parent.lock()==receiver&&array->impl_->ordered.empty()&&(owner.base_.flags_40&8u),
                "Grid init actual nonNULL78/OR8 suffix changed");
            for(std::size_t i=0;i<grid->common_layer_returns_.size();++i){
                const auto& layer=grid->common_layer_returns_[i];
                require(layer.first<owner.parser_layers_.size()&&owner.parser_layers_[layer.first].children.empty()&&
                    layer.second&&layer.second->impl_,"Grid init reached Style/common return missing");
                for(std::size_t j=0;j<i;++j)require(grid->common_layer_returns_[j].first!=layer.first,
                    "Grid init common layer return replay");
                const auto r=layer.second->impl_;const auto returned_core=r->core.lock();
                require(r->creator==std::this_thread::get_id()&&returned_core==core&&
                    !returned_core.owner_before(core)&&!core.owner_before(returned_core)&&
                    r->occurrence.lock()==node.layout_receiver_occurrence&&r->selected.lock()==xml&&
                    r->receiver.lock()==receiver&&r->children.lock()==array&&r->serial==frame.serial&&r->count==0,
                    "Grid init opaque common return belongs to another actual allocation/Source");
            }
        };
        frame.pure();check_common(frame.domains.at(domain).core,frame.baseline_owned(key));frame.pure();
        //Grid3 earned this receiver/common suffix, but did not earn virtual38
        //attachment. The captured parent (including Context local0) cannot
        //substitute for actual parent38 or authorize5761/base_anchor stores.
        //Until that distinct owner is implemented, only a genuinely captured
        //NULL parent together with the constructor's actual NULL38 is eligible.
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey local){
            view.require_full();view.require_pure();
            auto& actual=Parent::node(view,local);
            check_common(view.binding_->domains.front().core,*actual.state);
            require(!actual.parent&&actual.state->base_.parent_38.expired(),
                "Grid native parent38/Context attachment producer required before5761");
            view.require_pure();
        });
        frame.first();frame.full();
        check_common(frame.domains.at(domain).core,frame.baseline_owned(key));frame.pure();
        grid->initialization_entered=true;
        //Native70F8E6 calls5761 FIRST, including ALL actual children and bounds.
        //No nine-slice/UV stores before the independently owned common return.
        initialize_control_5761_child_owned(frame,domain,id);frame.first();frame.full();
        check_common(frame.domains.at(domain).core,frame.baseline_owned(key));frame.pure();
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey local){
            view.require_full();view.require_pure();
            check_common(view.binding_->domains.front().core,*Parent::node(view,local).state);view.require_pure();
            const auto current=Parent::node(view,local).state->grid_;
            require(current.get()==grid.get()&&!current.owner_before(grid)&&!grid.owner_before(current),
                "Grid constructor changed after actual5761 children");
            require(!(Parent::node(view,local).state->base_.flags_40&0x400000u),
                "Grid70F8F6 AutoSize563190 mandatory producer unimplemented");
            view.preflight_store_owned(local);
            Parent::node(view,local).state->history_.reserve(Parent::node(view,local).state->history_.size()+1);
            view.require_full();view.require_pure();
            auto& actual=*Parent::node(view,local).state;
            require(actual.grid_.get()==grid.get(),"Grid actual field changed before inline geometry");
            grid->initialize_geometry_70f8_owned(actual);
            actual.record_owned(Baseline::Mutation::receiver_prefix);
            view.record_store_owned(local,Parent::Store::Kind::grid);
            view.require_full();view.require_pure();
        });
        frame.first();frame.full();require(frame.baseline_owned(key).grid_.get()==grid.get()&&!grid->failed,
            "Grid actual field/lifetime changed after geometry View destructor");
        check_common(frame.domains.at(domain).core,frame.baseline_owned(key));frame.pure();
        grid->initialization_returned=true;frame.first();frame.full();
        //Only enclosing genuine dispatcher/body+DTOR may publish completion.
        //This DATA flag alone never grants Paint, Frame or public readiness.
    }catch(...){frame.poison();throw;}
}
