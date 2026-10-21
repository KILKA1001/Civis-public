// After genuine CommonRoot, before arena/Material. Only Tree issues calls.
struct detail::BnwNormalVisibleLabelCall::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    mutable std::atomic<bool> alive{true};
    const BnwNormalRootBatchHistory::Guard root;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<NotificationLayoutStandaloneLabelState> label;
    const std::uint64_t control,serial;
    const std::size_t ordinal;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    Impl(BnwNormalRootBatchHistory::Guard r,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_receiver,
        std::shared_ptr<NotificationLayoutStandaloneLabelState> actual_label,
        std::uint64_t id,std::uint64_t s,std::size_t index)
        :root(std::move(r)),receiver(std::move(actual_receiver)),label(std::move(actual_label)),
         control(id),serial(s),ordinal(index){}
    static bool same(const auto& a,const auto& b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    }
    void first()const{BnwWorldViewControlTree::require_normal_visible_label_call_current_without_callbacks(*this);}
    void full(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        first();root.require_source_owned(op);first();
    }
};
void BnwWorldViewControlTree::require_normal_visible_label_call_current_without_callbacks(
    const detail::BnwNormalVisibleLabelCall::Impl& call){
        const auto& root=call.root;const auto& receiver=call.receiver;const auto& label=call.label;
        const auto creator=call.creator;const auto serial=call.serial;const auto ordinal=call.ordinal;
        const auto control=call.control;
        require(creator==std::this_thread::get_id()&&call.alive.load()&&call.sealed,
            "Visible Label call retired/foreign/unsealed");
        root.require_physical_current_without_callbacks();
        const auto actual=root.impl_;const auto prefix=actual->prefix;
        const auto common=prefix->common_root;
        require(actual->label_traversal_active&&!actual->label_traversal_completed&&
            actual->serial==serial&&detail::BnwNormalVisibleLabelCall::Impl::same(
                actual->current_visible_label.lock(),call.canonical_self.lock())&&
            call.canonical_self.lock().get()==&call&&common&&!common->continuing&&
            common->stage==decltype(common)::element_type::Stage::child18_required&&
            common->cursor==ordinal&&ordinal<common->count&&!common->whole_return&&
            detail::BnwNormalVisibleLabelCall::Impl::same(common->ordered[ordinal],receiver)&&control,
            "Visible Label call not SAME CURRENT enclosing Root/ordered78 occurrence");
        // This is a genuine receiver issued by the subtype constructor, not a
        // tag-derived virtual function or generic Node.parent association.
        const auto r=receiver->impl_;const auto tree=actual->tree;const auto owner=tree->core_;
        const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
        require(r&&r->creator==creator&&r->id==control&&same(r->canonical.lock(),receiver)&&
            same(r->core.lock(),owner)&&same(r->standalone_label.lock(),label),
            "Visible Label actual receiver/Tree/subtype differs");
        const auto& node=BnwWorldViewControlTree::Core::node(owner->nodes,control);
        require(node.layout_baseline&&same(node.layout_baseline->standalone_label_,label)&&
            same(node.layout_baseline->base_.receiver_owned,receiver)&&
            same(node.layout_baseline->base_.parent_38.lock(),common->root),
            "Visible Label actual paired78/38/Label changed");
        const auto fields=label->impl_;fields->first();
        const auto word=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(word&&*word==node.layout_baseline->base_.flags_40&&!(*word&0x2002u),
            "Visible Label actual native40 skip branch changed");
        root.require_physical_current_without_callbacks();
        require(call.alive.load()&&actual->current_visible_label.lock().get()==&call,
            "Visible Label CURRENT control retired during pure cut");
}

detail::BnwNormalVisibleLabelCall::BnwNormalVisibleLabelCall(std::shared_ptr<const Impl> actual,const IssuerKey&)
    :impl_(std::move(actual)){}
