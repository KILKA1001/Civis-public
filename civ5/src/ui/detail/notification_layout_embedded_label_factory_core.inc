// Inside civ5::ui AFTER complete active String loan/Child/parent links.
struct detail::NotificationLayoutEmbeddedLabelFactoryReturn::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<NotificationLayoutRuntimeDimensions::State> parser;
    const std::weak_ptr<NotificationLayoutButtonState> parent;
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState> child;
    const std::shared_ptr<BnwLabelProcessModuleOwner> module;
    const std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> allocation;
    const std::size_t layer;
    const std::uint64_t source_serial;
    std::atomic<bool> alive{true};
    std::uint64_t published_sequence=0;
    Impl(std::shared_ptr<NotificationLayoutRuntimeDimensions::State> p,
        std::shared_ptr<NotificationLayoutButtonState> b,
        std::shared_ptr<NotificationLayoutEmbeddedLabelState> c,
        std::shared_ptr<BnwLabelProcessModuleOwner> m,
        std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> a,std::size_t index)
        :parser(std::move(p)),parent(b),child(std::move(c)),module(std::move(m)),
         allocation(std::move(a)),layer(index),source_serial(parser->serial){}
};
detail::NotificationLayoutEmbeddedLabelFactoryReturn::NotificationLayoutEmbeddedLabelFactoryReturn(
    std::shared_ptr<Impl> saved):impl_(std::move(saved)){}
detail::NotificationLayoutEmbeddedLabelFactoryReturn::~NotificationLayoutEmbeddedLabelFactoryReturn(){
    impl_->alive.store(false); //No successful subtype destruction/pool release grant.
}
void detail::NotificationLayoutRuntimeDimensions::require_label_factory_return_owned(
    const std::shared_ptr<State>& saved,
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>& returned,std::size_t index){
    require(returned&&returned->impl_&&returned->impl_->creator==std::this_thread::get_id()&&
        returned->impl_->alive.load(),"Label constructor return retired/foreign before parent access");
    require_storage_without_callbacks(saved);
    const auto state=returned->impl_;
    require(state->parser.get()==saved.get()&&!state->parser.owner_before(saved)&&!saved.owner_before(state->parser)&&
        state->layer==index&&state->source_serial==saved->serial,
        "Label factory return belongs to another actual parser/source");
    const auto parent=state->parent.lock();
    require(parent&&saved->baseline->button_&&parent.get()==saved->baseline->button_.get()&&
        !parent.owner_before(saved->baseline->button_)&&!saved->baseline->button_.owner_before(parent),
        "Label factory parent allocation changed");
    state->module->require_label_allocation_owned(*saved->operation,state->allocation,state->child,true);
    state->child->require_ownership_origin_owned(parent,*saved->operation,index);
    require_button_string_layer_owned(saved,saved->active_button_string_layer_,index);
    if(state->published_sequence){
        require(!parent->pending_label_178_&&parent->label_178_&&parent->label_178_.get()==state->child.get()&&
            !parent->label_178_.owner_before(state->child)&&!state->child.owner_before(parent->label_178_)&&
            parent->label_178_factory_return_&&parent->label_178_factory_return_.get()==returned.get()&&
            !parent->label_178_factory_return_.owner_before(returned)&&!returned.owner_before(parent->label_178_factory_return_)&&
            saved->baseline->sequence_==state->published_sequence,
            "Label178 published constructor return/store history changed");
    }else{
        require_pending_label_178_prefix_owned(saved,state->child.get(),index);
        require(!parent->label_178_&&!parent->label_178_factory_return_,"Label constructor return already published");
    }
    require_storage_without_callbacks(saved);
}
std::shared_ptr<const detail::NotificationLayoutEmbeddedLabelFactoryReturn>
detail::NotificationLayoutRuntimeDimensions::construct_embedded_label_factory_owned(std::size_t index)const{
    using Child=NotificationLayoutEmbeddedLabelState;using Return=NotificationLayoutEmbeddedLabelFactoryReturn;
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto reached=saved->active_button_string_layer_;require_button_string_layer_owned(saved,reached,index);
    const auto parent=saved->baseline->button_;const auto module=saved->operation->label_module_owned();
    require_button_string_layer_owned(saved,reached,index);
    std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> allocation;
    try{
        allocation=module->begin_label_fresh_allocation_owned(*saved->operation);
        require_button_string_layer_owned(saved,reached,index);
        auto loan=std::shared_ptr<Child::ConstructorLoan>(new Child::ConstructorLoan(saved,reached,parent,module,index));
        loan->first();
        auto child=Child::allocate_pending_owned(loan);loan->first();
        // Actual x64 allocation returned; native703157 accounting precedes ctor.
        module->finish_label_fresh_allocation_owned(*saved->operation,allocation,child);
        retain_pending_label_178_prefix_owned(saved,child,index);
        child->construct_literal_prefix_owned(loan); //full764170 OWN effects/actual RET
        loan->first();require_pending_label_178_prefix_owned(saved,child.get(),index);
        module->finish_label_constructor_return_owned(*saved->operation,allocation,child);
        require_pending_label_178_prefix_owned(saved,child.get(),index);
        // Only THIS returned real call path can issue the opaque capability.
        auto result=std::shared_ptr<const Return>(new Return(std::make_shared<Return::Impl>(
            saved,parent,child,module,allocation,index)));
        require_label_factory_return_owned(saved,result,index);return result;
    }catch(...){module->fail_label_allocation_owned(allocation);throw;}
}
void detail::NotificationLayoutRuntimeDimensions::publish_embedded_label_factory_owned(
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn>& returned,std::size_t index)const{
    const auto saved=state_;require_label_factory_return_owned(saved,returned,index);
    const auto stamp=returned->impl_;require(!stamp->published_sequence,"Label178 factory publication replay");
    const auto parent=stamp->parent.lock();const auto active=saved->active_button_string_layer_;
    // No callback occurs inside these actual stores. Copy owns the SAME child;
    // native178 publication is after constructor RETURN and before5E parsing.
    parent->label_178_=stamp->child;
    parent->label_178_factory_return_=returned;
    parent->pending_label_178_.reset();
    parent->child_178_observation=NotificationLayoutButtonState::Child178Observation::factory_child;
    NotificationLayoutBaseline* actual_baseline=nullptr;
    {const auto actual=saved->core.lock();require(actual&&saved->id&&saved->id<=actual->nodes.size()&&
        actual->nodes[saved->id-1].layout_baseline,"Label178 canonical mutable parent absent");
        actual_baseline=&*actual->nodes[saved->id-1].layout_baseline;
        require(actual_baseline==saved->baseline,"Label178 canonical allocation changed");}
    actual_baseline->record_owned(NotificationLayoutBaseline::Mutation::parser);
    stamp->published_sequence=actual_baseline->sequence_;
    active->published_child_=stamp->child;
    active->baseline_sequence_=stamp->published_sequence; //earned THIS store, not stale epoch repair
    require_label_factory_return_owned(saved,returned,index);
}
