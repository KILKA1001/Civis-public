// Inside civ5::ui. Root alone can construct these complete private tuples.
// No native mapper, Common invocation, Bank initializer or publication is issued here.
namespace detail {
namespace {
struct NonFontCapturedWord final {
    BnwNormalNonFontNativeRegion region;
    std::size_t offset;
    std::bitset<4> written;
};
static void nonfont_provider_region(const BnwNormalNonFontNativeRegion& r,std::size_t bytes){
    require(r.owner&&r.bytes&&r.extent>=bytes&&r.native_base&&
        r.extent<=std::uint64_t{0x100000000ull}-r.native_base,
        "NonFont mapped original owner/address/extent unavailable");
}
static std::uint32_t nonfont_provider_word(const NonFontCapturedWord& word){
    nonfont_provider_region(word.region,4);
    require(word.written.all()&&word.offset<=word.region.extent-4,
        "NonFont operand has no actual written four-byte coverage");
    std::uint32_t result;std::memcpy(&result,word.region.bytes+word.offset,4);return result;
}
template<class A,class B>static bool nonfont_provider_same_owner(const A& a,const B& b){
    return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
}

struct BnwNormalNonFontCurrentReceiverGuard::Impl final {
    struct SourceFields final {
        BnwNormalNonFontNativeRegion region;
        std::array<NonFontCapturedWord,3> fields_10c_110_11c;
    };
    struct CapturedFields final {
        std::shared_ptr<const BnwNormalCommonChildCall> original_call;
        std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
        BnwNormalRootPaintCallFrame::Guard frame;
        BnwNormalRootBatchHistory::Guard root;
        const runtime::BnwWorldViewSourceOperationLoan* operation;
        std::uint64_t serial;
        std::uint64_t control;
        std::size_t ordinal;
        std::uint32_t outer_flags40;
        BnwNormalNonFontNativeRegion receiver;
        std::bitset<0x60> written_receiver;
        std::array<std::optional<SourceFields>,2> sources;
        std::array<NonFontCapturedWord,5> argument_operands;
        std::array<NonFontCapturedWord,2> fixed_global_operands;
        std::array<NonFontCapturedWord,2> arithmetic_literal_operands;
        NonFontCapturedWord material_selector_operand;
    };
    const std::thread::id creator=std::this_thread::get_id();
    const CapturedFields fields;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    struct Snapshot final {
        std::array<std::byte,0x60> receiver;
        std::array<std::array<std::uint32_t,3>,2> source_words;
        std::array<std::uint32_t,5> arguments;
        std::array<std::uint32_t,2> fixed_globals,literals;
        std::uint32_t material_selector;
    };
    const Snapshot snapshot;
    static Snapshot capture(const Impl& original){
        // The real Tree helper earns authority before ANY mapping is borrowed.
        BnwWorldViewControlTree::require_nonfont_receiver_inputs_without_callbacks(original);
        const auto& f=original.fields;Snapshot s{};
        std::memcpy(s.receiver.data(),f.receiver.bytes,s.receiver.size());
        for(std::size_t i=0;i!=2;++i)if(f.sources[i])
            for(std::size_t k=0;k!=3;++k)s.source_words[i][k]=nonfont_provider_word(f.sources[i]->fields_10c_110_11c[k]);
        for(std::size_t i=0;i!=5;++i)s.arguments[i]=nonfont_provider_word(f.argument_operands[i]);
        for(std::size_t i=0;i!=2;++i){s.fixed_globals[i]=nonfont_provider_word(f.fixed_global_operands[i]);
            s.literals[i]=nonfont_provider_word(f.arithmetic_literal_operands[i]);}
        s.material_selector=nonfont_provider_word(f.material_selector_operand);
        BnwWorldViewControlTree::require_nonfont_receiver_inputs_without_callbacks(original);return s;
    }
    explicit Impl(CapturedFields captured):fields(std::move(captured)),snapshot(capture(*this)){}
    void inputs()const{BnwWorldViewControlTree::require_nonfont_receiver_inputs_without_callbacks(*this);}
    void first()const{
        require(creator==std::this_thread::get_id()&&sealed&&canonical_self.lock().get()==this,
            "NonFont original receiver tuple unsealed/retired/foreign");inputs();
        require(std::memcmp(snapshot.receiver.data(),fields.receiver.bytes,snapshot.receiver.size())==0,
            "NonFont original receiver changed after capture");
        for(std::size_t i=0;i!=2;++i)if(fields.sources[i])for(std::size_t k=0;k!=3;++k)
            require(snapshot.source_words[i][k]==nonfont_provider_word(fields.sources[i]->fields_10c_110_11c[k]),
                "NonFont source10C/110/11C current words changed");
        for(std::size_t i=0;i!=5;++i)require(snapshot.arguments[i]==nonfont_provider_word(fields.argument_operands[i]),
            "NonFont current argument operand changed");
        for(std::size_t i=0;i!=2;++i){require(snapshot.fixed_globals[i]==nonfont_provider_word(fields.fixed_global_operands[i]),
            "NonFont current fixed-slot global changed");
            require(snapshot.literals[i]==nonfont_provider_word(fields.arithmetic_literal_operands[i]),
                "NonFont qualified F32 operand changed");}
        require(snapshot.material_selector==nonfont_provider_word(fields.material_selector_operand),
            "NonFont current material selector operand changed");inputs();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();require(&operation==fields.operation,"NonFont receiver different original Source operation");
        fields.original_call->require_source_owned(operation);first();
        fields.frame.require_source_owned(operation);first();
        fields.root.require_source_owned(operation);first();
        operation.require_operation(fields.serial);first();
        fields.root.require_source_owned(operation);first();
    }
};

struct BnwNormalNonFontResourceBank::Impl final {
    struct Entry final {
        NonFontCapturedWord native_slot;
        BnwNormalNonFontNativeRegion native_record;
        std::shared_ptr<const BnwNormalNonFontResourceLoan> original_loan;
        std::weak_ptr<const BnwNormalNonFontResourceLoan> canonical_loan;
    };
    struct CapturedFields final {
        BnwNormalRootBatchHistory::Guard root;
        std::shared_ptr<const BnwWorldViewControlRenderPublication> publication;
        const runtime::BnwWorldViewSourceOperationLoan* operation;
        std::uint64_t serial;
        BnwNormalNonFontNativeRegion canonical_bank_storage;
        NonFontCapturedWord live_count;
        std::vector<Entry> entries;
    };
    const std::thread::id creator=std::this_thread::get_id();
    const CapturedFields fields;
    const std::uint32_t captured_live_count;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    explicit Impl(CapturedFields captured):fields(std::move(captured)),captured_live_count(capture_count(*this)){}
    static std::uint32_t capture_count(const Impl& original){
        BnwWorldViewControlTree::require_nonfont_bank_inputs_without_callbacks(original);
        const auto count=nonfont_provider_word(original.fields.live_count);
        require(original.fields.entries.size()==count,"NonFont Bank incomplete actual native ID domain");
        return count;
    }
    void inputs()const{
        BnwWorldViewControlTree::require_nonfont_bank_inputs_without_callbacks(*this);
        require(nonfont_provider_word(fields.live_count)==captured_live_count&&fields.entries.size()==captured_live_count,
            "NonFont Bank actual liveCount changed after capture");
        for(std::size_t i=0;i!=fields.entries.size();++i){const auto& e=fields.entries[i];
            nonfont_provider_region(e.native_record,1);
            require(e.original_loan&&nonfont_provider_same_owner(e.canonical_loan.lock(),e.original_loan),
                "NonFont Bank original canonical resource facade replaced");
            require(nonfont_provider_word(e.native_slot)==e.native_record.native_base,
                "NonFont Bank actual native ID slot changed");
            e.original_loan->require_physical_without_callbacks();
            e.original_loan->require_same_root_without_callbacks(fields.root);
            e.original_loan->require_actual_resource_owner_without_callbacks(e.native_record.owner);
            require(e.original_loan->resource_id_without_callbacks()==i,"NonFont Bank present native ID association differs");
        }
        BnwWorldViewControlTree::require_nonfont_bank_inputs_without_callbacks(*this);
    }
    void first()const{
        require(creator==std::this_thread::get_id()&&sealed&&canonical_self.lock().get()==this,
            "NonFont original Bank tuple unsealed/retired/foreign");
        BnwWorldViewControlTree::require_nonfont_bank_current_without_callbacks(*this);
        inputs();
        BnwWorldViewControlTree::require_nonfont_bank_current_without_callbacks(*this);
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
        first();require(&operation==fields.operation,"NonFont Bank different original Source operation");
        fields.root.require_source_owned(operation);first();operation.require_operation(fields.serial);first();
        for(const auto& e:fields.entries){e.original_loan->require_source_owned(operation);first();}
        fields.root.require_source_owned(operation);first();
    }
};
}

// These member helpers use EXISTING Tree friendship of canonical lenders.
// They call no Source/FULL/native/supplier callback and never issue an owner.
void BnwWorldViewControlTree::require_nonfont_receiver_inputs_without_callbacks(
    const detail::BnwNormalNonFontCurrentReceiverGuard::Impl& data){
    const auto& f=data.fields;
    require(data.creator==std::this_thread::get_id()&&f.original_call&&f.publication&&f.operation,
        "NonFont receiver original immutable owners absent/foreign");
    const auto call=f.original_call;const auto pub=f.publication;
    call->require_current_without_callbacks();f.frame.require_current_without_callbacks();
    f.root.require_physical_current_without_callbacks();
    require(call->impl_->kind==detail::BnwNormalCommonChildCall::Kind::button&&
        call->impl_->phase==detail::BnwNormalCommonChildCall::Phase::button_own118&&
        call->impl_->serial==f.serial,"NonFont receiver is not original pending own118");
    require(f.control==call->impl_->control.id_&&f.ordinal==call->impl_->ordinal&&
        f.outer_flags40==call->impl_->physical_flags,"NonFont captured control/ordinal/OUTER flags differs");
    const auto captured_root=call->captured_root_without_callbacks();captured_root.require_same_root_without_callbacks(f.root);
    pub->require_actual_receiver_for_paint_without_callbacks();
    const auto original=pub->original_owner_without_callbacks();
    require(detail::nonfont_provider_same_owner(original,pub)&&pub->tree_&&pub->control_&&pub->renderer_&&
        pub->adjusted_frame_painting_&&pub->enclosing_context_entry_&&!pub->root_context_entry_,
        "NonFont receiver current original indexed Publication missing");
    const auto tree=pub->tree_;const auto actual=tree->core_;actual->layout_origin(true);
    const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
    require(&fresh==f.operation&&fresh.operation_serial()==f.serial&&
        pub->actual_checked_control_id_for_paint_without_callbacks()==call->impl_->control.id_,
        "NonFont receiver original Source/control changed before mapping");
    f.frame.require_actual_publication_without_callbacks(*pub);
    f.frame.require_actual_control_without_callbacks(*tree,call->impl_->control.id_);
    f.frame.require_actual_renderer_without_callbacks(*pub->renderer_);
    f.frame.require_actual_frame_owner_without_callbacks(pub->adjusted_frame_owner_);
    f.root.require_actual_renderer_without_callbacks(*pub->renderer_);
    const auto entry=call->captured_root_context_entry_without_callbacks();
    pub->enclosing_context_entry_->require_same_entry_without_callbacks(entry);
    const auto button=call->impl_->original_button;
    require(button&&button->backing_118_&&f.written_receiver.all(),"NonFont actual written Button backing missing");
    button->backing_118_->require_same_button_storage_without_callbacks(button);
    detail::nonfont_provider_region(f.receiver,0x60);
    for(std::uint32_t offset=0;offset!=0x60;offset+=4){
        std::uint32_t word;std::memcpy(&word,f.receiver.bytes+offset,4);
        require(word==button->backing_118_->require_written_word_without_callbacks(offset),
            "NonFont mapped own118 is not SAME current executed backing");
    }
    for(std::size_t i=0;i!=2;++i){std::uint32_t pointer;
        std::memcpy(&pointer,f.receiver.bytes+(i?0x48:0x44),4);
        require(bool(f.sources[i])==(pointer!=0),"NonFont actual source44/48 branch differs");
        if(f.sources[i]){const auto& s=*f.sources[i];detail::nonfont_provider_region(s.region,0x120);
            require(s.region.native_base==pointer,"NonFont source native32 association differs");
            for(std::size_t k=0;k!=3;++k){const auto& word=s.fields_10c_110_11c[k];
                require(detail::nonfont_provider_same_owner(word.region.owner,s.region.owner)&&
                    word.region.bytes==s.region.bytes&&word.region.native_base==s.region.native_base&&
                    word.offset==std::array<std::size_t,3>{0x10c,0x110,0x11c}[k],
                    "NonFont source actual covered field association missing");
                (void)detail::nonfont_provider_word(word);}}
    }
    for(const auto& word:f.argument_operands)(void)detail::nonfont_provider_word(word);
    for(const auto& word:f.fixed_global_operands)(void)detail::nonfont_provider_word(word);
    for(std::size_t i=0;i!=2;++i){const auto& word=f.arithmetic_literal_operands[i];
        require(std::uint64_t{word.region.native_base}+word.offset==std::array<std::uint32_t,2>{0x9eb1e0,0x9dd390}[i],
            "NonFont F32 operand is not the qualified actual Original address");
        (void)detail::nonfont_provider_word(word);}
    (void)detail::nonfont_provider_word(f.material_selector_operand);
    const auto offsets=call->actual_child_offset_words_without_callbacks();
    require(detail::nonfont_provider_word(f.argument_operands[0])==offsets[0]&&
        detail::nonfont_provider_word(f.argument_operands[1])==offsets[1]&&
        detail::nonfont_provider_word(f.argument_operands[2])==call->actual_child_alpha_word_without_callbacks(),
        "NonFont first three arguments are not SAME actual Common call");
    call->require_current_without_callbacks();f.frame.require_current_without_callbacks();f.root.require_physical_current_without_callbacks();
}
void BnwWorldViewControlTree::require_nonfont_bank_inputs_without_callbacks(
    const detail::BnwNormalNonFontResourceBank::Impl& data){
    const auto& f=data.fields;require(data.creator==std::this_thread::get_id()&&f.publication&&f.operation,
        "NonFont Bank original Root/Publication/Source missing");
    f.root.require_physical_current_without_callbacks();const auto pub=f.publication;
    const auto original=pub->original_owner_without_callbacks();
    require(detail::nonfont_provider_same_owner(original,pub)&&pub->tree_&&pub->renderer_,
        "NonFont Bank original canonical Publication missing");
    const auto actual=pub->tree_->core_;actual->layout_origin(true);
    const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
    require(&fresh==f.operation&&fresh.operation_serial()==f.serial,"NonFont Bank current Source changed before mapping");
    f.root.require_actual_renderer_without_callbacks(*pub->renderer_);
    detail::nonfont_provider_region(f.canonical_bank_storage,1);
    require(detail::nonfont_provider_same_owner(f.live_count.region.owner,f.canonical_bank_storage.owner),
        "NonFont Bank liveCount has different original storage owner");
    (void)detail::nonfont_provider_word(f.live_count);f.root.require_physical_current_without_callbacks();
}
void BnwWorldViewControlTree::require_nonfont_bank_current_without_callbacks(
    const detail::BnwNormalNonFontResourceBank::Impl& data){
    require_nonfont_bank_inputs_without_callbacks(data);
    // A future genuine Bank issuer must publish this weak current control ONLY
    // after its native8E1990 tuple/resource owners are earned. This leaf never stores it.
    const auto original=data.fields.publication->tree_->core_->normal_nonfont_current_resource_bank.lock();
    require(original&&original->impl_.get()==&data&&
        detail::nonfont_provider_same_owner(data.canonical_self.lock(),original->impl_),
        "NonFont Bank has no SAME actual outside current publisher");
    require_nonfont_bank_inputs_without_callbacks(data);
}

namespace detail {
BnwNormalNonFontCurrentReceiverGuard::BnwNormalNonFontCurrentReceiverGuard(
    std::shared_ptr<const Impl> data,const BnwNormalNonFontCaptureKey&):impl_(std::move(data)){
    require(bool(impl_),"NonFont receiver tuple missing");impl_->first();
}
void BnwNormalNonFontCurrentReceiverGuard::require_current_without_callbacks()const{
    const auto data=impl_;require(bool(data),"NonFont receiver tuple missing");data->first();
}
void BnwNormalNonFontCurrentReceiverGuard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto data=impl_;require(bool(data),"NonFont receiver tuple missing");data->full(operation);
}
void BnwNormalNonFontCurrentReceiverGuard::require_same_receiver_without_callbacks(const BnwNormalNonFontCurrentReceiverGuard& borrowed)const{
    const auto a=impl_;const auto b=borrowed.impl_;require(a&&b,"NonFont receiver controls missing");a->first();b->first();
    require(nonfont_provider_same_owner(a,b),"NonFont original receiver control substituted");a->first();b->first();
}
void BnwNormalNonFontCurrentReceiverGuard::require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard& borrowed)const{
    const auto data=impl_;const auto root=borrowed;data->first();root.require_physical_current_without_callbacks();
    data->fields.root.require_same_root_without_callbacks(root);data->first();
}
void BnwNormalNonFontCurrentReceiverGuard::require_actual_publication_without_callbacks(const BnwWorldViewControlRenderPublication& publication)const{
    const auto data=impl_;data->first();require(data->fields.publication.get()==&publication,"NonFont different original Publication");
    data->fields.frame.require_actual_publication_without_callbacks(publication);data->first();
}
void BnwNormalNonFontCurrentReceiverGuard::require_actual_frame_without_callbacks(const BnwNormalRootPaintCallFrame::Guard& borrowed)const{
    const auto data=impl_;const auto frame=borrowed;data->first();frame.require_current_without_callbacks();
    require(nonfont_provider_same_owner(frame.impl_,data->fields.frame.impl_),"NonFont different original Frame control");data->first();
}
void BnwNormalNonFontCurrentReceiverGuard::require_actual_renderer_without_callbacks(const UiScreenGlRenderer& renderer)const{
    const auto data=impl_;data->first();data->fields.frame.require_actual_renderer_without_callbacks(renderer);
    data->fields.root.require_actual_renderer_without_callbacks(renderer);data->first();
}
std::uint64_t BnwNormalNonFontCurrentReceiverGuard::actual_control_without_callbacks()const{
    const auto data=impl_;data->first();return data->fields.control;
}
std::size_t BnwNormalNonFontCurrentReceiverGuard::actual_ordered_ordinal_without_callbacks()const{
    const auto data=impl_;data->first();return data->fields.ordinal;
}
std::uint32_t BnwNormalNonFontCurrentReceiverGuard::flags_word40_without_callbacks()const{
    const auto data=impl_;data->first();return data->fields.outer_flags40;
}
BnwNormalNonFontNativeRegion BnwNormalNonFontCurrentReceiverGuard::receiver_region_without_callbacks()const{
    const auto data=impl_;data->first();return data->fields.receiver;
}
std::optional<BnwNormalNonFontNativeRegion> BnwNormalNonFontCurrentReceiverGuard::source_region_without_callbacks(std::size_t lane)const{
    const auto data=impl_;data->first();require(lane<2,"NonFont source lane out of domain");
    if(!data->fields.sources[lane])return std::nullopt;return data->fields.sources[lane]->region;
}
std::array<std::uint32_t,5> BnwNormalNonFontCurrentReceiverGuard::argument_words_without_callbacks()const{
    const auto data=impl_;data->first();return data->snapshot.arguments;
}
std::array<std::uint32_t,2> BnwNormalNonFontCurrentReceiverGuard::fixed_slot_global_words_without_callbacks()const{
    const auto data=impl_;data->first();return data->snapshot.fixed_globals;
}
std::array<std::uint32_t,2> BnwNormalNonFontCurrentReceiverGuard::arithmetic_literal_words_without_callbacks()const{
    const auto data=impl_;data->first();return data->snapshot.literals;
}
std::uint32_t BnwNormalNonFontCurrentReceiverGuard::material_selector_without_callbacks()const{
    const auto data=impl_;data->first();return data->snapshot.material_selector;
}
BnwNormalNonFontResourceBank::BnwNormalNonFontResourceBank(std::shared_ptr<const Impl> data,const BnwNormalNonFontCaptureKey&)
    :impl_(std::move(data)){require(bool(impl_),"NonFont Bank tuple missing");impl_->inputs();}
