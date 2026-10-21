// After CommonCall, complete original embedded Label/Factory/Run and Root
// storage. This issuer captures a genuine pending178; it never creates a
// Label, replays initialization, emits a paint index or completes a CPU body.
struct detail::BnwNormalButtonCaptionCall::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const BnwNormalCommonChildCall enclosing_call;
    const BnwNormalRootBatchHistory::Guard root;
    const BnwNormalRootContextEntry entry;
    const BnwWorldViewControlTree::Control control;
    const std::shared_ptr<NotificationLayoutButtonState> button;
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState> label;
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> factory;
    const std::shared_ptr<const BnwPositionedLabelRunLoan> run;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const content::XmlNode reached_layer;
    const std::uint64_t serial,base_sequence,child_sequence,constructor_serial;
    const std::size_t ordinal,parser_layer;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    struct PaintArguments final {
        std::array<std::uint32_t,2> incoming_position;
        std::uint32_t incoming_alpha;
        std::array<std::uint32_t,2> button_position;
        std::uint32_t button_alpha;
        std::array<std::uint32_t,3> result;
        std::array<std::uint32_t,2> numeric;
    };
    mutable std::optional<PaintArguments> paint_arguments;
    mutable bool projecting=false;
    mutable std::optional<std::weak_ptr<const BnwNormalButtonCaptionPaintEmission::Impl>> original_emission_control;
    mutable std::optional<std::weak_ptr<const BnwNormalCaptionFp32RectLoan::Impl>> original_fp32_rect_control;
    mutable bool capturing_fp32_rect=false; //Reentry only, never readiness/completion.
    Impl(BnwNormalCommonChildCall actual_call,BnwNormalRootBatchHistory::Guard actual_root,
        BnwNormalRootContextEntry actual_entry,BnwWorldViewControlTree::Control actual_control,
        std::shared_ptr<NotificationLayoutButtonState> actual_button,
        std::shared_ptr<NotificationLayoutEmbeddedLabelState> actual_label,
        std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> actual_factory,
        std::shared_ptr<const BnwPositionedLabelRunLoan> actual_run,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_receiver,
        content::XmlNode actual_layer,std::uint64_t actual_serial,std::uint64_t actual_base_sequence,
        std::uint64_t actual_child_sequence,std::uint64_t actual_constructor_serial,
        std::size_t actual_ordinal,std::size_t actual_parser_layer)
        :enclosing_call(std::move(actual_call)),root(std::move(actual_root)),entry(std::move(actual_entry)),
         control(std::move(actual_control)),button(std::move(actual_button)),label(std::move(actual_label)),
         factory(std::move(actual_factory)),run(std::move(actual_run)),receiver(std::move(actual_receiver)),
         reached_layer(std::move(actual_layer)),serial(actual_serial),base_sequence(actual_base_sequence),
         child_sequence(actual_child_sequence),constructor_serial(actual_constructor_serial),
         ordinal(actual_ordinal),parser_layer(actual_parser_layer){}
};

