// Inside civ5::ui AFTER actual Common5 and Caption7 origin definitions.
// This is an actual receiver-origin capsule, not a manager, prefix return,
// Draw/queue/Frame permission, constructor completion or current-state flag.
struct detail::BnwNormalCommonManagerReceiverCall::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const Origin origin;
    const BnwNormalRootBatchHistory::Guard root;
    const NativeReceiver receiver;
    const std::array<std::uint32_t,4> receiver_rect;
    const std::array<std::uint32_t,3> incoming;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    Impl(BnwNormalCommonChildCall actual,BnwNormalRootBatchHistory::Guard actual_root,
        std::shared_ptr<NotificationLayoutButtonState> actual_receiver,
        std::array<std::uint32_t,4> rect,std::array<std::uint32_t,3> arguments)
        :origin(std::in_place_type<BnwNormalCommonChildCall>,std::move(actual)),
         root(std::move(actual_root)),receiver(std::move(actual_receiver)),
         receiver_rect(rect),incoming(arguments){}
    Impl(BnwNormalEmbeddedLabelCommon56Call actual,BnwNormalRootBatchHistory::Guard actual_root,
        std::shared_ptr<NotificationLayoutEmbeddedLabelState> actual_receiver,
        std::array<std::uint32_t,4> rect,std::array<std::uint32_t,3> arguments)
        :origin(std::in_place_type<BnwNormalEmbeddedLabelCommon56Call>,std::move(actual)),
         root(std::move(actual_root)),receiver(std::move(actual_receiver)),
         receiver_rect(rect),incoming(arguments){}
};

std::array<std::uint32_t,4>
BnwWorldViewControlTree::read_normal_common_manager_button_rect_without_callbacks(
    const detail::BnwNormalCommonChildCall& offered){
    // Both offered facade and physical Core are retained before any raw field.
    const auto call=offered;
    call.require_current_without_callbacks();
    require(call.actual_kind_without_callbacks()==detail::BnwNormalCommonChildCall::Kind::button&&
        call.actual_phase_without_callbacks()==detail::BnwNormalCommonChildCall::Phase::common56f630,
        "Common manager ordinary origin requires the actual entered Button56F630");
    const auto root=call.captured_root_without_callbacks();
    root.require_physical_current_without_callbacks();
    const auto tree=root.impl_->tree;
    require(bool(tree),"Common manager original Root Tree absent");
    const auto core=tree->core_;
    require(bool(core),"Common manager original physical Core absent");
    const auto fields=call.impl_->receiver->impl_;
    const auto& node=Core::node(core->nodes,fields->id);
    require(bool(node.layout_baseline),"Common manager actual Button baseline absent");
    const auto& base=node.layout_baseline->base_;
    const std::array<std::uint32_t,4> words{
        std::bit_cast<std::uint32_t>(base.offset_28_2c[0]),
        std::bit_cast<std::uint32_t>(base.offset_28_2c[1]),
        std::bit_cast<std::uint32_t>(base.size_30_34[0]),
        std::bit_cast<std::uint32_t>(base.size_30_34[1])};
    root.require_physical_current_without_callbacks();
    call.require_current_without_callbacks();
    return words;
}

