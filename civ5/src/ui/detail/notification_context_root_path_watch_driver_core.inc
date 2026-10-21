// PRIVATE inside civ5::ui after actual ContextRootReadFileFrame and watcher11.
// Supported sameBank loaded loose path DATA. This does not assert Original
// CRC/bit19 resolver completion or fullRootRead/Controls/initialization.
// Exact effectiveSmall policy and branch proof are separate mandatory inputs.
struct BnwWorldViewControlTree::ContextRootWatcherCustody final {
    struct QuarantinedField final {
        const std::shared_ptr<detail::NotificationContextRootFileWatcher> watcher;
        QuarantinedField*next=nullptr;
        explicit QuarantinedField(std::shared_ptr<detail::NotificationContextRootFileWatcher> value)
            :watcher(std::move(value)){}
    };
    // The completed native Context DTOR must call GUI676800 before B8 DTOR.
    // That GUI owner is still unavailable; retain this independently live B8
    // field rather than report false cleanup or close handles out of order.
    std::unique_ptr<QuarantinedField> field;
    explicit ContextRootWatcherCustody(std::unique_ptr<QuarantinedField> value):field(std::move(value)){}
    ~ContextRootWatcherCustody(){
        static thread_local QuarantinedField*head=nullptr;
        if(field){auto*retained=field.release();retained->next=head;head=retained;}
    }
};