void BnwWorldViewControlTree::require_normal_button_caption_call_current_without_callbacks(
    const detail::BnwNormalButtonCaptionCall& offered){
    using Call=detail::BnwNormalCommonChildCall;
    using Child=detail::NotificationLayoutEmbeddedLabelState;
    using Fields=Child::Impl;
    const auto saved=offered.impl_;
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(saved&&saved->creator==std::this_thread::get_id()&&saved->sealed&&
        same(saved->canonical_self.lock(),saved),
        "Caption178 pending owner is foreign/unsealed/noncanonical before borrow");
    // Genuine CURRENT enclosing dispatcher/Root is FIRST. Strong physical
    // originals never make a retired Common call CURRENT again.
    require_normal_common_child_call_current_without_callbacks(saved->enclosing_call);
    const auto common=saved->enclosing_call.impl_;
    require(common->kind==Call::Kind::button&&common->phase==Call::Phase::button_caption178&&
        (common->physical_flags&0x08000000u)&&
        common->serial==saved->serial&&common->base_sequence==saved->base_sequence&&
        common->ordinal==saved->ordinal&&same(common->original_button,saved->button)&&
        same(common->receiver,saved->receiver)&&common->original_caption_call_control&&
        same(common->original_caption_call_control->lock(),saved),
        "Caption178 is not SAME current Button continuation after Common56");
    const auto traversal=common->traversal.lock();const auto execution=traversal->execution.lock();
    saved->root.require_same_root_without_callbacks(execution->root);
    saved->entry.require_same_entry_without_callbacks(traversal->entry);
    const auto history=saved->root.impl_;const auto tree=history->tree;const auto owner=tree->core_;
    owner->layout_origin(true);saved->entry.require_same_tree_without_callbacks(*tree);
    require(owner->busy&&same(saved->control.owner_.lock(),owner)&&
        saved->control.id_==common->control.id_&&owner->checked(saved->control)==common->control.id_,
        "Caption178 requires SAME live canonical parent Control and entered Root scope");
    const auto& current=owner->source->current_operation_loan();
    current.require_origin_without_callbacks();
    require(&current==history->operation&&current.operation_serial()==saved->serial,
        "Caption178 canonical Source changed before saved operands");
    const auto& node=Core::node(owner->nodes,common->control.id_);
    require(node.layout_baseline&&node.alive&&!node.dynamic,
        "Caption178 current physical Button baseline retired");
    const auto& baseline=*node.layout_baseline;
    require(baseline.sequence_==saved->base_sequence&&same(baseline.button_,saved->button)&&
        same(baseline.base_.receiver_owned,saved->receiver)&&baseline.parsed_source_serial_&&
        saved->button->child_178_observation&&
        *saved->button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::factory_child&&
        !saved->button->pending_label_178_&&!saved->button->label_retirement_&&
        same(saved->button->label_178_,saved->label)&&same(saved->button->label_178_factory_return_,saved->factory),
        "Caption178 positive factory publication/actual parent membership changed");
    const auto factory=saved->factory->impl_;const auto fields=saved->label->impl_;
    fields->first();
    require(factory&&factory->creator==saved->creator&&factory->alive.load()&&factory->published_sequence&&
        factory->source_serial==saved->constructor_serial&&factory->layer==saved->parser_layer&&
        same(factory->child,saved->label)&&same(factory->parent.lock(),saved->button)&&
        same(factory->module,fields->module)&&fields->source_serial==saved->constructor_serial&&
        fields->actual_layer==saved->parser_layer&&fields->entered&&!fields->busy&&
        fields->retirement==Fields::Retirement::none&&fields->sequence==saved->child_sequence&&
        fields->init_stage==Fields::InitStage::bounds_returned&&
        same(fields->construction_parent.lock(),saved->button)&&same(fields->completed_run,saved->run),
        "Caption178 actual constructor/module/initializer/completed Run changed");
    const auto* vptr=std::get_if<Fields::Vptr>(&fields->fields[0]);
    require(vptr&&*vptr==Fields::Vptr::label_a48dd0,
        "Caption178 actual returned Label subtype18 is not genuine A48DD0");
    require(saved->parser_layer<baseline.parser_layers_.size()&&
        baseline.parser_layers_[saved->parser_layer].name==saved->reached_layer.name&&
        baseline.parser_layers_[saved->parser_layer].attributes==saved->reached_layer.attributes&&
        std::any_of(saved->reached_layer.attributes.begin(),saved->reached_layer.attributes.end(),
            [](const auto& attribute){return attribute.first=="String";}),
        "Caption178 is not actual retained String PRESENT parser layer");
    // No expired Runtime/String/Initialization loan is called. This checks
    // immutable actual allocation ancestry and CURRENT live physical storage.
    fields->module->require_label_live_allocation_without_callbacks(
        current,factory->allocation,saved->label,saved->constructor_serial);
    const auto run=saved->run->impl_;
    require(run&&run->creator==saved->creator&&run->alive.load()&&run->node_id==common->control.id_&&
        same(run->core.lock(),owner)&&same(run->child.lock(),saved->label)&&
        same(run->parent.lock(),saved->button)&&same(run->factory.lock(),saved->factory)&&
        same(run->module,fields->module)&&same(run->allocation,factory->allocation)&&
        run->constructor_serial==saved->constructor_serial&&run->child_sequence==saved->child_sequence&&
        run->parent_parser_serial==*baseline.parsed_source_serial_&&run->topology==owner->topology_revision,
        "Caption178 requires SAME genuine post-Button completed embedded Run");
    run->require_published_data_without_callbacks();
    run->validate_child_without_callbacks(current,saved->label);
    const auto* parent=std::get_if<Fields::ParentBorrow>(&fields->fields[0x38/4]);
    const auto* realm=std::get_if<Fields::ParsedRealm>(&fields->fields[0x3c/4]);
    require(parent&&parent->baseline==&baseline&&same(parent->parent.lock(),saved->button)&&
        realm&&realm->value&&baseline.base_.context_realm_3c&&
        baseline.base_.context_3c_observation==detail::NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
        same(realm->value,baseline.base_.context_realm_3c)&&same(owner->context_realm_identity,realm->value),
        "Caption178 actual initialized parent38/Realm3C association changed");
    const auto realm_control=realm->value->impl_;
    require(realm_control&&realm_control->creator==saved->creator&&realm_control->alive.load()&&
        !realm_control->failed.load()&&same(realm_control->context.lock(),owner)&&
        same(realm_control->source.lock(),owner->source)&&
        same(realm_control->constructor,owner->context_construction_return),
        "Caption178 actual entered Realm physical health/custody changed");
    // PURE terminal cuts; no callback/allocation or current-stamp renewal.
    fields->first();current.require_origin_without_callbacks();owner->layout_origin(true);
    saved->entry.require_current_without_callbacks();saved->root.require_physical_current_without_callbacks();
    require_normal_common_child_call_current_without_callbacks(saved->enclosing_call);
}

