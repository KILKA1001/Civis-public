//Inside assets AFTER complete Bank State/Entry/SourcePin; one genuine loaded
//loose byte lender. Existing XML lender and its parsed-document grant untouched.
struct detail::CivisLoadedBytesPhysicalPathLoan::Impl final {
    const std::weak_ptr<CivisResourceBankState> bank;
    const std::uint64_t epoch;
    const CivisResourceBankState::Entry*const entry;
    const AssetHandle*const winner;
    const SourcePin*const source;
    const std::string physical_id;
    const fs::path path;
    const std::shared_ptr<const Bytes> bytes;
    const runtime::FileIdentity payload;
    std::atomic<bool> alive{true};
    Impl(const std::shared_ptr<CivisResourceBankState>& s,const CivisResourceBankState::Entry* e,
        const AssetHandle* h,const SourcePin* pin,std::string id,fs::path p,
        std::shared_ptr<const Bytes> b,runtime::FileIdentity identity)
        :bank(s),epoch(s->epoch),entry(e),winner(h),source(pin),physical_id(std::move(id)),path(std::move(p)),
         bytes(std::move(b)),payload(std::move(identity)){}
    std::shared_ptr<CivisResourceBankState> first()const {
        const auto s=bank.lock();
        need(alive.load()&&s&&s->epoch==epoch&&s->published,"Byte physical loan Bank retired/epoch changed FIRST");
        s->pure();
        need(entry&&winner&&source&&std::get_if<AssetHandle>(&entry->winner.asset)==winner,
            "Byte physical original winning AssetHandle changed");
        const auto held=s->pins.find(path);
        need(held!=s->pins.end()&&held->second.get()==source&&source->path==path,
            "Byte physical original SourcePin/path changed");
        const auto at=s->cache.find(physical_id);
        need(at!=s->cache.end()&&bytes,"Byte physical original loaded cache retired");
        const auto& loaded=at->second;
        need(loaded.bytes.get()==bytes.get()&&!loaded.bytes.owner_before(bytes)&&!bytes.owner_before(loaded.bytes)&&
            loaded.payload_identity.bytes==payload.bytes&&loaded.payload_identity.sha256==payload.sha256&&
            bytes->size()==payload.bytes,"Byte physical original loaded bytes/payload changed");
        const auto size=(static_cast<std::uint64_t>(source->stamp.nFileSizeHigh)<<32)|source->stamp.nFileSizeLow;
        need(size==payload.bytes,"Byte physical held file is not this complete payload");
        s->pure();return s;
    }
};
detail::CivisLoadedBytesPhysicalPathLoan::CivisLoadedBytesPhysicalPathLoan(std::shared_ptr<Impl> p):impl_(std::move(p)){}
detail::CivisLoadedBytesPhysicalPathLoan::~CivisLoadedBytesPhysicalPathLoan(){impl_->alive.store(false);}
void detail::CivisLoadedBytesPhysicalPathLoan::require_current_without_callbacks()const{(void)impl_->first();}
void detail::CivisLoadedBytesPhysicalPathLoan::require_same_bank_without_callbacks(const CivisResourceBankOwner& bank)const{
    const auto p=impl_;const auto s=p->first();const auto actual=s->canonical_owner.lock();
    need(actual&&actual.get()==&bank,"Byte physical source is not SAME canonical Bank");
    const auto returned=bank.retain_owner();
    need(returned.get()==actual.get()&&!returned.owner_before(actual)&&!actual.owner_before(returned),
        "Byte physical canonical Bank control block differs");
    (void)p->first();
}
fs::path detail::CivisLoadedBytesPhysicalPathLoan::physical_path_owned()const{
    const auto p=impl_;(void)p->first();auto result=p->path;(void)p->first();return result;
}
std::shared_ptr<const Bytes> detail::CivisLoadedBytesPhysicalPathLoan::actual_bytes_owned()const{
    const auto p=impl_;(void)p->first();const auto result=p->bytes;(void)p->first();return result;
}
runtime::FileIdentity detail::CivisLoadedBytesPhysicalPathLoan::payload_identity_owned()const{
    const auto p=impl_;(void)p->first();auto result=p->payload;(void)p->first();return result;
}
std::shared_ptr<const detail::CivisLoadedBytesPhysicalPathLoan>
CivisResourceBankOwner::capture_loaded_bytes_physical_path_owned(
    const std::shared_ptr<CivisResourceBankState>& state,const AssetKey& key){
    using Loan=detail::CivisLoadedBytesPhysicalPathLoan;
    const auto s=state;s->pure();need(s->published,"Byte physical source requires published Bank");
    const auto entry=s->entry(key);need(entry,"Byte physical source indexed key missing; actual miss branch unissued");
    const auto h=std::get_if<AssetHandle>(&entry->winner.asset);
    need(h&&h->candidate().provenance.kind==SourceKind::loose&&h->offset_==0&&
        !h->candidate().provenance.archive_relative_path,
        "Byte physical selected loose/offset0 required; archive/ambiguous branch unissued");
    const auto id=physical_id(*h);s->pure();
    const auto at=s->cache.find(id);need(at!=s->cache.end(),"Byte physical source must already be loaded; no lazy IO");
    const auto loaded=at->second;
    need(loaded.bytes&&loaded.payload_identity.bytes==h->candidate().payload_size&&
        loaded.bytes->size()==loaded.payload_identity.bytes,"Byte physical payload is not actual loaded winner");
    const auto held=s->pins.find(h->file_);need(held!=s->pins.end(),"Byte physical winning file has no held SourcePin");
    const auto*const source=held->second.get();
    const auto size=(static_cast<std::uint64_t>(source->stamp.nFileSizeHigh)<<32)|source->stamp.nFileSizeLow;
    need(size==loaded.payload_identity.bytes,"Byte physical full file extent differs from actual loaded payload");
    const auto path=source->path;s->pure(); //capture actual VALUE before allocating; no recipe reconstruction
    auto data=std::make_shared<Loan::Impl>(s,entry,h,source,id,path,loaded.bytes,loaded.payload_identity);
    (void)data->first();
    auto returned=std::shared_ptr<const Loan>(new Loan(data));(void)data->first();return returned;
}
std::shared_ptr<const detail::CivisLoadedBytesPhysicalPathLoan>
CivisResourceBankCapability::loaded_bytes_physical_path_owned(const AssetKey& key)const{
    require_current();const auto s=state_.lock();need(s&&s->epoch==epoch_,"Byte path capability Bank epoch changed");
    return CivisResourceBankOwner::capture_loaded_bytes_physical_path_owned(s,key);
}
