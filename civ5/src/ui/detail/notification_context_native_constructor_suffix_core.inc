// Inside civ5::ui after Source6. Portable OWN40F9C0 suffix; no caller/order
// relocation, load name, VM518/setup, RootRead, children or cleanup-return grant.
namespace context_constructor_suffix_detail {
struct InactiveDigest final {
    const std::uint32_t handle_0=0xffffffff,handle_4=0xffffffff;
    const bool opened_8=false;
    const std::array<std::byte,0x208> buffer_c{}; // actual4F13F0 memset208
    const std::array<std::uint32_t,4> buffer_words{}; //208/20C/210/214
    const std::nullptr_t pointer_224=nullptr,pointer_228=nullptr;
};
struct NamedSentinel final {
    NamedSentinel* left=this;
    NamedSentinel* parent=this;
    NamedSentinel* right=this;
    const std::uint8_t color_14=1,is_nil_15=1;
    NamedSentinel()=default;
    NamedSentinel(const NamedSentinel&)=delete;
};
struct EmptyNamedMap {
    std::unique_ptr<NamedSentinel> sentinel;
    const std::uint32_t count=0;
    explicit EmptyNamedMap(std::unique_ptr<NamedSentinel> p):sentinel(std::move(p)){}
};
// OWN40F map588 keeps its real numeric cells separate from the immutable
// completed-constructor sentinel/count witness. Strong receiver cells preserve
// native non-NULL identity; expired weak external ownership is not NULL.
struct NamedReceiverMap588 final : EmptyNamedMap {
    using Cells=std::map<std::uint32_t,std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>>;
    Cells cells;
    std::uint64_t revision=0;
    explicit NamedReceiverMap588(std::unique_ptr<NamedSentinel> p):EmptyNamedMap(std::move(p)){}
};
struct ReachedSuffix final {
    enum class Effect {b4_empty,digest_b8,digest_2e4,wide510_empty,wide514_empty,
        null518,nil51c,nil524,nil52c,narrow534_empty,null538,colors53c,
        map57c,map588,assign534_empty_noop,return_this};
    std::array<Effect,16> effects;
    std::size_t count=0;
    std::shared_ptr<const detail::NotificationContextNativeWideStringValue> initial_b4,wide510,wide514;
    std::shared_ptr<const detail::NotificationContextNativeWideStringValue> retired_current_b4;
    std::optional<InactiveDigest> digest_b8,digest_2e4;
    std::optional<std::nullptr_t> constructor_null518;
    std::optional<context_constructor_prefix_detail::NilLuaRef> lua51c,lua524,lua52c;
    std::optional<EmptyNamedMap> map57c;
    std::optional<NamedReceiverMap588> map588;
    detail::NotificationLabelColorSetKnownBytes constructor_colors;
    ReachedSuffix* quarantine_next=nullptr;
    void record(Effect e){require(count<effects.size(),"Context constructor suffix journal exhausted");effects[count++]=e;}
    void require_completed()const {
        static constexpr std::array expected{Effect::b4_empty,Effect::digest_b8,Effect::digest_2e4,
            Effect::wide510_empty,Effect::wide514_empty,Effect::null518,Effect::nil51c,Effect::nil524,
            Effect::nil52c,Effect::narrow534_empty,Effect::null538,Effect::colors53c,
            Effect::map57c,Effect::map588,Effect::assign534_empty_noop,Effect::return_this};
        require(count==expected.size()&&effects==expected&&initial_b4&&wide510==initial_b4&&wide514==initial_b4&&
            digest_b8&&digest_2e4&&constructor_null518&&*constructor_null518==nullptr&&
            lua51c&&!lua51c->state&&lua51c->reference==-2&&lua524&&!lua524->state&&lua524->reference==-2&&
            lua52c&&!lua52c->state&&lua52c->reference==-2&&map57c&&map588&&
            map57c->sentinel&&map588->sentinel&&!constructor_colors[63],
            "Context constructor suffix physical fields/order/unknown byte63 changed");
        // This validates the completed constructor's original zero/sentinel
        // witness; later actual515120 writes are held by map588.cells/revision.
        for(const auto* map:std::array<const EmptyNamedMap*,2>{&*map57c,&*map588}){
            const auto* s=map->sentinel.get();require(!map->count&&s->left==s&&s->parent==s&&s->right==s&&
                s->color_14==1&&s->is_nil_15==1,"Context constructor named-map sentinel changed");
        }
    }
};
inline thread_local ReachedSuffix* retired_constructor_suffixes=nullptr;
inline void quarantine(std::unique_ptr<ReachedSuffix> port)noexcept{
    if(!port)return;auto* retained=port.release();retained->quarantine_next=retired_constructor_suffixes;
    retired_constructor_suffixes=retained;
}
}
struct detail::NotificationContextNativeConstructorReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> projection;
    const std::shared_ptr<const NotificationContextNativeConstructorPrefix> prefix;
    const std::uint64_t serial;
    std::unique_ptr<context_constructor_suffix_detail::ReachedSuffix> port;
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> actual,
        std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> state,
        std::shared_ptr<const NotificationContextNativeConstructorPrefix> before,std::uint64_t s)
        :core(std::move(actual)),source(std::move(owner)),projection(std::move(state)),prefix(std::move(before)),serial(s){}
    ~Impl(){alive.store(false);context_constructor_suffix_detail::quarantine(std::move(port));}
    void first_constructor_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "Context constructor suffix retired/foreign/failed FIRST");
        const auto actual=core.lock();const auto owner=source.lock();const auto state=projection.lock();
        require(actual&&owner&&state&&prefix&&prefix->impl_,"Context constructor suffix actual owner retired");
        prefix->impl_->first(operation);prefix->impl_->port->require_reached();
        require(&actual->source->current_operation_loan()==&operation,
            "Context constructor suffix canonical operation changed before raw access");
        operation.require_origin_without_callbacks();actual->layout_origin();
        require(serial&&serial==operation.operation_serial()&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,state)&&
            NotificationLayoutContextOrdinaryState::Impl::same(state->impl_->native_constructor_prefix,prefix),
            "Context constructor suffix allocation/Source/operation changed");
    }
};
detail::NotificationContextNativeConstructorReturn::NotificationContextNativeConstructorReturn(std::shared_ptr<Impl> value)
    :impl_(std::move(value)){}
