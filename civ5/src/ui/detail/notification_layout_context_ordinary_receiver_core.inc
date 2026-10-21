// Inside civ5::ui after Core/constructor/ordinary storage. Constructor projection ONLY.
// No canonical id0 parser hook. Root chooses a separate genuine cold constructor
// caller; this entry never creates518, RootRead, v64,78, attach or fullContext.
struct detail::NotificationLayoutContextOrdinaryState::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::shared_ptr<const NotificationLayoutContextConstructionReturn> constructor;
    const std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    const std::uint64_t serial;
    std::shared_ptr<const NotificationContextNativeConstructorPrefix> native_constructor_prefix;
    std::shared_ptr<const NotificationContextNativeWideStringValue> b4_value;
    std::shared_ptr<const NotificationContextNativeWideStringValue> file_510_value;
    //Live514 is distinct from the immutable completed constructor snapshot.
    std::shared_ptr<const NotificationContextNativeWideStringValue> file_514_value;
    std::uint64_t file_514_revision=0;
    struct LuaName514PrefixReturn final {
        enum class Effect {copy_current_b4,append_lua};
        const std::thread::id creator=std::this_thread::get_id();
        const std::weak_ptr<const NotificationContextRootReadCompletionSlot> read;
        const std::shared_ptr<const NotificationContextNativeWideStringValue> b4,before,copied,value;
        const std::uint64_t source_scope,serial,b4_revision,topology,core_effects,core_sequence,root_sequence;
        const std::array<Effect,2> effects;
        LuaName514PrefixReturn(std::shared_ptr<const NotificationContextRootReadCompletionSlot> r,
            std::shared_ptr<const NotificationContextNativeWideStringValue> b,
            std::shared_ptr<const NotificationContextNativeWideStringValue> old,
            std::shared_ptr<const NotificationContextNativeWideStringValue> copy,
            std::shared_ptr<const NotificationContextNativeWideStringValue> result,
            std::uint64_t scope,std::uint64_t s,std::uint64_t br,std::uint64_t t,std::uint64_t e,
            std::uint64_t q,std::uint64_t rq,std::array<Effect,2> ordered)
            :read(std::move(r)),b4(std::move(b)),before(std::move(old)),copied(std::move(copy)),value(std::move(result)),
             source_scope(scope),serial(s),b4_revision(br),topology(t),core_effects(e),core_sequence(q),root_sequence(rq),effects(ordered){}
    };
    std::shared_ptr<const LuaName514PrefixReturn> lua_name514_prefix_return;
    std::shared_ptr<BnwWorldViewControlTree::ContextRootWatcherCustody> file_watcher_b8;
    //Actual2E4 field is separately driven by real postRead71C790, never XMLB8 alias.
    std::shared_ptr<BnwWorldViewControlTree::ContextRootWatcherCustody> file_watcher_2e4;
    struct Lua514PathReturn final {
        struct Temporary final {
            const std::thread::id creator=std::this_thread::get_id();
            const std::shared_ptr<const assets::detail::CivisLoadedBytesPhysicalPathLoan> physical;
            const std::shared_ptr<const NotificationContextNativeWideStringValue> value;
            const std::array<char16_t,260> fixed;
            const std::nullptr_t reference214=nullptr,reference20c=nullptr;
            const std::int32_t append_result,canonical_result;
            Temporary(std::shared_ptr<const assets::detail::CivisLoadedBytesPhysicalPathLoan> p,
                std::shared_ptr<const NotificationContextNativeWideStringValue> v,std::array<char16_t,260> f,std::int32_t a,std::int32_t c)
                :physical(std::move(p)),value(std::move(v)),fixed(std::move(f)),append_result(a),canonical_result(c){}
        };
        const std::thread::id creator=std::this_thread::get_id();
        const std::weak_ptr<const NotificationContextRootReadCompletionSlot> read;
        const std::shared_ptr<const LuaName514PrefixReturn> prefix;
        const std::shared_ptr<const Temporary> temporary;
        const std::shared_ptr<const assets::detail::CivisLoadedBytesPhysicalPathLoan> physical;
        const std::shared_ptr<const assets::Bytes> bytes;
        const runtime::FileIdentity identity;
        const std::shared_ptr<const NotificationContextNativeWideStringValue> value;
        const std::shared_ptr<BnwWorldViewControlTree::ContextRootWatcherCustody> watcher;
        const std::uint64_t scope,serial,topology,effects,sequence,root_sequence;
        const std::string query;
        Lua514PathReturn(std::shared_ptr<const NotificationContextRootReadCompletionSlot> r,
            std::shared_ptr<const LuaName514PrefixReturn> before,std::shared_ptr<const Temporary> t,
            std::shared_ptr<const assets::detail::CivisLoadedBytesPhysicalPathLoan> p,
            std::shared_ptr<const assets::Bytes> b,runtime::FileIdentity id,
            std::shared_ptr<const NotificationContextNativeWideStringValue> v,
            std::shared_ptr<BnwWorldViewControlTree::ContextRootWatcherCustody> w,
            std::uint64_t sc,std::uint64_t s,std::uint64_t topo,std::uint64_t fx,std::uint64_t q,std::uint64_t rq,std::string key)
            :read(std::move(r)),prefix(std::move(before)),temporary(std::move(t)),physical(std::move(p)),bytes(std::move(b)),
             identity(std::move(id)),value(std::move(v)),watcher(std::move(w)),scope(sc),serial(s),topology(topo),effects(fx),
             sequence(q),root_sequence(rq),query(std::move(key)){}
    };
    std::shared_ptr<const Lua514PathReturn> lua514_path_return;
    std::shared_ptr<const NotificationContextPostreadGlobalsReference> postread_globals_reference;
    std::shared_ptr<const NotificationContextPostreadRootCDataReturn> postread_root_cdata_return;
    std::shared_ptr<BnwWorldViewControlTree::ContextRootXml84> xml84_current;
    std::uint64_t b4_revision=0;
    std::shared_ptr<const NotificationLayoutContextRealmRef> wrapper_context_realm_3c;
    std::weak_ptr<const NotificationLayoutOrdinaryReceiver> actual_parent_38;
    std::shared_ptr<const NotificationContextNativeConstructorReturn> native_constructor_return;
    //Unknown before the true completed native constructor. The live fields
    //are distinct from immutable prefix/suffix NIL LuaRef snapshots.
    std::optional<std::array<std::shared_ptr<const void>,3>> lua_functions_90_51c_524;
    template<class T>static bool same(const std::shared_ptr<T>& a,const std::shared_ptr<T>& b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    }
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> actual,
        std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> actual_source,
        std::shared_ptr<const NotificationLayoutContextConstructionReturn> ctor,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> allocation,
        std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> xml,std::uint64_t s)
        :core(std::move(actual)),source(std::move(actual_source)),constructor(std::move(ctor)),occurrence(std::move(allocation)),
         selected(std::move(xml)),serial(s){}
    void first_constructor_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "Context Base projection retired/foreign/failed FIRST");
        const auto actual=core.lock();const auto owner=source.lock();
        require(actual&&owner,"Context Base projection actual allocation/Source retired");
        actual->layout_origin();require(&actual->source->current_operation_loan()==&operation,
            "Context constructor canonical operation replaced before old raw loan access");
        operation.require_origin_without_callbacks();
        require(constructor&&occurrence&&selected,"Context Base projection missing actual constructor/selected XML");
        const auto selected_source=selected->source.lock();
        require(same(actual->context_construction_return,constructor)&&same(actual->source,owner)&&
            selected_source&&same(selected_source,owner)&&same(actual->root_full_xml_occurrence,selected)&&
            !selected->dynamic&&selected->parent.expired()&&occurrence->id==0&&occurrence->instance==0&&
            occurrence->constructor_serial==serial&&serial&&serial==operation.operation_serial()&&
            actual->context_layout_baseline&&!actual->context_layout_baseline->parsed_source_serial_&&
            &actual->source->current_operation_loan()==&operation,
            "Context Base projection constructor/XML/operation changed before cold borrower");
        actual->layout_origin();require(&actual->source->current_operation_loan()==&operation,
            "Context constructor canonical operation replaced before old raw loan access");
        operation.require_origin_without_callbacks();
    }
};
detail::NotificationLayoutContextOrdinaryState::NotificationLayoutContextOrdinaryState(std::shared_ptr<Impl> value)
    :impl_(std::move(value)){}