void BnwWorldViewControlTree::require_normal_common_manager_receiver_current_without_callbacks(
    const detail::BnwNormalCommonManagerReceiverCall& offered){
    const auto actual=offered.impl_;
    const auto same=[](const auto&a,const auto&b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(actual&&actual->creator==std::this_thread::get_id()&&actual->sealed&&
        same(actual->canonical_self.lock(),actual),
        "Common manager receiver origin foreign/unsealed/noncanonical FIRST");
    if(const auto call=std::get_if<detail::BnwNormalCommonChildCall>(&actual->origin)){
        call->require_current_without_callbacks();
        require(call->actual_kind_without_callbacks()==detail::BnwNormalCommonChildCall::Kind::button&&
            call->actual_phase_without_callbacks()==detail::BnwNormalCommonChildCall::Phase::common56f630,
            "Common manager ordinary origin left its actual Button56 phase");
        const auto root=call->captured_root_without_callbacks();
        root.require_same_root_without_callbacks(actual->root);
        const auto receiver=std::get_if<std::shared_ptr<detail::NotificationLayoutButtonState>>(&actual->receiver);
        require(receiver&&*receiver&&same(*receiver,call->impl_->original_button),
            "Common manager ordinary actual Button allocation changed");
        const auto offset=call->actual_child_offset_words_without_callbacks();
        const std::array<std::uint32_t,3> incoming{
            offset[0],offset[1],call->actual_child_alpha_word_without_callbacks()};
        require(read_normal_common_manager_button_rect_without_callbacks(*call)==actual->receiver_rect&&
            incoming==actual->incoming,
            "Common manager actual ordinary rect/incoming operands changed");
        call->require_current_without_callbacks();
    }else{
        const auto embedded=std::get_if<detail::BnwNormalEmbeddedLabelCommon56Call>(&actual->origin);
        require(bool(embedded),"Common manager receiver has no genuine typed origin");
        embedded->require_current_without_callbacks();
        const auto root=embedded->captured_root_without_callbacks();
        root.require_same_root_without_callbacks(actual->root);
        const auto receiver=std::get_if<std::shared_ptr<detail::NotificationLayoutEmbeddedLabelState>>(&actual->receiver);
        const auto label=embedded->original_label_without_callbacks();
        require(receiver&&*receiver&&same(*receiver,label)&&
            embedded->actual_receiver_rect_words_without_callbacks()==actual->receiver_rect&&
            embedded->actual_incoming_caption_words_without_callbacks()==actual->incoming,
            "Common manager embedded actual Label/rect/caller changed");
        // Caption7 guards its own actual Material Frame/Paint/DATA/Root and
        // pass RETURN/spriteNULL. It never acquires a Button Common56 phase.
        embedded->require_current_without_callbacks();
    }
    actual->root.require_physical_current_without_callbacks();
    require(same(actual->canonical_self.lock(),actual),
        "Common manager origin canonical identity changed at PURE POST");
}

void BnwWorldViewControlTree::require_normal_common_manager_receiver_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonManagerReceiverCall& offered){
    // An owned VALUE protects the capsule facade across genuine Source FULL.
    const auto call=offered;
    const auto first=[&]{require_normal_common_manager_receiver_current_without_callbacks(call);};
    first();const auto actual=call.impl_;
    if(const auto ordinary=std::get_if<detail::BnwNormalCommonChildCall>(&actual->origin))
        ordinary->require_source_owned(operation);
    else std::get<detail::BnwNormalEmbeddedLabelCommon56Call>(actual->origin).require_source_owned(operation);
    first();actual->root.require_source_owned(operation);first();
}

