// Inside civ5::ui after RuntimeState/Realm argument and actual Core/Gui types.
// Closed selected ordinary Box#Base: 6A374A ->438170 through4381F5.
// This successor enters actual86AC20/commonColor/ID, then mandatory child-factory STOP. No parse return.
void detail::NotificationLayoutRuntimeDimensions::run_box_common_prefix_owned(
    const content::XmlNode& reached_layer,std::size_t ordinal)const{
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();const auto manager=saved->manager.lock();
    require(core&&manager&&saved->id&&core->context=="UnitPanel",
        "Box common requires SAME ordinary UnitPanel allocation");
    const auto box=saved->baseline->box_;
    require(bool(box),"Box common actual constructor storage absent");
    const auto argument=context_realm_argument_owned();require_context_argument_owned(saved,argument);
    const auto realm=argument->impl_->realm;
    require(bool(realm),"Box common real ordinary Realm argument absent");
    bool journal_started=false,journal_reserved=false;
    std::uint64_t expected_sequence=0;
    std::size_t expected_history_size=0,reserved_end=0;
    const auto pure=[&](bool entered){
        require_storage_without_callbacks(saved);require_context_argument_owned(saved,argument);
        manager->impl_->first(*saved->operation);
        require((_mm_getcsr()&0xe040u)==0,"Box common Full conversion unsupported current SSE mode");
        require(saved->baseline->kind_==NotificationLayoutBaseline::Kind::box&&
            saved->baseline->actual_type_=="Box"&&saved->baseline->box_.get()==box.get()&&
            !saved->baseline->box_.owner_before(box)&&!box.owner_before(saved->baseline->box_)&&
            saved->baseline->parser_runtime_dimensions_==saved->facade&&!saved->baseline->parsed_source_serial_&&
            !saved->registering&&!saved->assigning_context_3c&&!box->failed&&box->parser_attempted&&
            box->absent_style_returned&&!box->color_querying&&box->color_stored&&
            box->color_source_serial==saved->serial&&box->common_querying==entered&&
            ordinal==0&&saved->baseline->parser_layers_.size()==1,
            "Box common actual subtype/occurrence/phase changed or reentered");
        require(box->frontier==NotificationLayoutBoxState::Frontier::common_parser_required||
            (entered&&box->frontier==NotificationLayoutBoxState::Frontier::resize_registration_required),
            "Box common real callsite frontier changed");
        const auto& actual=BnwWorldViewControlTree::Core::node(core->nodes,saved->id);
        require(actual.authored.name=="Box"&&actual.authored.attribute("ID")=="Base",
            "Box common receiver is not selected actual Base");
        if(journal_started)require(saved->baseline->sequence_==expected_sequence&&
            saved->baseline->history_.size()==expected_history_size&&
            (!journal_reserved||saved->baseline->history_.capacity()>=reserved_end),
            "Box common actual journal/control changed across supplier boundary");
        // Physical progress checks, never a completion/initializer authority.
        if(box->common_context_stored){const auto& b=saved->baseline->base_;
            require(b.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
                b.context_realm_3c.get()==realm.get()&&!b.context_realm_3c.owner_before(realm)&&
                !realm.owner_before(b.context_realm_3c),"Box common actual3C VALUE/control changed");}
        const auto dimensions=dimensions_from_storage_without_callbacks(saved);
        for(std::size_t axis=0;axis<box->common_size_axes;++axis)
            require(std::bit_cast<std::uint32_t>(saved->baseline->base_.size_30_34[axis])==
                std::bit_cast<std::uint32_t>(static_cast<float>(dimensions[axis])),
                "Box common actual Size VALUE changed across supplier boundary");
        if(box->common_full_flags_stored)
            require((saved->baseline->base_.flags_40&0x300000u)==0x300000u,
                "Box common accumulated Full flags changed");
    };
    const auto matches=[&](bool entered){pure(entered);
        const auto& layer=saved->baseline->parser_layers_[ordinal];
        bool id=false,color=false,size=false;
        for(const auto& a:layer.attributes){
            if(a.first=="ID"){require(!id&&a.second=="Base","Box common selected ID changed");id=true;}
            else if(a.first=="Color"){require(!color&&a.second=="0,0,0,0","Box common selected Color changed");color=true;}
            else if(a.first=="Size"){require(!size&&a.second=="Full,Full","Box common selected Full pair changed");size=true;}
            else require(false,"Box common outside selected finite attributes; real owner required");
        }
        require(id&&color&&size,"Box common actual selected attribute absent");
    };
    matches(false);
    require(!box->common_context_stored&&!box->common_size_axes&&!box->common_full_flags_stored&&
        BnwWorldViewControlTree::Core::same_layout_xml(saved->baseline->parser_layers_[ordinal],reached_layer),
        "Box common replay/foreign source layer");
    box->common_querying=true;
    struct End{std::shared_ptr<NotificationLayoutBoxState> box;~End(){box->common_querying=false;}}end{box};
    const auto current=[&]{matches(true);};
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        // Selected HideBox ABSENT at6A370F: actual skip to6A374A, no OR400000.
        // Selected Offset ABSENT at4381C0: preserves constructor70/74.
        // Reserve OWN journal separately from semantic stores. FULL POST after
        // allocation precedes any raw Base store or later borrowed XML read.
        full();auto& owner=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
        require(owner.sequence_<=UINT64_MAX-4&&owner.history_.max_size()-owner.history_.size()>=4,
            "Box common actual journal exhausted");
        const auto sequence=owner.sequence_,history_size=owner.history_.size();
        expected_sequence=sequence;expected_history_size=history_size;
        reserved_end=history_size+4;journal_started=true;
        owner.history_.reserve(history_size+4);full();
        require(owner.sequence_==sequence&&owner.history_.size()==history_size&&
            owner.history_.capacity()-history_size>=4,"Box common journal changed across allocation");
        journal_reserved=true;
        const auto journal=[&](NotificationLayoutBaseline::Mutation mutation){
            // All four entries are trivial/nonallocating in retained capacity.
            owner.history_.push_back({++owner.sequence_,mutation});
            ++expected_sequence;++expected_history_size;
        };
        // Real4381AC stores ordinary second argument, never outerButton3C.
        owner.base_.context_realm_3c=realm;
        owner.base_.context_3c_observation=NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;
        box->common_context_stored=true;journal(NotificationLayoutBaseline::Mutation::context_realm_3c);full();
        // Native76DC50 selected Full consumes SAME GUI signed32 dimensions,
        // CVTSI2SS X before Y. No parent dimensions or supplied numeric DTO.
        for(std::size_t axis=0;axis<2;++axis){
            current();const auto dimensions=dimensions_from_storage_without_callbacks(saved);current();
            const float value=static_cast<float>(dimensions[axis]);full();
            const auto now=dimensions_from_storage_without_callbacks(saved);current();
            require(dimensions==now,"Box common GUI dimensions changed before Full store");
            owner.base_.size_30_34[axis]=value;
            box->common_size_axes=static_cast<std::uint8_t>(axis+1);
            journal(NotificationLayoutBaseline::Mutation::parser);full();
        }
        //4381F2 ORs returned300000 only AFTER helper returned both stores.
        owner.base_.flags_40|=0x300000u;box->common_full_flags_stored=true;
        journal(NotificationLayoutBaseline::Mutation::parser);full();
        require(owner.sequence_==sequence+4&&owner.history_.size()==history_size+4,
            "Box common actual prefix journal changed across POST");
        box->frontier=NotificationLayoutBoxState::Frontier::resize_registration_required;current();
        //4381F5 true: enter genuine sameGui registration and commonColor/ID.
        //Recursive factories may change histories after this prefix returns;
        //use the actual opaque return rather than replaying the old +4 snapshot.
        run_box_post_size_prefix_owned(ordinal);
        require_box_common_return_from_parser_owned();
    }catch(...){box->failed=true;fail_storage_without_callbacks(saved);throw;}
}
