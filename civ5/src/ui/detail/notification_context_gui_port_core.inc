// Inside civ5::ui after the complete canonical Core. Supported client profile.
struct detail::NotificationContextGuiPort::Impl {
    using Operand=detail::NotificationContextGuiClientOperand;
#include "notification_selected_color_set_registry_fields.inc"
#include "notification_selected_style_registry_fields.inc"
#include "notification_selected_provider_activation_fields.inc"
    // Root-approved partial known-field normalization, NOT a40F9C0 or full
    //7A5F20/GUI constructor receipt. Fields are disengaged until real stores.
    struct TooltipLife final {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> alive{true};mutable std::atomic<bool> failed{false};
        void first()const{
            if(creator!=std::this_thread::get_id()){failed.store(true);throw std::runtime_error("Tooltip manager foreign creator");}
            require(alive.load()&&!failed.load(),"Tooltip manager retired/failed");
        }
    };
    struct TooltipKnownFields final {
        struct Null final {};
        struct ControlIdentity final {
            std::weak_ptr<BnwWorldViewControlTree::Core> core;
            std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> allocation;
            std::uint64_t id,instance,constructor_serial;
        };
        using Nullable=std::variant<Null,ControlIdentity>;
        const std::shared_ptr<TooltipLife> life=std::make_shared<TooltipLife>();
        const std::weak_ptr<Impl> gui;
        const std::uint64_t constructor_source_serial;
        std::array<std::optional<Nullable>,8> fields_5a0_5bc;
        enum class Store {known_null,retained_registration};
        struct Entry {std::uint64_t sequence,source_serial;Store kind;std::uint32_t field;};
        std::vector<Entry> history;std::uint64_t sequence=0,hover_store_sequence=0,registration_sequence=0;
        bool busy=false;
        explicit TooltipKnownFields(std::weak_ptr<Impl> owner,std::uint64_t serial):gui(std::move(owner)),constructor_source_serial(serial){}
        void record(std::uint64_t serial,Store kind,std::uint32_t field){
            life->first();require(serial&&sequence<UINT64_MAX,"Tooltip manager store occurrence invalid/exhausted");
            history.push_back({sequence+1,serial,kind,field});++sequence;
        }
        ~TooltipKnownFields(){life->alive.store(false);}
    };
    struct TooltipRegistration final {
        enum class Receiver {tooltip_728020};
        Receiver receiver=Receiver::tooltip_728020;
        std::weak_ptr<TooltipKnownFields> target;
        std::weak_ptr<detail::NotificationContextTooltipManagerValue> whole_target;
        std::function<void(const runtime::BnwWorldViewSourceOperationLoan&,std::uint32_t)> callable;
        std::uint64_t store_sequence;
    };
    std::shared_ptr<TooltipKnownFields> tooltip_manager;
    std::shared_ptr<detail::NotificationContextTooltipManagerValue> tooltip_value;
    std::shared_ptr<detail::NotificationContextFontRegistryFields> font_registry;
    std::shared_ptr<detail::BnwLabelSystemFontRegistryFields> system_font_registry;
    bool system_font_creating=false;
    std::weak_ptr<detail::NotificationContextGuiPort> canonical_gui;
    std::vector<TooltipRegistration> tooltip_registrations;
    ~Impl(){if(selected_provider_activation)selected_provider_activation->retire();if(system_font_registry)system_font_registry->life->alive.store(false);if(selected_styles)selected_styles->alive.store(false);if(selected_color_sets)selected_color_sets->alive.store(false);if(tooltip_manager)tooltip_manager->life->alive.store(false);if(tooltip_value)tooltip_value->retire_owned();}

    const std::thread::id creator=std::this_thread::get_id();
    std::shared_ptr<detail::BnwLabelProcessModuleOwner> label_module;
    std::shared_ptr<detail::BnwWindowedRendererSharedDescriptorIdentity::Impl> windowed_shared_descriptor;
    mutable std::atomic<bool> failed{false};std::atomic<bool> facade_alive{true};bool busy=false;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<const SelectedBnwPreviewHost> host;
    const std::weak_ptr<const runtime::LiveSessionState> world;
    const std::weak_ptr<BnwClientSceneFrame> scene;
    const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    const std::uint64_t generation,viewport_revision;
    const std::uintptr_t hwnd,occurrence;
    const std::uint32_t process,ui_thread;
    std::int32_t field_644=0,field_648=0;
    struct Language980ARefresh final {std::uint8_t value;std::string language;std::uint64_t clear_sequence,row_sequence,source_serial;bool completed=false;};
    std::optional<Language980ARefresh> language_980a;
    bool refreshing_language_980a=false;
    std::int32_t cursor_6dc=0; // Earned native5BB172 ctor port, mutable thereafter.
    std::array<HCURSOR,64> cursor_a80{};
    bool cursor_constructor_returned=false;
    const bool no_custom_cursor_graph; // Actual supported-port ctor policy.

