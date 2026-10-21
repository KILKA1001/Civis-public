// Inside civ5::ui after complete Core, GuiPort and RuntimeDimensions::State.
// Root/Events supplies the actual54395B String-layer and pending-parent lenders.
// There is no public/test issuer or metadata-only caller acceptance here.
struct detail::NotificationLayoutEmbeddedLabelState::ConstructorLoan {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<NotificationLayoutRuntimeDimensions::State> parser;
    const std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan> reached_string;
    const std::weak_ptr<NotificationLayoutButtonState> parent;
    const std::shared_ptr<BnwLabelProcessModuleOwner> module;
    const std::size_t actual_layer;
    const std::uint64_t source_serial;
    ConstructorLoan(std::shared_ptr<NotificationLayoutRuntimeDimensions::State> p,
        std::shared_ptr<const NotificationLayoutEmbeddedLabelStringLayerLoan> reached,
        std::shared_ptr<NotificationLayoutButtonState> b,
        std::shared_ptr<BnwLabelProcessModuleOwner> m,std::size_t layer)
        :parser(std::move(p)),reached_string(std::move(reached)),parent(b),module(std::move(m)),
         actual_layer(layer),source_serial(parser->serial){}
    void first()const{
        require(creator==std::this_thread::get_id(),"Embedded Label constructor foreign creator");
        NotificationLayoutRuntimeDimensions::require_storage_without_callbacks(parser);
        NotificationLayoutRuntimeDimensions::require_button_string_layer_owned(
            parser,reached_string,actual_layer);
        require(module&&parser->serial==source_serial,"Embedded Label module/Source borrower absent");
        module->require_current_without_callbacks();
        const auto current=parser->operation->label_module_owned();
        require(current&&current.get()==module.get()&&!current.owner_before(module)&&
            !module.owner_before(current),"Embedded Label actual module changed");
        const auto b=parent.lock();require(bool(b),"Embedded Label actual Button owner retired");
        // The canonical runtime helper validates Core/allocation and actual
        // Button field identity; a cloned ButtonState pointer is insufficient.
        NotificationLayoutRuntimeDimensions::require_button_string_layer_owned(
            parser,reached_string,actual_layer);
        module->require_current_without_callbacks();
    }
};
struct detail::NotificationLayoutEmbeddedLabelState::Impl {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};bool busy=false,entered=false;
    const std::weak_ptr<NotificationLayoutButtonState> construction_parent;
    const std::shared_ptr<BnwLabelProcessModuleOwner> module;
    const std::uint64_t source_serial;
    const std::size_t actual_layer;
    enum class Vptr {reference_a67fa8,base_a40cb8,flags_a40ca4,label_a48dd0};
    struct Null{};
    struct SelfFlags{const void* actual_cell;};
    struct SelfInline140{const void* actual_inline_cell;};
    // Canonical owned constructor cells, not an Original memory overlay.
    // Unknown/unwritten cells remain disengaged; vptr/null/self-pointer tags
    // are typed observations, never fabricated host pointers or ControlIDs.
    struct ParsedRealm{std::shared_ptr<const NotificationLayoutContextRealmRef> value;};
    struct FontBorrow{std::shared_ptr<const BnwCompletedFontLoan> loan;};
    struct TextObject{
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> retired{false};std::weak_ptr<Impl> allocation_child;
        std::uint64_t source_serial=0;std::vector<char> bytes;
    };
    struct TextBorrow{std::shared_ptr<TextObject> object;};
    struct SystemFontBorrow{std::shared_ptr<const BnwLabelSystemFontBorrower> loan;};
    struct ParentBorrow{std::weak_ptr<NotificationLayoutButtonState> parent;const NotificationLayoutBaseline* baseline;};
    enum class InitStage{none,parent_linked,font_recalculated,defaults_returned,common_returned,glyph_generated,positioned_returned,bounds_returned,live_reinit_entered};
    InitStage init_stage=InitStage::none;
    std::shared_ptr<const BnwPositionedLabelRunLoan> completed_run; //Only actual post-Button-return publisher.
    std::optional<std::uint8_t> inline_segment_newline_154;
    std::optional<std::uint8_t> segment_mutated_164;
    struct PositionedGlyph{std::uint32_t byte_begin,byte_count,codepoint,lookup_codepoint,cursor;std::array<std::uint32_t,9> bits;};
    std::vector<PositionedGlyph> positioned_glyphs;
    using Cell=std::variant<std::monostate,std::uint32_t,Vptr,Null,SelfFlags,SelfInline140,ParsedRealm,FontBorrow,TextBorrow,SystemFontBorrow,ParentBorrow>;
    std::array<Cell,0x174/4> fields;
    std::array<std::optional<std::uint8_t>,4> color_metadata_c0_c3;
    std::optional<std::uint8_t> style_c8,truncated_16e;
    std::optional<std::int16_t> wrap_168,leading_16a,truncate_16c;
    struct Entry{std::uint64_t sequence;std::uint32_t offset;};
    std::vector<Entry> history;std::uint64_t sequence=0;
    enum class Retirement {none,entered,subtype_returned,reclaimed,quarantined};
    Retirement retirement=Retirement::none;
    std::array<std::uint32_t,96> cleanup_offsets{};std::size_t cleanup_count=0;
    explicit Impl(const ConstructorLoan& loan):construction_parent(loan.parent),module(loan.module),source_serial(loan.source_serial),actual_layer(loan.actual_layer){}
    void first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "Embedded Label allocation retired/failed before constructor state");
        require(!construction_parent.expired()&&module,"Embedded Label actual parent/module retired");
        module->require_current_without_callbacks();
    }
};
detail::NotificationLayoutEmbeddedLabelState::NotificationLayoutEmbeddedLabelState(
    std::shared_ptr<Impl> state):impl_(std::move(state)){}