void detail::NotificationContextNativeConstructorReturn::retire_owned(
    std::shared_ptr<const NotificationContextNativeWideStringValue> b4)const noexcept{
    impl_->alive.store(false);if(impl_->port)impl_->port->retired_current_b4=std::move(b4);
}
detail::NotificationContextNativeConstructorReturn::~NotificationContextNativeConstructorReturn(){impl_->alive.store(false);}

void BnwWorldViewControlTree::construct_context_native_suffix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Return=detail::NotificationContextNativeConstructorReturn;
    using Wide=detail::NotificationContextNativeWideStringValue;
    using Port=context_constructor_suffix_detail::ReachedSuffix;
    const auto actual=core_;actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation,
        "Context constructor entry canonical operation differs before raw loan access");
    operation.require_origin_without_callbacks();
    const auto owner=actual->source;const auto state=actual->context_ordinary_state;
    require(owner&&state&&state->impl_,"Context constructor suffix requires genuine physical projection");
    const auto prefix=state->impl_->native_constructor_prefix;
    require(prefix&&prefix->impl_&&prefix->impl_->port,"Context constructor suffix requires reached XMLAC prefix");
    prefix->impl_->first(operation);prefix->impl_->port->require_reached();
    const auto serial=operation.operation_serial();
    // Capture actual already-owned narrow534/Font538/Color53C constructor fields
    // before any allocator. They are existing genuine Core ctor ports, not new
    // Root XML/default snapshots or guessed native DWORD15.
    const auto colors=actual->context_default_font_style.colors_53c;
    const auto defaults_current=[&]{
        const auto& d=actual->context_default_font_style;
        require(d.creator==std::this_thread::get_id()&&d.allocation==actual.get()&&d.value.empty()&&
            !d.font_538&&!d.reading&&!d.read_source_serial&&d.read_occurrence.expired()&&d.effect_count==3&&
            d.effects[0]==Core::ContextDefaultFontStyle::Effect::ctor_style_empty&&
            d.effects[1]==Core::ContextDefaultFontStyle::Effect::ctor_font_null&&
            d.effects[2]==Core::ContextDefaultFontStyle::Effect::ctor_colors&&
            d.colors_53c==colors&&colors==Core::ContextDefaultFontStyle::construct_colors()&&!colors[63],
            "Context constructor genuine534/538/53C ctor field owner changed");
    };
    const auto current=[&]{
        prefix->impl_->first(operation);prefix->impl_->port->require_reached();defaults_current();
        require(detail::NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,state)&&
            detail::NotificationLayoutContextOrdinaryState::Impl::same(state->impl_->native_constructor_prefix,prefix)&&
            !state->impl_->native_constructor_return&&!state->impl_->lua_functions_90_51c_524&&
            !state->impl_->b4_value&&!state->impl_->b4_revision&&
            !state->impl_->file_514_value&&!state->impl_->file_514_revision&&!state->impl_->lua_name514_prefix_return,
            "Context constructor suffix replay/early name store/replacement");
    };
    const auto full=[&]{current();operation.require_current();current();};
    std::shared_ptr<Return::Impl> data;
    try{
        full();data=std::make_shared<Return::Impl>(actual,owner,state,prefix,serial);full();
        data->port=std::make_unique<Port>();full();auto& port=*data->port;
        // Actual81F040 shared-empty semantic port, not the PE global address.
        auto wide=std::shared_ptr<const Wide>(new Wide());full();
        require(wide->value_.empty()&&*wide->terminated_data_without_callbacks()==u'\0',
            "Context constructor canonical-empty UTF16 value changed");
        port.initial_b4=wide;port.record(Port::Effect::b4_empty);
        port.digest_b8.emplace();port.record(Port::Effect::digest_b8);
        port.digest_2e4.emplace();port.record(Port::Effect::digest_2e4);
        port.wide510=wide;port.record(Port::Effect::wide510_empty);
        port.wide514=wide;port.record(Port::Effect::wide514_empty);
        port.constructor_null518.emplace(nullptr);port.record(Port::Effect::null518);
        port.lua51c.emplace();port.record(Port::Effect::nil51c);
        port.lua524.emplace();port.record(Port::Effect::nil524);
        port.lua52c.emplace();port.record(Port::Effect::nil52c);
        defaults_current();port.record(Port::Effect::narrow534_empty);
        port.record(Port::Effect::null538);port.constructor_colors=colors;port.record(Port::Effect::colors53c);
        auto map57=std::make_unique<context_constructor_suffix_detail::NamedSentinel>();full();
        port.map57c.emplace(std::move(map57));port.record(Port::Effect::map57c);
        auto map58=std::make_unique<context_constructor_suffix_detail::NamedSentinel>();full();
        port.map588.emplace(std::move(map58));port.record(Port::Effect::map588);
        // Actual81E890(0, literalempty, 0): alreadyempty534 skips overwrite.
        defaults_current();port.record(Port::Effect::assign534_empty_noop);
        data->first_constructor_owned(operation);full();port.record(Port::Effect::return_this);
        port.require_completed();
        auto returned=std::shared_ptr<const Return>(new Return(data));full();
        data->first_constructor_owned(operation);port.require_completed();
        state->impl_->b4_value=wide; // actual physical B4 custody; loadsetter separate
        state->impl_->file_510_value=wide; // constructor empty510 physical custody, snapshot stays immutable
        state->impl_->file_514_value=wide; // actual constructor-empty514, later copy/append owns its distinct live value
        state->impl_->lua_functions_90_51c_524.emplace(); // actual90/51C/524 NIL ctor publication
        state->impl_->native_constructor_return=returned;
        const auto published=[&]{
            data->first_constructor_owned(operation);port.require_completed();defaults_current();
            require(detail::NotificationLayoutContextOrdinaryState::Impl::same(
                state->impl_->native_constructor_return,returned)&&state->impl_->lua_functions_90_51c_524&&
                std::all_of(state->impl_->lua_functions_90_51c_524->begin(),state->impl_->lua_functions_90_51c_524->end(),
                    [](const auto& field){return !field;})&&
                detail::NotificationLayoutContextOrdinaryState::Impl::same(state->impl_->b4_value,wide)&&
                !state->impl_->b4_revision&&detail::NotificationLayoutContextOrdinaryState::Impl::same(
                    state->impl_->file_514_value,wide)&&!state->impl_->file_514_revision&&!state->impl_->lua_name514_prefix_return,
                "Context constructor suffix actual field/return replaced");
        };
        published();operation.require_current();published();
        // Constructor return has completed; liveLoadName/518/preReadsetup/RootRead
        // and mandatory Context destruction are deliberately separate owners.
    }catch(...){if(data)data->failed.store(true);actual->failed.store(true);throw;}
}