    // Native PE writable virtual .data tail initially has two +0 dwords.
    // This is an explicitly scoped Civis port property, not Original memory.
    std::array<float,2> arithmetic{std::bit_cast<float>(std::uint32_t{0}),std::bit_cast<float>(std::uint32_t{0})};
    #include "bnw_windowed_renderer_matrix_fields.inc"
#include "bnw_normal_gui_batch_fields.inc"
    enum class Store {width_644,height_648,arithmetic_pair,windowed_ortho,cursor_ctor_table,cursor_class_return,cursor_defwindow_return,cursor_current_store,cursor_set_return,resize_registration,language_980a_clear,language_980a_row};
    std::vector<std::shared_ptr<NotificationLayoutResizeRegistration>> resize_registrations;
    struct Entry {std::uint64_t sequence,source_serial;Store kind;};
    std::vector<Entry> history;std::uint64_t revision=0;
    explicit Impl(const Operand& input):source(input.source_),host(input.host_),world(input.world_),scene(input.scene_),
        window(input.window_),generation(input.generation_),viewport_revision(input.viewport_revision_),
        hwnd(input.hwnd_),occurrence(input.occurrence_),process(input.process_),ui_thread(input.ui_thread_),no_custom_cursor_graph(true){}
    void first(const runtime::BnwWorldViewSourceOperationLoan& operation,bool expected_busy=false)const {
        try{
            require(creator==std::this_thread::get_id()&&facade_alive.load()&&!failed.load()&&busy==expected_busy,
                "GUI client port foreign creator/failed/reentered before borrow");
            operation.require_origin_without_callbacks();
            require(!source.expired()&&!host.expired()&&!world.expired()&&!scene.expired()&&!window.expired(),
                "GUI client port canonical realm retired");
            operation.require_context_gui_port_identity_without_callbacks(source,host,world,scene,window,generation,
                hwnd,occurrence,viewport_revision,process,ui_thread);
            operation.require_origin_without_callbacks();
            if(label_module){
                label_module->require_current_without_callbacks();
                // Health retains existing module life only; no Grid/frame/ctor renewal.
                if(!operation.unitflag_health_operation_without_callbacks()){
                    const auto current=operation.label_module_owned();
                    require(current&&current.get()==label_module.get()&&
                        !current.owner_before(label_module)&&!label_module.owner_before(current),
                        "GUI canonical module differs from actual renderer Source lender");
                }
                label_module->require_current_without_callbacks();
                operation.require_origin_without_callbacks();
            }
        }catch(...){failed.store(true);throw;}
    }
    void record(const runtime::BnwWorldViewSourceOperationLoan& operation,Store kind){
        require(revision<UINT64_MAX,"GUI client port store history exhausted");
        history.push_back({revision+1,operation.operation_serial(),kind});++revision;
    }
};
#include "notification_context_tt_manager_value_constructor_owned.inc"
#include "bnw_windowed_shared_descriptor_identity_core.inc"
detail::NotificationContextGuiPort::NotificationContextGuiPort(std::shared_ptr<Impl> value):impl_(std::move(value)){}
detail::NotificationContextGuiPort::~NotificationContextGuiPort(){
    if(impl_->selected_provider_activation)impl_->selected_provider_activation->retire();
    impl_->facade_alive.store(false);
    if(impl_->system_font_registry)impl_->system_font_registry->life->alive.store(false);
    if(impl_->selected_color_sets)impl_->selected_color_sets->alive.store(false);
    if(impl_->selected_styles)impl_->selected_styles->alive.store(false);
    if(impl_->tooltip_manager)impl_->tooltip_manager->life->alive.store(false);
    if(impl_->tooltip_value)impl_->tooltip_value->retire_owned();
}
std::shared_ptr<detail::NotificationContextGuiPort> detail::NotificationContextGuiPort::capture_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    operation.require_origin_without_callbacks();
    const auto operand=operation.context_gui_client_operand_owned();operation.require_origin_without_callbacks();
    auto state=std::make_shared<Impl>(operand);state->first(operation);
    state->label_module=operation.label_module_owned();state->first(operation);
    // Canonical Source initial load precedes every selected Context/Button parse.
    // All three actual VFS/DLC sheets are retained together; no Base-only substitute.
    const auto style_sheets=operation.selected_style_resources_owned();state->first(operation);
    state->selected_styles=std::make_shared<Impl::SelectedStyleRegistry>(style_sheets,operation.operation_serial());
    state->selected_styles->first();operation.require_current();state->first(operation);state->selected_styles->first();
    const auto color_atlas=operation.color_atlas_resources_owned();state->first(operation);
    require((_mm_getcsr()&0xe040u)==0,"ColorSet load unsupported current SSE normalization mode");
    const auto staged_color_sets=std::make_shared<Impl::SelectedColorSetRegistry>(color_atlas,operation.operation_serial());
    state->first(operation);operation.require_current();state->first(operation);staged_color_sets->first();
    state->selected_color_sets=staged_color_sets; //Nonalloc actual Gui owner publication.
    state->selected_color_sets->first();operation.require_current();state->first(operation);state->selected_color_sets->first();
