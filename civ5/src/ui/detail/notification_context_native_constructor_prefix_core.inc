// Inside civ5::ui, AFTER the Source3 genuine Core/projection definitions.
// Own x64 memory ports preserve the reached constructor's pointer/sentinel
// relations. No x86 pointer is executed or presented as a live Civis object.
namespace context_constructor_prefix_detail {
struct EmptyXmlArena final {
    // Unwritten inline payload remains uninitialized; only its actual head/cursor
    // relations and capacity are admitted. No parsed XML or allocator callback.
    std::array<std::byte,0x400> inline_storage;
    void* first_0=nullptr;
    void* last_4=nullptr;
    void* nodes_10=nullptr;
    void* nodes_14=nullptr;
    void* nodes_18=nullptr;
    void* nodes_20=nullptr;
    EmptyXmlArena* current_30=this;
    std::uint32_t remaining_34=0x400;
    EmptyXmlArena* next_arena=nullptr;
    std::byte* inline_cursor=inline_storage.data()+8;
    // These typed functions implement own x64 storage, not native address aliases.
    static void* allocate(std::size_t n){return ::operator new(n);}
    static void release(void* p)noexcept{::operator delete(p);}
    const decltype(&allocate) allocate_440=&allocate;
    const decltype(&release) release_444=&release;
    EmptyXmlArena(){} // User-provided: do not value-initialize unwritten payload.
    EmptyXmlArena(const EmptyXmlArena&)=delete;
};
struct EmptyXmlDocument84 final {
    enum class Subtype {document_8346a0};
    const Subtype subtype=Subtype::document_8346a0;
    void* selected_4=nullptr;
    std::unique_ptr<EmptyXmlArena> arena_8;
    explicit EmptyXmlDocument84(std::unique_ptr<EmptyXmlArena> arena):arena_8(std::move(arena)){}
};
struct NilLuaRef final {
    void* state=nullptr;
    std::int32_t reference=-2;
};
struct InstanceSentinel final {
    InstanceSentinel* left=this;
    InstanceSentinel* parent=this;
    InstanceSentinel* right=this;
    const std::uint8_t color_18=1;
    const std::uint8_t is_nil_19=1;
    // No key/value payload exists for the sentinel. Do not read unspecified data.
    InstanceSentinel()=default;
    InstanceSentinel(const InstanceSentinel&)=delete;
};
struct EmptyInstanceMapA0 final {
    std::unique_ptr<InstanceSentinel> sentinel;
    const std::uint32_t constructor_count0=0; // immutable reached constructor history
    struct Cursor final {
        enum class Subtype {cursor_835540};
        const Subtype subtype=Subtype::cursor_835540;
        std::shared_ptr<const content::XmlNode> selected;
    };
    using Cells=std::map<std::uint32_t,Cursor>;
    Cells cells; // genuine mutable A0 selected XML cursor registry
    std::uint32_t count=0;
    std::uint64_t revision=0;
    explicit EmptyInstanceMapA0(std::unique_ptr<InstanceSentinel> p):sentinel(std::move(p)){}
};
struct EmptyXmlCursorAc final {
    enum class Subtype {cursor_835540};
    const Subtype subtype=Subtype::cursor_835540;
    void* selected_4=nullptr;
};
struct ReachedPort final {
    enum class Subtype {context_a368d0};
    enum class Effect {base_projection,context_subtype,xml84,lua90,lua98,instance_a0,xml_ac};
    std::array<Effect,7> effects;
    std::size_t count=0;
    std::optional<Subtype> subtype;
    std::optional<EmptyXmlDocument84> xml84;
    std::optional<NilLuaRef> lua90,lua98;
    std::optional<EmptyInstanceMapA0> instance_a0;
    std::optional<EmptyXmlCursorAc> xml_ac;
    ReachedPort* quarantine_next=nullptr;
    void record(Effect event){require(count<effects.size(),"Context constructor prefix journal exhausted");effects[count++]=event;}
    void require_reached()const {
        static constexpr std::array expected{Effect::base_projection,Effect::context_subtype,
            Effect::xml84,Effect::lua90,Effect::lua98,Effect::instance_a0,Effect::xml_ac};
        require(count==expected.size()&&effects==expected&&subtype==Subtype::context_a368d0&&xml84&&xml84->arena_8&&
            !xml84->selected_4&&lua90&&!lua90->state&&lua90->reference==-2&&
            lua98&&!lua98->state&&lua98->reference==-2&&instance_a0&&instance_a0->sentinel&&
            instance_a0->constructor_count0==0&&xml_ac&&!xml_ac->selected_4,
            "Context constructor reached-prefix physical fields/journal changed");
        const auto& arena=*xml84->arena_8;
        const auto* sentinel=instance_a0->sentinel.get();
        require(!arena.first_0&&!arena.last_4&&!arena.nodes_10&&!arena.nodes_14&&
            !arena.nodes_18&&!arena.nodes_20&&arena.current_30==&arena&&arena.remaining_34==0x400&&
            !arena.next_arena&&arena.inline_cursor==arena.inline_storage.data()+8&&
            sentinel->left==sentinel&&sentinel->parent==sentinel&&sentinel->right==sentinel&&
            sentinel->color_18==1&&sentinel->is_nil_19==1,
            "Context constructor owned XML arena/Instance sentinel relations changed");
    }
};
// No allocation, callback, fake active decrement, cleanup-return or pool reuse.
// This is explicit quarantine until the mandatory genuine Gui28::676800 and
// reached Context subtype retirement are closed by their own current owner.
inline thread_local ReachedPort* retired_constructor_prefixes=nullptr;
inline void quarantine(std::unique_ptr<ReachedPort> port)noexcept{
    if(!port)return;
    auto* retained=port.release();
    retained->quarantine_next=retired_constructor_prefixes;
    retired_constructor_prefixes=retained;
}
}
struct detail::NotificationContextNativeConstructorPrefix::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> projection;
    const std::shared_ptr<const NotificationLayoutContextConstructionReturn> constructor;
    const std::uint64_t serial;
    std::unique_ptr<context_constructor_prefix_detail::ReachedPort> port;
    Impl(std::shared_ptr<BnwWorldViewControlTree::Core> actual,
        std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> owner,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> state,
        std::shared_ptr<const NotificationLayoutContextConstructionReturn> base,std::uint64_t s)
        :core(std::move(actual)),source(std::move(owner)),projection(std::move(state)),
         constructor(std::move(base)),serial(s){}
    ~Impl(){alive.store(false);context_constructor_prefix_detail::quarantine(std::move(port));}
    void first(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "Context constructor prefix retired/foreign/failed FIRST");
        const auto actual=core.lock();const auto owner=source.lock();const auto state=projection.lock();
        require(actual&&owner&&state,"Context constructor prefix actual Source/projection retired");
        state->impl_->first_constructor_owned(operation);
        require(&actual->source->current_operation_loan()==&operation,
            "Context constructor prefix canonical operation changed before raw access");
        operation.require_origin_without_callbacks();actual->layout_origin();
        require(serial&&serial==operation.operation_serial()&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,state)&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->context_construction_return,constructor)&&
            &owner->current_operation_loan()==&operation,
            "Context constructor prefix actual allocation/Source/operation changed");
    }
};
detail::NotificationContextNativeConstructorPrefix::NotificationContextNativeConstructorPrefix(std::shared_ptr<Impl> value)
    :impl_(std::move(value)){}
