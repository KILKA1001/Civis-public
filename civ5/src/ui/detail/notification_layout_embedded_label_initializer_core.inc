// Included inside civ5::ui AFTER genuine ContextChildSlot10Frame/Binding and
// complete Child/Factory/Realm. The sole lexical issuer is actual6F6C80 below.
struct detail::NotificationLayoutEmbeddedLabelState::InitializationLoan {
    using Parent=NotificationLayoutParentReceiver;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    enum class Role{initial_button,live_set_text};
    const Role role;
    const BnwWorldViewControlTree*const setter_tree=nullptr;
    const std::optional<BnwWorldViewControlTree::Control> setter_control;
    mutable std::uint64_t live_child_sequence=0;
    const std::uint64_t setter_serial;
    Parent::TreeView*const view;
    const std::weak_ptr<Parent::TreeView::Binding> binding;
    const Parent::NodeKey parent_key;
    NotificationLayoutBaseline*const parent_baseline;
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> returned;
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState> child;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    mutable std::uint64_t parent_sequence;
    InitializationLoan(Parent::TreeView& actual,Parent::NodeKey key)
        :role(Role::initial_button),setter_serial(actual.binding_->serial),view(&actual),binding(actual.binding_),parent_key(key),
         parent_baseline(Parent::node(actual,key).state),
         returned(parent_baseline->button_->label_178_factory_return_),
         child(parent_baseline->button_->label_178_),operation(actual.binding_->operation),
         parent_sequence(parent_baseline->sequence_){first();}
    // Fresh lexical constructor: only the actual checked Tree setter can mint
    // this role. Factory data below authenticates allocation ancestry only;
    // no historical Runtime/String/Factory/initialization scope is invoked.
    InitializationLoan(Parent::TreeView& actual,Parent::NodeKey key,
        const BnwWorldViewControlTree& tree,BnwWorldViewControlTree::Control control)
        :role(Role::live_set_text),setter_tree(&tree),setter_control(std::move(control)),
         setter_serial(actual.binding_->serial),view(&actual),binding(actual.binding_),parent_key(key),
         parent_baseline(Parent::node(actual,key).state),
         returned(parent_baseline->button_->label_178_factory_return_),
         child(parent_baseline->button_->label_178_),operation(actual.binding_->operation),
         parent_sequence(parent_baseline->sequence_){
        require(bool(child),"Live SetText missing actual embedded child");
        live_child_sequence=child->impl_->sequence;first_live();
    }
    void first_live()const{
        require(role==Role::live_set_text&&creator==std::this_thread::get_id()&&alive.load(),
            "Live SetText lexical issuer retired/foreign");
        const auto owner=binding.lock();require(owner&&owner->graph_active&&!owner->finished&&view,
            "Live SetText actual Binding retired");
        owner->pure(true);require(view->binding_.get()==owner.get()&&
            !view->binding_.owner_before(owner)&&!owner.owner_before(view->binding_),
            "Live SetText substituted View/control block");
        require(setter_tree&&setter_control&&owner->serial==setter_serial&&setter_serial&&
            owner->operation==operation&&parent_key.domain==0&&parent_key.id&&
            owner->domains.front().core.get()==setter_tree->core_.get()&&
            !owner->domains.front().core.owner_before(setter_tree->core_)&&
            !setter_tree->core_.owner_before(owner->domains.front().core)&&
            setter_tree->core_->checked(*setter_control)==parent_key.id&&
            Parent::node(*view,parent_key).state==parent_baseline,
            "Live SetText requires SAME actual Tree/Control/Source occurrence");
        operation->require_origin_without_callbacks();view->require_pure();
        const auto& p=*parent_baseline;
        require(p.kind_==NotificationLayoutBaseline::Kind::button&&p.actual_type_=="Button"&&
            p.parsed_source_serial_&&p.sequence_==parent_sequence&&(p.base_.flags_40&0x08000000u)&&p.button_&&returned&&child&&
            p.button_->child_178_observation==NotificationLayoutButtonState::Child178Observation::factory_child&&
            !p.button_->pending_label_178_&&p.button_->label_178_.get()==child.get()&&
            !p.button_->label_178_.owner_before(child)&&!child.owner_before(p.button_->label_178_)&&
            p.button_->label_178_factory_return_.get()==returned.get()&&
            !p.button_->label_178_factory_return_.owner_before(returned)&&
            !returned.owner_before(p.button_->label_178_factory_return_),
            "Live SetText actual parsed parent/178/allocation membership changed");
        const auto fields=child->impl_;fields->first();
        const auto& allocation=*returned->impl_;
        require(allocation.creator==creator&&allocation.alive.load()&&allocation.published_sequence&&
            allocation.child.get()==child.get()&&allocation.source_serial==fields->source_serial&&
            allocation.module.get()==fields->module.get()&&
            !allocation.module.owner_before(fields->module)&&!fields->module.owner_before(allocation.module)&&
            fields->sequence==live_child_sequence,
            "Live SetText immutable constructor allocation or current child journal changed");
        fields->module->require_label_live_allocation_without_callbacks(
            *operation,allocation.allocation,child,fields->source_serial);fields->first();
        const auto parent=fields->construction_parent.lock();
        const auto* stored_parent=std::get_if<Impl::ParentBorrow>(&fields->fields[0x38/4]);
        const auto* stored_realm=std::get_if<Impl::ParsedRealm>(&fields->fields[0x3c/4]);
        const auto linked=stored_parent?stored_parent->parent.lock():nullptr;
        require(parent&&parent.get()==p.button_.get()&&!parent.owner_before(p.button_)&&
            !p.button_.owner_before(parent)&&stored_parent&&stored_parent->baseline==&p&&linked&&
            linked.get()==p.button_.get()&&!linked.owner_before(p.button_)&&!p.button_.owner_before(linked)&&
            stored_realm&&stored_realm->value&&p.base_.context_realm_3c&&
            p.base_.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
            stored_realm->value.get()==p.base_.context_realm_3c.get()&&
            !stored_realm->value.owner_before(p.base_.context_realm_3c)&&
            !p.base_.context_realm_3c.owner_before(stored_realm->value)&&
            owner->domains.front().core->context_realm_identity.get()==stored_realm->value.get(),
            "Live SetText current actual child38/3C/Context custody changed");
        //Current allocation identity only; no ContextArgument loan revival.
        stored_realm->value->impl_->first(*operation);view->require_pure();fields->first();
    }