std::shared_ptr<const detail::BnwNormalCommonManagerReceiverCall>
BnwWorldViewControlTree::capture_normal_common_manager_receiver_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& offered)const{
    using Call=detail::BnwNormalCommonManagerReceiverCall;
    const auto origin=offered; // VALUE before any FIRST/FULL/allocator callback.
    origin.require_current_without_callbacks();
    require(origin.actual_kind_without_callbacks()==detail::BnwNormalCommonChildCall::Kind::button&&
        origin.actual_phase_without_callbacks()==detail::BnwNormalCommonChildCall::Phase::common56f630,
        "Manager origin requires actual ordinary Button56F630 entry");
    const auto root=origin.captured_root_without_callbacks();
    root.require_physical_current_without_callbacks();
    const auto tree=root.impl_->tree;
    require(tree&&tree.get()==this,"Manager origin capture is not SAME actual Root Tree");
    const auto core=tree->core_;
    require(bool(core),"Manager origin capture actual Core absent");
    const auto entry=origin.captured_root_context_entry_without_callbacks();
    entry.require_same_tree_without_callbacks(*tree);
    const auto receiver=origin.impl_->original_button;
    const auto rect=read_normal_common_manager_button_rect_without_callbacks(origin);
    const auto offset=origin.actual_child_offset_words_without_callbacks();
    const std::array<std::uint32_t,3> incoming{
        offset[0],offset[1],origin.actual_child_alpha_word_without_callbacks()};
    const auto same=[](const auto&a,const auto&b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{
        origin.require_current_without_callbacks();
        require(origin.actual_kind_without_callbacks()==detail::BnwNormalCommonChildCall::Kind::button&&
            origin.actual_phase_without_callbacks()==detail::BnwNormalCommonChildCall::Phase::common56f630,
            "Manager origin actual Button phase changed across callback/allocator");
        const auto fresh_root=origin.captured_root_without_callbacks();
        fresh_root.require_same_root_without_callbacks(root);
        entry.require_same_tree_without_callbacks(*tree);
        const auto fresh_offset=origin.actual_child_offset_words_without_callbacks();
        const std::array<std::uint32_t,3> fresh_incoming{
            fresh_offset[0],fresh_offset[1],origin.actual_child_alpha_word_without_callbacks()};
        require(receiver&&same(receiver,origin.impl_->original_button)&&same(tree,root.impl_->tree)&&
            same(core,tree->core_)&&read_normal_common_manager_button_rect_without_callbacks(origin)==rect&&
            fresh_incoming==incoming,"Manager origin actual Button/Root/Core/operands changed");
        root.require_physical_current_without_callbacks();origin.require_current_without_callbacks();
    };
    const auto full=[&]{first();origin.require_source_owned(operation);first();
        root.require_source_owned(operation);first();};
    first();full();first();
    auto actual=std::make_shared<Call::Impl>(origin,root,receiver,rect,incoming);
    first();full();first();
    actual->canonical_self=actual;actual->sealed=true; // Callback-free own seal.
    const Call::IssuerKey key;
    auto result=std::shared_ptr<const Call>(new Call(actual,key));
    first();full();first();
    result->require_current_without_callbacks();result->require_source_owned(operation);
    result->require_current_without_callbacks();return result;
}

std::shared_ptr<const detail::BnwNormalCommonManagerReceiverCall>
BnwWorldViewControlTree::capture_normal_common_manager_receiver_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalEmbeddedLabelCommon56Call& offered)const{
    using Call=detail::BnwNormalCommonManagerReceiverCall;
    const auto origin=offered; // Genuine Caption7 VALUE; no new shared facade.
    origin.require_current_without_callbacks();
    const auto root=origin.captured_root_without_callbacks();
    root.require_physical_current_without_callbacks();
    const auto tree=root.impl_->tree;
    require(tree&&tree.get()==this,"Embedded manager origin is not SAME actual Root Tree");
    const auto core=tree->core_;
    require(bool(core),"Embedded manager origin actual Core absent");
    const auto receiver=origin.original_label_without_callbacks();
    const auto rect=origin.actual_receiver_rect_words_without_callbacks();
    const auto incoming=origin.actual_incoming_caption_words_without_callbacks();
    const auto same=[](const auto&a,const auto&b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{
        origin.require_current_without_callbacks();
        const auto fresh_root=origin.captured_root_without_callbacks();
        fresh_root.require_same_root_without_callbacks(root);
        require(receiver&&same(receiver,origin.original_label_without_callbacks())&&
            same(tree,root.impl_->tree)&&same(core,tree->core_)&&
            origin.actual_receiver_rect_words_without_callbacks()==rect&&
            origin.actual_incoming_caption_words_without_callbacks()==incoming,
            "Embedded manager origin actual Label/Root/Core/operands changed");
        root.require_physical_current_without_callbacks();origin.require_current_without_callbacks();
    };
    const auto full=[&]{first();origin.require_source_owned(operation);first();
        root.require_source_owned(operation);first();};
    first();full();first();
    auto actual=std::make_shared<Call::Impl>(origin,root,receiver,rect,incoming);
    first();full();first();
    actual->canonical_self=actual;actual->sealed=true;
    const Call::IssuerKey key;
    auto result=std::shared_ptr<const Call>(new Call(actual,key));
    first();full();first();
    result->require_current_without_callbacks();result->require_source_owned(operation);
    result->require_current_without_callbacks();return result;
}

detail::BnwNormalCommonManagerReceiverCall::BnwNormalCommonManagerReceiverCall(
    std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){
    BnwWorldViewControlTree::require_normal_common_manager_receiver_current_without_callbacks(*this);
}
void detail::BnwNormalCommonManagerReceiverCall::require_current_without_callbacks()const{
    BnwWorldViewControlTree::require_normal_common_manager_receiver_current_without_callbacks(*this);
}
void detail::BnwNormalCommonManagerReceiverCall::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    BnwWorldViewControlTree::require_normal_common_manager_receiver_source_owned(operation,*this);
}

void BnwWorldViewControlTree::execute_normal_common56_required_manager_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonManagerReceiverCall& offered)const{
    const auto origin=offered;
    origin.require_current_without_callbacks();
    const auto tree=origin.impl_->root.impl_->tree;
    require(tree&&tree.get()==this,"Common manager prefix is not SAME original Root Tree");
    origin.require_source_owned(operation);
    origin.require_current_without_callbacks();
    // Actual typed receiver/caller and SAME original Root/Gui are now guarded.
    // They are not the actual global owner/constructor/writer/55F690 returns.
    require(false,"Common56F630 required actual02DC1B1C/+6ECbit10/+6E0 manager prefix unissued; conditional55F690 cannot be skipped");
}

void BnwWorldViewControlTree::execute_normal_common56_required_manager_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalEmbeddedLabelCommon56Call& offered)const{
    const auto call=offered;
    const auto origin=capture_normal_common_manager_receiver_owned(operation,call);
    execute_normal_common56_required_manager_prefix_owned(operation,*origin);
}