detail::NotificationLayoutEmbeddedLabelState::~NotificationLayoutEmbeddedLabelState(){
    retire_construction_authority_owned();
}
void detail::NotificationLayoutEmbeddedLabelState::retire_construction_authority_owned()noexcept{
    impl_->alive.store(false);
    // This owns only pending constructor storage. It does NOT replay7A0630,
    // pool return, initialized5126B0/64C8C0 or native child destruction receipt.
}
void detail::NotificationLayoutEmbeddedLabelState::require_ownership_origin_owned(
    const std::shared_ptr<NotificationLayoutButtonState>& parent,
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::size_t layer)const{
    const auto saved=impl_;saved->first();operation.require_origin_without_callbacks();saved->first();
    const auto actual=saved->construction_parent.lock();
    require(actual&&parent&&actual.get()==parent.get()&&!actual.owner_before(parent)&&
        !parent.owner_before(actual)&&saved->actual_layer==layer&&
        saved->source_serial==operation.operation_serial(),"Embedded Label pending ownership origin changed");
    saved->first();
    // Ownership only. Root pending checker MUST independently verify canonical
    // parent Core/allocation and active String-layer loan. No factory grant.
}
std::shared_ptr<detail::NotificationLayoutEmbeddedLabelState>
detail::NotificationLayoutEmbeddedLabelState::allocate_pending_owned(
    const std::shared_ptr<ConstructorLoan>& loan){
    require(bool(loan),"Embedded Label genuine constructor loan absent");loan->first();
    auto state=std::make_shared<Impl>(*loan);loan->first();state->first();
    auto child=std::shared_ptr<NotificationLayoutEmbeddedLabelState>(
        new NotificationLayoutEmbeddedLabelState(state));
    loan->first();state->first();return child;
}
void detail::NotificationLayoutEmbeddedLabelState::construct_literal_prefix_owned(
    const std::shared_ptr<ConstructorLoan>& loan){
    const auto saved=impl_;saved->first();require(bool(loan),"Embedded Label constructor loan missing");
    if(saved->busy||saved->entered){saved->failed.store(true);throw std::runtime_error("Embedded Label constructor reentry/replay");}
    const auto pure=[&]{
        saved->first();loan->first();saved->first();
        const auto child_parent=saved->construction_parent.lock();const auto caller_parent=loan->parent.lock();
        require(child_parent&&caller_parent&&child_parent.get()==caller_parent.get()&&
            !child_parent.owner_before(caller_parent)&&!caller_parent.owner_before(child_parent)&&
            saved->source_serial==loan->source_serial&&saved->module.get()==loan->module.get()&&
            !saved->module.owner_before(loan->module)&&!loan->module.owner_before(saved->module),
            "Embedded Label constructor child membership/source changed");
        NotificationLayoutRuntimeDimensions::require_pending_label_178_prefix_owned(
            loan->parser,this,loan->actual_layer);
        saved->first();loan->first();
    };
    struct Scope{std::shared_ptr<Impl> state;~Scope(){state->busy=false;}};
    saved->busy=true;saved->entered=true;Scope scope{saved};
    try{
        pure();
        const auto record=[&](std::uint32_t offset){
            require(saved->sequence<UINT64_MAX,"Embedded Label constructor history exhausted");
            saved->history.push_back({saved->sequence+1,offset});pure();++saved->sequence;
        };
        const auto cell=[&](std::uint32_t offset,Impl::Cell value){record(offset);pure();saved->fields[offset/4]=std::move(value);};
        const auto word=[&](std::uint32_t offset,std::uint32_t value){cell(offset,value);};
        const auto reference=[&](std::uint32_t offset){
            cell(offset,Impl::Vptr::reference_a67fa8);word(offset+4,0);word(offset+8,0);word(offset+12,UINT32_MAX);
        };
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
        // Literal field/module prefix is retained.703120 pool/allocator return,
        // actual parsedContextRealm3C and5E3A90 invocation remain unissued.
    }catch(...){saved->failed.store(true);throw;}
}