    ~InitializationLoan(){alive.store(false);}
    void first()const{
        if(role==Role::live_set_text){first_live();return;}
        // FIRST precedes borrowed View/Node/Factory storage. This is never an
        // ordinary resurrected Runtime/ContextArgument/String lexical loan.
        require(creator==std::this_thread::get_id()&&alive.load(),"Label init lexical borrower retired/foreign");
        const auto owner=binding.lock();require(owner&&owner->graph_active&&!owner->finished&&view,
            "Label init genuine active Binding retired");
        require(view->binding_.get()==owner.get()&&!view->binding_.owner_before(owner)&&
            !owner.owner_before(view->binding_),"Label init substituted View/Binding");
        view->require_pure();operation->require_origin_without_callbacks();view->require_pure();
        require(owner->operation==operation&&parent_key.domain<owner->domains.size()&&parent_key.id&&
            Parent::node(*view,parent_key).state==parent_baseline,
            "Label init SAME canonical ordinary parent absent");
        const auto& parent=*parent_baseline;
        require(parent.kind_==NotificationLayoutBaseline::Kind::button&&parent.actual_type_=="Button"&&
            parent.parsed_source_serial_&&parent.sequence_==parent_sequence&&parent.button_&&
            parent.button_->child_178_observation==NotificationLayoutButtonState::Child178Observation::factory_child&&
            !parent.button_->pending_label_178_&&parent.button_->label_178_.get()==child.get()&&
            !parent.button_->label_178_.owner_before(child)&&!child.owner_before(parent.button_->label_178_)&&
            parent.button_->label_178_factory_return_.get()==returned.get()&&
            !parent.button_->label_178_factory_return_.owner_before(returned)&&
            !returned.owner_before(parent.button_->label_178_factory_return_),
            "Label init actual parsed Button/178/Factory membership changed");
        require(returned&&returned->impl_->creator==creator&&returned->impl_->alive.load()&&
            returned->impl_->published_sequence&&returned->impl_->child.get()==child.get(),
            "Label init genuine full Factory return retired");
        const auto fields=child->impl_;fields->first();
        const auto construction_parent=fields->construction_parent.lock();
        require(construction_parent&&construction_parent.get()==parent.button_.get()&&
            !construction_parent.owner_before(parent.button_)&&!parent.button_.owner_before(construction_parent)&&
            fields->source_serial==returned->impl_->source_serial&&
            returned->impl_->module.get()==fields->module.get()&&
            !returned->impl_->module.owner_before(fields->module)&&!fields->module.owner_before(returned->impl_->module),
            "Label init constructor allocation/Source/module changed");
        fields->module->require_label_live_allocation_without_callbacks(*operation,returned->impl_->allocation,child,fields->source_serial);
        require(parent.base_.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
            parent.base_.context_realm_3c,"Label init parent3C is not earned ordinary parsed Realm");
        parent.base_.context_realm_3c->impl_->first(*operation);fields->first();view->require_pure();
    }
    void full()const{first();view->require_full();first();}
    void parent_bounds_94_owned()const{
        full();const auto saved=binding.lock();
        require(saved&&saved->operation==operation,"Label94 actual parent Binding absent");
        Parent::slot94(*view,parent_key); //Actual A3C9C0+94=6EBC30, genuine ancestor fanout.
        //Parent receiver itself completed its Source/View FIRST+POST. Validate
        //the SAME still-active identity before renewing this actual journal
        //observation; no arbitrary changed Parent is accepted by first().
        require(alive.load()&&creator==std::this_thread::get_id()&&saved->graph_active&&!saved->finished&&
            view->binding_.get()==saved.get(),"Label94 parent return retired Binding");
        view->require_pure();operation->require_origin_without_callbacks();view->require_pure();
        require(Parent::node(*view,parent_key).state==parent_baseline&&
            parent_baseline->button_&&parent_baseline->button_->label_178_.get()==child.get()&&
            parent_baseline->button_->label_178_factory_return_.get()==returned.get()&&
            parent_baseline->sequence_>=parent_sequence,
            "Label94 real parent return membership/journal changed");
        parent_sequence=parent_baseline->sequence_;first();full();
    }
    void child_store(std::uint32_t offset)const{
        first();view->preflight_store_owned(parent_key);
        auto& fields=*child->impl_;
        require(fields.sequence<UINT64_MAX&&fields.history.size()<fields.history.capacity(),
            "Label init child journal was not preflighted");
        fields.history.push_back({++fields.sequence,offset});
        if(role==Role::live_set_text)live_child_sequence=fields.sequence;
        // Binding tracks the actual embedded child mutation under its real
        // parent; this is not a new ordinary Tree control or guessed ControlID.
        view->record_store_owned(parent_key,Parent::Store::Kind::button);first();
    }
};