#include "notification_selected_provider_activation_startup.inc"
    // Allocation and admission precede physical prefix. Same native client
    // rectangle supplier supplies both, in the original644 then648 order.
    state->record(operation,Impl::Store::width_644);state->field_644=operand.width_;
    state->record(operation,Impl::Store::height_648);state->field_648=operand.height_;
    // Separate known-field history preserves existing Gui width/cursor order.
    //Actual GUI-owned7A5F20 value birth: distinct40F/base594 storage precedes
    //known stores. The historical empty sentinel is not loaded-registry readiness.
    state->first(operation);
    auto tooltip_data=std::make_shared<detail::NotificationContextTooltipManagerValue::Impl>(state,operation.operation_serial());
    tooltip_data->construct_base40_owned(operation);state->first(operation);
    const auto tooltip_value=std::shared_ptr<detail::NotificationContextTooltipManagerValue>(
        new detail::NotificationContextTooltipManagerValue(tooltip_data));
    state->tooltip_value=tooltip_value;
    const auto manager=tooltip_data->known;
    manager->history.reserve(9);
    for(std::size_t i=0;i<8;++i){
        state->first(operation);manager->life->first();
        manager->record(operation.operation_serial(),Impl::TooltipKnownFields::Store::known_null,0x5a0+4*i);
        manager->fields_5a0_5bc[i]=Impl::TooltipKnownFields::Null{};
        if(i==1)manager->hover_store_sequence=manager->sequence;
    }
    tooltip_data->record(detail::NotificationContextTooltipManagerValue::Impl::Step::known_stores);
    state->tooltip_manager=manager;
    const auto independent_life=manager->life;
    const std::weak_ptr<Impl> weak_gui=state;
    const std::weak_ptr<Impl::TooltipKnownFields> weak_manager=manager;
    const std::weak_ptr<detail::NotificationContextTooltipManagerValue> weak_value=tooltip_value;
    const auto value_life=tooltip_data->life;
    auto callable=[independent_life,weak_gui,weak_manager,weak_value,value_life](const runtime::BnwWorldViewSourceOperationLoan& op,std::uint32_t event){
        value_life->first();independent_life->first(); // Before any weak State pin.
        const auto gui=weak_gui.lock();const auto actual=weak_manager.lock();const auto value=weak_value.lock();
        try{
            require(gui&&actual&&value,"Tooltip retained whole receiver owner gone");
            value->require_current_owned(op);
            const auto pure=[&]{value_life->first();independent_life->first();gui->first(op);actual->life->first();
                require(gui->tooltip_value==value&&value->impl_->known==actual&&value->impl_->life==value_life,
                    "Tooltip728020 actual THIS value allocation changed");
                require(actual->life.get()==independent_life.get()&&gui->tooltip_manager.get()==actual.get()&&
                    !gui->tooltip_manager.owner_before(actual)&&!actual.owner_before(gui->tooltip_manager),
                    "Tooltip callback actual retained THIS changed");};
            pure();require(!actual->busy,"Tooltip callback reentered");actual->busy=true;
            {struct End{std::shared_ptr<Impl::TooltipKnownFields> p;~End(){p->busy=false;}}end{actual};
                pure();op.require_current();pure();
                if(event==1||event==5)throw std::runtime_error("Tooltip728020 reached4978D0; event1 also actual virtual10: genuine effects required");
                // Other events have literalRET4 in the frozen body. No fake1/5 return.
                op.require_current();pure();
            }
            pure();require(!actual->busy,"Tooltip callback cleanup failed");
        }catch(...){independent_life->failed.store(true);if(gui)gui->failed.store(true);throw;}
    };
    state->tooltip_registrations.reserve(1);
    manager->record(operation.operation_serial(),Impl::TooltipKnownFields::Store::retained_registration,0x728020);
    state->tooltip_registrations.push_back({Impl::TooltipRegistration::Receiver::tooltip_728020,manager,tooltip_value,std::move(callable),manager->sequence});
    manager->registration_sequence=manager->sequence;
    tooltip_data->record(detail::NotificationContextTooltipManagerValue::Impl::Step::receiver728020);
    tooltip_data->record(detail::NotificationContextTooltipManagerValue::Impl::Step::returned);
    tooltip_value->require_current_owned(operation);
    state->first(operation);manager->life->first();operation.require_current();state->first(operation);manager->life->first();
    // Native5BB2AF constructs96CC before5BB4D2. ONLY this known field
    // stage is earned; no fullUIManager ctor or Font/load readiness claim.
    state->first(operation);operation.require_current();state->first(operation);
    state->font_registry=std::shared_ptr<detail::NotificationContextFontRegistryFields>(
        new detail::NotificationContextFontRegistryFields(operation.operation_serial()));
    state->first(operation);operation.require_current();state->first(operation);
    // Native5BB4D2 captures class cursor once and initializes all64 table cells.
    // NULL is permitted only as the actual successful SAME HWND query result.
    state->first(operation);operation.require_current();state->first(operation);SetLastError(0);
    const auto actual_cursor=GetClassLongPtrW(reinterpret_cast<HWND>(state->hwnd),GCLP_HCURSOR);
    const auto cursor_error=GetLastError();state->first(operation);operation.require_current();state->first(operation);
    require(actual_cursor!=0||cursor_error==0,"GUI cursor constructor actual class query failed");
    state->history.reserve(state->history.size()+64);
    for(auto& cell:state->cursor_a80){state->record(operation,Impl::Store::cursor_ctor_table);cell=reinterpret_cast<HCURSOR>(actual_cursor);}
    state->cursor_constructor_returned=true;

    state->first(operation);operation.require_current();state->first(operation);
    auto result=std::shared_ptr<NotificationContextGuiPort>(new NotificationContextGuiPort(state));
    state->first(operation);operation.require_current();state->first(operation);
    state->canonical_gui=result;
    // Execute the OWN equivalent of the selected1A/F0 constant constructor.
    // The canonical resource is distinct from Graph's own144-byte descriptor.
    // This publishes identity/layout only; no unproved frame words are granted.
    state->first(operation);operation.require_current();state->first(operation);
    require(!state->windowed_shared_descriptor,"Shared descriptor cannot replace prior canonical resource");
    const auto shared_descriptor=std::make_shared<detail::BnwWindowedRendererSharedDescriptorIdentity::Impl>(state,result);
    state->first(operation);operation.require_current();state->first(operation);
    require(!state->windowed_shared_descriptor,"Shared descriptor constructor callback replaced resource");
    state->windowed_shared_descriptor=shared_descriptor;
    shared_descriptor->first();state->first(operation);operation.require_current();state->first(operation);
    shared_descriptor->first();
    result->require_current_owned(operation);
    result->refresh_language_980a_owned(operation); // Mandatory selected4630A0 phase, never ctor zero.
    state->selected_provider_activation->require_current_owned(operation,state);
    result->require_current_owned(operation);state->selected_provider_activation->first(state);return result;
}
#include "bnw_windowed_renderer_matrix_data_core.inc"
#include "bnw_normal_gui_batch_prefix_core.inc"
#include "notification_gui_language_refresh_core.inc"
void detail::NotificationContextGuiPort::refresh_tooltip_if_hovered_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwWorldViewControlTree& tree,std::uint64_t actual_control_id){
    if(impl_->creator!=std::this_thread::get_id()){impl_->failed.store(true);throw std::runtime_error("Tooltip refresh GUI foreign creator");}
    const auto saved=impl_;std::shared_ptr<BnwWorldViewControlTree::Core> core;
    std::shared_ptr<Impl::TooltipKnownFields> manager;
    try{
        saved->first(operation);tree.core_->layout_origin(true);core=tree.core_;
        operation.require_origin_without_callbacks();core->layout_origin(true);
        manager=saved->tooltip_manager;require(bool(manager),"Tooltip manager known-store owner absent");manager->life->first();
        require(!manager->busy,"Tooltip refresh reentered");
        // Complete value identity and current field snapshot BEFORE any FULL.
        const auto expected_facade=&tree;
        const auto topology=core->topology_revision,effects=core->effect_revision;
        const auto source_serial=operation.operation_serial();
        const auto allocation=actual_control_id?BnwWorldViewControlTree::Core::node(core->nodes,actual_control_id).layout_receiver_occurrence:
            core->context_receiver_occurrence;
        require(bool(allocation),"Tooltip actual canonical receiver allocation absent");
        const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> allocation_key=allocation;
        const auto instance=allocation->instance,constructor=allocation->constructor_serial;
        require(manager->fields_5a0_5bc[1].has_value(),"Tooltip hover5A4 has no actual initialization store");
        const auto hover=*manager->fields_5a0_5bc[1];
        const auto hover_store=manager->hover_store_sequence,manager_sequence=manager->sequence;
        const auto pure=[&]{
            manager->life->first();saved->first(operation);core->layout_origin(true);operation.require_origin_without_callbacks();
            require(saved->tooltip_manager.get()==manager.get()&&!saved->tooltip_manager.owner_before(manager)&&
                !manager.owner_before(saved->tooltip_manager)&&manager->constructor_source_serial&&
                manager->history.size()>=9&&manager->registration_sequence&&saved->tooltip_registrations.size()==1,
                "Tooltip canonical known-store/registration owner changed");
            {const auto actual=manager->gui.lock();require(actual&&actual.get()==saved.get()&&
                !actual.owner_before(saved)&&!saved.owner_before(actual),"Tooltip manager actual Gui control changed");}
            require(&core->source->current_operation_loan()==&operation&&operation.operation_serial()==source_serial&&
                core->context_gui_port&&core->context_gui_port->impl_.get()==saved.get()&&
                core->layout_parser_facade==expected_facade&&core->layout_parser_borrower&&core->layout_parser_control_id&&
                *core->layout_parser_control_id==actual_control_id&&core->layout_parser_occurrence&&
                core->layout_parser_occurrence.get()==allocation.get()&&
                !core->layout_parser_occurrence.owner_before(allocation_key)&&!allocation_key.owner_before(core->layout_parser_occurrence)&&
                core->topology_revision==topology&&core->effect_revision==effects,
                "Tooltip refresh lacks SAME current Gui/Core/Source/parser occurrence");
            const auto current=actual_control_id?BnwWorldViewControlTree::Core::node(core->nodes,actual_control_id).layout_receiver_occurrence:
                core->context_receiver_occurrence;
            require(current&&current.get()==allocation.get()&&!current.owner_before(allocation_key)&&!allocation_key.owner_before(current)&&
                current->id==actual_control_id&&current->instance==instance&&current->constructor_serial==constructor&&constructor&&
                manager->sequence==manager_sequence&&manager->hover_store_sequence==hover_store&&hover_store&&
                manager->fields_5a0_5bc[1].has_value()&&manager->fields_5a0_5bc[1]->index()==hover.index(),
                "Tooltip actual receiver/current hover store changed");
            const auto& registration=saved->tooltip_registrations.front();const auto retained=registration.target.lock();
            require(retained&&retained.get()==manager.get()&&!retained.owner_before(manager)&&!manager.owner_before(retained)&&
                registration.store_sequence==manager->registration_sequence&&bool(registration.callable),
                "Tooltip fixed retained registration changed");
            manager->life->first();core->layout_origin(true);saved->first(operation);
        };
        pure();manager->busy=true;
        {struct End{std::shared_ptr<Impl::TooltipKnownFields> p;~End(){p->busy=false;}}end{manager};
            pure();operation.require_current();pure();
            if(std::holds_alternative<Impl::TooltipKnownFields::ControlIdentity>(hover)){
                const auto& key=std::get<Impl::TooltipKnownFields::ControlIdentity>(hover);
                // Expired or invalid nonNULL is NEVER converted into constructorNULL.
                const auto actual_core=key.core.lock();const auto actual_allocation=key.allocation.lock();
                require(actual_core&&actual_allocation,"Tooltip nonNULL hover receiver retired");actual_core->layout_origin(true);
                if(actual_core.get()==core.get()&&!actual_core.owner_before(core)&&!core.owner_before(actual_core)&&
                    actual_allocation.get()==allocation.get()&&!actual_allocation.owner_before(allocation_key)&&
                    !allocation_key.owner_before(actual_allocation)&&key.id==actual_control_id&&key.instance==instance&&key.constructor_serial==constructor)
                    throw std::runtime_error("Tooltip524CC0 actual match requires genuine6C4A50 refresh effect");
                throw std::runtime_error("Tooltip actual input5A4 writer/current nonNULL receiver issuer required");
            }
            // Proven524CC0 nonmatch: real current constructorNULL vs actual
            // nonNULL canonical receiver. No base594/40F9C0/Box/input grant.
            operation.require_current();pure();
        }
        manager->life->first();core->layout_origin(true);saved->first(operation);pure();
        require(!manager->busy,"Tooltip refresh cleanup failed");
    }catch(...){if(manager)manager->life->failed.store(true);if(core)core->failed.store(true);saved->failed.store(true);throw;}
}

