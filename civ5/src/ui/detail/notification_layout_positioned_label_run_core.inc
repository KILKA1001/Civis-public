// PRIVATE source-only successor, included after genuine init/Factory/Binding.
struct detail::BnwPositionedLabelRunLoan::Impl {
    using Tree=BnwWorldViewControlTree;
    using Child=NotificationLayoutEmbeddedLabelState;
    using F=Child::Impl;
    enum class Origin{initial_button,live_set_text};
    const Origin origin;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const std::weak_ptr<Tree::Core> core;
    const std::uint64_t node_id,topology,parent_parser_serial,child_sequence;
    const std::weak_ptr<Child> child;
    const std::weak_ptr<NotificationLayoutButtonState> parent;
    const std::weak_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> factory;
    const std::shared_ptr<BnwLabelProcessModuleOwner> module;
    const std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> allocation;
    const std::uint64_t constructor_serial;
    const std::shared_ptr<F::TextObject> text;
    const std::shared_ptr<const BnwCompletedFontLoan> font;
    const std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    struct Value final {
        std::vector<char> c4;
        std::vector<GlyphData> glyphs;
        std::array<std::uint32_t,10> geometry;
        std::uint32_t alpha_44;
        std::array<std::uint32_t,3> colors;
        std::array<std::uint32_t,12> raw_colors;
        std::array<std::uint32_t,8> descriptor; //140,144,148,14C,150,158,15C,160.
        std::uint32_t count,flags;
        std::uint8_t style,newline;
        std::int16_t wrap,leading,truncate;
    };
    const Value value;
    // Earned only by the actual Root tail publisher after final C4 return,
    // View destruction, Binding's destruction_passed and frame FULL POST.
    struct PublishedButtonReturn final {
        std::uint64_t source_serial,receiver_return_sequence,topology,effects;
    };
    std::optional<PublishedButtonReturn> published;
    struct PublishedSetTextReturn final{
        std::uint64_t source_serial,child_sequence,topology,effects;
        const void* actual_c4;
    };
    std::optional<PublishedSetTextReturn> live_published;
    //Typed return DATA only. Caller still needs its distinct cold/entered-paint
    //target and actual completed_run checks; this is never an issuer or ready flag.
    void require_published_data_without_callbacks()const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Run typed return DATA retired/foreign");
        if(origin==Origin::live_set_text){
            require(live_published&&!published&&live_published->source_serial&&
                live_published->topology==topology&&live_published->child_sequence==child_sequence&&
                live_published->actual_c4==text.get(),"Run live SetText actual typed return DATA absent");
        }else require(origin==Origin::initial_button&&published&&!live_published&&
            published->topology==topology&&published->source_serial&&published->receiver_return_sequence,
            "Run initial Button actual typed return DATA absent");
    }
    static Value capture_value(const std::shared_ptr<Child::InitializationLoan>& init,
        const std::shared_ptr<F::TextObject>& object,std::uint64_t sequence){
        init->first();const auto fields=init->child->impl_;
        const auto first=[&]{init->first();
            const auto* actual=std::get_if<F::TextBorrow>(&fields->fields[0xc4/4]);
            require(actual&&actual->object.get()==object.get()&&!actual->object.owner_before(object)&&
                !object.owner_before(actual->object)&&object&&!object->retired.load()&&
                fields->sequence==sequence&&fields->init_stage==F::InitStage::bounds_returned,
                "Run pending actual completed C4/child revision changed");};
        first();Value out;
        //Strong TextObject and Child/Impl locals precede EVERY allocating copy.
        out.c4=object->bytes;first();
        const auto glyphs=fields->positioned_glyphs;first();
        out.glyphs.reserve(glyphs.size());first();
        for(const auto& glyph:glyphs)out.glyphs.push_back({glyph.byte_begin,glyph.byte_count,
            glyph.codepoint,glyph.lookup_codepoint,glyph.cursor,glyph.bits});
        first();
        const auto word=[&](std::uint32_t offset){first();
            const auto* n=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
            require(n,"Run pending actual known word unavailable");return *n;};
        for(unsigned i=0;i<10;++i)out.geometry[i]=word(0x10+4*i);
        out.alpha_44=word(0x44);
        const std::array<std::uint32_t,8> offsets{0x140,0x144,0x148,0x14c,0x150,0x158,0x15c,0x160};
        for(unsigned i=0;i<offsets.size();++i)out.descriptor[i]=word(offsets[i]);
        out.count=word(0x138);out.flags=word(0x40);
        out.colors={word(0x148),word(0x14c),word(0x150)};
        for(unsigned i=0;i<12;++i)out.raw_colors[i]=word(0x84+4*i);
        require(fields->style_c8&&fields->inline_segment_newline_154&&fields->wrap_168&&
            fields->leading_16a&&fields->truncate_16c,"Run pending actual selected byte/short fields unavailable");
        out.style=*fields->style_c8;out.newline=*fields->inline_segment_newline_154;
        out.wrap=*fields->wrap_168;out.leading=*fields->leading_16a;out.truncate=*fields->truncate_16c;
        first();init->full();first();require(out.c4==object->bytes,
            "Run pending actual C4 VALUE changed across full Source POST");return out;
    }
    static Child::InitializationLoan& checked(const std::shared_ptr<Child::InitializationLoan>& init){
        require(bool(init),"Run pending actual init absent");init->first();return *init;
    }
    Impl(std::shared_ptr<Child::InitializationLoan> init,std::shared_ptr<Tree::Core> actual_core,
        std::shared_ptr<Child> actual_child,std::shared_ptr<NotificationLayoutButtonState> actual_parent,
        std::shared_ptr<const NotificationLayoutEmbeddedLabelFactoryReturn> actual_factory,
        std::shared_ptr<BnwLabelProcessModuleOwner> actual_module,
        std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> ticket,
        std::shared_ptr<F::TextObject> object,std::shared_ptr<const BnwCompletedFontLoan> cc,
        std::shared_ptr<const BnwLabelSystemFontBorrower> cache,
        std::uint64_t actual_topology,std::uint64_t parser_serial,std::uint64_t sequence)
        :origin(checked(init).role==Child::InitializationLoan::Role::initial_button?Origin::initial_button:Origin::live_set_text),
         core(actual_core),node_id(checked(init).parent_key.id),topology(actual_topology),
         parent_parser_serial(parser_serial),child_sequence(sequence),child(actual_child),parent(actual_parent),
         factory(actual_factory),module(std::move(actual_module)),allocation(std::move(ticket)),
         constructor_serial(checked(init).child->impl_->source_serial),text(std::move(object)),
         font(std::move(cc)),system(std::move(cache)),value(capture_value(init,text,sequence)){
        init->first();validate_child_without_callbacks(*init->operation,actual_child);
        init->full();init->first();validate_child_without_callbacks(*init->operation,actual_child);
    }
    void validate_child_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan& op,
        const std::shared_ptr<Child>& object)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&object,
            "Run actual child DATA retired/foreign");
        const auto fields=object->impl_;fields->first();
        module->require_label_live_allocation_without_callbacks(op,allocation,object,constructor_serial);fields->first();
        const auto* actual_text=std::get_if<F::TextBorrow>(&fields->fields[0xc4/4]);
        const auto* actual_font=std::get_if<F::FontBorrow>(&fields->fields[0xcc/4]);
        const auto* actual_system=std::get_if<F::SystemFontBorrow>(&fields->fields[0x170/4]);
        const auto text_allocation=text->allocation_child.lock();
        require(fields->source_serial==constructor_serial&&fields->module.get()==module.get()&&
            !fields->module.owner_before(module)&&!module.owner_before(fields->module)&&
            fields->sequence==child_sequence&&fields->init_stage==F::InitStage::bounds_returned&&
            actual_text&&actual_text->object.get()==text.get()&&!actual_text->object.owner_before(text)&&
            !text.owner_before(actual_text->object)&&!text->retired.load()&&text->creator==creator&&
            text_allocation&&text_allocation.get()==fields.get()&&!text_allocation.owner_before(fields)&&
            !fields.owner_before(text_allocation)&&text->source_serial==constructor_serial&&text->bytes==value.c4&&
            actual_font&&actual_font->loan.get()==font.get()&&!actual_font->loan.owner_before(font)&&
            !font.owner_before(actual_font->loan)&&font&&
            ((!system&&std::holds_alternative<F::Null>(fields->fields[0x170/4]))||
             (system&&actual_system&&actual_system->loan.get()==system.get()&&
              !actual_system->loan.owner_before(system)&&!system.owner_before(actual_system->loan))),
            "Run SAME actual allocation/Text/CC/170/control/revision changed");
        const auto word=[&](std::uint32_t offset){const auto* n=std::get_if<std::uint32_t>(&fields->fields[offset/4]);
            require(n,"Run actual known word became unavailable");return *n;};
        for(unsigned i=0;i<10;++i)require(word(0x10+4*i)==value.geometry[i],"Run actual geometry changed");
        require(word(0x44)==value.alpha_44,"Run actual physical Label alpha44 changed");
        const std::array<std::uint32_t,8> offsets{0x140,0x144,0x148,0x14c,0x150,0x158,0x15c,0x160};
        for(unsigned i=0;i<offsets.size();++i)require(word(offsets[i])==value.descriptor[i],"Run actual descriptor changed");
        for(unsigned i=0;i<12;++i)require(word(0x84+4*i)==value.raw_colors[i],"Run actual color fields changed");
        require(word(0x138)==value.count&&word(0x40)==value.flags&&fields->style_c8==value.style&&
            fields->inline_segment_newline_154==value.newline&&fields->wrap_168==value.wrap&&
            fields->leading_16a==value.leading&&fields->truncate_16c==value.truncate&&
            fields->positioned_glyphs.size()==value.glyphs.size(),"Run actual ordered text fields changed");
        for(std::size_t i=0;i<value.glyphs.size();++i){const auto& actual=fields->positioned_glyphs[i];const auto& saved=value.glyphs[i];
            require(actual.byte_begin==saved.byte_begin&&actual.byte_count==saved.byte_count&&
                actual.codepoint==saved.codepoint&&actual.lookup_codepoint==saved.lookup_codepoint&&
                actual.cursor==saved.cursor&&actual.bits==saved.bits,"Run actual positioned glyph DATA changed");}
        font->require_current_without_callbacks();fields->first();
        if(system){system->require_current_without_callbacks();fields->first();}
    }
    void validate_target_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Run actual completed DATA retired/foreign");
        const auto owner=core.lock();require(bool(owner),"Run canonical Core retired");owner->layout_origin();
        const auto& entered=owner->source->current_operation_loan();owner->layout_origin();
        require(&entered==&op,"Run requires SAME fresh actual canonical Source operation");
        op.require_origin_without_callbacks();owner->layout_origin();
        require(owner->topology_revision==topology,"Run actual topology changed");
        const auto object=child.lock();const auto actual_parent=parent.lock();const auto returned=factory.lock();
        require(object&&actual_parent&&returned,"Run actual child/parent/Factory retired");
        const auto& node=Tree::Core::node(owner->nodes,node_id);
        require(node.layout_baseline.has_value(),"Run actual Button baseline absent");
        const auto& baseline=*node.layout_baseline;
        require(baseline.parsed_source_serial_&&*baseline.parsed_source_serial_==parent_parser_serial&&
            baseline.kind_==NotificationLayoutBaseline::Kind::button&&baseline.actual_type_=="Button"&&
            baseline.button_.get()==actual_parent.get()&&!baseline.button_.owner_before(actual_parent)&&
            !actual_parent.owner_before(baseline.button_)&&actual_parent->label_178_.get()==object.get()&&
            !actual_parent->label_178_.owner_before(object)&&!object.owner_before(actual_parent->label_178_)&&
            actual_parent->label_178_factory_return_.get()==returned.get()&&
            !actual_parent->label_178_factory_return_.owner_before(returned)&&
            !returned.owner_before(actual_parent->label_178_factory_return_)&&!actual_parent->pending_label_178_,
            "Run actual canonical parsed Button/178/Factory membership changed");
        //Constructor/Runtime/String/Factory parser state is NOT invoked again.
        const auto& receipt=*returned->impl_;
        require(receipt.creator==creator&&receipt.alive.load()&&receipt.published_sequence&&
            receipt.source_serial==constructor_serial&&receipt.child.get()==object.get()&&
            receipt.allocation.get()==allocation.get()&&!receipt.allocation.owner_before(allocation)&&
            !allocation.owner_before(receipt.allocation),"Run actual Factory allocation return retired");
        const auto fields=object->impl_;fields->first();
        const auto* child_parent=std::get_if<F::ParentBorrow>(&fields->fields[0x38/4]);
        const auto* child_realm=std::get_if<F::ParsedRealm>(&fields->fields[0x3c/4]);
        const auto borrowed_parent=child_parent?child_parent->parent.lock():nullptr;
        const auto& parent_realm=baseline.base_.context_realm_3c;
        require(child_parent&&child_parent->baseline==&baseline&&borrowed_parent&&
            borrowed_parent.get()==actual_parent.get()&&!borrowed_parent.owner_before(actual_parent)&&
            !actual_parent.owner_before(borrowed_parent)&&child_realm&&child_realm->value&&parent_realm&&
            baseline.base_.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
            child_realm->value.get()==parent_realm.get()&&!child_realm->value.owner_before(parent_realm)&&
            !parent_realm.owner_before(child_realm->value)&&owner->context_realm_identity.get()==parent_realm.get()&&
            !owner->context_realm_identity.owner_before(parent_realm)&&!parent_realm.owner_before(owner->context_realm_identity),
            "Run actual child38/parsed3C SAME live parent/Context identity changed");
        parent_realm->require_live_run_origin_without_callbacks(op);owner->layout_origin();
        validate_child_without_callbacks(op,object);owner->layout_origin();op.require_origin_without_callbacks();owner->layout_origin();
    }
    void validate_paint_target_without_callbacks(const runtime::BnwWorldViewSourceOperationLoan& op,
        const std::shared_ptr<const Tree>& actual_tree,const BnwLabelActualPaintCallFrame::Guard& frame)const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Run actual completed DATA retired/foreign");
        frame.require_current_without_callbacks();
        require(bool(actual_tree),"Paint Run genuine entered Tree absent");
        frame.require_actual_control_without_callbacks(*actual_tree,node_id);
        const auto owner=core.lock();require(bool(owner),"Paint Run canonical Core retired");
        const auto paint_first=[&]{frame.require_current_without_callbacks();
            frame.require_actual_control_without_callbacks(*actual_tree,node_id);
            require(actual_tree->core_.get()==owner.get()&&!actual_tree->core_.owner_before(owner)&&
                !owner.owner_before(actual_tree->core_),"Paint Run genuine Frame/Tree/Core differs");
            owner->layout_origin(true);frame.require_current_without_callbacks();};paint_first();
        const auto& entered=owner->source->current_operation_loan();paint_first();
        require(&entered==&op,"Run requires SAME fresh actual canonical Source operation");
        op.require_origin_without_callbacks();paint_first();
        require(owner->topology_revision==topology,"Run actual topology changed");
        const auto object=child.lock();const auto actual_parent=parent.lock();const auto returned=factory.lock();
        require(object&&actual_parent&&returned,"Run actual child/parent/Factory retired");
        const auto& node=Tree::Core::node(owner->nodes,node_id);
        require(node.layout_baseline.has_value(),"Run actual Button baseline absent");
        const auto& baseline=*node.layout_baseline;
        require(baseline.parsed_source_serial_&&*baseline.parsed_source_serial_==parent_parser_serial&&
            baseline.kind_==NotificationLayoutBaseline::Kind::button&&baseline.actual_type_=="Button"&&
            baseline.button_.get()==actual_parent.get()&&!baseline.button_.owner_before(actual_parent)&&
            !actual_parent.owner_before(baseline.button_)&&actual_parent->label_178_.get()==object.get()&&
            !actual_parent->label_178_.owner_before(object)&&!object.owner_before(actual_parent->label_178_)&&
            actual_parent->label_178_factory_return_.get()==returned.get()&&
            !actual_parent->label_178_factory_return_.owner_before(returned)&&
            !returned.owner_before(actual_parent->label_178_factory_return_)&&!actual_parent->pending_label_178_,
            "Run actual canonical parsed Button/178/Factory membership changed");
        //Constructor/Runtime/String/Factory parser state is NOT invoked again.
        const auto& receipt=*returned->impl_;
        require(receipt.creator==creator&&receipt.alive.load()&&receipt.published_sequence&&
            receipt.source_serial==constructor_serial&&receipt.child.get()==object.get()&&
            receipt.allocation.get()==allocation.get()&&!receipt.allocation.owner_before(allocation)&&
            !allocation.owner_before(receipt.allocation),"Run actual Factory allocation return retired");
        const auto fields=object->impl_;fields->first();
        const auto* child_parent=std::get_if<F::ParentBorrow>(&fields->fields[0x38/4]);
        const auto* child_realm=std::get_if<F::ParsedRealm>(&fields->fields[0x3c/4]);
        const auto borrowed_parent=child_parent?child_parent->parent.lock():nullptr;
        const auto& parent_realm=baseline.base_.context_realm_3c;
        require(child_parent&&child_parent->baseline==&baseline&&borrowed_parent&&
            borrowed_parent.get()==actual_parent.get()&&!borrowed_parent.owner_before(actual_parent)&&
            !actual_parent.owner_before(borrowed_parent)&&child_realm&&child_realm->value&&parent_realm&&
            baseline.base_.context_3c_observation==NotificationLayoutBaseline::Base::Context3CObservation::parsed_realm&&
            child_realm->value.get()==parent_realm.get()&&!child_realm->value.owner_before(parent_realm)&&
            !parent_realm.owner_before(child_realm->value)&&owner->context_realm_identity.get()==parent_realm.get()&&
            !owner->context_realm_identity.owner_before(parent_realm)&&!parent_realm.owner_before(owner->context_realm_identity),
            "Run actual child38/parsed3C SAME live parent/Context identity changed");
        //Separate entered Paint DATA check; old parser/cold Realm methods remain exact.
        //These complete private Realm fields are in this SAME Root TU.
        paint_first();const auto realm=parent_realm->impl_;paint_first();
        require(realm->creator==std::this_thread::get_id()&&realm->alive.load()&&!realm->failed.load(),
            "Paint Run actual Realm retired/foreign/failed");
        const auto realm_core=realm->context.lock();const auto realm_source=realm->source.lock();paint_first();
        require(realm_core&&realm_source&&realm_core.get()==owner.get()&&
            !realm_core.owner_before(owner)&&!owner.owner_before(realm_core)&&
            realm->constructor&&owner->context_construction_return.get()==realm->constructor.get()&&
            !realm->constructor.owner_before(owner->context_construction_return)&&
            !owner->context_construction_return.owner_before(realm->constructor)&&
            owner->source.get()==realm_source.get()&&!owner->source.owner_before(realm_source)&&
            !realm_source.owner_before(owner->source),"Paint Run actual Realm/Core/constructor/Source changed");
        paint_first();
        validate_child_without_callbacks(op,object);paint_first();op.require_origin_without_callbacks();paint_first();
    }
    void first_for_paint(const runtime::BnwWorldViewSourceOperationLoan& op,
        const std::shared_ptr<const Tree>& actual_tree,const BnwLabelActualPaintCallFrame::Guard& frame)const{
        frame.require_current_without_callbacks();
        //DATA-only initial/live publication predicate supplied by liveSetText
        //owner; it grants neither busy/source/frame nor renewed constructor.
        require_published_data_without_callbacks();
        frame.require_current_without_callbacks();validate_paint_target_without_callbacks(op,actual_tree,frame);
        frame.require_current_without_callbacks();const auto object=child.lock();
        require(bool(object),"Paint Run actual published child retired");
        const auto& completed=object->impl_->completed_run;
        require(completed&&completed->impl_.get()==this,
            "Paint Run is not SAME actual child's current completed DATA");
        frame.require_current_without_callbacks();
    }
    void first(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        if(origin==Origin::live_set_text){
            require_published_data_without_callbacks();
            validate_target_without_callbacks(op);
            const auto object=child.lock();require(object&&object->impl_->completed_run&&
                object->impl_->completed_run->impl_.get()==this,"Live Run not actual child completed DATA");return;
        }
        require(creator==std::this_thread::get_id()&&alive.load()&&published&&
            published->topology==topology&&published->source_serial&&published->receiver_return_sequence,
            "Run actual post-Button-return DATA has not been published or is retired");
        validate_target_without_callbacks(op);
        const auto object=child.lock();require(bool(object),"Run actual published child retired");
        const auto& completed=object->impl_->completed_run;
        require(completed&&completed->impl_.get()==this,"Run is not SAME actual child's published completed DATA");
    }
};

