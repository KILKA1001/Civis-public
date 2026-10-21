// Inside civ5::ui after complete genuine Common Call/Root and Button backing.
// Distinct physical NULL child18 return. It is not an own118 or Root return.
struct BnwWorldViewControlTree::NormalCommonNullChildReturn final {
public:
    NormalCommonNullChildReturn(const NormalCommonNullChildReturn&)=delete;
    NormalCommonNullChildReturn&operator=(const NormalCommonNullChildReturn&)=delete;
    ~NormalCommonNullChildReturn()=default;
private:
    friend class BnwWorldViewControlTree;
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const detail::BnwNormalCommonChildCall::Impl> original_call;
    const detail::BnwNormalRootBatchHistory::Guard root;
    const std::shared_ptr<const detail::BnwNormalButtonSubobject118Backing> backing;
    const std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> children;
    const std::shared_ptr<const NormalCommonRecursiveReturn> common_return;
    // Only the closed Tree factory constructs this after the actual empty
    // Common branch has returned and the actual caption178 MISS was checked.
    NormalCommonNullChildReturn(
        std::shared_ptr<const detail::BnwNormalCommonChildCall::Impl> actual_call,
        detail::BnwNormalRootBatchHistory::Guard actual_root,
        std::shared_ptr<const detail::BnwNormalButtonSubobject118Backing> actual_backing,
        std::shared_ptr<detail::NotificationLayoutOrdinaryChildren78> actual_children)
        :original_call(std::move(actual_call)),root(std::move(actual_root)),
         backing(std::move(actual_backing)),children(std::move(actual_children)),common_return(original_call->common_return){}
    void require_physical_original_without_callbacks()const{
        BnwWorldViewControlTree::require_normal_common_null_child_return_physical_without_callbacks(*this);
    }
};

bool BnwWorldViewControlTree::normal_common_button_texture_is_nonnull_without_callbacks(
    const detail::BnwNormalCommonChildCall& actual_call){
    // CURRENT genuine Call/Root FIRST precedes the actual Button/backing read.
    actual_call.require_current_without_callbacks();const auto offered=actual_call.impl_;
    require(offered->kind==detail::BnwNormalCommonChildCall::Kind::button&&offered->original_button,
        "Common texture classifier requires SAME genuine current Button call");
    const auto button=offered->original_button;const auto backing=button->backing_118_;
    require(bool(backing),"Common Button physical118 texture backing producer missing");
    backing->require_same_button_storage_without_callbacks(button);
    const bool actual_null=backing->require_current_source44_null_branch_without_callbacks();
    // The Bank classifier proves covered44 plus its actual init store marker;
    // missing coverage/unknown native32 pointer throws before any own effects.
    actual_call.require_current_without_callbacks();
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(same(button,offered->original_button)&&same(backing,button->backing_118_),
        "Common Button current physical texture backing changed at PURE POST");
    backing->require_same_button_storage_without_callbacks(button);
    require(backing->require_current_source44_null_branch_without_callbacks()==actual_null,
        "Common Button actual physical44 branch changed at PURE POST");
    actual_call.require_current_without_callbacks();return !actual_null;
}

bool detail::BnwNormalCommonChildCall::actual_button_texture_is_nonnull_without_callbacks()const{
    return BnwWorldViewControlTree::normal_common_button_texture_is_nonnull_without_callbacks(*this);
}

