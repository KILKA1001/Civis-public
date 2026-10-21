// Reused pure Source6/8 constructor memory models; no root Context receipt.
namespace tooltip_manager_value_prefix_detail {
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
}
namespace tooltip_manager_value_prefix_detail {
struct ConstructorWide {std::u16string value_;const char16_t*terminated_data_without_callbacks()const{return value_.c_str();}};
}
namespace tooltip_manager_value_suffix_detail {
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
    std::shared_ptr<const tooltip_manager_value_prefix_detail::ConstructorWide> initial_b4,wide510,wide514;
    std::shared_ptr<const tooltip_manager_value_prefix_detail::ConstructorWide> retired_current_b4;
    std::optional<InactiveDigest> digest_b8,digest_2e4;
    std::optional<std::nullptr_t> constructor_null518;
    std::optional<tooltip_manager_value_prefix_detail::NilLuaRef> lua51c,lua524,lua52c;
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
}
// Inside civ5::ui, after NotificationContextGuiPort::Impl. Separate actual
// allocation and constructor fields; no root Context completion is adopted.
struct detail::NotificationContextTooltipManagerValue::Impl final {
    using Gui=NotificationContextGuiPort::Impl;
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<Gui::TooltipLife> life=std::make_shared<Gui::TooltipLife>();
    const std::weak_ptr<Gui> gui;
    const std::uint64_t constructor_serial;
    std::unique_ptr<tooltip_manager_value_prefix_detail::ReachedPort> prefix;
    std::unique_ptr<tooltip_manager_value_suffix_detail::ReachedSuffix> suffix;
    std::optional<tooltip_manager_value_suffix_detail::EmptyNamedMap> constructor_594;
    std::optional<std::string> narrow534;
    std::optional<std::nullptr_t> font538;
    struct ObjectHeader {std::nullptr_t owner=nullptr,next=nullptr;std::uint32_t sentinel=0xffffffffu;};
    ObjectHeader base_0,component_5c;
    std::array<std::optional<std::uint32_t>,33> base_dwords;
    tooltip_manager_value_prefix_detail::NilLuaRef lua4c;
    const void*component_6c=nullptr;
    std::shared_ptr<Gui::TooltipKnownFields> known;
    // Actual loaded definitions/type-v64 receivers have no current producer.
    // The historical constructor-empty sentinel is not a current empty-registry.
    std::optional<std::uint64_t> loaded_type_registry_occurrence;
    enum class Step {allocated,base40,registry594,known_stores,receiver728020,returned};
    std::array<Step,6> history{};std::size_t count=0;
    explicit Impl(std::shared_ptr<Gui>g,std::uint64_t serial):gui(g),constructor_serial(serial){record(Step::allocated);}
    void record(Step s){require(count<history.size(),"TTManager constructor journal exhausted");history[count++]=s;}
    void first(const runtime::BnwWorldViewSourceOperationLoan&op)const{
        life->first(); // actual GUI::first below validates the same Source origin before any field use.
        const auto actual=gui.lock();require(actual!=nullptr,"TTManager actual GUI constructor owner retired");
        actual->first(op);life->first();require(creator==std::this_thread::get_id()&&constructor_serial,
            "TTManager actual constructor creator/source serial changed");
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan&op)const{first(op);op.require_current();first(op);}
    void construct_base40_owned(const runtime::BnwWorldViewSourceOperationLoan&op){
        first(op);require(count==1&&!prefix&&!suffix&&!constructor_594,"TTManager base constructor replay");
        //4100D0 twice, then4F9470's exact reached fields. Other DWORDs stay
        //disengaged; native vtables are identity metadata, never call targets.
        for(std::size_t i=4;i<=18;++i)base_dwords[i]=0;
        base_dwords[17]=0x3f800000u; //actual9D144C readonly float1 bits
        base_dwords[21]=0;base_dwords[22]=0;
        for(std::size_t i=28;i<=32;++i)base_dwords[i]=0;
        component_6c=&base_dwords[16];full(op);
        prefix=std::make_unique<tooltip_manager_value_prefix_detail::ReachedPort>();full(op);
        prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::base_projection);
        prefix->subtype=tooltip_manager_value_prefix_detail::ReachedPort::Subtype::context_a368d0;
        prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::context_subtype);
        prefix->xml84.emplace(std::make_unique<tooltip_manager_value_prefix_detail::EmptyXmlArena>());
        prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::xml84);full(op);
        prefix->lua90.emplace();prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::lua90);
        prefix->lua98.emplace();prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::lua98);
        prefix->instance_a0.emplace(std::make_unique<tooltip_manager_value_prefix_detail::InstanceSentinel>());
        prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::instance_a0);full(op);
        prefix->xml_ac.emplace();prefix->record(tooltip_manager_value_prefix_detail::ReachedPort::Effect::xml_ac);
        prefix->require_reached();suffix=std::make_unique<tooltip_manager_value_suffix_detail::ReachedSuffix>();full(op);
        auto&port=*suffix;using Effect=tooltip_manager_value_suffix_detail::ReachedSuffix::Effect;
        auto wide=std::make_shared<const tooltip_manager_value_prefix_detail::ConstructorWide>();full(op);
        port.initial_b4=wide;port.record(Effect::b4_empty);port.digest_b8.emplace();port.record(Effect::digest_b8);
        port.digest_2e4.emplace();port.record(Effect::digest_2e4);port.wide510=wide;port.record(Effect::wide510_empty);
        port.wide514=wide;port.record(Effect::wide514_empty);port.constructor_null518.emplace(nullptr);port.record(Effect::null518);
        port.lua51c.emplace();port.record(Effect::nil51c);port.lua524.emplace();port.record(Effect::nil524);
        port.lua52c.emplace();port.record(Effect::nil52c);
        //81D060+81E890 shared-empty534/no-op assignment, real NULL538.
        narrow534.emplace();port.record(Effect::narrow534_empty);font538.emplace(nullptr);port.record(Effect::null538);
        for(std::size_t group=0;group<3;++group){
            for(std::size_t component=0;component<4;++component){
                const auto offset=group*16+component*4;
                port.constructor_colors[offset]=std::uint8_t{0};port.constructor_colors[offset+1]=std::uint8_t{0};
                port.constructor_colors[offset+2]=std::uint8_t{0x80};port.constructor_colors[offset+3]=std::uint8_t{0x3f};
            }
            for(std::size_t j=0;j<4;++j)port.constructor_colors[48+group*4+j]=std::uint8_t{0};
            port.constructor_colors[60+group]=std::uint8_t{0};
        } //byte63 remains genuinely disengaged.
        port.record(Effect::colors53c);
        port.map57c.emplace(std::make_unique<tooltip_manager_value_suffix_detail::NamedSentinel>());port.record(Effect::map57c);full(op);
        port.map588.emplace(std::make_unique<tooltip_manager_value_suffix_detail::NamedSentinel>());port.record(Effect::map588);full(op);
        port.record(Effect::assign534_empty_noop);port.record(Effect::return_this);port.require_completed();record(Step::base40);
        //7A5F2B/31 actual distinct map at THIS+594, same4EA7C0 relations.
        constructor_594.emplace(std::make_unique<tooltip_manager_value_suffix_detail::NamedSentinel>());
        full(op);record(Step::registry594);
        //The known-field helper is created AFTER this actual base+594 owner.
        const auto actual=gui.lock();require(actual!=nullptr,"TTManager GUI retired before known stores");
        known=std::make_shared<Gui::TooltipKnownFields>(actual,constructor_serial);full(op);
    }
    void require_constructor_owned()const{
        life->first();static constexpr std::array expected{Step::allocated,Step::base40,Step::registry594,
            Step::known_stores,Step::receiver728020,Step::returned};
        require(count==expected.size()&&history==expected&&prefix&&suffix&&constructor_594&&known&&narrow534&&
            narrow534->empty()&&font538&&*font538==nullptr,
            "TTManager actual constructor/value owner incomplete");
        known->life->first();require(known->history.size()>=9&&known->registration_sequence&&
            known->registration_sequence<=known->sequence,"TTManager known-store/receiver history missing");
        for(std::size_t i=0;i<8;++i)require(known->history[i].source_serial==constructor_serial&&
            known->history[i].kind==Gui::TooltipKnownFields::Store::known_null&&known->history[i].field==0x5a0+4*i,
            "TTManager actual known NULL constructor store order changed");
        require(known->constructor_source_serial==constructor_serial&&known->history[8].source_serial==constructor_serial&&
            known->history[8].kind==Gui::TooltipKnownFields::Store::retained_registration&&
            known->history[8].field==0x728020&&known->history[8].sequence==known->registration_sequence,
            "TTManager actual728020 constructor receiver store changed");
        const auto actual=gui.lock();require(actual&&actual->tooltip_registrations.size()==1,
            "TTManager actual GUI retained receiver absent");
        const auto&registration=actual->tooltip_registrations.front();
        const auto retained_known=registration.target.lock();const auto retained_value=registration.whole_target.lock();
        require(registration.receiver==Gui::TooltipRegistration::Receiver::tooltip_728020&&bool(registration.callable)&&
            registration.store_sequence==known->registration_sequence&&retained_known==known&&retained_value&&
            retained_value->impl_.get()==this&&actual->tooltip_value==retained_value&&actual->tooltip_manager==known,
            "TTManager actual retained728020 THIS allocation changed");
        prefix->require_reached();suffix->require_completed();
        const auto*s=constructor_594->sentinel.get();
        require(s&&!constructor_594->count&&s->left==s&&s->parent==s&&s->right==s&&s->color_14==1&&s->is_nil_15==1,
            "TTManager real594 constructor sentinel relations changed");
        require(component_6c==&base_dwords[16]&&!base_0.owner&&!base_0.next&&base_0.sentinel==0xffffffffu&&
            !component_5c.owner&&!component_5c.next&&component_5c.sentinel==0xffffffffu&&
            !lua4c.state&&lua4c.reference==-2&&base_dwords[17]==0x3f800000u,
            "TTManager real Base4100/4F9470 constructor fields/relations changed");
        for(std::size_t i=4;i<=18;++i)if(i!=17)require(base_dwords[i]&&*base_dwords[i]==0,
            "TTManager actual Base constructor rectangle/flags fields changed");
        require(base_dwords[21]&&*base_dwords[21]==0&&base_dwords[22]&&*base_dwords[22]==0,
            "TTManager actual Base constructor parent fields changed");
        for(std::size_t i=28;i<=32;++i)require(base_dwords[i]&&*base_dwords[i]==0,
            "TTManager actual embedded component constructor fields changed");
    }
};
void detail::NotificationContextTooltipManagerValue::retire_owned()const noexcept{impl_->life->alive.store(false);}
detail::NotificationContextTooltipManagerValue::~NotificationContextTooltipManagerValue(){retire_owned();}
void detail::NotificationContextTooltipManagerValue::require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&op)const{
    impl_->life->first();impl_->first(op);impl_->require_constructor_owned();
    const auto gui=impl_->gui.lock();require(gui&&gui->tooltip_value.get()==this&&gui->tooltip_manager==impl_->known,
        "TTManager actual GUI embedded value slot replaced");
    impl_->full(op);impl_->require_constructor_owned();impl_->first(op);
    require(gui->tooltip_value.get()==this&&gui->tooltip_manager==impl_->known,"TTManager GUI value changed at Source POST");
}
void detail::NotificationContextTooltipManagerOperand::require_current_owned(const runtime::BnwWorldViewSourceOperationLoan&op)const{
    if(creator_!=std::this_thread::get_id())throw std::runtime_error("TTManager genuine value operand foreign creator");
    require(value_&&cdata_==value_->impl_.get(),"TTManager operand has another actual value allocation");value_->require_current_owned(op);
}
std::shared_ptr<const detail::NotificationContextTooltipManagerOperand>
detail::NotificationContextGuiPort::capture_tooltip_manager_operand_owned(const runtime::BnwWorldViewSourceOperationLoan&op)const{
    impl_->first(op);const auto actual=impl_->tooltip_value;
    require(actual!=nullptr,"TTManager actual GUI7A5F20 value constructor did not run");actual->require_current_owned(op);
    const auto result=std::shared_ptr<const detail::NotificationContextTooltipManagerOperand>(
        new detail::NotificationContextTooltipManagerOperand(actual,actual->impl_.get()));
    result->require_current_owned(op);return result;
}