struct detail::BnwLabelCurrentTextLoan::Impl {
    const std::shared_ptr<StandaloneImpl> standalone{};
    explicit Impl(std::shared_ptr<StandaloneImpl>);
    void first_standalone_owned()const;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const std::weak_ptr<NotificationLayoutEmbeddedLabelState::InitializationLoan> initialization;
    const std::weak_ptr<NotificationLayoutEmbeddedLabelState::Impl> child;
    const std::shared_ptr<NotificationLayoutEmbeddedLabelState::Impl::TextObject> text;
    const NotificationLayoutEmbeddedLabelState::Impl::Cell*const actual_c4_address;
    const std::shared_ptr<const BnwCompletedFontLoan> cc;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t operation_serial;
    const Phase phase;
    const std::vector<char> value; //FULL actual immutable C4 bytes, including true terminator.
    static NotificationLayoutEmbeddedLabelState::InitializationLoan& checked(
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState::InitializationLoan>& init){
        require(bool(init),"Label C4 no genuine initializer");init->first();return *init;
    }
    static std::vector<char> copy_value(
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState::InitializationLoan>& init,
        const std::shared_ptr<NotificationLayoutEmbeddedLabelState::Impl::TextObject>& actual){
        const auto validate=[&]{checked(init);
            const auto* cell=std::get_if<NotificationLayoutEmbeddedLabelState::Impl::TextBorrow>(
                &init->child->impl_->fields[0xc4/4]);
            require(actual&&cell&&cell->object.get()==actual.get()&&
                !cell->object.owner_before(actual)&&!actual.owner_before(cell->object)&&
                actual->creator==std::this_thread::get_id()&&!actual->retired.load()&&
                !actual->bytes.empty()&&actual->bytes.size()<=0x4000&&actual->bytes.back()=='\0'&&
                std::find(actual->bytes.begin(),actual->bytes.end(),'\0')==actual->bytes.end()-1,
                "Label C4 FIRST before allocating VALUE copy");};
        validate();auto copied=actual->bytes;validate();
        require(copied==actual->bytes,"Label C4 VALUE changed across allocating copy");return copied;
    }
    Impl(std::shared_ptr<NotificationLayoutEmbeddedLabelState::InitializationLoan> init,
        std::shared_ptr<NotificationLayoutEmbeddedLabelState::Impl::TextObject> actual,
        std::shared_ptr<const BnwCompletedFontLoan> font,
        std::shared_ptr<const BnwLabelSystemFontBorrower> cache,Phase mode)
        :initialization(init),child(checked(init).child->impl_),text(std::move(actual)),
         actual_c4_address(&checked(init).child->impl_->fields[0xc4/4]),cc(std::move(font)),system(std::move(cache)),
         operation(checked(init).operation),operation_serial(checked(init).operation->operation_serial()),
         phase(mode),value(copy_value(init,text)){first();}
    void first()const{
        if(standalone){first_standalone_owned();return;}
        require(creator==std::this_thread::get_id()&&alive.load(),"Label C4 lexical input retired/foreign");
        const auto init=initialization.lock();require(bool(init),"Label C4 actual init frame expired");
        init->first();require(init->operation==operation&&operation_serial&&operation->operation_serial()==operation_serial,"Label C4 bound Source operation changed");
        const auto fields=child.lock();require(fields&&fields.get()==init->child->impl_.get(),
            "Label C4 actual child allocation expired or substituted");fields->first();
        require(&fields->fields[0xc4/4]==actual_c4_address,"Label C4 actual field identity changed");
        using F=NotificationLayoutEmbeddedLabelState::Impl;
        const auto* slot=std::get_if<F::TextBorrow>(actual_c4_address);
        const auto* font=std::get_if<F::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* cache=std::get_if<F::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto allocation=text->allocation_child.lock();
        require(slot&&slot->object.get()==text.get()&&!slot->object.owner_before(text)&&!text.owner_before(slot->object)&&
            text->creator==creator&&!text->retired.load()&&allocation&&allocation.get()==fields.get()&&
            !allocation.owner_before(fields)&&!fields.owner_before(allocation)&&
            text->source_serial==fields->source_serial&&text->source_serial&&
            !text->bytes.empty()&&text->bytes.size()<=0x4000&&text->bytes.back()=='\0'&&
            std::find(text->bytes.begin(),text->bytes.end(),'\0')==text->bytes.end()-1&&
            text->bytes==value&&
            font&&font->loan.get()==cc.get()&&!font->loan.owner_before(cc)&&!cc.owner_before(font->loan)&&
            cc&&system&&cache&&cache->loan.get()==system.get()&&
            !cache->loan.owner_before(system)&&!system.owner_before(cache->loan),
            "Label C4 actual published Text/CC/170 identity or VALUE changed");
        //The zero-count reset qualifies only the actual issuer. Real7505
        //subsequently appends records before following glyph callbacks; the
        //unique lexical Loan's alive gate and genuine phase remain authority.
        require((phase==Phase::position_7505&&fields->init_stage==F::InitStage::glyph_generated)||
            (phase==Phase::size_5077&&fields->init_stage==F::InitStage::positioned_returned),
            "Label C4 producer phase not at actual preparation callsite");
        init->child->require_initialization_font_current_owned(init);init->first();
    }
};
detail::BnwLabelCurrentTextLoan::BnwLabelCurrentTextLoan(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
detail::BnwLabelCurrentTextLoan::~BnwLabelCurrentTextLoan(){impl_->alive.store(false);}
detail::BnwLabelCurrentTextLoan::Guard::Guard(std::shared_ptr<Impl> impl):impl_(std::move(impl)){}
detail::BnwLabelCurrentTextLoan::Guard detail::BnwLabelCurrentTextLoan::capture_guard_without_callbacks()const{
    const auto data=impl_;data->first();return Guard(data);
}
void detail::BnwLabelCurrentTextLoan::Guard::require_current_without_callbacks()const{impl_->first();}
void detail::BnwLabelCurrentTextLoan::Guard::require_system_font_borrower_without_callbacks(
    const BnwLabelSystemFontBorrower& actual)const{
    impl_->first();
    //first() already proves the actual live Child170 shared object/control
    //equals this captured system descriptor. A reference has no independent
    //control block; SAME object address now proves that exact borrowed operand.
    require(impl_->system&&impl_->system.get()==&actual,
        "Label C4 input belongs to another actual system-font borrower");
    impl_->first();
}
void detail::BnwLabelCurrentTextLoan::Guard::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    impl_->first();require(&op==impl_->operation&&op.operation_serial()==impl_->operation_serial,
        "Label C4 foreign Source operation/serial");
    const auto init=impl_->initialization.lock();require(bool(init),"Label C4 init expired before full Source check");
    init->full();impl_->first();init->child->require_initialization_font_source_owned(init);
    impl_->first();init->full();impl_->first();
}
std::string_view detail::BnwLabelCurrentTextLoan::Guard::actual_utf8_without_callbacks()const{
    impl_->first();return std::string_view(impl_->value.data(),impl_->value.size()-1);
}
std::string_view detail::BnwLabelCurrentTextLoan::Guard::actual_c4_bytes_without_callbacks()const{
    impl_->first();return std::string_view(impl_->value.data(),impl_->value.size());
}
detail::BnwLabelCurrentTextLoan::Phase detail::BnwLabelCurrentTextLoan::Guard::phase_without_callbacks()const{
    impl_->first();return impl_->phase;
}
void detail::BnwLabelCurrentTextLoan::require_current_without_callbacks()const{
    const auto guard=capture_guard_without_callbacks();guard.require_current_without_callbacks();
}
void detail::BnwLabelCurrentTextLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto guard=capture_guard_without_callbacks();guard.require_source_owned(op);
}
std::string_view detail::BnwLabelCurrentTextLoan::actual_utf8_without_callbacks()const{
    const auto guard=capture_guard_without_callbacks();return guard.actual_utf8_without_callbacks();
}
std::string_view detail::BnwLabelCurrentTextLoan::actual_c4_bytes_without_callbacks()const{
    const auto guard=capture_guard_without_callbacks();return guard.actual_c4_bytes_without_callbacks();
}
detail::BnwLabelCurrentTextLoan::Phase detail::BnwLabelCurrentTextLoan::phase_without_callbacks()const{
    const auto guard=capture_guard_without_callbacks();return guard.phase_without_callbacks();
}
void detail::NotificationLayoutEmbeddedLabelState::require_initialization_font_current_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    loan->first();require(loan->child.get()==this,"Label current Font guard wrong actual child");
    const auto* font=std::get_if<Impl::FontBorrow>(&impl_->fields[0xcc/4]);
    const auto* system=std::get_if<Impl::SystemFontBorrow>(&impl_->fields[0x170/4]);
    require(font&&font->loan&&system&&system->loan,"Label current actualCC/170 missing");
    font->loan->require_current_without_callbacks();loan->first();
    system->loan->require_current_without_callbacks();loan->first();
}
void detail::NotificationLayoutEmbeddedLabelState::require_initialization_font_source_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    require_initialization_font_current_owned(loan);
    const auto font=std::get<Impl::FontBorrow>(impl_->fields[0xcc/4]).loan;
    const auto system=std::get<Impl::SystemFontBorrow>(impl_->fields[0x170/4]).loan;
    font->require_source_owned(*loan->operation);loan->first();
    system->require_source_owned(*loan->operation);loan->first();
    require_initialization_font_current_owned(loan);
}