void detail::NotificationLayoutRuntimeDimensions::begin_embedded_label_prefix_owned(
    std::size_t actual_parser_layer)const{
    using Child=NotificationLayoutEmbeddedLabelState;
    const auto saved=state_;require_storage_without_callbacks(saved);
    const auto reached=saved->active_button_string_layer_;
    // This checker/loan comes solely from Events' real PRESENT/OR lexical path.
    // No matching XML record or size_t selector can issue it.
    require_button_string_layer_owned(saved,reached,actual_parser_layer);
    require(saved->baseline->kind_==NotificationLayoutBaseline::Kind::button&&
        saved->baseline->actual_type_=="Button"&&saved->baseline->button_,
        "Embedded Label prefix actual ordinary Button receiver absent");
    const auto parent=saved->baseline->button_;
    const auto module=saved->operation->label_module_owned();
    require_button_string_layer_owned(saved,reached,actual_parser_layer);
    auto loan=std::shared_ptr<Child::ConstructorLoan>(new Child::ConstructorLoan(
        saved,reached,parent,module,actual_parser_layer));loan->first();
    auto child=Child::allocate_pending_owned(loan);loan->first();
    retain_pending_label_178_prefix_owned(saved,child,actual_parser_layer);
    require_pending_label_178_prefix_owned(saved,child.get(),actual_parser_layer);
    child->construct_literal_prefix_owned(loan);loan->first();
    require_pending_label_178_prefix_owned(saved,child.get(),actual_parser_layer);
    // Native178 remains constructorNULL. No5E3A90/3C/Font reader is entered
    // before an actual703120 allocation/pool/fullfactory-return lender exists.
    throw std::runtime_error("Actual703120 factory return and parsed ContextRealm issuer required after retained embedded Label constructor prefix");
}