detail::BnwPositionedLabelRunLoan::BnwPositionedLabelRunLoan(std::shared_ptr<Impl> data):impl_(std::move(data)){}
detail::BnwPositionedLabelRunLoan::~BnwPositionedLabelRunLoan(){impl_->alive.store(false);}
detail::BnwPositionedLabelRunLoan::Guard::Guard(std::shared_ptr<Impl> data,
    const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t serial)
    :impl_(std::move(data)),operation_(&op),serial_(serial){}
detail::BnwPositionedLabelRunLoan::Guard detail::BnwPositionedLabelRunLoan::capture_guard_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{
    const auto data=impl_;data->first(op);const auto serial=op.operation_serial();data->first(op);
    return Guard(data,op,serial);
}
void detail::BnwPositionedLabelRunLoan::Guard::require_current_without_callbacks()const{
    if(paint_frame_){
        //Actual lexical Frame FIRST before Tree/Core/Realm/borrowed op reads.
        paint_frame_->require_current_without_callbacks();
        require(paint_tree_&&impl_->creator==std::this_thread::get_id()&&impl_->alive.load(),
            "Paint Run Guard Tree/DATA retired/foreign");
        paint_frame_->require_actual_control_without_callbacks(*paint_tree_,impl_->node_id);
        const auto owner=impl_->core.lock();require(bool(owner),"Paint Run Guard canonical Core retired");
        owner->layout_origin(true);paint_frame_->require_current_without_callbacks();
        const auto& current=owner->source->current_operation_loan();paint_frame_->require_current_without_callbacks();
        require(&current==operation_&&current.operation_serial()==serial_,"Paint Run Guard fresh operation expired/replaced");
        impl_->first_for_paint(current,paint_tree_,*paint_frame_);return;
    }
    //Resolve fresh canonical current operation BEFORE any borrowed op access.
    require(impl_->creator==std::this_thread::get_id()&&impl_->alive.load(),"Run Guard retired/foreign");
    const auto owner=impl_->core.lock();require(bool(owner),"Run Guard canonical Core retired");owner->layout_origin();
    const auto& current=owner->source->current_operation_loan();owner->layout_origin();
    require(&current==operation_&&current.operation_serial()==serial_,"Run Guard fresh operation expired/replaced");
    impl_->first(current);
}
void detail::BnwPositionedLabelRunLoan::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& op)const{
    require_current_without_callbacks();require(&op==operation_,"Run Guard foreign Source operand");
    op.require_current();require_current_without_callbacks();
    impl_->font->require_source_owned(op);require_current_without_callbacks();
    if(impl_->system){impl_->system->require_source_owned(op);require_current_without_callbacks();}
    op.require_current();require_current_without_callbacks();
}
void detail::BnwPositionedLabelRunLoan::Guard::require_system_font_borrower_without_callbacks(
    const BnwLabelSystemFontBorrower& actual)const{
    require_current_without_callbacks();require(impl_->system&&impl_->system.get()==&actual,
        "Run Guard belongs to another actual170 operand");require_current_without_callbacks();
}
void detail::BnwPositionedLabelRunLoan::Guard::require_font_without_callbacks(const BnwCompletedFontLoan& actual)const{
    require_current_without_callbacks();require(impl_->font&&impl_->font.get()==&actual,
        "Run Guard belongs to another actual CC operand");require_current_without_callbacks();
}
void detail::BnwPositionedLabelRunLoan::Guard::require_actual_control_without_callbacks(
    const BnwWorldViewControlTree& actual_tree,std::uint64_t actual_id)const{
    require_current_without_callbacks();
    const auto owner=impl_->core.lock();const auto actual=actual_tree.core_;
    require(owner&&actual&&owner.get()==actual.get()&&!owner.owner_before(actual)&&!actual.owner_before(owner)&&
        impl_->node_id==actual_id,"Run does not match the actual renderer Tree/entered Control");
    if(paint_frame_){
        paint_frame_->require_current_without_callbacks();
        require(paint_tree_.get()==&actual_tree,"Paint Run matching operand differs from entered Tree");
        paint_frame_->require_actual_control_without_callbacks(actual_tree,actual_id);
        actual->layout_origin(true);paint_frame_->require_current_without_callbacks();
    }else actual->layout_origin();
    //FIRST already validated this exact alive canonical node's parsed
    //Button/178/Factory/allocation/CC/170/Realm/current completed Run.
    const auto& node=BnwWorldViewControlTree::Core::node(actual->nodes,actual_id);
    require(node.alive&&node.layout_baseline&&node.layout_baseline->button_&&
        node.layout_baseline->button_->label_178_&&
        node.layout_baseline->button_->label_178_->impl_->completed_run&&
        node.layout_baseline->button_->label_178_->impl_->completed_run->impl_.get()==impl_.get(),
        "Run actual renderer Control no longer publishes this exact completed DATA");
    require_current_without_callbacks();
}
std::string_view detail::BnwPositionedLabelRunLoan::Guard::actual_c4_bytes_without_callbacks()const{
    require_current_without_callbacks();return {impl_->value.c4.data(),impl_->value.c4.size()};
}
std::span<const detail::BnwPositionedLabelRunLoan::GlyphData>
detail::BnwPositionedLabelRunLoan::Guard::positioned_glyphs_without_callbacks()const{
    require_current_without_callbacks();return impl_->value.glyphs;
}
std::array<std::uint32_t,10> detail::BnwPositionedLabelRunLoan::Guard::actual_geometry_bits_without_callbacks()const{
    require_current_without_callbacks();return impl_->value.geometry;
}
std::uint32_t detail::BnwPositionedLabelRunLoan::Guard::actual_alpha_bits_without_callbacks()const{
    require_current_without_callbacks();
    const auto result=impl_->value.alpha_44;
    require_current_without_callbacks();return result;
}
detail::BnwPositionedLabelRunLoan::RecordData detail::BnwPositionedLabelRunLoan::Guard::actual_record_without_callbacks()const{
    require_current_without_callbacks();const auto& v=impl_->value;
    return {v.count,v.descriptor[0],v.descriptor[1],v.descriptor[5],v.descriptor[6],v.descriptor[7],v.colors,v.newline,v.style,
        v.wrap,v.leading,v.truncate,v.flags};
}
const detail::BnwCompletedFontLoan& detail::BnwPositionedLabelRunLoan::Guard::actual_font_without_callbacks()const{
    require_current_without_callbacks();return *impl_->font;
}