void BnwWorldViewControlTree::resolve_and_watch_context_root_file_selected_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::shared_ptr<ContextRootReadFileFrame> frame,
    const detail::NotificationContextRootSmallUiPolicyLoan& actual_policy){
    using Wide=detail::NotificationContextNativeWideStringValue;
    using Watcher=detail::NotificationContextRootFileWatcher;
    require(frame!=nullptr,"RootRead path requires actual entered read frame");frame->full();
    const auto actual=core_;const auto state=frame->state.lock();
    require(actual&&state&&state->impl_&&frame->operation==&operation&&
        frame->effect_count==2&&actual->context_root_read_file_frame.lock()==frame,
        "RootRead path before real copy510/appendXML/current read frame");
    const auto policy=actual_policy.capture_guard_without_callbacks(operation);
    frame->full();policy.require_current_without_callbacks(operation);policy.require_source_owned(operation);frame->full();
    const auto filename=frame->expected_510;require(filename!=nullptr,"RootRead actual510 missing");
    // ASCII selected filename is exactly CP_ACP and UTF8; do not admit a
    // Unicode key via guessed active CRT locale or CRC-only native identity.
    std::string query;query.reserve(filename->value_.size());frame->full();
    for(const auto unit:filename->value_){
        require(unit>0&&unit<128,"RootRead nonASCII resource key needs actual ACP/locale owner");
        query.push_back(static_cast<char>(unit));
    }
    frame->full();policy.require_current_without_callbacks(operation);
    const auto key=assets::AssetKey(std::move(query));frame->full();
    const auto physical=operation.context_xml_physical_source_owned(key);frame->full();
    require(physical!=nullptr,"RootRead missing actual loaded loose physicalsource; no recipe fallback");
    physical->require_current_without_callbacks();
    const auto bank=operation.selected_provider_startup_bank_owned();frame->full();
    physical->require_same_bank_without_callbacks(*bank);
    auto path=physical->physical_path_owned();frame->full();physical->require_same_bank_without_callbacks(*bank);
    const auto loaded_xml=physical->actual_xml_owned();frame->full();physical->require_same_bank_without_callbacks(*bank);
    require(loaded_xml!=nullptr,"RootRead physicalwinner lacks actual parsed XML DATA");
    // Two independent real issuers must refer to the exact selected document.
    // Source FullXmlOccurrence owns a VALUE copy, not the Bank allocation.
    const auto root=actual->root_full_xml_occurrence;
    require(root&&root->source.lock()==actual->source&&!root->dynamic&&root->parent.expired(),
        "RootRead source root document identity retired");
    const auto equal=[](const auto&self,const content::XmlNode&a,const content::XmlNode&b)->bool{
        if(a.name!=b.name||a.text!=b.text||a.attributes!=b.attributes||a.children.size()!=b.children.size())return false;
        for(std::size_t i=0;i<a.children.size();++i)if(!self(self,a.children[i],b.children[i]))return false;
        return true;
    };
    require(equal(equal,root->value,*loaded_xml),"RootRead chosen Bank XML differs from actual source-owned full root");
    frame->full();policy.require_current_without_callbacks(operation);
    // This issuer must prove the literal native branch, not merely ini DATA.
    // A true branch requires _small resolver +rawfallback/CWD/FileAPI owner.
    // Never infer false from namespace MISS or PE constructor defaultzero.
    if(policy.branch_taken_without_callbacks())throw std::runtime_error(
        "required RootRead SmallUI resolved-or-raw fallback/CWD/GetFileAttributesW producer missing");
    policy.require_source_owned(operation);frame->full();physical->require_same_bank_without_callbacks(*bank);
    auto wide_path=path.u16string();frame->full();physical->require_same_bank_without_callbacks(*bank);
    require(!wide_path.empty()&&wide_path.size()<256&&
        std::find(wide_path.begin(),wide_path.end(),char16_t{})==wide_path.end(),
        "RootRead physicalpath exceeds supported terminated256 watcher scan/260 buffer");
    // Preserve full owned value before allocation; ctor510 snapshot untouched.
    auto resolved=std::shared_ptr<const Wide>(new Wide(std::move(wide_path)));frame->full();
    policy.require_current_without_callbacks(operation);physical->require_same_bank_without_callbacks(*bank);
    require(!frame->temporary68,"RootRead actual path68 constructor replay");
    auto path68=std::make_shared<ContextRootReadFileFrame::PathTemporary>(
        ContextRootReadFileFrame::PathTemporary::Origin::selected_loose7493b0,physical,resolved);frame->full();
    physical->require_same_bank_without_callbacks(*bank);path68->require_live_without_callbacks();
    require(!frame->temporary68,"RootRead actual path68 changed during constructor allocation");frame->temporary68=path68;
    state->impl_->file_510_value=resolved;frame->expected_510=resolved;frame->full();
    // Native7A0030 uses wcscpy_s(...,260). Internal trailing buffer bytes are
    // normalized x64 memory only; true NUL/full selected UTF16VALUE retained.
    std::array<char16_t,260> fixed{};
    std::copy(resolved->value_.begin(),resolved->value_.end(),fixed.begin());frame->full();
    require(!frame->temporary280,"RootRead actual path280 constructor replay");
    auto path280=std::make_shared<ContextRootReadFileFrame::PathTemporary>(
        ContextRootReadFileFrame::PathTemporary::Origin::watch_path7a0030,physical,resolved);frame->full();
    physical->require_same_bank_without_callbacks(*bank);path280->require_live_without_callbacks();
    require(!frame->temporary280,"RootRead actual path280 changed during constructor allocation");frame->temporary280=path280;frame->full();
    require(!state->impl_->file_watcher_b8,"RootRead B8 reload/update path needs actual prior watcher retirement");
    auto watcher=Watcher::allocate_first_owned(fixed);frame->full();
    const auto retained_watcher=watcher;
    auto quarantine=std::make_unique<ContextRootWatcherCustody::QuarantinedField>(retained_watcher);frame->full();
    auto custody=std::make_shared<ContextRootWatcherCustody>(std::move(quarantine));frame->full();
    require(!state->impl_->file_watcher_b8,"RootRead B8 changed during allocation");
    state->impl_->file_watcher_b8=custody;frame->expected_watcher_b8=custody;frame->full();
    //71C790 ordered effects. Native failures are real retained outcomes, not
    // invented TRUE: watcher11 retains Find/Register returns and dirty state.
    retained_watcher->capture_file_attributes_owned();frame->full();
    retained_watcher->open_change_notification_owned();frame->full();
    retained_watcher->register_wait_owned();frame->full();
    retained_watcher->require_reached_without_callbacks();
    physical->require_same_bank_without_callbacks(*bank);policy.require_source_owned(operation);frame->full();
    require(state->impl_->file_watcher_b8==custody&&frame->expected_510==resolved&&
        actual->root_full_xml_occurrence==root,"RootRead reached path/watch physical publication changed");
    read_context_root_xml84_name_prefix_owned(operation,frame,bank,physical,key);
    require_context_root_read_staged_before_frame_exit_owned(frame);
    return; // own all-node cleanup DATA; actual outer frame DTOR still mandatory
}