void detail::NotificationContextGuiPort::require_current_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto state=impl_;
    try{state->first(operation);state->busy=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{state->busy};
        const auto operand=operation.context_gui_client_operand_owned();state->first(operation,true);
        require(operand.hwnd_==state->hwnd&&operand.occurrence_==state->occurrence&&
            operand.viewport_revision_==state->viewport_revision&&operand.process_==state->process&&operand.ui_thread_==state->ui_thread&&
            operand.width_==state->field_644&&operand.height_==state->field_648&&state->history.size()>=2&&
            state->history[0].kind==Impl::Store::width_644&&state->history[1].kind==Impl::Store::height_648,
            "GUI client port actual dimensions/store producer changed");
        operation.require_current();state->first(operation,true);
    }catch(...){state->failed.store(true);throw;}
}
std::array<std::int32_t,2> detail::NotificationContextGuiPort::dimensions_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto state=impl_;state->first(operation);const auto result=std::array<std::int32_t,2>{state->field_644,state->field_648};
    state->first(operation);return result;
}
std::array<float,2> detail::NotificationContextGuiPort::arithmetic_pair_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto state=impl_;state->first(operation);const auto result=state->arithmetic;
    for(float x:result)require(std::isfinite(x),"GUI mutable arithmetic property nonfinite");
    state->first(operation);return result;
}
void detail::NotificationContextGuiPort::set_arithmetic_pair_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const std::array<float,2>& values){
    const auto state=impl_;
    try{state->first(operation);auto actual=values;for(float x:actual)require(std::isfinite(x),"GUI arithmetic setter nonfinite");
        require_current_owned(operation);state->first(operation);
        state->record(operation,Impl::Store::arithmetic_pair);state->arithmetic=actual;
        operation.require_current();state->first(operation);
    }catch(...){state->failed.store(true);throw;}
}