void detail::NotificationContextNativeConstructorPrefix::retire_owned()const noexcept{impl_->alive.store(false);}
detail::NotificationContextNativeConstructorPrefix::~NotificationContextNativeConstructorPrefix(){retire_owned();}

void BnwWorldViewControlTree::construct_context_native_prefix_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Prefix=detail::NotificationContextNativeConstructorPrefix;
    using Port=context_constructor_prefix_detail::ReachedPort;
    const auto actual=core_;actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation,
        "Context constructor entry canonical operation differs before raw loan access");
    operation.require_origin_without_callbacks();
    // EVERY shared operand is copied under FIRST before the first allocation.
    const auto owner=actual->source;const auto state=actual->context_ordinary_state;
    const auto constructor=actual->context_construction_return;
    require(owner&&state&&state->impl_&&constructor,"Context constructor actual cold Base projection absent");
    state->impl_->first_constructor_owned(operation);
    const auto serial=operation.operation_serial();
    const auto current=[&]{
        state->impl_->first_constructor_owned(operation);
        require(detail::NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner)&&
            detail::NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,state)&&
            detail::NotificationLayoutContextOrdinaryState::Impl::same(actual->context_construction_return,constructor)&&
            !state->impl_->native_constructor_prefix,
            "Context constructor prefix replay/replacement");
    };
    const auto full=[&]{current();operation.require_current();current();};
    std::shared_ptr<Prefix::Impl> data;
    try{
        full();data=std::make_shared<Prefix::Impl>(actual,owner,state,constructor,serial);full();data->first(operation);
        data->port=std::make_unique<Port>();full();
        // Base projection already earned by Source3. This records custody only;
        // neither repeat4F9470 nor use it as a full40F9C0 completion return.
        auto& port=*data->port;
        port.record(Port::Effect::base_projection);
        port.subtype=Port::Subtype::context_a368d0;
        port.record(Port::Effect::context_subtype);
        auto arena=std::make_unique<context_constructor_prefix_detail::EmptyXmlArena>();full();
        port.xml84.emplace(std::move(arena));port.record(Port::Effect::xml84);
        port.lua90.emplace();port.record(Port::Effect::lua90);
        port.lua98.emplace();port.record(Port::Effect::lua98);
        auto sentinel=std::make_unique<context_constructor_prefix_detail::InstanceSentinel>();full();
        port.instance_a0.emplace(std::move(sentinel));port.record(Port::Effect::instance_a0);
        port.xml_ac.emplace();port.record(Port::Effect::xml_ac);
        port.require_reached();data->first(operation);full();
        auto returned=std::shared_ptr<const Prefix>(new Prefix(data));full();
        data->first(operation);port.require_reached();
        state->impl_->native_constructor_prefix=returned;
        const auto published=[&]{
            data->first(operation);port.require_reached();
            require(detail::NotificationLayoutContextOrdinaryState::Impl::same(
                state->impl_->native_constructor_prefix,returned),
                "Context constructor prefix actual publication replaced");
        };
        published();operation.require_current();published();
        // STOP frontier is next actual81F040/B4 string store. No fullCtor issuer,
        // RootRead, VM preparation, childvector or nativecleanup return is minted.
    }catch(...){if(data)data->failed.store(true);actual->failed.store(true);throw;}
}
