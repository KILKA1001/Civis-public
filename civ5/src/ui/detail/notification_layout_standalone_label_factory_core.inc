//Inside civ5::ui after Core/RuntimeDimensions and genuine selected FullXmlOccurrence.
//529646->703120->764170 only; ordinary45EA90 parser return is NOT issued here.
struct detail::NotificationLayoutStandaloneLabelState::ConstructionLoan {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const std::shared_ptr<NotificationLayoutRuntimeDimensions::State> parser;
    const std::shared_ptr<BnwLabelProcessModuleOwner> module;
    const std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> ticket;
    const std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    ConstructionLoan(std::shared_ptr<NotificationLayoutRuntimeDimensions::State> p,
        std::shared_ptr<BnwLabelProcessModuleOwner> m,
        std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> t,
        std::shared_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> x)
        :parser(std::move(p)),module(std::move(m)),ticket(std::move(t)),selected(std::move(x)){first();}
    ~ConstructionLoan(){alive.store(false);}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load(),"Standalone constructor lexical issuer retired/foreign");
        NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(parser);
        const auto core=parser->core.lock();require(core&&parser->id,"Standalone actual source node absent");
        const auto& node=BnwWorldViewControlTree::Core::node(core->nodes,parser->id);
        const auto source=selected?selected->source.lock():nullptr;
        require(parser->constructing_standalone_label&&selected&&node.full_xml_occurrence==selected&&
            source&&source.get()==core->source.get()&&!source.owner_before(core->source)&&
            !core->source.owner_before(source)&&selected->value.name=="Label"&&node.authored.name=="Label"&&
            node.layout_receiver_occurrence==parser->occurrence.lock()&&node.layout_baseline&&
            &*node.layout_baseline==parser->baseline&&parser->baseline->kind_==NotificationLayoutBaseline::Kind::label&&
            parser->baseline->actual_type_=="Label"&&!parser->baseline->parsed_source_serial_&&
            !parser->baseline->standalone_label_&&module,
            "Standalone constructor needs SAME selected Source XML occurrence and physical node allocation");
        const auto actual=parser->operation->label_module_owned();
        require(actual&&actual.get()==module.get()&&!actual.owner_before(module)&&!module.owner_before(actual),
            "Standalone constructor actual module renderer supplier changed");
        module->require_current_without_callbacks();
    }
    void full()const{first();parser->operation->require_current();first();}
};
struct detail::NotificationLayoutStandaloneLabelState::ConstructorReturn {
    const std::uint64_t sequence,source_serial;
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    const std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> ticket;
    ConstructorReturn(const ConstructionLoan& loan,std::uint64_t n):sequence(n),source_serial(loan.parser->serial),
        core(loan.parser->core),occurrence(loan.parser->occurrence),selected(loan.selected),ticket(loan.ticket){}
};
struct detail::NotificationLayoutStandaloneLabelState::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};bool busy=false,entered=false;
    const std::shared_ptr<BnwLabelProcessModuleOwner> module;
    const std::uint64_t source_serial;
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::weak_ptr<const BnwWorldViewControlTree::Core::FullXmlOccurrence> selected;
    enum class Vptr{reference_a67fa8,base_a40cb8,flags_a40ca4,label_a48dd0};
    struct Null{};struct SelfFlags{const void* actual_cell;};struct SelfInline140{const void* actual_inline_cell;};
    struct ParsedRealm{std::shared_ptr<const NotificationLayoutContextRealmRef> value;};
    struct ParentReceiverBorrow{std::weak_ptr<const NotificationLayoutOrdinaryReceiver> value;};
    struct FontBorrow{std::shared_ptr<const BnwCompletedFontLoan> loan;};
    struct SystemFontBorrow{std::shared_ptr<const BnwLabelSystemFontBorrower> loan;};
    struct TextObject{const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> retired{false};
        std::weak_ptr<Impl> allocation_child;std::uint64_t source_serial=0;std::vector<char> bytes;};
    struct TextBorrow{std::shared_ptr<TextObject> object;};
    struct IDBorrow{std::shared_ptr<const NotificationLayoutControlID58> object;};
    struct TooltipBorrow{std::shared_ptr<const std::string> object;};
    struct ChildrenBorrow{std::shared_ptr<NotificationLayoutOrdinaryChildren78> object;};
    using Cell=std::variant<std::monostate,std::uint32_t,Vptr,Null,SelfFlags,SelfInline140,ParsedRealm,FontBorrow,SystemFontBorrow,TextBorrow,IDBorrow,TooltipBorrow,ChildrenBorrow,ParentReceiverBorrow>;
    std::array<Cell,0x174/4> fields;
    std::array<std::optional<std::uint8_t>,4> color_metadata_c0_c3;
    std::optional<std::uint8_t> style_c8,truncated_16e;
    std::optional<std::int16_t> wrap_168,leading_16a,truncate_16c;
    struct Entry{std::uint64_t sequence;std::uint32_t offset;};std::vector<Entry> history;std::uint64_t sequence=0;
    std::shared_ptr<const ConstructorReturn> returned;
    std::shared_ptr<const FieldsReturn> fields_returned;
    bool common_prefix_entered=false,common_prefix_returned=false;
    std::shared_ptr<const WrapperReturn> wrapper_returned;
    bool initialization_entered=false;
    enum class InitStage{none,defaults_returned,common_returned,glyph_generated,positioned_returned,bounds_returned};
    InitStage init_stage=InitStage::none;
    std::optional<std::uint8_t> inline_segment_newline_154,segment_mutated_164;
    struct PositionedGlyph{std::uint32_t byte_begin,byte_count,codepoint,lookup_codepoint,cursor;std::array<std::uint32_t,9> bits;};
    std::vector<PositionedGlyph> positioned_glyphs;
    std::shared_ptr<BnwStandalonePositionedLabelRunLoan> completed_standalone_run;
    std::shared_ptr<const InitializerFieldsReturn> initializer_fields_returned;
    explicit Impl(const ConstructionLoan& loan):module(loan.module),source_serial(loan.parser->serial),
        core(loan.parser->core),occurrence(loan.parser->occurrence),selected(loan.selected){}
    void first()const{require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
        "Standalone Label actual allocation retired/failed");module->require_current_without_callbacks();}
};
detail::NotificationLayoutStandaloneLabelState::NotificationLayoutStandaloneLabelState(std::shared_ptr<Impl> s):impl_(std::move(s)){}
detail::NotificationLayoutStandaloneLabelState::~NotificationLayoutStandaloneLabelState(){
    impl_->alive.store(false); //Memory retirement ONLY, no native7A0630/pool/subtype cleanup receipt.
}
std::shared_ptr<detail::NotificationLayoutStandaloneLabelState>
detail::NotificationLayoutStandaloneLabelState::allocate_pending_owned(const std::shared_ptr<ConstructionLoan>& loan){
    require(bool(loan),"Standalone actual constructor loan absent");loan->full();
    auto state=std::make_shared<Impl>(*loan);loan->full();state->first();
    auto result=std::shared_ptr<NotificationLayoutStandaloneLabelState>(new NotificationLayoutStandaloneLabelState(state));
    loan->full();state->first();return result;
}
void detail::NotificationLayoutStandaloneLabelState::construct_literal_owned(const std::shared_ptr<ConstructionLoan>& loan){
    const auto saved=impl_;saved->first();require(loan&&!saved->entered&&!saved->busy,"Standalone constructor replay/reentry");
    const auto pure=[&]{saved->first();loan->first();saved->first();
        require(loan->parser->pending_standalone_label&&loan->parser->pending_standalone_label.get()==this&&
            loan->parser->pending_standalone_label->impl_.get()==saved.get()&&saved->source_serial==loan->parser->serial&&
            saved->occurrence.lock()==loan->parser->occurrence.lock()&&saved->selected.lock()==loan->selected,
            "Standalone constructor actual pending allocation/Source changed");
        loan->module->require_label_allocation_owned(*loan->parser->operation,loan->ticket,
            loan->parser->pending_standalone_label,false);};
    const auto full=[&]{pure();loan->full();pure();};
    saved->busy=true;saved->entered=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{saved->busy};
    try{full();require(saved->history.max_size()>=128,"Standalone constructor journal capacity unavailable");
        saved->history.reserve(128);full();
        const auto record=[&](std::uint32_t offset){pure();require(saved->sequence<UINT64_MAX&&
            saved->history.size()<saved->history.capacity(),"Standalone constructor journal exhausted");
            saved->history.push_back({++saved->sequence,offset});pure();};
        const auto cell=[&](std::uint32_t offset,Impl::Cell value){record(offset);pure();saved->fields[offset/4]=std::move(value);};
        const auto word=[&](std::uint32_t offset,std::uint32_t value){cell(offset,value);};
        const auto reference=[&](std::uint32_t offset){cell(offset,Impl::Vptr::reference_a67fa8);word(offset+4,0);word(offset+8,0);word(offset+12,UINT32_MAX);};
        // Genuine4F9470 field order, with both complete literal callees.
        reference(0);cell(0,Impl::Vptr::base_a40cb8);
        for(std::uint32_t offset=0x10;offset<=0x34;offset+=4)word(offset,0);
        cell(0x38,Impl::Null{});cell(0x3c,Impl::Null{});word(0x40,0);word(0x44,0x3f800000);word(0x48,0);
        word(0x4c,0);word(0x50,0xfffffffe); //8F8490
        cell(0x54,Impl::Null{});word(0x58,0);reference(0x5c);
        cell(0x6c,Impl::SelfFlags{&saved->fields[0x40/4]});cell(0x5c,Impl::Vptr::flags_a40ca4);
        word(0x70,0);word(0x74,0);cell(0x78,Impl::Null{});cell(0x7c,Impl::Null{});cell(0x80,Impl::Null{});
        //764170 and540EB0. No font/style/layer registry grant from color cells.
        cell(0,Impl::Vptr::label_a48dd0);
        for(std::uint32_t n=0;n<3;++n){
            for(std::uint32_t k=0;k<4;++k)word(0x84+n*16+k*4,0x3f800000);
            word(0xb4+n*4,0);record(0xc0+n);pure();saved->color_metadata_c0_c3[n]=0;
        }
        cell(0xc4,Impl::Null{});record(0xc8);pure();saved->style_c8=0;cell(0xcc,Impl::Null{});
        cell(0x128,Impl::Null{});cell(0x12c,Impl::Null{});cell(0x130,Impl::Null{});
        cell(0x134,Impl::SelfInline140{&saved->fields[0x140/4]});word(0x138,0);word(0x13c,1);
        record(0x168);pure();saved->wrap_168=-1;record(0x16a);pure();saved->leading_16a=0;
        record(0x16c);pure();saved->truncate_16c=-1;record(0x16e);pure();saved->truncated_16e=0;cell(0x170,Impl::Null{});
        loan->module->constructor_space_prefix_owned(*loan->parser->operation);pure();
        for(std::uint32_t offset=0xd0;offset<=0x120;offset+=4)cell(offset,Impl::Null{});
        word(0x40,0x09000000);pure();
        full(); //Actual764170 constructor body RETURN, before issuing any opaque receipt.
    }catch(...){saved->failed.store(true);throw;}
}
void detail::NotificationLayoutRuntimeDimensions::construct_standalone_label_factory_owned()const{
    using Label=NotificationLayoutStandaloneLabelState;
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto core=saved->core.lock();require(core&&saved->id&&saved->baseline->kind_==NotificationLayoutBaseline::Kind::label&&
        !saved->constructing_standalone_label&&!saved->pending_standalone_label&&!saved->baseline->standalone_label_,
        "Standalone529646 factory needs distinct genuine selected node exactly once");
    const auto selected=BnwWorldViewControlTree::Core::node(core->nodes,saved->id).full_xml_occurrence;
    require(selected&&selected->value.name=="Label","Standalone factory selected XML occurrence absent");
    const auto module=saved->operation->label_module_owned();require_storage_without_callbacks(saved);
    saved->constructing_standalone_label=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{saved->constructing_standalone_label};
    std::shared_ptr<BnwLabelProcessModuleOwner::LabelAllocationAttempt> ticket;
    try{ticket=module->begin_standalone_label_fresh_allocation_owned(*saved->operation);
        require_storage_without_callbacks(saved);
        auto loan=std::shared_ptr<Label::ConstructionLoan>(new Label::ConstructionLoan(saved,module,ticket,selected));loan->full();
        auto label=Label::allocate_pending_owned(loan);loan->full();
        module->finish_label_fresh_allocation_owned(*saved->operation,ticket,label);loan->full();
        saved->pending_standalone_label=label;loan->full();
        label->construct_literal_owned(loan);loan->full();
        module->finish_label_constructor_return_owned(*saved->operation,ticket,label);loan->full();
        auto returned=std::shared_ptr<const Label::ConstructorReturn>(new Label::ConstructorReturn(*loan,label->impl_->sequence));loan->full();
        module->require_label_allocation_owned(*saved->operation,ticket,label,true);loan->first();
        require(!label->impl_->returned&&saved->pending_standalone_label==label,"Standalone constructor return already published");
        label->impl_->returned=returned;
        auto& baseline=*core->nodes.at(saved->id-1).layout_baseline;
        require(&baseline==saved->baseline,"Standalone constructor publication baseline changed");
        baseline.standalone_label_=label;saved->pending_standalone_label.reset();
        baseline.base_.flags_40=std::get<std::uint32_t>(label->impl_->fields[0x40/4]);
        require_storage_without_callbacks(saved);
        //ConstructorReturn authorizes ONLY real764170 cells and SAME physical
        //allocation. No parsed serial, ordinary38/3C/78 or Run is issued.
    }catch(...){module->fail_label_allocation_owned(ticket);
        if(saved->pending_standalone_label)saved->pending_standalone_label->impl_->failed.store(true);
        fail_storage_without_callbacks(saved);throw;}
}
//Actual standalone45EA90 wrapper is defined in the later standalone parser leaf.