void BnwWorldViewControlTree::require_normal_button_caption_call_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalButtonCaptionCall& call){
    require_normal_button_caption_call_current_without_callbacks(call);
    require_normal_common_child_call_source_owned(operation,call.impl_->enclosing_call);
    require_normal_button_caption_call_current_without_callbacks(call);
    const auto run=call.impl_->run->impl_;
    run->font->require_source_owned(operation);require_normal_button_caption_call_current_without_callbacks(call);
    if(run->system){run->system->require_source_owned(operation);require_normal_button_caption_call_current_without_callbacks(call);}
    require_normal_common_child_call_source_owned(operation,call.impl_->enclosing_call);
    require_normal_button_caption_call_current_without_callbacks(call);
}

detail::BnwNormalButtonCaptionCall BnwWorldViewControlTree::capture_normal_common_button_caption_call_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& actual_call)const{
    using Caption=detail::BnwNormalButtonCaptionCall;using Call=detail::BnwNormalCommonChildCall;
    // Own SAME original value before any callbackful FULL operation.
    const auto call=actual_call;
    const auto first=[&]{require_normal_common_child_call_current_without_callbacks(call);
        const auto actual_owner=call.impl_->control.owner_.lock();
        const auto actual_traversal=call.impl_->traversal.lock();
        const auto actual_execution=actual_traversal->execution.lock();
        require(call.impl_->kind==Call::Kind::button&&call.impl_->phase==Call::Phase::button_caption178&&
            (call.impl_->physical_flags&0x08000000u)&&
            actual_owner&&actual_owner.get()==core_.get()&&!actual_owner.owner_before(core_)&&
            !core_.owner_before(actual_owner)&&actual_execution->root.impl_->tree.get()==this,
            "Caption178 capture requires this actual Button caption continuation");};
    const auto full=[&]{first();require_normal_common_child_call_source_owned(operation,call);first();};
    first();full();first();const auto common=call.impl_;const auto traversal=common->traversal.lock();
    const auto execution=traversal->execution.lock();const auto root=execution->root;
    const auto entry=traversal->entry;const auto control=common->control;const auto button=common->original_button;
    require(!common->original_caption_call_control&&button&&button->label_178_&&
        button->label_178_factory_return_&&!button->pending_label_178_&&!button->label_retirement_,
        "Caption178 original positive factory allocation absent or call already issued");
    const auto label=button->label_178_;const auto factory=button->label_178_factory_return_;
    const auto fields=label->impl_;fields->first();const auto run=fields->completed_run;
    require(run&&factory->impl_&&factory->impl_->published_sequence&&
        factory->impl_->layer<Core::node(core_->nodes,control.id_).layout_baseline->parser_layers_.size(),
        "Caption178 initializer/run or actual parser occurrence has not returned");
    const auto parser_layer=factory->impl_->layer;
    // Actual retained source layer, not a loose XML name or caller DTO.
    content::XmlNode reached=Core::node(core_->nodes,control.id_).layout_baseline->parser_layers_[parser_layer];
    first();full();first();fields->first();
    require(!common->original_caption_call_control&&button->label_178_==label&&
        button->label_178_factory_return_==factory&&fields->completed_run==run,
        "Caption178 source copy FULL POST changed original allocation/call");
    const auto child_sequence=fields->sequence;const auto constructor_serial=fields->source_serial;
    auto owned=std::make_shared<Caption::Impl>(call,root,entry,control,button,label,factory,run,common->receiver,
        std::move(reached),common->serial,common->base_sequence,child_sequence,constructor_serial,
        common->ordinal,parser_layer);
    first();full();first();fields->first();
    require(!common->original_caption_call_control&&fields->sequence==child_sequence&&
        fields->source_serial==constructor_serial&&button->label_178_==label&&
        button->label_178_factory_return_==factory&&fields->completed_run==run,
        "Caption178 allocating capture POST changed physical child/source");
    const Caption::IssuerKey key;const Caption result(owned,key);
    // No callback/allocation between once-only stores. This is a pending
    // CURRENT identity, not a successful caption body/whole Root return.
    owned->canonical_self=owned;owned->sealed=true;
    common->original_caption_call_control=std::weak_ptr<const Caption::Impl>(owned);
    require_normal_button_caption_call_current_without_callbacks(result);
    require_normal_button_caption_call_source_owned(operation,result);
    require_normal_button_caption_call_current_without_callbacks(result);return result;
}