std::shared_ptr<detail::NotificationContextGuiPort> BnwWorldViewControlTree::capture_context_gui_port_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& actual_slots){
    operation.require_origin_without_callbacks();std::shared_ptr<detail::NotificationContextGuiPort> result;
    bool any=false,all=true;
    for(const auto& slot:actual_slots){if(!slot){all=false;continue;}any=true;slot->core_->layout_origin();
        const auto port=slot->core_->context_gui_port;require(bool(port),"Context canonical GUI port absent");
        if(!result)result=port;else require(result.get()==port.get()&&!result.owner_before(port)&&!port.owner_before(result),
            "Context slots name different canonical GUI managers");}
    require(!any||all,"Context GUI port cannot fill a mixed old/new canonical slot domain");
    if(!result)result=detail::NotificationContextGuiPort::capture_owned(operation);
    result->require_current_owned(operation);operation.require_origin_without_callbacks();return result;
}

struct detail::NotificationContextSlot10LoadOrigin::Impl {
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::shared_ptr<NotificationContextGuiPort> manager;
    const std::uint64_t topology,parser_serial,source_serial,before_effects,after_effects,before_sequence,after_sequence;
    const std::uint64_t sequence,effects; // aliases for historical initializer ABI only
    const std::uint32_t flags;
    const std::array<float,2> stored_size;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,
        const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint64_t parsed,
        std::uint64_t actual_before_sequence,std::uint64_t actual_before_effects,
        std::uint64_t observed_sequence,std::uint32_t actual_flags,std::array<float,2> size)
        :core(actual),manager(actual->context_gui_port),topology(actual->topology_revision),parser_serial(parsed),
         source_serial(operation.operation_serial()),before_effects(actual_before_effects),after_effects(actual->effect_revision),
         before_sequence(actual_before_sequence),after_sequence(observed_sequence),sequence(observed_sequence),effects(actual->effect_revision),
         flags(actual_flags),stored_size(size){}
};
void BnwWorldViewControlTree::prepare_context_slot10_load_origin_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Origin=detail::NotificationContextSlot10LoadOrigin;const auto saved=core_;saved->layout_origin();
    try{
        require(saved->context_construction_return&&saved->context_gui_port&&saved->context_layout_baseline&&
            saved->context_layout_baseline->parsed_source_serial_&&!saved->context_load_origin,
            "Context load prefix needs authentic constructor/parser/GUI port and cannot replay");
        const auto parser=*saved->context_layout_baseline->parsed_source_serial_;require(parser!=0,"Context actual parser serial absent");
        operation.require_current();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Context load SAME Source operation changed");
        auto before_sequence=saved->context_layout_baseline->sequence_;auto before_effects=saved->effect_revision;
        const bool deferred=(saved->context_layout_baseline->base_.flags_40&0x10000000U)!=0;
        const auto manager=saved->context_gui_port;
        if(!deferred){
            prepare_context_cursor_load_prefix_owned(operation);
            const auto cursor=require_context_cursor_load_prefix_owned(saved,operation);saved->layout_origin();
            const auto prepared=prepare_context_lua_state_owned(saved,operation,cursor);saved->layout_origin();
            require_context_lua_preparation_owned(saved,operation,prepared);saved->layout_origin();
            require(saved->context_gui_port.get()==manager.get()&&!saved->context_gui_port.owner_before(manager)&&
                !manager.owner_before(saved->context_gui_port)&&saved->context_layout_baseline&&
                saved->context_layout_baseline->parsed_source_serial_==parser&&
                saved->context_lua_preparation.get()==prepared.get()&&
                !saved->context_lua_preparation.owner_before(prepared)&&!prepared.owner_before(saved->context_lua_preparation),
                "Context actual cursor/Lua preparation changed canonical dimension receiver");
            // Native4572F1 begins only AFTER true physical policy, cursor and
            // real perContext518 coroutine/environment factory returns.
            before_sequence=saved->context_layout_baseline->sequence_;before_effects=saved->effect_revision;
            const auto before_flags=saved->context_layout_baseline->base_.flags_40;
            const auto before_size=saved->context_layout_baseline->base_.size_30_34;
            manager->require_current_owned(operation);saved->layout_origin();
            require_context_lua_preparation_owned(saved,operation,prepared);saved->layout_origin();
            require(saved->context_layout_baseline&&saved->context_layout_baseline->sequence_==before_sequence&&
                saved->context_layout_baseline->parsed_source_serial_==parser&&saved->effect_revision==before_effects&&
                saved->context_layout_baseline->base_.flags_40==before_flags&&
                saved->context_layout_baseline->base_.size_30_34==before_size&&
                saved->context_gui_port.get()==manager.get()&&!saved->context_gui_port.owner_before(manager)&&
                !manager.owner_before(saved->context_gui_port)&&saved->context_lua_preparation.get()==prepared.get(),
                "Context physical/VM callbacks changed dimension preimage");
            const auto pixels=manager->dimensions_without_callbacks(operation);saved->layout_origin();
            // This mutable state borrow follows all full callback bookends.
            {
                auto& state=*saved->context_layout_baseline;
                require(saved->effect_revision<=UINT64_MAX-3&&saved->layout_sequence<=UINT64_MAX-3,
                    "Context load native physical store history exhausted");
                volatile float width=static_cast<float>(pixels[0]),height=static_cast<float>(pixels[1]);
                require(std::isfinite(width)&&std::isfinite(height),"Context CVTSI2SS size unsupported");
                state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
                state.base_.size_30_34[0]=width;++saved->effect_revision;++saved->layout_sequence;
                state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
                state.base_.size_30_34[1]=height;++saved->effect_revision;++saved->layout_sequence;
                state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
                state.base_.flags_40|=0x300000U;++saved->effect_revision;++saved->layout_sequence;
            }
            const auto after_sequence=saved->context_layout_baseline->sequence_;const auto after_effects=saved->effect_revision;
            const auto after_flags=saved->context_layout_baseline->base_.flags_40;const auto after_size=saved->context_layout_baseline->base_.size_30_34;
            require_context_lua_preparation_owned(saved,operation,prepared);saved->layout_origin();
            require(saved->context_gui_port.get()==manager.get()&&!saved->context_gui_port.owner_before(manager)&&
                !manager.owner_before(saved->context_gui_port)&&saved->context_layout_baseline&&
                saved->context_layout_baseline->parsed_source_serial_==parser&&
                saved->context_layout_baseline->sequence_==after_sequence&&saved->effect_revision==after_effects&&
                saved->context_layout_baseline->base_.flags_40==after_flags&&saved->context_layout_baseline->base_.size_30_34==after_size&&
                after_sequence==before_sequence+3&&after_effects==before_effects+3&&saved->context_lua_preparation.get()==prepared.get(),
                "Context VM preparation changed actual dimension after-state before typed publication");
        }
        const auto sequence=saved->context_layout_baseline->sequence_;const auto effects=saved->effect_revision;
        const auto flags=saved->context_layout_baseline->base_.flags_40;const auto size=saved->context_layout_baseline->base_.size_30_34;
        operation.require_current();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation&&saved->context_layout_baseline&&
            saved->context_layout_baseline->sequence_==sequence&&saved->effect_revision==effects&&
            saved->context_layout_baseline->parsed_source_serial_==parser&&saved->context_gui_port.get()==manager.get()&&
            !saved->context_gui_port.owner_before(manager)&&!manager.owner_before(saved->context_gui_port),
            "Context load prefix physical state changed before opaque publication");
        const auto body=std::make_shared<const Origin::Impl>(saved,operation,parser,before_sequence,before_effects,sequence,flags,size);
        saved->layout_origin();saved->context_load_origin=std::shared_ptr<const Origin>(new Origin(body));
        // Deferred Context+10 returns at the original flag branch. It still
        // remains an attached real child; no initializer/Lua return is issued.
        if(!deferred)saved->layout_context_initialization_origin=std::shared_ptr<const LayoutContextInitializationOrigin>(
            new LayoutContextInitializationOrigin(saved,saved->context_construction_return));
        saved->layout_origin();
    }catch(...){saved->failed.store(true);throw;}
}