std::unique_ptr<detail::BnwLabelCurrentTextLoan>
detail::NotificationLayoutEmbeddedLabelState::current_position_text_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    loan->full();require(loan->child.get()==this,"Label C4 issuer wrong actual child");
    const auto reset=[&]{loan->first();
        const auto* count=std::get_if<std::uint32_t>(&impl_->fields[0x138/4]);
        require(impl_->init_stage==Impl::InitStage::glyph_generated&&count&&!*count,
            "Label C4 position issuer is not at actual75067C reset boundary");};
    reset();
    const auto* text=std::get_if<Impl::TextBorrow>(&impl_->fields[0xc4/4]);
    const auto* font=std::get_if<Impl::FontBorrow>(&impl_->fields[0xcc/4]);
    const auto* system=std::get_if<Impl::SystemFontBorrow>(&impl_->fields[0x170/4]);
    require(text&&text->object&&font&&font->loan&&system&&system->loan,"Label C4 actual sealed operands missing");
    //Snapshot all shared operands by VALUE while their exact field controls
    //are current. Never forward lvalue references into variant cells across
    //make_shared/new_handler or the allocating immutable VALUE copy.
    const auto init_snapshot=loan;
    const auto text_snapshot=text->object;
    const auto font_snapshot=font->loan;
    const auto system_snapshot=system->loan;reset();
    auto data=std::make_shared<BnwLabelCurrentTextLoan::Impl>(init_snapshot,text_snapshot,font_snapshot,system_snapshot,
        BnwLabelCurrentTextLoan::Phase::position_7505);loan->full();data->first();reset();
    auto result=std::unique_ptr<BnwLabelCurrentTextLoan>(new BnwLabelCurrentTextLoan(data));
    data->first();reset();loan->full();data->first();reset();
    //Full POST after both allocations, before the actual lexical return.
    return result;
}
std::unique_ptr<detail::BnwLabelCurrentTextLoan>
detail::NotificationLayoutEmbeddedLabelState::current_size_text_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    loan->full();require(loan->child.get()==this,"Label C4 size issuer wrong actual child");
    const auto phase=[&]{loan->first();require(impl_->init_stage==Impl::InitStage::positioned_returned,
        "Label C4 size issuer is not at actual5077 pre-ellipsis boundary");};phase();
    const auto* text=std::get_if<Impl::TextBorrow>(&impl_->fields[0xc4/4]);
    const auto* font=std::get_if<Impl::FontBorrow>(&impl_->fields[0xcc/4]);
    const auto* system=std::get_if<Impl::SystemFontBorrow>(&impl_->fields[0x170/4]);
    require(text&&text->object&&font&&font->loan&&system&&system->loan,"Label C4 size sealed operands missing");
    const auto init_snapshot=loan;
    const auto text_snapshot=text->object;
    const auto font_snapshot=font->loan;
    const auto system_snapshot=system->loan;phase();
    auto data=std::make_shared<BnwLabelCurrentTextLoan::Impl>(init_snapshot,text_snapshot,font_snapshot,system_snapshot,
        BnwLabelCurrentTextLoan::Phase::size_5077);loan->full();data->first();phase();
    auto result=std::unique_ptr<BnwLabelCurrentTextLoan>(new BnwLabelCurrentTextLoan(data));
    data->first();phase();loan->full();data->first();phase();
    return result;
}

void detail::NotificationLayoutEmbeddedLabelState::initialize_from_parent_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;fields->first();require(loan&&loan->child.get()==this,"Label init wrong actualchild");
    try{
        loan->full();require(loan->role==InitializationLoan::Role::initial_button,
            "Label initial Button entry cannot issue live SetText reinitialization");
        require(fields->init_stage==Impl::InitStage::none,"Label actual init reentry/replay");
        require(fields->history.max_size()-fields->history.size()>=96&&fields->sequence<=UINT64_MAX-96,
            "Label init journal exhausted before first child store");
        fields->history.reserve(fields->history.size()+96);loan->full();
        const auto parent=loan->parent_baseline->button_;
        const auto realm=loan->parent_baseline->base_.context_realm_3c;
        //6F6C8A/96: genuine SAME parent THIS and actually parsed parent3C.
        loan->child_store(0x38);fields->fields[0x38/4]=Impl::ParentBorrow{parent,loan->parent_baseline};loan->first();
        loan->child_store(0x3c);fields->fields[0x3c/4]=Impl::ParsedRealm{realm};loan->first();
        if(!(loan->parent_baseline->base_.flags_40&0x40000000u)){
            loan->child_store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=0x90u;loan->first();
        }
        fields->init_stage=Impl::InitStage::parent_linked;
        //Actual vslot10=710270. NonNULL actualCC earns the missingFont538 /
        //Gui-default branch short-circuit. An unknown Font is never NULL.
        const auto* font_cell=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        require(font_cell&&font_cell->loan,"Label710270 missingCC requires genuine Context538/defaultFont supplier");
        const auto font=font_cell->loan;loan->first();font->require_source_owned(*loan->operation);loan->first();
        const auto binding=loan->binding.lock();require(bool(binding),"Label710270 Binding retired");
        const auto core=binding->domains.at(loan->parent_key.domain).core;
        const auto gui=core->context_gui_port;require(bool(gui),"Label710270 actual Gui retired");
        auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
        bool extended=false;
        if(!(flags&0x40000000u)){
            extended=gui->label_language_980a_owned(*loan->operation)!=0;loan->first();
            flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
        }
        //7102DC asks actual Font40C10 cache. Existing typed170 is only a weak
        //descriptor, so replacing it never destroys the global Gui cache.
        if((flags&0x40000000u)||(extended&&!(flags&0x20000000u))){
            auto system=gui->begin_system_font_lookup_owned(*loan->operation,*font);loan->first();
            font->require_source_owned(*loan->operation);loan->first();
            if(system){system->require_source_owned(*loan->operation);loan->first();}
            loan->child_store(0x170);
            if(system)fields->fields[0x170/4]=Impl::SystemFontBorrow{std::move(system)};
            else fields->fields[0x170/4]=Impl::Null{}; //Only issuer's genuine SQL MISS.
        }else{loan->child_store(0x170);fields->fields[0x170/4]=Impl::Null{};}
        loan->first();fields->init_stage=Impl::InitStage::font_recalculated;
        require(fields->style_c8.has_value(),"Label7102FC C8 unknown");
        if(!*fields->style_c8){
            //Actual parent3C still-live Context owns534; destroyed Runtime /
            //retired ContextArgument can never issue this DATA getter.
            auto style=realm->default_font_style_owned(*loan->operation);loan->first();
            font->require_source_owned(*loan->operation);loan->first();
            const auto ordinal=font->style_ordinal_without_callbacks(style);loan->first();
            loan->child_store(0xc8);fields->style_c8=std::uint8_t(ordinal);loan->first();
            require(!(std::get<std::uint32_t>(fields->fields[0x40/4])&1u),
                "Label710329 preCommon6A is not actual earlyRET state");
        }
        require(std::get<std::uint32_t>(fields->fields[0x40/4])&0x04000000u,
            "Label71032E missing Color flag requires genuine Context53C default64bytes supplier");
        fields->init_stage=Impl::InitStage::defaults_returned;loan->full();
        initialize_common_5761_owned(loan);loan->full();
        rebuild_initialized_layout_owned(loan);loan->full();
    }catch(...){fields->failed.store(true);if(loan)if(const auto b=loan->binding.lock())b->poison();throw;}
}