void detail::NotificationLayoutContextOrdinaryState::retire_owned()const noexcept{
    impl_->alive.store(false);
    if(impl_->native_constructor_prefix)impl_->native_constructor_prefix->retire_owned();
    if(impl_->native_constructor_return)impl_->native_constructor_return->retire_owned(impl_->b4_value);
}
detail::NotificationLayoutContextOrdinaryState::~NotificationLayoutContextOrdinaryState(){retire_owned();}

void BnwWorldViewControlTree::publish_context_ordinary_receiver_projection_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using State=detail::NotificationLayoutContextOrdinaryState;
    using Receiver=detail::NotificationLayoutOrdinaryReceiver;
    const auto actual=core_;actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation,
        "Context constructor entry canonical operation differs before raw loan access");
    operation.require_origin_without_callbacks();
    const auto constructor=actual->context_construction_return;
    const auto source=actual->source;
    const auto xml=actual->root_full_xml_occurrence;
    require(constructor&&constructor->impl_&&xml,"Context projection genuine constructor/Source XML absent");
    const auto serial=constructor->impl_->constructor_source_serial;
    require(bool(actual->context_layout_baseline),"Context Base projection actual Base absent");
    const auto*const baseline=&*actual->context_layout_baseline;
    const auto sequence=baseline->sequence_;const auto effects=actual->effect_revision;
    const auto topology=actual->topology_revision;
    const auto current=[&]{
        actual->layout_origin();require(&actual->source->current_operation_loan()==&operation,
            "Context constructor canonical operation replaced before old raw loan access");
        operation.require_origin_without_callbacks();
        require(State::Impl::same(actual->context_construction_return,constructor)&&
            State::Impl::same(actual->source,source)&&
            State::Impl::same(actual->root_full_xml_occurrence,xml)&&!xml->dynamic&&xml->parent.expired()&&
            actual->context_layout_baseline&& &*actual->context_layout_baseline==baseline&&
            actual->context_layout_baseline->sequence_==sequence&&actual->effect_revision==effects&&
            actual->topology_revision==topology&&!actual->context_layout_baseline->parsed_source_serial_&&
            actual->context_layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::context&&
            constructor->impl_->constructor_state.kind_==detail::NotificationLayoutBaseline::Kind::context&&
            constructor->impl_->constructor_state.sequence_==1&&constructor->impl_->constructor_state.history_.size()==1&&
            constructor->impl_->constructor_state.history_.front().command==detail::NotificationLayoutBaseline::Mutation::construct&&
            constructor->impl_->constructor_state.history_.front().sequence==1&&serial&&serial==operation.operation_serial()&&
            &actual->source->current_operation_loan()==&operation&&!actual->context_ordinary_state&&
            !actual->context_ordinary_occurrence&&!actual->layout_parser_control_id&&!actual->layout_parser_borrower&&
            !actual->layout_parser_occurrence&&!actual->context_receiver_occurrence&&!actual->context_load_origin&&
            !actual->context_layout_baseline->base_.receiver_owned&&!actual->context_layout_baseline->base_.children_78&&
            !actual->context_layout_baseline->base_.common_return_owned,
            "Context Base projection replay/phase/constructor/Source changed");
    };
    const auto full=[&]{current();operation.require_current();current();};
    try{
        full();
        auto occurrence=std::make_shared<const Core::LayoutReceiverOccurrence>(0,0,serial);full();
        auto data=std::make_shared<State::Impl>(actual,source,constructor,occurrence,xml,serial);full();
        auto projection=std::shared_ptr<State>(new State(data));full();data->first_constructor_owned(operation);
        auto fields=std::make_shared<Receiver::Impl>(actual,occurrence,projection,serial);full();
        auto receiver=std::shared_ptr<const Receiver>(new Receiver(fields));full();
        require(fields->context.lock()==projection&&fields->image.expired()&&fields->box.expired()&&
            fields->core.lock()==actual&&fields->occurrence.lock()==occurrence&&fields->id==0&&fields->serial==serial,
            "Context Base projection actual receiver allocation changed");
        // Nonallocating partial owner publication. No Context subtype/RootRead grant.
        fields->canonical=receiver;
        actual->context_ordinary_occurrence=occurrence;
        actual->context_ordinary_state=projection;
        actual->context_layout_baseline->base_.receiver_owned=receiver;
        const auto published=[&]{
            data->first_constructor_owned(operation);
            require(actual->context_layout_baseline&& &*actual->context_layout_baseline==baseline&&
                actual->context_layout_baseline->sequence_==sequence&&actual->effect_revision==effects&&
                actual->topology_revision==topology&&
                State::Impl::same(actual->context_ordinary_state,projection)&&
                State::Impl::same(actual->context_ordinary_occurrence,occurrence)&&
                State::Impl::same(actual->context_layout_baseline->base_.receiver_owned,receiver)&&
                State::Impl::same(fields->canonical.lock(),receiver)&&fields->context.lock()==projection,
                "Context Base projection physical publication replaced");
        };
        published();operation.require_current();published();
    }catch(...){actual->failed.store(true);throw;}
}