void detail::BnwNormalVisibleLabelCall::require_current_without_callbacks()const{
    const auto saved=impl_;require(bool(saved),"Visible Label call has no control");saved->first();
}
void detail::BnwNormalVisibleLabelCall::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto saved=impl_;saved->full(op);saved->first();
}
void detail::BnwNormalVisibleLabelCall::require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard& root)const{
    const auto saved=impl_;saved->first();saved->root.require_same_root_without_callbacks(root);saved->first();
}
void detail::BnwNormalVisibleLabelCall::require_actual_publication_without_callbacks(
    const BnwWorldViewControlRenderPublication& publication)const{
    const auto saved=impl_;saved->first();
    BnwWorldViewControlTree::require_normal_visible_label_call_publication_without_callbacks(*saved,publication);
    saved->first();
}
void BnwWorldViewControlTree::require_normal_visible_label_call_publication_without_callbacks(
    const detail::BnwNormalVisibleLabelCall::Impl& call,const BnwWorldViewControlRenderPublication& publication){
    call.first();const auto actual=call.root.impl_;
    const auto original=publication.original_owner_without_callbacks();
    require(original.get()==&publication&&detail::BnwNormalVisibleLabelCall::Impl::same(original->tree_,actual->tree)&&
        original->renderer_==actual->renderer&&
        original->actual_checked_control_id_for_paint_without_callbacks()==call.control,
        "Visible Label original Publication belongs to another Tree/renderer/control");
    call.first();
}
std::uint64_t detail::BnwNormalVisibleLabelCall::actual_control_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->control;saved->first();return value;
}
std::size_t detail::BnwNormalVisibleLabelCall::actual_ordered_ordinal_without_callbacks()const{
    const auto saved=impl_;saved->first();const auto value=saved->ordinal;saved->first();return value;
}

void BnwWorldViewControlTree::require_normal_root_epoch_prefix_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix){
    require(prefix.creator==std::this_thread::get_id()&&prefix.alive.load()&&prefix.sealed&&prefix.batch&&
        prefix.batch->alive.load(),"Root epoch original Prefix physically retired");
    const auto manager=prefix.gui.lock();require(bool(manager),"Root epoch actual manager retired");
    const auto root=manager->normal_root_history.lock();require(bool(root),"Root epoch capture not issued");
    root->first();
    require(root->prefix.get()==&prefix&&!root->prefix.owner_before(manager->normal_gui_batch_prefix.lock())&&
        !manager->normal_gui_batch_prefix.lock().owner_before(root->prefix)&&
        root->serial==prefix.serial&&root->operation==prefix.operation,
        "Root epoch not SAME captured original Prefix/Source");
    root->first();
}
void BnwWorldViewControlTree::require_normal_root_epoch_prefix_source_owned(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix,const runtime::BnwWorldViewSourceOperationLoan& op){
    require_normal_root_epoch_prefix_without_callbacks(prefix);
    const auto manager=prefix.gui.lock();const auto root=manager->normal_root_history.lock();
    root->full(op);require_normal_root_epoch_prefix_without_callbacks(prefix);
}

void BnwWorldViewControlTree::require_normal_pending_label_item_without_callbacks(
    const detail::BnwNormalGuiBatchPrefix::Impl& prefix,const detail::BnwNormalItemOccurrence::Guard& item){
    require_normal_root_epoch_prefix_without_callbacks(prefix);item.require_physical_current_without_callbacks();
    const auto actual=item.impl_;const auto root=actual->root.impl_;
    require(root->prefix.get()==&prefix&&actual->tree.get()==prefix.tree.get()&&
        actual->serial==prefix.serial&&actual->operation==prefix.operation,
        "Pending Label Item not SAME Root/Prefix/Source");
    if(root->label_traversal_active){
        const auto current=root->current_visible_label.lock();require(bool(current),"Pending Label call not issued");
        current->first();require(current->control==actual->control&&
            prefix.common_root&&prefix.common_root->cursor==current->ordinal,
            "Pending Label Item belongs to another actual receiver/cursor");
        const detail::BnwNormalVisibleLabelCall::IssuerKey key;
        const detail::BnwNormalVisibleLabelCall call(current,key);
        call.require_actual_publication_without_callbacks(*actual->publication);
    }else require(actual->publication.get()==prefix.publication&&actual->index==prefix.index&&
        actual->control==prefix.control,"Legacy selected Label not SAME original Prefix item");
    item.require_physical_current_without_callbacks();require_normal_root_epoch_prefix_without_callbacks(prefix);
}