void detail::NotificationLayoutEmbeddedLabelState::initialize_common_5761_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    require(fields->init_stage==Impl::InitStage::defaults_returned,"Label5761 missing actual710270 prefix");
    loan->child_store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=1u;loan->first(); //5761EC BEFORE4C
    //Actual vslot4C=79D300, ParentButton kind48=49FDF0 returns1, so the
    //ScrollPanel kind12/calculated-size branch is genuinely unreachable.
    const auto f=[&](std::uint32_t offset){
        const auto* bits=std::get_if<std::uint32_t>(&fields->fields[offset/4]);require(bits,"Label4C unissued geometry");
        const auto value=std::bit_cast<float>(*bits);require(std::isfinite(value),"Label4C nonfinite geometry");return value;
    };
    const auto put=[&](std::uint32_t offset,float value){
        require(std::isfinite(value),"Label4C/5761 geometry overflow");loan->child_store(offset);
        fields->fields[offset/4]=std::bit_cast<std::uint32_t>(value);loan->first();
    };
    put(0x2c,f(0x74));put(0x28,f(0x70)); //79D309/79D312 literal order.
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    require((flags&0x3f0u)==0||(flags&0x3f0u)==0x90u,
        "Label79D300 unknown anchor branches need genuine qualifier");
    const auto add=[](float a,float b){volatile float value=a+b;require(std::isfinite(value),"Label SSE add overflow");return value;};
    const auto centered=[&](float parent,float own,float margin){
        require(std::isfinite(parent),"Label79D300 actual parent size nonfinite");
        volatile float delta=parent-own;volatile float half=delta*0.5f;
        require(std::isfinite(half),"Label79D300 centered operand overflow");
        const auto rounded=half>=0?std::floor(double(half)+0.5):std::ceil(double(half)-0.5);
        return add(margin,float(rounded)); //actual rawPE .5 / zero comparison.
    };
    const auto& parent_size=loan->parent_baseline->base_.size_30_34;
    if(flags&0x10u)put(0x28,centered(parent_size[0],f(0x30),f(0x28)));
    if(flags&0x80u)put(0x2c,centered(parent_size[1],f(0x34),f(0x2c)));
    //Actual4C returned before5761 geometry. Two finite float operands have
    //the same finalFP32 extent under normalized x64 arithmetic.
    put(0x20,float(std::fabs(static_cast<long double>(f(0x70)))+f(0x30)));
    put(0x24,float(std::fabs(static_cast<long double>(f(0x74)))+f(0x34)));
    put(0x14,f(0x2c));put(0x10,f(0x28));
    put(0x18,add(f(0x10),f(0x30)));put(0x1c,add(f(0x14),f(0x34)));
    require(std::holds_alternative<Impl::Null>(fields->fields[0x78/4]),
        "Label57624F nonNULL ordinary children require actual slot10 traversal");
    fields->init_stage=Impl::InitStage::common_returned;loan->full();
}

void detail::NotificationLayoutEmbeddedLabelState::generate_text_segments_5a_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    require((loan->role==InitializationLoan::Role::initial_button&&fields->init_stage==Impl::InitStage::common_returned)||
        (loan->role==InitializationLoan::Role::live_set_text&&fields->init_stage==Impl::InitStage::live_reinit_entered),
        "Label5A needs earned initial5761 or genuine initialized6A live setter entry");
    const auto* font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    require(font&&font->loan&&text&&text->object,
        "Label5A selected nonNULL CC/C4 missing actual producers");
    const auto object=text->object;const auto allocation=object->allocation_child.lock();
    require(object->creator==std::this_thread::get_id()&&!object->retired.load()&&
        allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
        !fields.owner_before(allocation)&&object->source_serial==fields->source_serial&&
        !object->bytes.empty()&&object->bytes.size()<=0x4000&&object->bytes.back()=='\0'&&
        std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1,
        "Label5A text is not this actual OWN UTF8+NUL object");
    //5A75CE/5A75DD happen before color conversion or markup resolution.
    loan->child_store(0x40);std::get<std::uint32_t>(fields->fields[0x40/4])|=0x00800000u;loan->first();
    loan->child_store(0x138);fields->fields[0x138/4]=std::uint32_t(0);loan->first();
    std::array<std::uint32_t,3> colors{};
    const auto component=[&](std::uint32_t offset){
        const auto* bits=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(bits,"Label5A actual color component unknown");
        const auto f=std::bit_cast<float>(*bits);
        //Selected ColorAtlas authored components are finite [0,1]. This is
        //a qualifier, never Original-clamping or a guessed unknown63 byte.
        require(std::isfinite(f)&&f>=0.0f&&f<=1.0f,"Label5A color conversion outside qualified selected operands");
        return std::uint32_t(std::int64_t(std::trunc(double(f)*255.0)));
    };
    for(std::uint32_t layer=0;layer<3;++layer){
        const auto base=0x84u+layer*16;
        auto value=component(base+12)<<8;
        value=(value|(component(base+8)&255u))<<8;
        value=(value|(component(base+4)&255u))<<8;
        colors[layer]=value|(component(base)&255u);loan->first();
    }
    //Original global scratch color-stack resets count on EVERY call, pushes
    //the first packed color and performs no external callback on plain text.
    //For this plain/one-record branch the local value is observationally the
    //same scratch top; no native global capacity/address/counter is claimed.
    require(fields->style_c8.has_value(),"Label726E90 actual style byte unknown");
    const auto append_inline=[&](std::uint32_t begin,std::uint32_t count){
        const auto* inline_cell=std::get_if<Impl::SelfInline140>(&fields->fields[0x134/4]);
        const auto* size=std::get_if<std::uint32_t>(&fields->fields[0x138/4]);
        const auto* capacity=std::get_if<std::uint32_t>(&fields->fields[0x13c/4]);
        require(inline_cell&&inline_cell->actual_inline_cell==&fields->fields[0x140/4]&&
            size&&!*size&&capacity&&*capacity==1,
            "Label726E90 additional record requires genuine OWN grown segment vector");
        const auto put=[&](std::uint32_t offset,std::uint32_t value){
            loan->child_store(offset);fields->fields[offset/4]=value;loan->first();
        };
        loan->child_store(0x164);fields->segment_mutated_164=std::uint8_t(0);loan->first(); //726EE1 before capacity/copy
        put(0x140,begin);put(0x144,count);
        for(std::uint32_t layer=0;layer<3;++layer)put(0x148+layer*4,colors[layer]);
        //726E90 writes byte14 only. The other three padding bytes are still
        //unknown; do not invent a zero DWORD at154.
        loan->child_store(0x154);fields->inline_segment_newline_154=std::uint8_t(0);loan->first();
        put(0x158,std::uint32_t(*fields->style_c8));put(0x15c,0xffffffffu);put(0x160,0xffffffffu);
        put(0x138,1); //Actual count increment strictly after record stores.
    };
    const auto length=std::uint32_t(object->bytes.size()-1);
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    for(std::uint32_t at=0;at<length;++at){
        if(object->bytes[at]=='['&&(flags&0x08000000u)){
            //5A776F/5A7784: doubled '[' appends the preceding bytes PLUS
            //the first literal '[' even at offset zero. Escape continuation
            //and any further segment remain mandatory; this is no admission.
            if(at+1<length&&object->bytes[at+1]=='[')append_inline(0,at+1);
            else if(at)append_inline(0,at);
            throw std::runtime_error("Label5A reached markup/escape: real ordered color/icon/newline producer remains mandatory");
        }
    }
    append_inline(0,length); //5A7D9E includes the actual empty-caption record.
    fields->init_stage=Impl::InitStage::glyph_generated;loan->full();
}