BnwNormalNonFontResourceBank::~BnwNormalNonFontResourceBank()=default;
void BnwNormalNonFontResourceBank::require_physical_without_callbacks()const{
    const auto data=impl_;require(bool(data),"NonFont Bank tuple missing");data->first();
}
void BnwNormalNonFontResourceBank::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto data=impl_;require(bool(data),"NonFont Bank tuple missing");data->full(operation);
}
void BnwNormalNonFontResourceBank::require_same_root_without_callbacks(const BnwNormalRootBatchHistory::Guard& borrowed)const{
    const auto data=impl_;const auto root=borrowed;data->first();root.require_physical_current_without_callbacks();
    data->fields.root.require_same_root_without_callbacks(root);data->first();
}
std::shared_ptr<const BnwNormalNonFontResourceLoan> BnwNormalNonFontResourceBank::resolve_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint32_t id)const{
    const auto data=impl_;require(bool(data),"NonFont Bank tuple missing");data->first();data->full(operation);data->first();
    if(id>=data->captured_live_count){data->full(operation);data->first();return nullptr;}
    const auto original=data->fields.entries[id].original_loan;require(bool(original),"NonFont present native resource is unqualified");
    original->require_physical_without_callbacks();data->first();original->require_source_owned(operation);data->first();
    require(original->resource_id_without_callbacks()==id,"NonFont lookup returned different original native resource");
    data->full(operation);data->first();original->require_physical_without_callbacks();data->first();return original;
}
}