detail::BnwNormalButtonCaptionCall::BnwNormalButtonCaptionCall(
    std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
void detail::BnwNormalButtonCaptionCall::require_current_without_callbacks()const{
    BnwWorldViewControlTree::require_normal_button_caption_call_current_without_callbacks(*this);
}
void detail::BnwNormalButtonCaptionCall::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    BnwWorldViewControlTree::require_normal_button_caption_call_source_owned(operation,*this);
}
BnwWorldViewControlTree::Control detail::BnwNormalButtonCaptionCall::actual_control_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->control;require_current_without_callbacks();return result;
}
detail::BnwNormalRootContextEntry detail::BnwNormalButtonCaptionCall::captured_root_context_entry_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->entry;require_current_without_callbacks();return result;
}
detail::BnwNormalRootBatchHistory::Guard detail::BnwNormalButtonCaptionCall::captured_root_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->root;require_current_without_callbacks();return result;
}
std::size_t detail::BnwNormalButtonCaptionCall::actual_ordered_ordinal_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->ordinal;require_current_without_callbacks();return result;
}
std::shared_ptr<const detail::BnwPositionedLabelRunLoan>
detail::BnwNormalButtonCaptionCall::positioned_run_for_actual_caption_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->run;require_current_without_callbacks();return result;
}