void detail::NotificationLayoutEmbeddedLabelState::position_plain_segments_7505_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    require(fields->init_stage==Impl::InitStage::glyph_generated&&fields->truncate_16c&&
        *fields->truncate_16c<=0,"Label7505 actual generated/plain untruncated branch unavailable");
    require(fields->wrap_168&&*fields->wrap_168==-1,
        "Label7505 actual6204A0 nontrivial word-wrap producer required");
    const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    const auto* font_cell=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    require(text&&text->object&&!text->object->retired.load()&&font_cell&&font_cell->loan,
        "Label7505 actual C4/CC retired");
    //Snapshot ALL physical operand owners by VALUE before any allocation or
    //Source/backend callback. Holding memory never revives retired C4/CC/170.
    const auto object=text->object;const auto font=font_cell->loan;
    std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    if(const auto* cell=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
        require(bool(cell->loan),"Label7505 actual system descriptor empty");system=cell->loan;
    }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),
        "Label7505 unknown170 is not NULL");
    const auto validate_operands=[&]{
        loan->first();
        const auto* actual_text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
        const auto* actual_font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* actual_system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto allocation=object->allocation_child.lock();
        require(actual_text&&actual_text->object.get()==object.get()&&
            !actual_text->object.owner_before(object)&&!object.owner_before(actual_text->object)&&
            object->creator==std::this_thread::get_id()&&!object->retired.load()&&
            allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
            !fields.owner_before(allocation)&&object->source_serial==fields->source_serial&&
            object->source_serial&&!object->bytes.empty()&&object->bytes.size()<=0x4000&&
            object->bytes.back()=='\0'&&std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1&&
            actual_font&&actual_font->loan.get()==font.get()&&!actual_font->loan.owner_before(font)&&
            !font.owner_before(actual_font->loan)&&
            ((!system&&std::holds_alternative<Impl::Null>(fields->fields[0x170/4]))||
             (system&&actual_system&&actual_system->loan.get()==system.get()&&
              !actual_system->loan.owner_before(system)&&!system.owner_before(actual_system->loan)))&&
            fields->init_stage==Impl::InitStage::glyph_generated,
            "Label7505 SAME actual C4/CC/170 object/control/phase retired or changed");
        font->require_current_without_callbacks();loan->first();
        if(system){system->require_current_without_callbacks();loan->first();}
    };
    const auto full_operands=[&]{
        validate_operands();loan->full();validate_operands();
        font->require_source_owned(*loan->operation);validate_operands();
        if(system){system->require_source_owned(*loan->operation);validate_operands();}
        loan->full();validate_operands();
    };
    validate_operands();
    const auto bytes=object->bytes; //Strong TextObject snapshot precedes allocating vector copy.
    validate_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed across copy");
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed across Source POST");
    //Qualified plain one-record producer owns real begin/count/newline/style;
    //copy all descriptor data BEFORE actual138 reset as Original75060E does.
    const auto word=[&](std::uint32_t offset){
        const auto* value=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(value,"Label7505 generated record word unknown");return *value;
    };
    require(word(0x138)==1&&word(0x140)==0&&word(0x144)==bytes.size()-1&&
        fields->inline_segment_newline_154&&!*fields->inline_segment_newline_154&&
        word(0x15c)==0xffffffffu&&word(0x160)==0xffffffffu,
        "Label7505 needs genuine additional/newline/icon record producers");
    std::array<std::uint32_t,9> record{};
    for(unsigned i=0;i<9;++i)if(i!=5)record[i]=word(0x140+i*4);
    const auto newline=*fields->inline_segment_newline_154; //Other three padding bytes remain unknown.
    loan->child_store(0x138);fields->fields[0x138/4]=std::uint32_t(0);loan->first();
    std::unique_ptr<BnwLabelCurrentTextLoan> input;
    std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepared;
    if(system){
        input=current_position_text_owned(loan);input->require_source_owned(*loan->operation);loan->first();
        prepared=system->prepare_label_text_run_owned(*loan->operation,*input);loan->first();
        input->require_source_owned(*loan->operation);loan->first();
        require(bool(prepared),"Label7506AA actual410900 did not return completed preparation");
        prepared->require_source_owned(*loan->operation);loan->first();
    }
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed before positioning");
    //621320 consumes the actual record style DWORD; no guessed Label-wide style.
    (void)font->style_layer_texture_indices_without_callbacks(record[6]);loan->first();
    fields->positioned_glyphs.clear();fields->positioned_glyphs.reserve(bytes.size()-1);
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed across glyph reserve");
    std::uint32_t cursor=0;
    for(std::uint32_t at=0;at<bytes.size()-1;){
        const auto begin=at;const auto lead=std::uint8_t(bytes[at++]);
        unsigned count=0;std::uint32_t cp=lead;
        if(lead>=0xc2&&lead<=0xdf){count=1;cp=lead&0x1fu;}
        else if(lead>=0xe0&&lead<=0xef){count=2;cp=lead&0x0fu;}
        else if(lead>=0xf0&&lead<=0xf4){count=3;cp=lead&7u;}
        else require(lead<0x80,"Label486330 input outside qualified valid UTF8");
        require(at+count<=bytes.size()-1,"Label486330 truncated actual UTF8");
        for(unsigned i=0;i<count;++i){const auto next=std::uint8_t(bytes[at++]);
            require((next&0xc0)==0x80,"Label486330 invalid continuation byte");cp=(cp<<6)|(next&0x3fu);}
        require((count!=1||cp>=0x80)&&(count!=2||cp>=0x800)&&(count!=3||cp>=0x10000)&&
            cp<=0x10ffff&&!(cp>=0xd800&&cp<=0xdfff),"Label486330 noncanonical actual UTF8");
        if(std::get<std::uint32_t>(fields->fields[0x40/4])&0x80000000u)cp=0x2au;
        //Actual6204A0 wrap=-1 returns before glyph access. Font7640C0 then
        //735AB0 are distinct map-presence and real record lookups.
        const auto decoded=cp;std::array<std::uint32_t,9> bits{};
        if(system){
            //Original extended753CF0 is literal TRUE; no SPACE/GDI-has
            //substitution. The closed method still validates actual operands.
            const auto present=system->has_glyph_owned(*loan->operation,*input,cp);loan->first();
            require(present,"Label extended753CF0 must return genuine literalTRUE");
            bits=system->glyph_bits_owned(*loan->operation,*input,cp);loan->first();
            prepared->require_source_owned(*loan->operation);loan->first();
        }else{
            auto glyph=font->glyph_bits_without_callbacks(cp);loan->first();
            if(!glyph){cp=0x20;glyph=font->glyph_bits_without_callbacks(cp);loan->first();}
            require(glyph.has_value(),"Label735AB0 missingSPACE requires actual firstGlyph fallback owner");bits=*glyph;
        }
        const auto advance=std::bit_cast<float>(bits[8]);
        require(std::isfinite(advance)&&double(advance)>=-2147483648.0&&double(advance)<=2147483647.0,
            "Label75096A advance outside qualified x87 int64/lowword conversion");
        fields->positioned_glyphs.push_back({begin,at-begin,decoded,cp,cursor,bits});loan->first();
        cursor+=std::uint32_t(std::int64_t(std::trunc(double(advance)))); //Actual unsigned lowword addition.
    }
    if(record[1]){
        //Actual7509B7 resets byte164 then copies the SAME full descriptor.
        //Unknown padding remains represented separately instead of forged bits.
        loan->child_store(0x164);fields->segment_mutated_164=std::uint8_t(0);loan->first();
        for(unsigned i=0;i<9;++i){loan->child_store(0x140+i*4);
            if(i==5)fields->inline_segment_newline_154=newline;
            else fields->fields[(0x140+i*4)/4]=record[i];loan->first();}
        loan->child_store(0x138);fields->fields[0x138/4]=std::uint32_t(1);loan->first();
    } //Actual empty record is not re-emitted by7509A9 on this plain branch.
    full_operands();require(bytes==object->bytes,"Label7505 C4 VALUE changed before actual return");
    fields->init_stage=Impl::InitStage::positioned_returned;loan->full();
}

