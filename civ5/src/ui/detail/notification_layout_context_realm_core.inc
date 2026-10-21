// Inside civ5::ui, AFTER complete Core/GuiPort/RuntimeDimensions::State.
// Ordinary construction7AA012 supplies actual Context THIS to529520, whose
// second parameter reaches vslot44. Root/id0 caller authority is UNISSUED.
struct detail::NotificationLayoutContextRealmRef::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<BnwWorldViewControlTree::Core> context;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::shared_ptr<const NotificationLayoutContextConstructionReturn> constructor;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual)
        :context(actual),source(actual->source),constructor(actual->context_construction_return){}
    // RootRead7AA012 actual parentTHIS supplier. All values captured before
    // allocation; old ordinary constructor remains byte-preserved above.
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> actual,
         std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
         std::shared_ptr<const NotificationLayoutContextConstructionReturn> ctor)
        :context(std::move(actual)),source(std::move(owner)),constructor(std::move(ctor)){}
    void first(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
        // FIRST: independently retired identity before weak owner/source access.
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "Context realm identity retired/foreign/failed");
        const auto actual=context.lock();const auto original_source=source.lock();
        require(actual&&original_source,"Context realm allocation/Source retired");
        actual->layout_origin(true);operation.require_origin_without_callbacks();
        require(constructor&&actual->context_construction_return.get()==constructor.get()&&
            !constructor.owner_before(actual->context_construction_return)&&
            !actual->context_construction_return.owner_before(constructor)&&
            actual->source.get()==original_source.get()&&
            !actual->source.owner_before(original_source)&&!original_source.owner_before(actual->source)&&
            &actual->source->current_operation_loan()==&operation,
            "Context realm actual allocation/constructor/Source changed");
        actual->layout_origin(true);operation.require_origin_without_callbacks();
    }
};
struct detail::NotificationLayoutContextRealmArgumentLoan::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const std::weak_ptr<NotificationLayoutRuntimeDimensions::State> parser;
    const std::shared_ptr<const NotificationLayoutContextRealmRef> realm;
    Impl(const std::shared_ptr<NotificationLayoutRuntimeDimensions::State>& actual,
         std::shared_ptr<const NotificationLayoutContextRealmRef> value)
        :parser(actual),realm(std::move(value)){}
};
detail::NotificationLayoutContextRealmRef::NotificationLayoutContextRealmRef(std::shared_ptr<Impl> p)
    :impl_(std::move(p)){}
detail::NotificationLayoutContextRealmRef::~NotificationLayoutContextRealmRef(){retire_owned();}
void detail::NotificationLayoutContextRealmRef::retire_owned()const noexcept{impl_->alive.store(false);}
detail::NotificationLayoutContextRealmArgumentLoan::NotificationLayoutContextRealmArgumentLoan(std::shared_ptr<Impl> p)
    :impl_(std::move(p)){}
detail::NotificationLayoutContextRealmArgumentLoan::~NotificationLayoutContextRealmArgumentLoan(){retire_owned();}
void detail::NotificationLayoutContextRealmArgumentLoan::retire_owned()const noexcept{impl_->alive.store(false);}