std::shared_ptr<const detail::NotificationContextSlot10LoadOrigin>
BnwWorldViewControlTree::require_context_slot10_load_origin_owned(const std::shared_ptr<Core>& actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
    const auto receipt=actual->context_load_origin;require(receipt&&receipt->impl_,"Context typed load prefix absent");
    const auto& load=*receipt->impl_;const auto owner=load.core.lock();
    require(owner&&owner.get()==actual.get()&&!owner.owner_before(actual)&&!actual.owner_before(owner)&&
        &actual->source->current_operation_loan()==&operation&&load.source_serial==operation.operation_serial()&&
        actual->context_gui_port&&load.manager.get()==actual->context_gui_port.get()&&
        !load.manager.owner_before(actual->context_gui_port)&&!actual->context_gui_port.owner_before(load.manager)&&
        load.topology==actual->topology_revision&&actual->context_layout_baseline&&
        actual->context_layout_baseline->parsed_source_serial_==load.parser_serial&&load.parser_serial!=0&&
        actual->context_layout_baseline->sequence_==load.after_sequence&&actual->effect_revision==load.after_effects&&
        actual->context_layout_baseline->base_.flags_40==load.flags&&actual->context_layout_baseline->base_.size_30_34==load.stored_size,
        "Context typed load prefix Source/Core/physical after-state differs");
    const auto count=(load.flags&0x10000000U)?0U:3U;
    require(load.before_effects<=UINT64_MAX-count&&load.before_sequence<=UINT64_MAX-count&&
        load.after_effects==load.before_effects+count&&load.after_sequence==load.before_sequence+count,
        "Context typed load prefix native before/after store history differs");
    // No numerical after-snapshot grants an epoch: this is the one immutable
    // receipt issued by the actual three stores (or real deferred branch).
    // ChildFrame adoption remains the separate Indexed owner responsibility.
    actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();return receipt;
}