// Inside civ5::ui AFTER genuine CommonChildCall and original Context Scope.
// Executable association/readiness stage; no CURRENT is minted on a gap.
void BnwWorldViewControlTree::require_normal_nonfont_button_receiver_ready_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& offered,
    const detail::BnwNormalRootPaintCallFrame::Guard& offered_frame,
    const detail::BnwNormalRootBatchHistory::Guard& offered_root,
    std::shared_ptr<const BnwWorldViewControlRenderPublication> original_publication)const{
    // Retain the genuine immutable controls before the first Source boundary.
    const auto call=offered;const auto frame=offered_frame;const auto root=offered_root;
    const auto publication=std::move(original_publication);
    const auto first=[&]{
        call.require_current_without_callbacks();frame.require_current_without_callbacks();
        root.require_physical_current_without_callbacks();
        require(publication&&call.actual_kind_without_callbacks()==detail::BnwNormalCommonChildCall::Kind::button&&
            call.actual_phase_without_callbacks()==detail::BnwNormalCommonChildCall::Phase::button_own118,
            "Button current supplier is not the genuine pending own118 call");
        const auto entry=call.captured_root_context_entry_without_callbacks();
        entry.require_current_without_callbacks();entry.require_same_tree_without_callbacks(*this);
        const auto actual=core_;actual->layout_origin(true);
        const auto& fresh=actual->source->current_operation_loan();fresh.require_origin_without_callbacks();
        require(&fresh==&operation&&fresh.operation_serial()==call.impl_->serial,
            "Button current supplier SAME current Source changed before borrower");
        const auto actual_root=call.captured_root_without_callbacks();actual_root.require_same_root_without_callbacks(root);
        frame.require_actual_publication_without_callbacks(*publication);
        publication->require_actual_receiver_for_paint_without_callbacks();
        require(publication->tree_&&publication->tree_.get()==this&&publication->control_&&
            !publication->root_context_entry_&&publication->enclosing_context_entry_&&
            publication->control_->id_==call.impl_->control.id_&&
            publication->tree_->core_.get()==actual.get()&&publication->adjusted_frame_painting_&&
            !(call.impl_->physical_flags&0x10000000u),
            "Button current supplier needs SAME indexed child Publication/Frame in enclosing Context");
        frame.require_actual_control_without_callbacks(*this,call.impl_->control.id_);
        frame.require_actual_renderer_without_callbacks(*publication->renderer_);
        frame.require_actual_frame_owner_without_callbacks(publication->adjusted_frame_owner_);
        root.require_actual_renderer_without_callbacks(*publication->renderer_);
        const auto original=publication->original_owner_.lock();
        require(original&&original.get()==publication.get()&&!original.owner_before(publication)&&
            !publication.owner_before(original)&&publication->adjusted_frame_operation_==&fresh&&
            publication->adjusted_frame_source_serial_&&*publication->adjusted_frame_source_serial_==call.impl_->serial,
            "Button current supplier canonical Publication/adjusted Source control changed");
        publication->enclosing_context_entry_->require_same_entry_without_callbacks(entry);
        const auto receiver=call.impl_->receiver;const auto fields=receiver->impl_;
        const auto button=fields->button.lock();const auto& node=Core::node(actual->nodes,fields->id);
        require(button&&node.layout_baseline&&node.layout_baseline->button_.get()==button.get()&&
            !node.layout_baseline->button_.owner_before(button)&&!button.owner_before(node.layout_baseline->button_)&&
            node.layout_baseline->base_.receiver_owned.get()==receiver.get()&&
            node.layout_baseline->base_.flags_40==call.impl_->physical_flags&&button->backing_118_,
            "Button current supplier actual constructor/native40/backing association missing");
        button->backing_118_->require_same_button_storage_without_callbacks(button);
        entry.require_current_without_callbacks();frame.require_current_without_callbacks();
        root.require_physical_current_without_callbacks();call.require_current_without_callbacks();
    };
    const auto full=[&]{
        first();call.require_source_owned(operation);first();
        frame.require_source_owned(operation);first();root.require_source_owned(operation);first();
    };
    full();
    const auto button=call.impl_->receiver->impl_->button.lock();const auto backing=button->backing_118_;
    first();
    // Each byte read requires an actual executed store. No uninitialized
    // object copy, ctorzero reuse after a setter, or XML-derived byte image.
    for(std::uint32_t offset=0;offset!=0x60;offset+=4){
        (void)backing->require_written_word_without_callbacks(offset);first();
    }
    full();
}

