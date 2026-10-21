// Inside civ5::ui after complete actual Core and canonical GUI-port Impl.
class detail::NotificationLayoutResizeRegistration final {
    friend class NotificationLayoutRuntimeDimensions;
    friend class ::civ5::ui::BnwWorldViewControlTree;
    friend class NotificationContextGuiPort;
    friend class NotificationContextLoadNameLoan;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<NotificationContextGuiPort> manager;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::uint64_t id,constructor_serial,registration_source_serial;
    const std::uint32_t registered_flags;
    mutable std::atomic<bool> failed{false};
    enum class Role {parser_receiver,context_load_668750};
    const Role role=Role::parser_receiver;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> context_receiver;
    const std::weak_ptr<const NotificationContextNativeConstructorReturn> context_constructor;
    NotificationLayoutResizeRegistration(const std::shared_ptr<BnwWorldViewControlTree::Core>& c,
        const std::shared_ptr<NotificationContextGuiPort>& m,
        const std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence>& o,
        const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint32_t flags)
        :core(c),manager(m),occurrence(o),id(o->id),constructor_serial(o->constructor_serial),
         registration_source_serial(operation.operation_serial()),registered_flags(flags){}
    //Box closed issuer snapshots every operand VALUE before allocator callbacks.
    NotificationLayoutResizeRegistration(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<NotificationContextGuiPort> m,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> o,
        std::uint64_t actual_id,std::uint64_t ctor_serial,std::uint64_t source_serial,std::uint32_t flags)
        :core(std::move(c)),manager(std::move(m)),occurrence(std::move(o)),id(actual_id),
         constructor_serial(ctor_serial),registration_source_serial(source_serial),registered_flags(flags){}
    NotificationLayoutResizeRegistration(std::shared_ptr<BnwWorldViewControlTree::Core> c,
        std::shared_ptr<NotificationContextGuiPort> m,std::shared_ptr<NotificationLayoutContextOrdinaryState> state,
        std::shared_ptr<const NotificationContextNativeConstructorReturn> ctor,
        std::uint64_t ctor_serial,std::uint64_t source_serial,std::uint32_t flags)
        :core(std::move(c)),manager(std::move(m)),occurrence(),id(0),constructor_serial(ctor_serial),
         registration_source_serial(source_serial),registered_flags(flags),role(Role::context_load_668750),
         context_receiver(std::move(state)),context_constructor(std::move(ctor)){}
    void dispatch_context_owned(const runtime::BnwWorldViewSourceOperationLoan&)const;
    void dispatch_owned(const runtime::BnwWorldViewSourceOperationLoan& operation,std::int32_t event_arg)const {
        require(creator==std::this_thread::get_id()&&!failed.load(),"Full resize registration foreign/failed");
        try{
            // Real event0 alone takes the size branch. This private callback is
            // retained by the actual GUI manager; no public event DTO issuer.
            if(event_arg!=0)return;
            if(role==Role::context_load_668750){dispatch_context_owned(operation);return;}
            BnwWorldViewControlTree::run_layout_resize_98_owned(*this,operation);
        }catch(...){failed.store(true);if(const auto c=core.lock())c->failed.store(true);throw;}
    }
};
struct detail::NotificationLayoutRuntimeDimensions::State {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};bool registering=false;
    bool common_child_suspended=false; //lexical suspension, never a parser-return grant
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<NotificationContextGuiPort> manager;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const BnwWorldViewControlTree* const tree_facade;
    const runtime::BnwWorldViewSourceOperationLoan* const operation;
    const std::uint64_t id,serial,topology;
    const NotificationLayoutBaseline* const baseline;
    const NotificationLayoutRuntimeDimensions* facade=nullptr;
    std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> context_realm_argument;
    bool binding_context_realm=false,assigning_context_3c=false;
    struct StylePrefixLoan final {
        const std::weak_ptr<const BnwWorldViewControlTree::Core::SelectedContextStyles> local;
        const std::weak_ptr<NotificationContextGuiPort::Impl::SelectedStyleRegistry> global;
        const std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> argument;
        const std::uint64_t parser_serial;
        std::set<std::size_t> entered;
        bool busy=false;
        StylePrefixLoan(const std::shared_ptr<const BnwWorldViewControlTree::Core::SelectedContextStyles>& l,
            const std::shared_ptr<NotificationContextGuiPort::Impl::SelectedStyleRegistry>& g,
            std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> a,std::uint64_t serial)
            :local(l),global(g),argument(std::move(a)),parser_serial(serial){}
    };
    std::unique_ptr<StylePrefixLoan> button_style_loan;
    struct GridStylePrefixLoan final {
        const std::weak_ptr<const BnwWorldViewControlTree::Core::SelectedContextStyles> local;
        const std::weak_ptr<NotificationContextGuiPort::Impl::SelectedStyleRegistry> global;
        const std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> argument;
        const std::uint64_t parser_serial;
        std::set<std::size_t> entered;
        bool busy=false;
        GridStylePrefixLoan(const std::shared_ptr<const BnwWorldViewControlTree::Core::SelectedContextStyles>& l,
            const std::shared_ptr<NotificationContextGuiPort::Impl::SelectedStyleRegistry>& g,
            std::shared_ptr<const NotificationLayoutContextRealmArgumentLoan> a,std::uint64_t serial)
            :local(l),global(g),argument(std::move(a)),parser_serial(serial){}
    };
    std::unique_ptr<GridStylePrefixLoan> grid_style_loan;
    std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan> active_button_string_layer_;
    bool constructing_standalone_label=false;
    std::shared_ptr<NotificationLayoutStandaloneLabelState> pending_standalone_label;
    std::weak_ptr<NotificationLayoutStandaloneLabelState::ParserLoan> active_standalone_parser;
    State(BnwWorldViewControlTree& tree,std::uint64_t control,
        const runtime::BnwWorldViewSourceOperationLoan& op,const std::shared_ptr<NotificationContextGuiPort>& gui,
        const std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence>& o,
        const NotificationLayoutBaseline& b)
        :core(tree.core_),manager(gui),occurrence(o),tree_facade(&tree),operation(&op),id(control),
         serial(op.operation_serial()),topology(tree.core_->topology_revision),baseline(&b){}
};
#include "notification_layout_ordinary_child_storage_core.inc"
#include "notification_layout_context_ordinary_receiver_core.inc"
#include "notification_context_native_constructor_prefix_core.inc"
#include "notification_context_native_constructor_suffix_core.inc"
#include "notification_context_load_name_core.inc"
#include "notification_context_root_read_b4_guard_core.inc"
#include "notification_context_root_read_file_prefix_core.inc"
#include "notification_layout_common_child_parser_binding_core.inc"
#include "notification_context_root_ordinary_parser_frame_core.inc"
#include "notification_layout_context_realm_core.inc"
#include "notification_context_default_font_style_core.inc"
detail::NotificationLayoutRuntimeDimensions::NotificationLayoutRuntimeDimensions(std::shared_ptr<State> s):state_(std::move(s)){
    require(bool(state_),"Runtime dimensions actual State absent");state_->facade=this;
}
detail::NotificationLayoutRuntimeDimensions::~NotificationLayoutRuntimeDimensions(){
    const auto s=state_;if(!s)return;
    if(s->context_realm_argument)s->context_realm_argument->retire_owned();
    s->alive.store(false); // FIRST, also on foreign final release.
    if(s->creator==std::this_thread::get_id())if(const auto c=s->core.lock())
        if(c->layout_parser_borrower==this)c->layout_parser_borrower=nullptr;
}
std::shared_ptr<detail::NotificationLayoutRuntimeDimensions>
detail::NotificationLayoutRuntimeDimensions::capture_owned(BnwWorldViewControlTree& tree,std::uint64_t id,
    const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<NotificationContextGuiPort>& gui){
    const auto actual=tree.core_;actual->layout_origin(true);operation.require_origin_without_callbacks();
    require(actual->layout_parser_control_id&&*actual->layout_parser_control_id==id&&
        actual->layout_parser_facade==&tree&&!actual->layout_parser_borrower&&actual->layout_parser_occurrence&&
        actual->context_gui_port&&gui&&gui.get()==actual->context_gui_port.get()&&
        !gui.owner_before(actual->context_gui_port)&&!actual->context_gui_port.owner_before(gui)&&
        &actual->source->current_operation_loan()==&operation&&operation.operation_serial()!=0,
        "Runtime dimensions require canonical actual parser construction scope");
    const NotificationLayoutBaseline* baseline=nullptr;
    if(id==0){require(actual->context_construction_return&&actual->context_layout_baseline&&
        actual->context_receiver_occurrence==actual->layout_parser_occurrence,
        "Runtime dimensions id0 lacks earned actual root construction");baseline=&*actual->context_layout_baseline;
    }else{const auto& node=BnwWorldViewControlTree::Core::node(actual->nodes,id);
        require(node.alive&&node.layout_baseline&&node.layout_receiver_occurrence==actual->layout_parser_occurrence,
            "Runtime dimensions actual node occurrence missing");baseline=&*node.layout_baseline;}
    require(!baseline->parsed_source_serial_&&!baseline->parser_runtime_dimensions_,"Runtime dimensions actual parser replay");
    auto result=std::shared_ptr<NotificationLayoutRuntimeDimensions>(new NotificationLayoutRuntimeDimensions(
        std::make_shared<State>(tree,id,operation,gui,actual->layout_parser_occurrence,*baseline)));
    actual->layout_parser_borrower=result.get();require_storage_without_callbacks(result->state_);
    if(actual->layout_parser_child_frame)
        actual->layout_parser_child_frame->bind_child_runtime_owned(result->state_);
    else if(actual->layout_parser_root_child_frame)
        actual->layout_parser_root_child_frame->bind_child_runtime_owned(result->state_);
    if(id!=0)result->capture_ordinary_context_argument_owned();
    return result;
}
void detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(const std::shared_ptr<State>& s){
    require(s&&s->creator==std::this_thread::get_id()&&s->alive.load()&&!s->failed.load()&&!s->common_child_suspended&&s->facade,
        "Runtime dimensions foreign/retired/failed facade before actual source access");
    const auto actual=s->core.lock();require(bool(actual),"Runtime dimensions actual Core retired");
    if(actual->layout_parser_root_child_frame)actual->layout_parser_root_child_frame->require_root_without_callbacks();
    actual->layout_origin(true);s->operation->require_origin_without_callbacks();
    const auto manager=s->manager.lock();const auto occurrence=s->occurrence.lock();
    require(manager&&occurrence&&actual->layout_parser_control_id&&*actual->layout_parser_control_id==s->id&&
        actual->layout_parser_facade==s->tree_facade&&actual->layout_parser_borrower==s->facade&&
        actual->layout_parser_occurrence&&actual->layout_parser_occurrence.get()==occurrence.get()&&
        !actual->layout_parser_occurrence.owner_before(occurrence)&&!occurrence.owner_before(actual->layout_parser_occurrence)&&
        actual->topology_revision==s->topology&&actual->context_gui_port&&
        actual->context_gui_port.get()==manager.get()&&!manager.owner_before(actual->context_gui_port)&&
        !actual->context_gui_port.owner_before(manager) && &actual->source->current_operation_loan()==s->operation&&
        s->serial!=0&&s->operation->operation_serial()==s->serial,
        "Runtime dimensions canonical parser/manager/occurrence/source changed");
    const NotificationLayoutBaseline* state=nullptr;
    if(s->id==0){require(actual->context_construction_return&&actual->context_layout_baseline&&
        actual->context_receiver_occurrence==occurrence,"Runtime dimensions actual root allocation changed");
        state=&*actual->context_layout_baseline;
    }else{const auto& n=BnwWorldViewControlTree::Core::node(actual->nodes,s->id);
        require(n.alive&&n.layout_baseline&&n.layout_receiver_occurrence==occurrence,
            "Runtime dimensions actual node allocation changed");state=&*n.layout_baseline;}
    require(state==s->baseline&&(!state->parser_runtime_dimensions_||state->parser_runtime_dimensions_==s->facade)&&
        (!state->parsed_source_serial_||*state->parsed_source_serial_==s->serial),
        "Runtime dimensions temporary/cloned parser baseline refused");
    (void)manager->dimensions_without_callbacks(*s->operation);
    actual->layout_origin(true);s->operation->require_origin_without_callbacks();
    // All checker storage pins end here; no full callback occurs in this method.
}
std::array<std::int32_t,2> detail::NotificationLayoutRuntimeDimensions::dimensions_from_storage_without_callbacks(
    const std::shared_ptr<State>& s){
    require_storage_without_callbacks(s);
    require(!s->registering&&s->baseline->parser_runtime_dimensions_==s->facade,
        "Runtime Size dimensions read outside actual per-layer parser");
    std::array<std::int32_t,2> result;
    {const auto manager=s->manager.lock();require(bool(manager),"Runtime dimensions GUI retired");
        result=manager->dimensions_without_callbacks(*s->operation);}
    require_storage_without_callbacks(s);return result;
}
void detail::NotificationLayoutRuntimeDimensions::fail_storage_without_callbacks(const std::shared_ptr<State>& s)noexcept{
    if(!s)return;s->failed.store(true);if(const auto c=s->core.lock())c->failed.store(true);
    if(const auto m=s->manager.lock())m->impl_->failed.store(true);
}
void detail::NotificationLayoutRuntimeDimensions::register_resize_from_storage_owned(const std::shared_ptr<State>& s,std::uint32_t flags){
    require_storage_without_callbacks(s);
    require(!s->registering&&s->baseline->parser_runtime_dimensions_==s->facade&&
        s->baseline->base_.flags_40==flags&&(flags&0x300000u),"Runtime resize wrong actual flags/reentered");
    s->registering=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{s->registering};
    s->operation->require_current();require_storage_without_callbacks(s);
    std::shared_ptr<NotificationLayoutResizeRegistration> entry;
    {const auto actual=s->core.lock();const auto manager=s->manager.lock();const auto occurrence=s->occurrence.lock();
        require(actual&&manager&&occurrence,"Runtime resize real owner expired before registration");
        auto gui=manager->impl_;gui->first(*s->operation);
        require(gui->revision<UINT64_MAX,"Runtime resize actual GUI history exhausted");
        gui->resize_registrations.reserve(gui->resize_registrations.size()+1);
        gui->history.reserve(gui->history.size()+1);
        entry=std::shared_ptr<NotificationLayoutResizeRegistration>(new NotificationLayoutResizeRegistration(
            actual,manager,occurrence,*s->operation,flags));
        // Actual owned86AC20 analogue: retaining THIS weak canonical receiver
        // in SAME manager callback list. No temporary baseline/facade retained.
        gui->resize_registrations.push_back(entry);
        gui->record(*s->operation,NotificationContextGuiPort::Impl::Store::resize_registration);
        gui->first(*s->operation);}
    // Saved registration uses weak owners; release checker locks BEFORE full POST.
    s->operation->require_current();require_storage_without_callbacks(s);
    {const auto manager=s->manager.lock();require(manager&&
        !manager->impl_->resize_registrations.empty()&&manager->impl_->resize_registrations.back()==entry,
        "Runtime resize actual registration superseded across POST");}
    require_storage_without_callbacks(s);
}
void detail::NotificationContextGuiPort::dispatch_resize_event_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::int32_t event_arg){
    // A future genuine window/GUI event owner must call this private dispatcher.
    // Current initial WindowLease retires on WM_SIZE; preserve that refusal.
    const auto gui=impl_;
    try{require_current_owned(operation);gui->first(operation);
        const auto entries=gui->resize_registrations;
        for(const auto& entry:entries){require(bool(entry),"GUI resize null registered callback");
            entry->dispatch_owned(operation,event_arg);require_current_owned(operation);gui->first(operation);}
    }catch(...){gui->failed.store(true);throw;}
}
#include "notification_layout_button_style_core.inc"
#include "notification_layout_grid_style_core.inc"
#include "notification_layout_grid_common_empty_core.inc"
#include "notification_layout_button_string_layer_core.inc"
#include "notification_layout_embedded_label_prefix_core.inc"
#include "notification_layout_standalone_label_factory_core.inc"
#include "notification_layout_button_label_link_core.inc"
#include "notification_layout_embedded_label_factory_core.inc"
#include "notification_layout_embedded_label_parse_core.inc"
#include "notification_layout_embedded_label_5e_font_prefix_core.inc"
#include "notification_layout_embedded_label_retiring_cleanup_core.inc"
#include "notification_layout_image_color_core.inc"
#include "notification_layout_box_common_receiver_core.inc"
#include "notification_layout_image_common_core.inc"
#include "notification_layout_box_color_core.inc"
#include "notification_layout_box_post_size_prefix_core.inc"
#include "notification_layout_box_common_prefix_core.inc"
#include "notification_layout_label_language_980a_core.inc"
#include "notification_layout_label_color_set_core.inc"
#include "notification_layout_button_tooltip_core.inc"
#include "notification_layout_standalone_label_parser_core.inc"
#include "notification_layout_standalone_label_common_prefix_core.inc"
#include "notification_layout_standalone_label_common_empty_core.inc"
#include "notification_layout_standalone_root_context3c_store_core.inc"
#include "bnw_normal_button_subobject118_backing_core.inc"
#include "notification_layout_button_common_core.inc"
#include "notification_layout_stack_common_core.inc"
std::unique_ptr<BnwWorldViewControlTree::CommonChildParserReturn>
BnwWorldViewControlTree::capture_layout_node_canonical_owned(std::uint64_t id,
    const std::map<std::string,content::XmlNode,std::less<>>& styles,std::uint64_t serial){
    const auto actual=core_;actual->layout_origin(true);const auto& operation=actual->source->current_operation_loan();
    require(serial&&serial==operation.operation_serial()&&!actual->layout_parser_control_id&&
        !actual->layout_parser_borrower,"Canonical actual parser scope replay/reentry");
    if(id&&Core::node(actual->nodes,id).layout_baseline){
        //Flat traversal is only a locator. Skip solely an actual child already attached by SAME parent return.
        const auto attached=[&]{
            actual->layout_origin(true);operation.require_origin_without_callbacks();
            const auto& n=Core::node(actual->nodes,id);
            require(n.alive&&n.layout_baseline,"Flat child actual canonical receiver retired");
            const auto& child=*n.layout_baseline;
            const auto parent=child.base_.parent_38.lock();
            require(parent&&parent->impl_&&child.base_.receiver_owned&&child.base_.common_return_owned&&
                parent->impl_->creator==std::this_thread::get_id()&&parent->impl_->serial==serial&&
                child.base_.receiver_owned->impl_->serial==serial&&!n.dynamic,
                "Flat already-parsed child lacks actual parent attachment return");
            const auto parent_core=parent->impl_->core.lock();const auto parent_canonical=parent->impl_->canonical.lock();
            require(parent_core&&parent_core.get()==actual.get()&&!parent_core.owner_before(actual)&&
                !actual.owner_before(parent_core)&&parent_canonical==parent,
                "Flat child actual parent owner/control replaced");
            const auto& p=Core::node(actual->nodes,parent->impl_->id);
            require(p.alive&&p.layout_baseline,"Flat child actual parent canonical receiver retired");
            const auto& base=p.layout_baseline->base_;const auto child_xml=n.full_xml_occurrence;
            const auto parent_xml=child_xml?child_xml->parent.lock():nullptr;
            require(p.alive&&p.layout_baseline&&p.layout_receiver_occurrence&&base.receiver_owned==parent&&
                base.common_return_owned&&base.children_78&&base.children_78->impl_&&child_xml&&
                parent_xml==p.full_xml_occurrence&&child_xml->ordinal<base.children_78->impl_->ordered.size()&&
                base.children_78->impl_->ordered[child_xml->ordinal]==child.base_.receiver_owned&&
                child.base_.receiver_owned->impl_->occurrence.lock()==n.layout_receiver_occurrence&&
                base.common_return_owned->impl_->selected.lock()==p.full_xml_occurrence,
                "Flat child SAME ordered virtual38 attachment/native common receipt changed");
        };
        attached();operation.require_current();attached();return {};
    }
    detail::NotificationLayoutBaseline* state=nullptr;content::XmlNode authored;
    std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence;
    if(id==0){
        // Actual file-root5D/children belong AFTER installed518/preRead setup,
        // not this cold metadata/common parser. No successful skip or stamp.
        require(actual->context_ordinary_state&&actual->context_ordinary_state->impl_&&
            actual->context_ordinary_state->impl_->native_constructor_return&&
            !actual->context_default_font_style.read_source_serial,
            "Root wrapper requires genuine Context ctor; old cold default-read stamp cannot be renewed");
        throw std::runtime_error("required actual4FEE80 wrapper/FileName/DeferLoad return -> genuine731370 input/518/fullpreRead setup -> RootRead; "
            "cold root_authored cannot replace parsed XML84 or ordinary root file children");
    }else{(void)Core::node(actual->nodes,id);auto& node=actual->nodes[id-1];require(node.alive&&!node.layout_baseline&&
        !node.layout_receiver_occurrence,"Canonical node constructor/parser replay");
        if(!node.dynamic){
            require(bool(node.full_xml_occurrence),"Canonical static parser selected full XML occurrence missing");
            const auto selected_source=node.full_xml_occurrence->source.lock();
            require(selected_source&&selected_source.get()==actual->source.get()&&
                !selected_source.owner_before(actual->source)&&!actual->source.owner_before(selected_source),
                "Canonical static parser actual selected XML Source control changed");
            authored=node.full_xml_occurrence->value;
            operation.require_current();actual->layout_origin(true);
            const auto& fresh=Core::node(actual->nodes,id);
            require(&actual->source->current_operation_loan()==&operation&&fresh.alive&&!fresh.layout_baseline,
                "Canonical full XML allocation Source/node POST changed");
        }else authored=node.layout_parser_authored?*node.layout_parser_authored:node.authored;
        const auto constructor_topology=actual->topology_revision;
        const auto constructor_instance=Core::node(actual->nodes,id).instance;
        auto constructed=detail::NotificationLayoutBaseline::construct_owned(authored);
        operation.require_current();actual->layout_origin(true);operation.require_origin_without_callbacks();
        require(&actual->source->current_operation_loan()==&operation&&actual->topology_revision==constructor_topology&&
            Core::node(actual->nodes,id).alive&&!Core::node(actual->nodes,id).layout_baseline&&
            !Core::node(actual->nodes,id).layout_receiver_occurrence,
            "Canonical ordinary constructor allocation full Source POST changed");
        occurrence=std::make_shared<const Core::LayoutReceiverOccurrence>(id,constructor_instance,serial);
        operation.require_current();actual->layout_origin(true);operation.require_origin_without_callbacks();
        // Keep the canonical validity check; select the owned mutable slot.
        (void)Core::node(actual->nodes,id);
        auto& current_node=actual->nodes.at(static_cast<std::size_t>(id-1));
        require(&actual->source->current_operation_loan()==&operation&&actual->topology_revision==constructor_topology&&
            current_node.alive&&current_node.instance==constructor_instance&&!current_node.layout_baseline&&
            !current_node.layout_receiver_occurrence,"Canonical occurrence allocation full Source POST changed");
        current_node.layout_baseline.emplace(std::move(constructed));
        current_node.layout_receiver_occurrence=occurrence;state=&*current_node.layout_baseline;}
    const auto entered_frame=actual->layout_parser_child_frame;
    const auto entered_root_frame=actual->layout_parser_root_child_frame;
    actual->layout_parser_styles=&styles;
    actual->layout_parser_control_id=id;actual->layout_parser_occurrence=occurrence;actual->layout_parser_facade=this;
    struct Scope{Core& core;~Scope(){core.layout_parser_borrower=nullptr;core.layout_parser_facade=nullptr;
        core.layout_parser_styles=nullptr;core.layout_parser_occurrence.reset();core.layout_parser_control_id.reset();}}scope{*actual};
    const auto runtime=detail::NotificationLayoutRuntimeDimensions::capture_owned(*this,id,operation,actual->context_gui_port);
    // Ordinary Button owns real5C23C0 dispatch. Do not pre-flatten its Style
    // into caller metadata; that would lose local/global lookup and call order.
    if(authored.name=="Label")runtime->construct_standalone_label_factory_owned();
    const auto layers=(authored.name=="Button"||authored.name=="Grid"||authored.name=="Label"||authored.name=="Stack")?std::vector<content::XmlNode>{authored}:Core::layout_parser_layers(authored,styles);
    operation.require_current();detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(runtime->state_);
    state->parse_canonical_owned(layers,styles,serial,runtime.get());
    detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(runtime->state_);
    // Retained genuine player/UI commands follow parser; no subtype4C issued.
    if(id){auto& value=Core::node(actual->nodes,id);
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::label)
            require(!value.mouse_consume_command&&!value.layout_hidden_command&&!value.offset&&
                !value.size_y_override&&!value.text&&!value.tooltip&&!value.color,
                "Standalone retained live setter required; flat command replay cannot renew actual wrapper return");
        require(&*value.layout_baseline==state&&value.layout_receiver_occurrence==occurrence,
            "Canonical parser actual receiver changed before retained commands");
        if(value.mouse_consume_command){const auto& command=*value.mouse_consume_command;
            require((command.bit800==0||command.bit800==0x800U)&&command.source_serial&&command.effect_revision,
                "Layout retained actual MouseConsumeCommand history malformed");
            state->record_owned(detail::NotificationLayoutBaseline::Mutation::consume_mouse_over);
            state->base_.flags_40=(state->base_.flags_40&~0x800U)|command.bit800;}
        if(value.layout_hidden_command)state->hide_owned(*value.layout_hidden_command);
        if(value.offset)state->offset_prefix_owned(*value.offset);
        if(value.size_y_override)state->size_y_prefix_owned(*value.size_y_override);
        if(value.text)state->text_prefix_owned(*value.text);
        require(!(value.tooltip&&
            ((state->kind_==detail::NotificationLayoutBaseline::Kind::button&&state->actual_type_=="Button")||
             (state->kind_==detail::NotificationLayoutBaseline::Kind::image&&state->actual_type_=="Image"))),
            "Retained Button/Image ToolTip cannot replay metadata without genuine54/flags4000/GUI524CC0 issuer");
        // A genuine retained data/API SetColor before this later canonical
        // construction is replayed AFTER authored Color, just like the prior
        // retained hide/offset commands. This earns no earlier Image receipt.
        require(!(value.color&&state->kind_==detail::NotificationLayoutBaseline::Kind::grid),
            "Retained Grid SetColor requires its own genuine6BF870 command receiver");
        if(value.color&&state->kind_==detail::NotificationLayoutBaseline::Kind::image){
            require(static_cast<bool>(state->image_),"Retained Image Color lacks actual embedded constructor");
            for(std::size_t i=0;i<4;++i)state->image_->color_98_a4[i]=static_cast<float>((*value.color)[i]);
        }
        if(value.color&&state->kind_==detail::NotificationLayoutBaseline::Kind::button){
            require(static_cast<bool>(state->button_),"Retained Button Color lacks actual embedded constructor");
            if(state->actual_type_=="Button")detail::BnwNormalButtonSubobject118Backing::invalidate_direct580780_color_before_store_owned(*state->button_);
            for(std::size_t i=0;i<4;++i)state->button_->color_12c_138[i]=static_cast<float>((*value.color)[i]);
        }}
    actual->layout_origin(true);
    if(entered_frame||entered_root_frame){
        //Only genuine native Image/Box returning factory bodies issue v44-return.
        require((state->kind_==detail::NotificationLayoutBaseline::Kind::image||
            state->kind_==detail::NotificationLayoutBaseline::Kind::box||
            state->kind_==detail::NotificationLayoutBaseline::Kind::label||
            state->kind_==detail::NotificationLayoutBaseline::Kind::grid||
            state->kind_==detail::NotificationLayoutBaseline::Kind::button||
            state->kind_==detail::NotificationLayoutBaseline::Kind::stack)&&state->base_.common_return_owned,
            "Actual child factory subtype parser has not completed its native returning path");
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::box)runtime->require_box_common_return_from_parser_owned();
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::label)runtime->require_standalone_label_common_return_from_parser_owned();
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::grid)runtime->require_grid_parser_owned(state->grid_.get());
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::button)runtime->require_button_common_return_from_parser_owned();
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::stack)runtime->require_stack_common_return_from_parser_owned();
        auto returned=entered_frame?std::unique_ptr<CommonChildParserReturn>(new CommonChildParserReturn(
            entered_frame,runtime->state_,occurrence,Core::node(actual->nodes,id).full_xml_occurrence)):
            std::unique_ptr<CommonChildParserReturn>(new CommonChildParserReturn(
            entered_root_frame,runtime->state_,occurrence,Core::node(actual->nodes,id).full_xml_occurrence));
        detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(runtime->state_);
        operation.require_current();detail::NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(runtime->state_);
        require(state->base_.common_return_owned&&actual->layout_parser_child_frame==entered_frame&&
            actual->layout_parser_root_child_frame==entered_root_frame,
            "Ordinary factory return allocation Source/frame POST changed");
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::label)runtime->require_standalone_label_common_return_from_parser_owned();
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::box)runtime->require_box_common_return_from_parser_owned();
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::button)runtime->require_button_common_return_from_parser_owned();
        if(state->kind_==detail::NotificationLayoutBaseline::Kind::stack)runtime->require_stack_common_return_from_parser_owned();
        return returned; //Runtime and actual Binding Scope DTOR run before caller can restore parent.
    }
    return {};
}

// After actual ordinary/standalone cells and paired Source7 store implementation.
#include "notification_context_root_child_attach_store_core.inc"
#include "notification_layout_standalone_root_parent38_store_core.inc"
#include "notification_context_root_child_attach_return_core.inc"
#include "notification_context_root_file_watcher_core.inc"
#include "notification_context_root_path_watch_driver_core.inc"
#include "notification_context_root_xml84_name_core.inc"
#include "notification_context_root_default_fields_core.inc"
#include "notification_context_direct_loader_core.inc"
#include "civ5/ui/detail/notification_context_root_read_children_loan.hpp"
#include "notification_context_xml_wrapper_core.inc"
#include "civ5/ui/detail/notification_context_lua_controls_return.hpp"
#include "notification_context_root_child_attempt_type_core.inc"
#include "notification_context_root_declaration_cursor_core.inc"
#include "notification_context_root_ordinary_factory_core.inc"
#include "civ5/ui/detail/notification_context_child_controls_export_loan.hpp"
#include "notification_context_child_factory_core.inc"
#include "notification_context_root_child_factory_return_core.inc"
#include "notification_context_child_controls_entry_core.inc"
#include "notification_context_ordinary_controls_v64_core.inc"
#include "notification_context_root_read_completion_body_core.inc"