std::shared_ptr<const detail::BnwLabelSystemFontBorrower>
detail::BnwPositionedLabelRunLoan::Guard::actual_system_font_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->system;
    //Impl::first verified current real child170: NULL only from actual typed
    //Null cell, never missing owner success. This physical descriptor copy
    //is DATA, not cache/Font/paint authority. Caller full-source gates bracket
    //the read before using the exact operand to create a lexical Paint loan.
    require_current_without_callbacks();return result;
}

std::shared_ptr<detail::BnwPositionedLabelRunLoan>
detail::NotificationLayoutEmbeddedLabelState::make_positioned_run_pending_owned(
    const std::shared_ptr<InitializationLoan>& init)const{
    init->full();require(init->child.get()==this,"Run pending issuer wrong actual child");
    const auto fields=impl_;
    require(fields->init_stage==Impl::InitStage::bounds_returned&&!fields->completed_run,
        "Run pending requires genuine710/5761/5A/7505/5077/4A return and no prior publication");
    const auto* text_cell=std::get_if<Impl::TextBorrow>(&fields->fields[0xc4/4]);
    const auto* font_cell=std::get_if<Impl::FontBorrow>(&fields->fields[0xcc/4]);
    require(text_cell&&text_cell->object&&font_cell&&font_cell->loan,"Run pending actual C4/CC absent");
    //ALL owners copied by VALUE before make_shared/new/shared-control allocators.
    const auto object=text_cell->object;const auto font=font_cell->loan;
    std::shared_ptr<const BnwLabelSystemFontBorrower> system;
    if(const auto* cell=std::get_if<Impl::SystemFontBorrow>(&fields->fields[0x170/4])){
        require(bool(cell->loan),"Run pending actual170 empty descriptor");system=cell->loan;
    }
    else require(std::holds_alternative<Impl::Null>(fields->fields[0x170/4]),"Run pending170 unknown");
    const auto binding=init->binding.lock();require(bool(binding),"Run pending actual active Binding expired");
    const auto core=binding->domains.at(init->parent_key.domain).core;
    const auto child=init->child;const auto parent=init->parent_baseline->button_;const auto returned=init->returned;
    const auto module=fields->module;const auto allocation=returned->impl_->allocation;
    const auto topology=core->topology_revision,parser=*init->parent_baseline->parsed_source_serial_,sequence=fields->sequence;
    init->first();
    auto data=std::make_shared<BnwPositionedLabelRunLoan::Impl>(init,core,child,parent,returned,module,
        allocation,object,font,system,topology,parser,sequence);
    init->first();data->validate_child_without_callbacks(*init->operation,child);init->full();init->first();
    auto pending=std::shared_ptr<BnwPositionedLabelRunLoan>(new BnwPositionedLabelRunLoan(data));
    init->first();data->validate_child_without_callbacks(*init->operation,child);init->full();init->first();
    //No completed_run store or Paint/Guard grant exists before outer return.
    return pending;
}