void BnwWorldViewControlTree::require_normal_common_null_child_return_physical_without_callbacks(
    const NormalCommonNullChildReturn& returned){
    using Call=detail::BnwNormalCommonChildCall;using Button=detail::NotificationLayoutButtonState;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    // Physical owned controls FIRST. The original pending Call may already be
    // retired: never invoke its CURRENT checker, old Entry, Frame or Prefix.
    require(returned.creator==std::this_thread::get_id()&&returned.original_call&&returned.backing&&returned.children,
        "Common NULL return physical original controls absent/foreign");
    const auto call=returned.original_call;const auto history=returned.root.impl_;
    require(call->creator==returned.creator&&call->sealed&&same(call->canonical_self.lock(),call)&&
        call->kind==Call::Kind::button&&call->phase==Call::Phase::button_caption178&&
        call->original_button&&!call->original_label&&!call->original_button_invocation_control&&
        history&&history->creator==returned.creator&&history->sealed&&history->tree&&history->serial==call->serial,
        "Common NULL return SAME actual child/suffix/Root controls changed");
    const auto traversal=call->traversal.lock();
    const auto execution=traversal?traversal->execution.lock():nullptr;
    require(traversal&&execution&&traversal->creator==returned.creator&&
        same(execution->common_traversal,traversal)&&same(execution->root.impl_,history),
        "Common NULL return retained original Execution/Root shared controls changed");
    // This is the existing separate historical Root Scope health, not a
    // renamed active Root guard or an active child publication permission.
    history->root_scope.require_physical_current_without_callbacks();
    const auto actual=history->tree->core_;actual->layout_origin(actual->busy);
    const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
    require(&fresh==history->operation&&fresh.operation_serial()==call->serial&&
        same(call->control.owner_.lock(),actual)&&call->receiver&&call->receiver->impl_&&call->children&&call->children->impl_&&
        call->ordinal<call->count&&call->children->impl_->ordered.size()==call->count&&
        same(call->children->impl_->ordered[call->ordinal],call->receiver)&&
        same(call->children->impl_->parent.lock(),call->parent),
        "Common NULL retained actual Source/Control/ordered78 association changed");
    const auto& node=Core::node(actual->nodes,call->control.id_);
    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence,
        "Common NULL return original constructor occurrence retired");
    const auto& baseline=*node.layout_baseline;const auto button=call->original_button;
    require(baseline.sequence_==call->base_sequence&&same(baseline.button_,button)&&
        same(baseline.base_.receiver_owned,call->receiver)&&same(baseline.base_.parent_38.lock(),call->parent)&&
        baseline.base_.flags_40==call->physical_flags&&(call->physical_flags&0x10000000u)&&
        same(baseline.base_.children_78,returned.children)&&returned.children->impl_&&
        returned.children->impl_->creator==returned.creator&&returned.common_return&&call->common_return==returned.common_return&&
        same(returned.children->impl_->parent.lock(),call->receiver)&&same(button->backing_118_,returned.backing),
        "Common NULL retained real Base/empty children78/physical texture actor changed");
    require_normal_common_recursive_return_physical_without_callbacks(*returned.common_return);
    // Genuine constructor/common journals prove the actual children count and
    // actual NULL178. An absent presentation DTO cannot satisfy these guards.
    require_button_native_common_return_owned(actual,call->control.id_,fresh,actual->busy);
    returned.backing->require_same_button_storage_without_callbacks(button);
    require(returned.backing->require_current_source44_null_branch_without_callbacks(),
        "Common NULL return actual covered44 no longer NULL");
    require(button->child_178_observation&&*button->child_178_observation==Button::Child178Observation::constructor_null&&
        !button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_&&!button->label_retirement_,
        "Common NULL return actual caption178 constructor/MISS changed");
    fresh.require_origin_without_callbacks();history->root_scope.require_physical_current_without_callbacks();
    require(same(button,call->original_button)&&same(returned.backing,button->backing_118_)&&
        same(Core::node(actual->nodes,call->control.id_).layout_baseline->base_.children_78,returned.children)&&
        returned.common_return==call->common_return&&actual->source->current_operation_loan().operation_serial()==call->serial,
        "Common NULL return retained physical originals changed at PURE POST");
}

std::shared_ptr<const BnwWorldViewControlTree::NormalCommonNullChildReturn>
BnwWorldViewControlTree::capture_normal_common_null_child_return_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwNormalCommonChildCall& actual_call){
    using Call=detail::BnwNormalCommonChildCall;using Button=detail::NotificationLayoutButtonState;
    actual_call.require_current_without_callbacks();const Call call=actual_call;call.require_current_without_callbacks();
    const auto offered=call.impl_;const auto traversal=offered->traversal.lock();
    const auto execution=traversal->execution.lock();const auto root=execution->root;
    const auto button=offered->original_button;require(bool(button),"Common NULL return actual Button absent");
    const auto backing=button->backing_118_;const auto actual=root.impl_->tree->core_;
    const auto children=Core::node(actual->nodes,offered->control.id_).layout_baseline->base_.children_78;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto first=[&]{call.require_current_without_callbacks();
        require(offered->kind==Call::Kind::button&&offered->phase==Call::Phase::button_caption178&&
            !offered->original_button_invocation_control&&(offered->physical_flags&0x10000000u)&&
            backing&&children&&children->impl_&&offered->common_return&&
            same(children->impl_->parent.lock(),offered->receiver)&&same(button,offered->original_button)&&
            same(backing,button->backing_118_)&&
            same(Core::node(actual->nodes,offered->control.id_).layout_baseline->base_.children_78,children),
            "Common NULL return requires actual empty Common RETURN and no own118 Invocation");
        require_normal_common_recursive_return_physical_without_callbacks(*offered->common_return);
        require(!normal_common_button_texture_is_nonnull_without_callbacks(call),
            "Common NULL return cannot issue for a nonNULL physical44 branch");
        require(button->child_178_observation&&*button->child_178_observation==Button::Child178Observation::constructor_null&&
            !button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_&&!button->label_retirement_,
            "Common NULL return actual caption178 MISS has not returned");
        call.require_current_without_callbacks();};
    const auto full=[&]{first();call.require_source_owned(operation);first();};
    first();full();first();
    auto returned=std::shared_ptr<const NormalCommonNullChildReturn>(
        new NormalCommonNullChildReturn(offered,root,backing,children));
    first();full();first();returned->require_physical_original_without_callbacks();return returned;
}