void detail::NotificationLayoutEmbeddedLabelState::anchor_initialized_4c_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();
    const auto f=[&](std::uint32_t offset){const auto* b=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(b,"Label79D300 unissued geometry");const auto v=std::bit_cast<float>(*b);
        require(std::isfinite(v),"Label79D300 nonfinite geometry");return v;};
    const auto put=[&](std::uint32_t o,float v){require(std::isfinite(v),"Label79D300 overflow");
        loan->child_store(o);fields->fields[o/4]=std::bit_cast<std::uint32_t>(v);loan->first();};
    put(0x2c,f(0x74));put(0x28,f(0x70));
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    require((flags&0x3f0u)==0||(flags&0x3f0u)==0x90u,"Label79D300 unsupported actual anchor");
    const auto centered=[&](float parent,float own,float margin){require(std::isfinite(parent),"Label parent size nonfinite");
        volatile float delta=parent-own;volatile float half=delta*0.5f;
        require(std::isfinite(half),"Label centered overflow");
        const auto round=half>=0?std::floor(double(half)+0.5):std::ceil(double(half)-0.5);
        volatile float out=margin+float(round);return float(out);};
    const auto& size=loan->parent_baseline->base_.size_30_34;
    if(flags&0x10u)put(0x28,centered(size[0],f(0x30),f(0x28)));
    if(flags&0x80u)put(0x2c,centered(size[1],f(0x34),f(0x2c)));
    loan->full();
}

void detail::NotificationLayoutEmbeddedLabelState::bounds_initialized_94_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();std::array<float,4> old{};
    const auto f=[&](std::uint32_t o){const auto* b=std::get_if<std::uint32_t>(&fields->fields[o/4]);
        require(b,"Label6EBC30 unknown geometry");const auto v=std::bit_cast<float>(*b);
        require(std::isfinite(v),"Label6EBC30 nonfinite geometry");return v;};
    for(unsigned i=0;i<4;++i)old[i]=f(0x10+i*4);
    require(std::holds_alternative<Impl::Null>(fields->fields[0x78/4]),
        "Label6EBC30 nonNULL ordinarychildren require actual forward extent union");
    const auto add=[](float a,float b){volatile float out=a+b;require(std::isfinite(out),"Label6EBC30 overflow");return float(out);};
    const std::array<float,4> next{f(0x28),f(0x2c),add(f(0x28),f(0x30)),add(f(0x2c),f(0x34))};
    for(unsigned i=0;i<4;++i){loan->child_store(0x10+i*4);fields->fields[(0x10+i*4)/4]=std::bit_cast<std::uint32_t>(next[i]);loan->first();}
    if(old!=next)loan->parent_bounds_94_owned(); //Actual6EBD51 same parent vslot94.
    loan->full();
}