std::shared_ptr<const detail::BnwPositionedLabelRunLoan>
BnwWorldViewControlTree::positioned_label_run_owned(Control actual,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto core=core_;core->layout_origin();
    const auto& current=core->source->current_operation_loan();core->layout_origin();
    require(&current==&operation,"Completed Label getter needs SAME fresh canonical Source operation");
    operation.require_origin_without_callbacks();core->layout_origin();
    const auto id=core->checked(actual);const auto& node=Core::node(core->nodes,id);
    require(node.layout_baseline.has_value(),"Completed Label getter actual baseline absent");
    const auto& base=*node.layout_baseline;
    require(base.kind_==detail::NotificationLayoutBaseline::Kind::button&&base.actual_type_=="Button"&&
        base.parsed_source_serial_&&base.button_,"Completed Label getter needs actual parsed Button");
    const auto button=base.button_;require(button->child_178_observation.has_value(),
        "Completed Label getter actual178 observation unavailable");
    if(*button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null){
        require(!button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_,
            "Completed Label genuine constructorNULL178 changed");
        operation.require_current();core->layout_origin();
        const auto& after=Core::node(core->nodes,id);require(after.layout_baseline&&after.layout_baseline->button_.get()==button.get()&&
            button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::constructor_null&&
            !button->label_178_&&!button->label_178_factory_return_&&!button->pending_label_178_,
            "Completed Label constructorNULL178 changed across Source POST");
        return nullptr; //Earned actual absence, never missing producer success.
    }
    require(*button->child_178_observation==detail::NotificationLayoutButtonState::Child178Observation::factory_child&&
        button->label_178_&&button->label_178_factory_return_&&!button->pending_label_178_,
        "Completed Label published actual child absent");
    const auto child=button->label_178_;const auto run=child->impl_->completed_run;
    require(bool(run),"Completed Label genuine post-Button-return producer unavailable");
    const auto guard=run->capture_guard_without_callbacks(operation);
    const auto same_control=[&]{
        guard.require_current_without_callbacks();core->layout_origin();
        const auto owner=run->impl_->core.lock();
        require(owner&&owner.get()==core.get()&&!owner.owner_before(core)&&!core.owner_before(owner)&&
            run->impl_->node_id==id&&core->checked(actual)==id,
            "Completed Label DATA belongs to another actual checked Control/Core");
        const auto& after=Core::node(core->nodes,id);
        require(after.layout_baseline&&after.layout_baseline->button_.get()==button.get()&&
            !after.layout_baseline->button_.owner_before(button)&&!button.owner_before(after.layout_baseline->button_)&&
            button->label_178_.get()==child.get()&&!button->label_178_.owner_before(child)&&
            !child.owner_before(button->label_178_)&&child->impl_->completed_run.get()==run.get()&&
            !child->impl_->completed_run.owner_before(run)&&!run.owner_before(child->impl_->completed_run),
            "Completed Label actual checked Control/178/published Run changed");
        guard.require_current_without_callbacks();
    };
    same_control();guard.require_source_owned(operation);same_control();return run;
}

// Private narrow successors in the complete Run TU. These DATA getters never
// issue a Font/name/style/run/paint grant and never read Font Impl cross-TU.
std::array<std::uint32_t,3>
detail::BnwPositionedLabelRunLoan::Guard::actual_style_pass_selectors_without_callbacks()const{
    require_current_without_callbacks();
    const auto result=impl_->font->style_layer_texture_indices_without_callbacks(impl_->value.descriptor[5]);
    require_current_without_callbacks();return result;
}
std::array<std::int32_t,3>
detail::BnwPositionedLabelRunLoan::Guard::actual_font_metrics_without_callbacks()const{
    require_current_without_callbacks();const auto result=impl_->font->metrics_without_callbacks();
    require_current_without_callbacks();return result;
}