void detail::NotificationLayoutRuntimeDimensions::capture_ordinary_context_argument_owned()const {
    const auto saved=state_;require_storage_without_callbacks(saved);
    require(saved->id!=0&&!saved->context_realm_argument&&!saved->binding_context_realm,
        "Ordinary Context realm argument requires actual nonroot parser exactly once");
    saved->binding_context_realm=true;
    struct Exit{bool& value;~Exit(){value=false;}}exit{saved->binding_context_realm};
    try{
        saved->operation->require_current();require_storage_without_callbacks(saved);
        auto actual=saved->core.lock();require(bool(actual),"Ordinary realm Context allocation retired");
        require(actual->context_construction_return&&actual->context_layout_baseline&&
            saved->baseline->kind_!=NotificationLayoutBaseline::Kind::context,
            "Ordinary realm requires existing actual Context allocation and distinct ordinary receiver");
        // Lazy identity allocation is not a native ctor/store receipt. Core
        // retains this ONE identity until real Tree retirement, across parsers.
        if(!actual->context_realm_identity){
            auto fields=std::make_shared<NotificationLayoutContextRealmRef::Impl>(actual);
            require_storage_without_callbacks(saved);fields->first(*saved->operation);
            auto realm=std::shared_ptr<const NotificationLayoutContextRealmRef>(
                new NotificationLayoutContextRealmRef(std::move(fields)));
            require_storage_without_callbacks(saved);realm->impl_->first(*saved->operation);
            require(!actual->context_realm_identity,"Context realm identity replaced during allocation");
            actual->context_realm_identity=std::move(realm);
        }
        const auto realm=actual->context_realm_identity;realm->impl_->first(*saved->operation);
        auto fields=std::make_shared<NotificationLayoutContextRealmArgumentLoan::Impl>(saved,realm);
        require_storage_without_callbacks(saved);realm->impl_->first(*saved->operation);
        auto loan=std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan>(
            new NotificationLayoutContextRealmArgumentLoan(std::move(fields)));
        require_storage_without_callbacks(saved);realm->impl_->first(*saved->operation);
        require(!saved->context_realm_argument,"Ordinary parser argument replaced during allocation");
        saved->context_realm_argument=std::move(loan);
        actual.reset(); // Release this helper's local lock before full Source POST.
        saved->binding_context_realm=false;
        saved->operation->require_current();require_storage_without_callbacks(saved);
        require_context_argument_owned(saved,saved->context_realm_argument);
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
void detail::NotificationLayoutRuntimeDimensions::require_context_argument_owned(
    const std::shared_ptr<State>& saved,
    const std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan>& loan){
    require(loan&&loan->impl_->creator==std::this_thread::get_id()&&loan->impl_->alive.load(),
        "Ordinary Context parser argument retired/foreign FIRST");
    require_storage_without_callbacks(saved);
    const auto parser=loan->impl_->parser.lock();const auto actual=saved->core.lock();
    require(saved->id!=0&&!saved->binding_context_realm&&parser&&actual&&
        parser.get()==saved.get()&&!parser.owner_before(saved)&&!saved.owner_before(parser)&&
        saved->context_realm_argument.get()==loan.get()&&
        !saved->context_realm_argument.owner_before(loan)&&!loan.owner_before(saved->context_realm_argument)&&
        loan->impl_->realm&&actual->context_realm_identity.get()==loan->impl_->realm.get()&&
        !actual->context_realm_identity.owner_before(loan->impl_->realm)&&
        !loan->impl_->realm.owner_before(actual->context_realm_identity),
        "Ordinary parser second argument is not SAME retained Context identity");
    loan->impl_->realm->impl_->first(*saved->operation);
    require_storage_without_callbacks(saved);
}
std::shared_ptr<const detail::NotificationLayoutContextRealmArgumentLoan>
detail::NotificationLayoutRuntimeDimensions::context_realm_argument_owned()const {
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto result=saved->context_realm_argument;require_context_argument_owned(saved,result);
    require(saved->baseline->parser_runtime_dimensions_==this&&!saved->baseline->parsed_source_serial_,
        "Context argument borrower outside current actual canonical parser");
    return result;
}
void detail::NotificationLayoutRuntimeDimensions::assign_common_context_3c_owned(
    NotificationLayoutBaseline& destination,const content::XmlNode& reached_layer)const {
    const auto saved=state_;const auto loan=context_realm_argument_owned();
    require(saved->baseline==&destination&&destination.kind_!=NotificationLayoutBaseline::Kind::context&&
        !saved->assigning_context_3c,
        "4381AC requires distinct ordinary canonical Base and real common parser callsite");
    // The private Baseline common reader invokes this directly at4381AC. It is
    // not an attribute scanner/selector issuer and cannot be invoked publicly.
    require(std::any_of(destination.parser_layers_.begin(),destination.parser_layers_.end(),
        [&](const auto& p){return BnwWorldViewControlTree::Core::same_layout_xml(p,reached_layer);}),
        "4381AC reached common layer is not retained actual parser source");
    saved->assigning_context_3c=true;
    struct Exit{bool& value;~Exit(){value=false;}}exit{saved->assigning_context_3c};
    try{
        saved->operation->require_current();require_context_argument_owned(saved,loan);
        require_context_argument_owned(saved,loan);
        require(destination.sequence_<UINT64_MAX,"4381AC history exhausted before native store");
        require(destination.history_.size()<destination.history_.max_size(),
            "4381AC history capacity exhausted before native store");
        const auto before=destination.sequence_;destination.history_.reserve(destination.history_.size()+1);
        require_context_argument_owned(saved,loan);
        require(destination.sequence_==before&&saved->baseline==&destination,
            "4381AC actual Base changed across allocation");
        destination.record_owned(NotificationLayoutBaseline::Mutation::context_realm_3c);
        destination.base_.context_realm_3c=loan->impl_->realm;
        destination.base_.context_3c_observation=NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm;
        require_context_argument_owned(saved,loan);
        saved->operation->require_current();require_context_argument_owned(saved,loan);
        // Exact executed common3C write only. Not Label5E, parent38, +10 copy,
        // 703120 factoryreturn, Font, or full subtype/parser completion.
    }catch(...){fail_storage_without_callbacks(saved);throw;}
}