std::shared_ptr<BnwWorldViewControlTree::Core> BnwWorldViewControlTree::require_context_construction_origin_owned(
    const std::shared_ptr<Core>& actual,
    const std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn>& constructor){
    actual->layout_origin();require(constructor&&constructor->impl_&&actual->context_construction_return.get()==constructor.get()&&
        !actual->context_construction_return.owner_before(constructor)&&!constructor.owner_before(actual->context_construction_return),
        "Context initializer constructor receipt canonical allocation replaced");
    const auto& born=*constructor->impl_;const auto birth=born.core.lock();
    require(born.creator==std::this_thread::get_id()&&birth&&birth.get()==actual.get()&&!birth.owner_before(actual)&&!actual.owner_before(birth),
        "Context initializer constructor SAME receiver/control block differs");
    require(actual->context_load_origin&&actual->context_load_origin->impl_&&actual->context_gui_port&&actual->context_layout_baseline,
        "Context initializer genuine load dimension prefix absent");
    const auto retained_load=actual->context_load_origin;
    const auto& loaded=*retained_load->impl_;const auto receiver=loaded.core.lock();
    const auto& operation=actual->source->current_operation_loan();actual->layout_origin();
    require(receiver&&receiver.get()==actual.get()&&!receiver.owner_before(actual)&&!actual.owner_before(receiver)&&
        loaded.manager.get()==actual->context_gui_port.get()&&!loaded.manager.owner_before(actual->context_gui_port)&&
        !actual->context_gui_port.owner_before(loaded.manager)&&!(loaded.flags&0x10000000U)&&
        (loaded.flags&0x300000U)==0x300000U&&loaded.source_serial==operation.operation_serial()&&
        loaded.topology==actual->topology_revision&&loaded.effects==actual->effect_revision&&
        actual->context_layout_baseline->parsed_source_serial_==loaded.parser_serial&&loaded.parser_serial!=0&&
        actual->context_layout_baseline->sequence_==loaded.sequence&&
        actual->context_layout_baseline->base_.size_30_34==loaded.stored_size,
        "Context initializer actual nondeferred load stores/parser/topology differ");
    const auto prepared=actual->context_lua_preparation;
    require(bool(prepared),"Context initialization requires actual perContext518 preparation");
    require_context_lua_preparation_owned(actual,operation,prepared);actual->layout_origin();
    actual->context_gui_port->require_current_owned(operation);actual->layout_origin();
    require(actual->context_lua_preparation.get()==prepared.get()&&
        !actual->context_lua_preparation.owner_before(prepared)&&!prepared.owner_before(actual->context_lua_preparation),
        "Context initialization perContext518 preparation allocation changed");
    require(actual->context_load_origin.get()==retained_load.get()&&actual->context_layout_baseline&&
        actual->context_construction_return.get()==constructor.get()&&actual->context_gui_port&&
        actual->context_gui_port.get()==loaded.manager.get()&&!actual->context_gui_port.owner_before(loaded.manager)&&
        !loaded.manager.owner_before(actual->context_gui_port)&&
        loaded.topology==actual->topology_revision&&loaded.effects==actual->effect_revision&&
        actual->context_layout_baseline->sequence_==loaded.sequence&&
        actual->context_layout_baseline->parsed_source_serial_==loaded.parser_serial&&
        actual->context_layout_baseline->base_.size_30_34==loaded.stored_size,
        "Context initializer actual load prefix changed across GUI callback");
    return actual;
}
BnwWorldViewControlTree::LayoutContextInitializationOrigin::LayoutContextInitializationOrigin(
    const std::shared_ptr<Core>& actual,
    std::shared_ptr<const detail::NotificationLayoutContextConstructionReturn> constructor)
    :tree_(require_context_construction_origin_owned(actual,constructor)),topology_(actual->topology_revision),
     parser_serial_(*actual->context_layout_baseline->parsed_source_serial_),
     initial_size_(actual->context_layout_baseline->base_.size_30_34),
     native_zero_(actual->context_gui_port->arithmetic_pair_without_callbacks(actual->source->current_operation_loan())),
     construction_(constructor){
    require_context_construction_origin_owned(actual,constructor);actual->layout_origin();
}