void detail::NotificationLayoutEmbeddedLabelState::calculate_plain_size_5077_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    const auto fields=impl_;loan->full();require(fields->init_stage==Impl::InitStage::positioned_returned,
        "Label5077 missing actual7505 return");
    const auto* text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    const auto* font_cell=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    require(text&&text->object&&!text->object->retired.load()&&font_cell&&font_cell->loan,
        "Label5077 actual C4/CC absent or retired");
    //Snapshot ALL physical operand owners by VALUE before any allocation or
    //Source/backend callback. Holding memory never revives retired C4/CC/170.
    const auto object=text->object;const auto font=font_cell->loan;
    std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    if(const auto* cell=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
        require(bool(cell->loan),"Label5077 actual system descriptor empty");system=cell->loan;
    }else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),
        "Label5077 unknown170 is not NULL");
    const auto validate_operands=[&]{
        loan->first();
        const auto* actual_text=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
        const auto* actual_font=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* actual_system=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto allocation=object->allocation_child.lock();
        require(actual_text&&actual_text->object.get()==object.get()&&
            !actual_text->object.owner_before(object)&&!object.owner_before(actual_text->object)&&
            object->creator==std::this_thread::get_id()&&!object->retired.load()&&
            allocation&&allocation.get()==fields.get()&&!allocation.owner_before(fields)&&
            !fields.owner_before(allocation)&&object->source_serial==fields->source_serial&&
            object->source_serial&&!object->bytes.empty()&&object->bytes.size()<=0x4000&&
            object->bytes.back()=='\0'&&std::find(object->bytes.begin(),object->bytes.end(),'\0')==object->bytes.end()-1&&
            actual_font&&actual_font->loan.get()==font.get()&&!actual_font->loan.owner_before(font)&&
            !font.owner_before(actual_font->loan)&&
            ((!system&&std::holds_alternative<Impl::Null>(fields->fields[0x170/4]))||
             (system&&actual_system&&actual_system->loan.get()==system.get()&&
              !actual_system->loan.owner_before(system)&&!system.owner_before(actual_system->loan)))&&
            fields->init_stage==Impl::InitStage::positioned_returned,
            "Label5077 SAME actual C4/CC/170 object/control/phase retired or changed");
        font->require_current_without_callbacks();loan->first();
        if(system){system->require_current_without_callbacks();loan->first();}
    };
    const auto full_operands=[&]{
        validate_operands();loan->full();validate_operands();
        font->require_source_owned(*loan->operation);validate_operands();
        if(system){system->require_source_owned(*loan->operation);validate_operands();}
        loan->full();validate_operands();
    };
    validate_operands();
    const auto flags=std::get<std::uint32_t>(fields->fields[0x40/4]);
    //Capture descriptor scalars and a physically owned positioned vector
    //before the first backend call; no variant pointer survives a callback.
    const auto* count_cell=std::get_if<std::uint32_t>(&fields->fields[0x138/4]);
    require(count_cell&&*count_cell<=1,"Label5077 needs actual multi-record size traversal");
    const auto count=*count_cell;
    std::uint32_t span_bytes=0,record_style=0;
    if(count){
        const auto* style=std::get_if<std::uint32_t>(&fields->fields[0x158/4]);
        const auto* begin=std::get_if<std::uint32_t>(&fields->fields[0x140/4]);
        const auto* length=std::get_if<std::uint32_t>(&fields->fields[0x144/4]);
        require(style&&fields->inline_segment_newline_154&&!*fields->inline_segment_newline_154&&
            begin&&!*begin&&length&&*length==object->bytes.size()-1,
            "Label5077 actual plain descriptor changed");
        span_bytes=*length;record_style=*style;
    }
    const auto captured_sequence=fields->sequence;
    const auto glyphs=fields->positioned_glyphs;
    validate_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed across allocating copy");
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed across Source POST");
    std::unique_ptr<BnwLabelCurrentTextLoan> input;
    std::shared_ptr<const BnwLabelSystemFontPreparedRun> prepared;
    if(system){
        input=current_size_text_owned(loan);input->require_source_owned(*loan->operation);loan->first();
    }
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed before ellipsis");
    const auto advance_for=[&](std::uint32_t cp){
        if(flags&0x80000000u)cp=0x2au;
        std::array<std::uint32_t,9> bits{};
        if(system){
            require(system->has_glyph_owned(*loan->operation,*input,cp),"Label5077 extended753CF0 did not return literalTRUE");loan->first();
            bits=system->glyph_bits_owned(*loan->operation,*input,cp);loan->first();
            input->require_source_owned(*loan->operation);loan->first();
        }else{
            auto glyph=font->glyph_bits_without_callbacks(cp);loan->first();
            if(!glyph){glyph=font->glyph_bits_without_callbacks(0x20);loan->first();}
            require(glyph.has_value(),"Label6C5F/5077 missing SPACE requires actual firstGlyph fallback owner");bits=*glyph;
        }
        const auto advance=std::bit_cast<float>(bits[8]);
        require(std::isfinite(advance)&&double(advance)>=-2147483648.0&&double(advance)<=2147483647.0,
            "Label6C5F/5077 advance outside qualified x87 conversion");
        return std::uint32_t(std::int64_t(std::trunc(double(advance))));
    };
    //6C5FB0 walks actual Original PE A484F8 "...\0". No wholetext
    //preparation is called here; its three real glyph lookups still occur.
    std::uint32_t ellipsis_width=0;
    for(unsigned i=0;i<3;++i)ellipsis_width+=advance_for(0x2eu);
    (void)ellipsis_width; //Untruncated branch does not use its returned width.
    if(system){ //507836 actual410900 AFTER actual6C5FB0 returned.
        prepared=system->prepare_label_text_run_owned(*loan->operation,*input);loan->first();
        input->require_source_owned(*loan->operation);loan->first();
        require(bool(prepared),"Label507837 repeated410900 did not return completion");
        prepared->require_source_owned(*loan->operation);loan->first();
    }
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed before size traversal");
    std::uint32_t width=0,cursor=0;
    if(count){
        (void)font->style_layer_texture_indices_without_callbacks(record_style);loan->first(); //Actual621320.
        //The validated UTF8/codepoint span was produced by actual7505 above;
        //repeat EACH actual glyph getter now rather than copying cached width.
        std::uint32_t covered=0;
        for(const auto& glyph:glyphs){
            require(glyph.byte_begin==covered&&glyph.byte_count&&glyph.byte_count<=span_bytes&&covered<=span_bytes-glyph.byte_count,
                "Label5077 same positioned UTF8 span changed");
            cursor+=advance_for(glyph.codepoint);validate_operands();
            require(fields->sequence==captured_sequence,"Label5077 actual positioned state changed during glyph callback");
            if(cursor>width)width=cursor; //507A15 unsigned CMP/JBE after EACH advance, including negative lowwords.
            covered+=glyph.byte_count;
        }
        require(covered==span_bytes,"Label5077 actual UTF8 traversal incomplete");
    }
    //507A74 width store precedes the mandatory7AC170 metric producer below.
    const auto put=[&](std::uint32_t offset,float value){require(std::isfinite(value),"Label5077 geometry overflow");
        loan->child_store(offset);fields->fields[offset/4]=std::bit_cast<std::uint32_t>(value);loan->first();};
    const auto get=[&](std::uint32_t offset){const auto* bits=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
        require(bits,"Label5077 margin unknown");const auto value=std::bit_cast<float>(*bits);
        require(std::isfinite(value),"Label5077 margin nonfinite");return value;};
    full_operands();require(fields->sequence==captured_sequence,
        "Label5077 actual positioned state changed before geometry stores");
    put(0x30,float(width)); //507A74 FSTP32 BEFORE actual7AC170 metric lookup.
    const auto metrics=font->metrics_without_callbacks();validate_operands(); //7AC170 actual40C08.
    const auto height=std::uint32_t(metrics[0]); //Native lowword plus zero newline height, unsigned conversion.
    put(0x34,float(height)); //507A94 FST32 retains extended integer height on x87 stack.
    put(0x20,float(std::fabs(double(get(0x70)))+double(get(0x30)))); //507AA4 reloads rounded FP32 width.
    put(0x24,float(std::fabs(double(get(0x74)))+double(height))); //507AAF uses retained unrounded integer height.
    bounds_initialized_94_owned(loan);loan->full();
}

void detail::NotificationLayoutEmbeddedLabelState::rebuild_initialized_layout_owned(
    const std::shared_ptr<InitializationLoan>& loan)const{
    loan->full();require(((loan->role==InitializationLoan::Role::initial_button&&impl_->init_stage==Impl::InitStage::common_returned)||
        (loan->role==InitializationLoan::Role::live_set_text&&impl_->init_stage==Impl::InitStage::live_reinit_entered))&&
        (std::get<std::uint32_t>(impl_->fields[0x40/4])&1u),"Label6A lacks earned initial or live setter entry");
    generate_text_segments_5a_owned(loan);loan->full();
    position_plain_segments_7505_owned(loan);loan->full();
    loan->child_store(0x16e);impl_->truncated_16e=std::uint8_t(0);loan->first(); //6A2EBD before5077
    calculate_plain_size_5077_owned(loan);loan->full();
    anchor_initialized_4c_owned(loan);loan->full();
    bounds_initialized_94_owned(loan);loan->full();
    require(std::holds_alternative<Impl::Null>(impl_->fields[0x78/4]),
        "Label4A7F10 nonNULL ordinarychild recursion required");
    impl_->init_stage=Impl::InitStage::bounds_returned;loan->full();
    //No run receipt is issued here: actual outer Button init, receiver DTOR
    //and Source POST remain mandatory for a future canonical paint borrower.
}