detail::BnwNormalNonFontPaintInvocation BnwWorldViewControlTree::capture_normal_common_button_invocation_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const detail::BnwNormalCommonChildCall& offered,
    const BnwWorldViewControlRenderPublication& actual_publication,
    const detail::BnwNormalRootPaintCallFrame& actual_frame)const{
    // The UI supplies actual entered owners, never captured byte/DTO fields.
    const auto call=offered;call.require_current_without_callbacks();
    const auto frame=actual_frame.capture_guard_without_callbacks();
    frame.require_current_without_callbacks();frame.require_actual_publication_without_callbacks(actual_publication);
    const auto publication=actual_publication.original_owner_without_callbacks();
    call.require_current_without_callbacks();frame.require_current_without_callbacks();
    const auto root=call.captured_root_without_callbacks();
    require_normal_nonfont_button_receiver_ready_owned(operation,call,frame,root,publication);
    // Unknown mandatory inputs remain a real readiness error, before all
    // record/index/source effects. No CURRENT/Invocation or attach is issued.
    // Once those actual actors exist, the SAME existing capture_owned takes
    // only internally obtained Pool/Slots/Bank/Matrix and Family attaches the
    // SAME Invocation::impl_ once before returning. No duplicate attach here.
    throw std::runtime_error(
        "Button own118 CURRENT remains unissued: actual native32 address/source10C/110/11C, "
        "dispatcher arg4/5 and global550/global5E8 plus canonical pool/slots/resource-bank supplier not produced");
}
