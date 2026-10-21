//Inside civ5::ui AFTER actual standalone parser/common prefix and ordinary storage.
struct detail::NotificationLayoutStandaloneLabelState::WrapperReturn {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    const std::weak_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::weak_ptr<NotificationLayoutOrdinaryChildren78> children;
    const std::weak_ptr<const NotificationLayoutCommonChildFactoryReturn> common;
    const std::uint64_t serial,sequence,base_sequence;
    WrapperReturn(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> x,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> r,
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> a,
        std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> done,
        std::uint64_t s,std::uint64_t q,std::uint64_t b)
        :core(std::move(c)),occurrence(std::move(o)),selected(std::move(x)),receiver(std::move(r)),
         children(std::move(a)),common(std::move(done)),serial(s),sequence(q),base_sequence(b){}
};
void detail::NotificationLayoutRuntimeDimensions::run_standalone_label_common_empty_children_owned()const{
    using Label=NotificationLayoutStandaloneLabelState;using Loan=Label::ParserLoan;
    const auto saved=state_;const auto loan=saved->active_standalone_parser.lock();
    require(bool(loan),"Standalone common suffix lexical45EA90 issuer absent");loan->full();
    const auto fields=loan->label->impl_;auto& base=*const_cast<NotificationLayoutBaseline*>(saved->baseline);
    const auto core=saved->core.lock();const auto occurrence=saved->occurrence.lock();
    const auto selected=loan->selected;const auto receiver=base.base_.receiver_owned;
    std::shared_ptr<NotificationLayoutOrdinaryChildren78> published_array;
    std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn> published_common;
    std::shared_ptr<const Label::WrapperReturn> published_wrapper;
    const auto current=[&]{loan->first();require(core&&occurrence&&receiver&&receiver->impl_&&
        loan->stage==Loan::Stage::common_required&&fields->common_prefix_entered&&fields->common_prefix_returned&&
        base.base_.receiver_owned==receiver&&receiver->impl_->core.lock()==core&&receiver->impl_->occurrence.lock()==occurrence&&
        receiver->impl_->canonical.lock()==receiver&&receiver->impl_->standalone_label.lock()==loan->label&&
        receiver->impl_->image.expired()&&receiver->impl_->box.expired()&&receiver->impl_->id==saved->id&&
        receiver->impl_->serial==loan->serial&&base.base_.children_78==published_array&&
        base.base_.common_return_owned==published_common&&fields->wrapper_returned==published_wrapper&&
        selected&&selected->value.children.empty(),"Standalone real zero-child common suffix control/order changed");};
    const auto full=[&]{current();saved->operation->require_current();current();};
    try{
        current();require(std::holds_alternative<Label::Impl::Null>(fields->fields[0x78/4]),
            "Standalone495E10 actual constructor78 NULL changed before allocator");full();
        //The selected full XML has zero DIRECT children: both wrapper MISS
        //queries and835620(1)=0 are real. A nonempty list, including wrappers,
        //fails above; no fake factoryNULL or successful skipped child.
        struct Cursor final{
            std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
            std::size_t next=0;
            void retire_owned(){selected.reset();}
        }cursor{selected};
        auto array_fields=std::make_shared<NotificationLayoutOrdinaryChildren78::Impl>(receiver);full();
        auto array=std::shared_ptr<NotificationLayoutOrdinaryChildren78>(new NotificationLayoutOrdinaryChildren78(array_fields));full();
        require(array_fields->ordered.empty()&&array_fields->parent.lock()==receiver&&!base.base_.children_78,
            "Standalone495E10 zero-length array allocator POST changed");
        loan->record(0x78);loan->base_record(NotificationLayoutBaseline::Mutation::receiver_prefix);
        fields->fields[0x78/4]=Label::Impl::ChildrenBorrow{array};base.base_.children_78=array;published_array=array;current();
        //Native438770 direct count +43877C reserve then43878F zero loop branch.
        array_fields->ordered.reserve(0);full();require(array_fields->ordered.empty(),"Standalone actual empty78 vector changed across reserve POST");
        //Native4387C1 OR8 is mandatory even count0. Both observations refer to
        //the SAME actual field and are stored without an intervening callback.
        loan->record(0x40);loan->base_record(NotificationLayoutBaseline::Mutation::receiver_prefix);
        std::get<std::uint32_t>(fields->fields[0x40/4])|=8u;
        base.base_.flags_40=std::get<std::uint32_t>(fields->fields[0x40/4]);current();
        require(cursor.next==0&&cursor.selected==selected,"Standalone genuine empty XML cursor changed");
        cursor.retire_owned(); //4387C9 actual lexical XML handle cleanup.
        full();require(!cursor.selected,"Standalone4387C9 temporary cleanup did not complete");
        const auto serial=loan->serial;
        auto done_fields=std::make_shared<NotificationLayoutCommonChildFactoryReturn::Impl>(core,occurrence,selected,
            receiver,array,serial,0);full();
        auto done=std::shared_ptr<const NotificationLayoutCommonChildFactoryReturn>(new NotificationLayoutCommonChildFactoryReturn(done_fields));full();
        require((base.base_.flags_40&8u)&&!base.base_.common_return_owned&&array_fields->ordered.empty(),
            "Standalone actual common return fields changed across allocation POST");
        base.base_.common_return_owned=done;published_common=done;full();
        const auto sequence=loan->child_sequence,base_sequence=loan->baseline_sequence;
        auto wrapper=std::shared_ptr<const Label::WrapperReturn>(new Label::WrapperReturn(core,occurrence,selected,
            receiver,array,done,serial,sequence,base_sequence));full();
        require(wrapper->sequence==fields->sequence&&wrapper->base_sequence==base.sequence_&&!fields->wrapper_returned,
            "Standalone45EA90 return allocator POST changed journal operands");
        fields->wrapper_returned=wrapper;published_wrapper=wrapper;full();loan->stage=Loan::Stage::common_returned;
        loan->full(); //Final actual completed return, not Init/Prepared/Run authority.
    }catch(...){fields->failed.store(true);loan->alive.store(false);fail_storage_without_callbacks(saved);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::require_standalone_label_common_return_from_parser_owned()const{
    const auto saved=state_;
    const auto current=[&]{
        require(saved&&saved->creator==std::this_thread::get_id()&&saved->alive.load()&&!saved->failed.load(),
            "Standalone parser returning receiver retired/foreign");
        const auto core=saved->core.lock();require(bool(core),"Standalone returning Core retired");core->layout_origin(true);
        const auto& actual_operation=core->source->current_operation_loan();core->layout_origin(true);
        require(&actual_operation==saved->operation&&actual_operation.operation_serial()==saved->serial,
            "Standalone returning Source replaced before historical borrowed access");
        require_storage_without_callbacks(saved);
        const auto& base=*saved->baseline;const auto label=base.standalone_label_;
        require(label&&base.kind_==NotificationLayoutBaseline::Kind::label&&base.actual_type_=="Label",
            "Standalone returning subtype owner absent");const auto fields=label->impl_;fields->first();
        const auto done=fields->wrapper_returned;const auto receiver=base.base_.receiver_owned;
        const auto array=base.base_.children_78;const auto common=base.base_.common_return_owned;
        const auto& node=BnwWorldViewControlTree::Core::node(core->nodes,saved->id);
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        require(done&&done->creator==std::this_thread::get_id()&&receiver&&receiver->impl_&&array&&array->impl_&&common&&common->impl_&&
            done->serial==saved->serial&&done->sequence==fields->sequence&&done->base_sequence==base.sequence_&&
            done->core.lock()==core&&done->occurrence.lock()==saved->occurrence.lock()&&done->selected.lock()==node.full_xml_occurrence&&
            done->receiver.lock()==receiver&&done->children.lock()==array&&done->common.lock()==common&&
            node.full_xml_occurrence&&!node.dynamic&&node.full_xml_occurrence->value.children.empty()&&
            receiver->impl_->standalone_label.lock()==label&&receiver->impl_->image.expired()&&receiver->impl_->box.expired()&&
            receiver->impl_->canonical.lock()==receiver&&receiver->impl_->core.lock()==core&&
            receiver->impl_->occurrence.lock()==saved->occurrence.lock()&&receiver->impl_->id==saved->id&&
            receiver->impl_->serial==saved->serial&&array->impl_->parent.lock()==receiver&&array->impl_->ordered.empty()&&
            common->impl_->creator==std::this_thread::get_id()&&common->impl_->core.lock()==core&&
            common->impl_->occurrence.lock()==saved->occurrence.lock()&&common->impl_->selected.lock()==node.full_xml_occurrence&&
            common->impl_->receiver.lock()==receiver&&common->impl_->children.lock()==array&&common->impl_->serial==saved->serial&&
            common->impl_->count==0&&(base.base_.flags_40&8u)&&fields->common_prefix_returned,
            "Standalone closed empty common/wrapper allocation/control/sequence changed");
        require(same(done->core.lock(),core)&&same(done->occurrence.lock(),saved->occurrence.lock())&&
            same(done->selected.lock(),node.full_xml_occurrence)&&same(done->receiver.lock(),receiver)&&
            same(done->children.lock(),array)&&same(done->common.lock(),common)&&
            same(receiver->impl_->standalone_label.lock(),label)&&same(receiver->impl_->canonical.lock(),receiver)&&
            same(receiver->impl_->core.lock(),core)&&same(receiver->impl_->occurrence.lock(),saved->occurrence.lock())&&
            same(array->impl_->parent.lock(),receiver)&&same(common->impl_->core.lock(),core)&&
            same(common->impl_->occurrence.lock(),saved->occurrence.lock())&&same(common->impl_->selected.lock(),node.full_xml_occurrence)&&
            same(common->impl_->receiver.lock(),receiver)&&same(common->impl_->children.lock(),array),
            "Standalone opaque returning allocations/control ownership changed");
        const auto* children=std::get_if<NotificationLayoutStandaloneLabelState::Impl::ChildrenBorrow>(&fields->fields[0x78/4]);
        const auto* flags=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(children&&same(children->object,array)&&flags&&*flags==base.base_.flags_40,
            "Standalone actual78/40 Base aliases changed after return");
        if(const auto* cc=std::get_if<NotificationLayoutStandaloneLabelState::Impl::FontBorrow>(&fields->fields[0xcc/4])){
            require(bool(cc->loan),"Standalone returning actual CC borrower empty");cc->loan->require_current_without_callbacks();
        }else require(std::holds_alternative<NotificationLayoutStandaloneLabelState::Impl::Null>(fields->fields[0xcc/4]),
            "Standalone returning CC has no actual constructor/Font issuer");
        if(const auto* cache=std::get_if<NotificationLayoutStandaloneLabelState::Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
            require(bool(cache->loan),"Standalone returning actual170 borrower empty");cache->loan->require_current_without_callbacks();
        }else require(std::holds_alternative<NotificationLayoutStandaloneLabelState::Impl::Null>(fields->fields[0x170/4]),
            "Standalone returning170 has no actual genuine NULL/cache issuer");
        if(const auto* text=std::get_if<NotificationLayoutStandaloneLabelState::Impl::TextBorrow>(&fields->fields[0xc4/4])){
            require(text->object&&text->object->creator==std::this_thread::get_id()&&!text->object->retired.load()&&
                same(text->object->allocation_child.lock(),fields)&&text->object->source_serial==fields->source_serial&&
                !text->object->bytes.empty()&&text->object->bytes.back()=='\0',
                "Standalone returning C4 allocation/link/storage retired");
        }else require(std::holds_alternative<NotificationLayoutStandaloneLabelState::Impl::Null>(fields->fields[0xc4/4]),
            "Standalone returning C4 has no genuine constructor/parser issuer");
        require_storage_without_callbacks(saved);
    };
    current();saved->operation->require_current();current();
}
