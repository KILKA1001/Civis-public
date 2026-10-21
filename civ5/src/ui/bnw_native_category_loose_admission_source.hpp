#pragma once
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_archive_basename_cache.hpp"
#include "bnw_native_package_category_source_owner.hpp"
#include "bnw_native_category_pattern_match.hpp"
#include "bnw_native_original_stat_service.hpp"
#include "bnw_native_category_directory_walk.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <clocale>
#include <functional>
#include "bnw_native_completed_cold_source_loan.hpp"
#include "civ5/ui/bnw_native_pinned_input_epoch.hpp"

namespace civ5::ui::detail {
class BnwNativeGlobalResourceWarmOwner;
class BnwNativeCategoryLooseAdmissionSource;
// A genuine successful stat/FindA/prefix operation retained by the actual Stage.
// No public constructor, scalar admission DTO, AssetHandle issuer or ready flag.
class BnwNativeCategoryLooseAdmissionReceipt final {
    friend class BnwNativeCategoryLooseAdmissionSource;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageCategorySourceOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    const std::thread::id completed_thread_=std::this_thread::get_id();
    std::weak_ptr<const BnwNativeCategoryLooseAdmissionSource> source_;
    std::shared_ptr<const BnwNativeOriginalStatRead> stat_;
    std::filesystem::path relative_;
    std::string basename_,query_;
    std::u16string prefix_;
    std::uint32_t prefix_index_=0,crc_=0;
    std::size_t operation_=0,event_=0;
    std::int32_t length_=0;
    std::int64_t mtime_=0;
    // Canonical transport is produced only AFTER real Original stat and byte
    // equality; it never supplies CRC, native length, prefix or replacement.
    assets::AssetHandle transport_;
    runtime::FileIdentity original_bytes_,canonical_bytes_;
    BnwNativeCategoryLooseAdmissionReceipt(
        std::weak_ptr<const BnwNativeCategoryLooseAdmissionSource> source,
        std::shared_ptr<const BnwNativeOriginalStatRead> stat,std::filesystem::path relative,
        std::string basename,std::string query,std::u16string prefix,
        std::uint32_t prefix_index,std::uint32_t crc,std::size_t operation,std::size_t event,
        std::int32_t length,std::int64_t mtime,assets::AssetHandle transport,
        runtime::FileIdentity original_bytes,runtime::FileIdentity canonical_bytes)
        :source_(std::move(source)),stat_(std::move(stat)),relative_(std::move(relative)),
         basename_(std::move(basename)),query_(std::move(query)),prefix_(std::move(prefix)),
         prefix_index_(prefix_index),crc_(crc),operation_(operation),event_(event),length_(length),
         mtime_(mtime),transport_(std::move(transport)),original_bytes_(std::move(original_bytes)),
         canonical_bytes_(std::move(canonical_bytes)){}
    void require_current()const;
    void require_current_completed(const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const;
  public:
    ~BnwNativeCategoryLooseAdmissionReceipt()=default;
    BnwNativeCategoryLooseAdmissionReceipt(const BnwNativeCategoryLooseAdmissionReceipt&)=delete;
};

// Private source dependency of the new owned-ParserDoc Stage. ONLY Warm/Parent
// can construct a subclass and bind it; its owner callback must authenticate
// SAME actual Loaded/Doc/54B8D0 output, active Stage and held Parent operation.
// The old physical DOM/raw Directory Stage cannot construct this supplier.
class BnwNativeCategoryLooseAdmissionSource {
    friend class BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeArchiveEntryAdmissionArena;
    friend class BnwNativeCategoryLooseAdmissionReceipt;
    using Receipt=BnwNativeCategoryLooseAdmissionReceipt;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeCategoryLooseAdmissionSource>;
    const std::thread::id constructor_thread_=std::this_thread::get_id();
    BnwNativeCallbackOwnerState state_;
    std::weak_ptr<const BnwNativeCategoryLooseAdmissionSource> self_;
    std::shared_ptr<const BnwNativeCategoryNamespaceChannel> channel_;
    std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena> arena_;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes_;
    std::shared_ptr<const BnwNativeOriginalStatService> stat_service_;
    std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache_;
    std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease> lease_;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input_;
    mutable const Guard* active_=nullptr;
    using Epoch=::civ5::ui::BnwNativePinnedInputEpoch;
    //Only synchronous completed borrowers create this stack. No stored Epoch,
    //Cold, Parent or phase flag can stand in for the actual opening loan.
    struct CompletedContext final {
        const BnwNativeCompletedColdSourceLoan& loan;
        const BnwNativeColdFilesystemOwner& cold;
        const Epoch& epoch;
        const Guard& guard;
        CompletedContext(const BnwNativeCompletedColdSourceLoan& l,const BnwNativeColdFilesystemOwner& c,
                         const Epoch& e,const Guard& g):loan(l),cold(c),epoch(e),guard(g){}
        CompletedContext(const CompletedContext&)=delete;
    };
    mutable const CompletedContext* active_completed_=nullptr;
    mutable bool failed_=false;
    struct Event {
        std::string query,name;
        DWORD attributes=0;
        std::uint64_t bytes=0,written=0;
        int kind=0; // 0 folder absent,1 excluded,2 folder,3 file,4 directory,5 FindA empty
        DWORD error=0;
        bool operator==(const Event&)const=default;
    };
    struct Operation {
        std::string request;
        std::vector<std::string> provider_rows;
        bool directory=false,xml_override=false,complete=false;
        UINT acp=0;
        std::vector<Event> events;
        std::vector<std::shared_ptr<const Receipt>> receipts;
        std::uint32_t returned=0;
    };
    mutable std::vector<std::unique_ptr<Operation>> operations_;
    BnwNativeCategoryLooseAdmissionSource(
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel> channel,
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
        std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes,
        std::shared_ptr<const BnwNativeOriginalStatService> stat,
        std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache,
        std::shared_ptr<const ::civ5::ui::BnwNativeEarlyInputLease> lease)
        :channel_(std::move(channel)),arena_(arena),prefixes_(std::move(prefixes)),
         stat_service_(std::move(stat)),cache_(std::move(cache)),lease_(std::move(lease)) {
        need(channel_&&arena&&prefixes_&&stat_service_&&cache_&&lease_,"Category loose source dependencies absent");
        // Actual cache owner is the proof of nonnull namespace cache storage;
        // an empty map alone never supplies that native operand.
        cache_->require_same_arena(*arena);
        prefixes_->require_same_input(*channel_->cold_owner());
    }
    BnwNativeCategoryLooseAdmissionSource(
        std::shared_ptr<const BnwNativeCategoryNamespaceChannel> channel,
        std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
        std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes,
        std::shared_ptr<const BnwNativeOriginalStatService> stat,
        std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache,
        std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> input)
        :channel_(std::move(channel)),arena_(arena),prefixes_(std::move(prefixes)),
         stat_service_(std::move(stat)),cache_(std::move(cache)),installation_input_(std::move(input)) {
        need(channel_&&arena&&prefixes_&&stat_service_&&cache_&&installation_input_&&!lease_&&
             channel_->installation_input_.get()==installation_input_.get(),"Installed category loose dependencies/input absent");
        cache_->require_same_arena(*arena);prefixes_->require_same_input(*channel_->cold_owner());
    }
    const std::filesystem::path& origin_game_root()const {
        return installation_input_?installation_input_->game_root():lease_->game_root();
    }
    const std::filesystem::path& origin_payload_root()const {
        return installation_input_?installation_input_->canonical_payload_root():lease_->canonical_payload_root();
    }
    // Called only after the issuer has retained this SAME source in its Stage.
    void bind_self(const std::shared_ptr<const BnwNativeCategoryLooseAdmissionSource>& source) {
        require_constructor_thread();
        need(source.get()==this&&self_.expired(),"Category loose self binding foreign/repeated");
        self_=source;
    }
    // Must also prove that the real Stage STILL retains this exact source;
    // this cannot be inferred from transient Receipt/Guard shared_ptr pins.
    virtual void require_source_current()const=0;
    // Only actual already-held Loaded/Stage/Parent fence identities, no
    // registry/stat/cache callbacks. Prefix registration holds its own fence.
    virtual void require_source_fence_without_callbacks()const=0;
    //Sealed actual issuer authenticates its terminal Stage/Namespace and the
    //exact private Parent opening before lending the SAME stack Epoch.
    virtual const Epoch& require_source_epoch_current_completed(
        const BnwNativeCompletedColdSourceLoan&,const BnwNativeColdFilesystemOwner&)const=0;
    static void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
    static void actual_c_locale() {
        const auto* value=std::setlocale(LC_CTYPE,nullptr);
        need(value&&std::string_view(value)=="C","Category loose CRC/filter requires actual retained C CRT branch");
    }
    static std::uint32_t loose_crc(std::string_view raw) {
        need(!raw.empty()&&raw.size()<=259&&raw.find_first_of("/\\:")==std::string_view::npos,
             "Category loose basename is outside FindA finite filename branch");
        actual_c_locale();native_category_patterns_detail::ascii_input(raw);
        std::uint32_t crc=0xffffffffu;
        for(auto byte:raw){auto c=static_cast<unsigned char>(byte);if(c>='A'&&c<='Z')c+=32;
            crc^=c;for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}
        actual_c_locale();return crc; // SAME83B4E0, no final complement.
    }
    std::filesystem::path relative_path(std::string_view original)const {
        native_category_patterns_detail::ascii_input(original);
        auto value=std::filesystem::path(native_loose_detail::acp_wide(std::string(original)));
        if(value.has_root_path()) {
            need(value.is_absolute(),"Category loose drive-relative/root-relative input needs actual CWD owner");
            // No canonical()/symlink rewrite, case fold or native prefix rewrite.
            // Reject '..' BEFORE lexical subtraction, then prove SAME root.
            for(const auto& part:value)need(part!=std::filesystem::path(".."),"Category loose absolute parent escape");
            auto root=origin_game_root().lexically_normal();
            if(root.has_relative_path()&&root.filename().empty())root=root.parent_path();
            const auto absolute=value.lexically_normal();auto at=absolute.begin();
            for(auto expected=root.begin();expected!=root.end();++expected,++at)
                need(at!=absolute.end()&&*at==*expected,"Category loose absolute source outside SAME Original root");
            need(at!=absolute.end(),"Category loose absolute root alone needs actual root/CWD source");
            value=absolute.lexically_relative(root);
            need(!value.empty()&&!value.has_root_path(),"Category loose absolute IO affinity subtraction failed");
        }
        (void)confined_original_stat_path(origin_game_root(),value);
        return value; // ONLY IO/stat/transport; receipt.query_/prefix_ are unchanged.
    }
    std::filesystem::path original_path(std::string_view original)const {
        return native_category_source_detail::source_path(origin_game_root(),relative_path(original));
    }
    static std::u16string ascii_prefix(std::string_view original) {
        native_category_patterns_detail::ascii_input(original);
        return std::u16string(original.begin(),original.end()); // ACP1251 ASCII exact branch.
    }
    void require_constructor_thread()const {
        // Reject outside all mutable-state catches: foreign calls must not
        // inspect active_/failed_/operations_ or poison another thread.
        need(constructor_thread_==std::this_thread::get_id(),"Category loose foreign constructor thread");
    }
    void check()const {
        require_constructor_thread();
        need(active_&&active_->valid()&&!failed_,"Category loose source lost/reentered/foreign operation");
        require_source_current();need(active_->valid()&&!failed_,"Category loose owner callback invalidated operation");
        channel_->require_current();need(active_->valid(),"Category loose namespace changed during owner callback");
        if(installation_input_)stat_service_->require_same_input(*installation_input_);
        else stat_service_->require_same_input(*lease_);need(active_->valid(),"Category loose stat/input affinity changed");
        actual_c_locale();
    }
    void check_without_callbacks()const {
        require_constructor_thread();
        need(active_&&active_->valid()&&!failed_,"Category loose source fence expired/reentered");
        require_source_fence_without_callbacks();
        need(active_->valid()&&!failed_,"Category loose actual owner fence changed");
    }
    template<class Consumer> decltype(auto) with_current(Consumer&& consume)const {
        require_constructor_thread(); // BEFORE active_ and BEFORE any failed_ catch.
        if(active_){try{check();return consume();}catch(...){failed_=true;throw;}}
        Guard guard(state_,self_);need(guard.valid(),"Category loose supplier lost persistent owner");
        struct Active {const BnwNativeCategoryLooseAdmissionSource& source;
            Active(const BnwNativeCategoryLooseAdmissionSource& s,const Guard& g):source(s){source.active_=&g;}
            ~Active(){source.active_=nullptr;}} active(*this,guard);
        try{check();return consume();}catch(...){failed_=true;throw;}
    }
    // Traversal and replay use the SAME source program. Prefix registration and
    // stat/store happen synchronously at the marked callbacks, never after a
    // captured directory vector or sorted records have replaced native order.
    template<class Folder,class File>
    std::vector<Event> observe(const Operation& op,Folder&& folder,File&& file)const {
        std::vector<Event> events;
        const auto emit=[&](Event event){events.push_back(std::move(event));return events.size()-1;};
        if(!op.directory) {
            //83AA80 direct lane;82A7A0 wildcard dispatch remains the caller's
            // actual FindA operation. A '?' without '*' is sent here directly.
            auto request=op.request;native_category_patterns_detail::ascii_input(request);
            std::replace(request.begin(),request.end(),'/','\\');
            need(request.find('*')==std::string::npos,"Category explicit File wildcard needs actual82A7A0 FindA dispatcher");
            const auto slash=request.rfind('\\');
            need(slash!=std::string::npos,"Category direct File bare/CWD prefix needs genuine CWD operand owner");
            const auto query=request.substr(0,slash+1),name=request.substr(slash+1);
            native_category_patterns_detail::ascii_input(name);
            check();const auto io=native_loose_detail::acp_path(original_path(request));
            const auto attributes=GetFileAttributesA(io.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();
                need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Category direct File attributes failure unknown");
                emit({request,{},0,0,0,0,error});return events;}
            need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Category direct File reparse source unsupported");
            const auto event=emit({query,name,attributes,0,0,6,0});
            const auto prefix=folder(query,event);check(); //83A570 precedes actual stat.
            file(query,name,prefix,event);check();return events;
        }
        const auto visit=[&](auto&& recurse,const std::string& query)->void {
            check();const auto io=native_loose_detail::acp_path(original_path(query));
            const auto attributes=GetFileAttributesA(io.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();
                need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Category loose attributes probe has unknown failure");
                emit({query,{},0,0,0,0,error});return;}
            need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Category loose query reparse target owner absent");
            if(op.directory&&native_category_patterns_detail::directory_excluded_ascii(query,op.provider_rows)){
                emit({query,{},attributes,0,0,1,0});check();return;}
            const auto directory_event=emit({query,{},attributes,0,0,2,0});
            const auto prefix=folder(query,directory_event);check(); //83A570 BEFORE FindFirstFileA.
            native_loose_detail::NativeEnumeration enumeration;
            auto cursor=enumeration.open(io+(io.ends_with('\\')?"":"\\")+"*.*");
            if(!cursor.valid()){const auto error=GetLastError();
                need(error==ERROR_FILE_NOT_FOUND,"Category loose FindA first failed outside empty-source branch");
                emit({query,{},0,0,0,5,error});check();return;}
            do {
                check();const auto& data=cursor.data;const std::string name=data.cFileName;
                const auto attr=data.dwFileAttributes;
                if((attr&6)!=0)continue;
                if((attr&FILE_ATTRIBUTE_DIRECTORY)&&(name=="."||name==".."))continue;
                native_category_patterns_detail::ascii_input(name);
                need(!(attr&FILE_ATTRIBUTE_REPARSE_POINT),"Category loose FindA reparse target owner absent");
                const auto bytes=(std::uint64_t(data.nFileSizeHigh)<<32)|data.nFileSizeLow;
                const auto written=(std::uint64_t(data.ftLastWriteTime.dwHighDateTime)<<32)|data.ftLastWriteTime.dwLowDateTime;
                const auto event=emit({query,name,attr,bytes,written,(attr&FILE_ATTRIBUTE_DIRECTORY)?4:3,0});
                if(attr&FILE_ATTRIBUTE_DIRECTORY)recurse(recurse,query+name+'\\');
                else file(query,name,prefix,event); // native stat → filter → SAME pool admission NOW.
                check();
            }while(cursor.next());
            need(cursor.error()==ERROR_NO_MORE_FILES,"Category loose FindNextA traversal incomplete");check();
        };
        visit(visit,native_category_directory_detail::normalized_query_ascii(op.request));
        return events;
    }
    std::shared_ptr<const Receipt> make_receipt(Operation& op,std::size_t operation,
        const std::string& query,const std::string& basename,std::uint32_t prefix,std::size_t event)const {
        check();const auto relative=relative_path(query+basename);
        auto stat=stat_service_->read_original_relative(relative);check();
        const auto& fields=stat->current();check();
        need(fields.return_value==0&&fields.size&&fields.mtime&&*fields.size>=0,
             "Category loose unchecked failed stat has no authentic signed32 length/mtime");
        const auto crc=loose_crc(basename);check(); // native83B4E0 after actual stat64i32.
        const auto source=original_path(query+basename);
        const auto canonical=native_category_source_detail::source_path(origin_payload_root(),relative);
        const auto source_id=runtime::identify_file(source),mapped_id=runtime::identify_file(canonical);check();
        need(source_id.bytes==std::uint64_t(*fields.size)&&mapped_id.bytes==source_id.bytes&&
             source_id.sha256==mapped_id.sha256,"Category loose transport bytes differ from actual Original stat/source");
        assets::LooseResourceIndex index(origin_payload_root());index.add_file(relative);
        auto hit=index.find(assets::AssetKey(basename));const auto* transport=std::get_if<assets::AssetHandle>(&hit);
        need(transport&&transport->candidate().provenance.kind==assets::SourceKind::loose&&
             transport->candidate().payload_size==std::uint64_t(*fields.size),"Category loose transport cannot read actual mapped bytes");
        auto receipt=std::shared_ptr<const Receipt>(new Receipt(self_,stat,relative,basename,query,ascii_prefix(query),
            prefix,crc,operation,event,*fields.size,*fields.mtime,*transport,source_id,mapped_id));
        op.receipts.push_back(receipt);check();return receipt;
    }
    std::uint32_t admit_directory_83a720(std::string actual_output,std::vector<std::string> actual_provider_rows,
                                        bool native_xml_override)const {
        require_constructor_thread();
        need(!active_,"Category loose mutation attempted inside existing source borrow");
        return with_current([&] {
            try {
                auto op=std::make_unique<Operation>();op->request=std::move(actual_output);
                op->provider_rows=std::move(actual_provider_rows);op->directory=true;op->xml_override=native_xml_override;op->acp=GetACP();
                const auto operation=operations_.size();operations_.push_back(std::move(op));auto& current=*operations_.back();
                const auto arena=arena_.lock();need(arena&&arena.use_count()>1,"Category loose SAME Stage arena lost");
                current.events=observe(current,
                    [&](const std::string& query,std::size_t){check();
                        prefixes_->require_same_input(*channel_->cold_owner());check();
                        return prefixes_->register_loose_source_prefix(ascii_prefix(query),[&]{check_without_callbacks();});},
                    [&](const std::string& query,const std::string& basename,std::uint32_t prefix,std::size_t event){
                        const auto receipt=make_receipt(current,operation,query,basename,prefix,event);
                        const auto filtered=current.xml_override&&native_category_patterns_detail::match_list_ascii(basename,"*.xml;*.ggxml");
                        const auto flags=filtered?2u:7u;
                        const auto result=arena->store_category_loose_record(receipt,flags);check();
                        need(result<=1,"Category loose native return outside0/1");current.returned+=result;});
                need(GetACP()==current.acp,"Category loose CP_ACP changed during traversal");current.complete=true;
                validate_operation(current);check();return current.returned;
            }catch(...){failed_=true;throw;}
        });
    }
    std::uint32_t admit_file_83aa80(std::string actual_output)const {
        require_constructor_thread();
        need(!active_,"Category direct File mutation during source borrow");
        return with_current([&]{
            try {
                auto op=std::make_unique<Operation>();op->request=std::move(actual_output);op->acp=GetACP();
                const auto operation=operations_.size();operations_.push_back(std::move(op));auto& current=*operations_.back();
                const auto arena=arena_.lock();need(arena&&arena.use_count()>1,"Category direct File actual Arena lost");
                current.events=observe(current,
                    [&](const std::string& query,std::size_t){check();
                        prefixes_->require_same_input(*channel_->cold_owner());check();
                        return prefixes_->register_loose_source_prefix(ascii_prefix(query),[&]{check_without_callbacks();});},
                    [&](const std::string& query,const std::string& name,std::uint32_t prefix,std::size_t event){
                        const auto receipt=make_receipt(current,operation,query,name,prefix,event);
                        current.returned=arena->store_category_loose_record(receipt,7);check();}); // No XML filter in83AA80.
                need(GetACP()==current.acp,"Category direct File CP_ACP changed");current.complete=true;
                validate_operation(current);check();return current.returned;
            }catch(...){failed_=true;throw;}
        });
    }
    void require_current()const {
        with_current([&]{
            prefixes_->require_same_input(*channel_->cold_owner());cache_->require_current();check();
            for(const auto& op:operations_){
                // The active currently executing operation is authenticated
                // only by its SAME source fence. It cannot issue completion.
                if(op->complete){validate_operation(*op);for(const auto& receipt:op->receipts)require_receipt_current(*receipt);}
            }
            check();
        });
    }
    void validate_operation(const Operation& op)const {
        need(op.complete&&GetACP()==op.acp,"Category loose operation incomplete/CP_ACP changed");
        const auto replay=observe(op,[](const std::string&,std::size_t){return 0u;},
            [](const std::string&,const std::string&,std::uint32_t,std::size_t){});
        need(replay==op.events,"Category loose actual FindA sequence/query changed after admission");
    }
    void require_receipt_current(const Receipt& receipt)const {
        with_current([&]{
            need(receipt.operation_<operations_.size(),"Category loose receipt lost actual operation");
            const auto& operation=*operations_[receipt.operation_];
            need(std::any_of(operation.receipts.begin(),operation.receipts.end(),[&](const auto& p){return p.get()==&receipt;}),
                 "Category loose selected receipt is copied/foreign");
            // During actual streaming admission the current walk has not yet
            // returned. Only the SAME held source operation may borrow it.
            if(operation.complete){validate_operation(operation);
                need(receipt.event_<operation.events.size()&&operation.events[receipt.event_].query==receipt.query_&&
                     operation.events[receipt.event_].name==receipt.basename_&&
                     (operation.events[receipt.event_].kind==3||operation.events[receipt.event_].kind==6),
                     "Category loose receipt lost actual FindA/direct-file event");}
            receipt.stat_->require_same_service(*stat_service_);const auto& fields=receipt.stat_->current();check();
            need(fields.return_value==0&&fields.size&&fields.mtime&&*fields.size==receipt.length_&&*fields.mtime==receipt.mtime_&&
                 receipt.stat_->relative_source()==receipt.relative_&&loose_crc(receipt.basename_)==receipt.crc_,
                 "Category loose SAME retained Original stat/node changed");
            need(prefixes_->read_registered_loose_prefix(receipt.prefix_index_)==receipt.prefix_,
                 "Category loose genuine registered original query prefix changed");
            const auto original=runtime::identify_file(original_path(receipt.query_+receipt.basename_));
            const auto mapped=runtime::identify_file(native_category_source_detail::source_path(origin_payload_root(),receipt.relative_));check();
            need(original.bytes==receipt.original_bytes_.bytes&&original.sha256==receipt.original_bytes_.sha256&&
                 mapped.bytes==receipt.canonical_bytes_.bytes&&mapped.sha256==receipt.canonical_bytes_.sha256,
                 "Category loose retained Original/canonical payload changed");
        });
    }
    void check_completed()const {
        require_constructor_thread();
        const auto* current=active_completed_;
        need(current&&!active_&&current->guard.valid()&&!failed_,
             "Completed loose source lacks genuine separate active stack guard");
        actual_c_locale();
        const auto& epoch=require_source_epoch_current_completed(current->loan,current->cold);
        need(active_completed_==current&& &epoch==&current->epoch&&current->guard.valid()&&!failed_,
             "Completed loose Stage/opening/Epoch changed during source check");
        channel_->require_current_completed(current->loan,current->cold);
        need(active_completed_==current&&current->guard.valid()&&!failed_,"Completed loose Namespace callback retired source");
        stat_service_->require_same_input_completed(*lease_,current->loan);
        need(active_completed_==current&&current->guard.valid()&&!failed_,"Completed loose stat input changed");
        const auto& after=require_source_epoch_current_completed(current->loan,current->cold);
        need(&after==&current->epoch&&active_completed_==current&&current->guard.valid()&&!failed_,
             "Completed loose source fence retired after supplier callback");
        actual_c_locale();
    }
    template<class Consumer> void with_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold,Consumer&& input)const {
        require_constructor_thread();
        need(lease_&&!installation_input_,"Mandatory actual installed completed loose source before completed borrowing");
        //Nested private receipt checks borrow ONLY this exact genuine stack.
        //They do not create a second Guard or silently select another domain.
        if(active_completed_){
            try{
                need(&active_completed_->loan==&loan&& &active_completed_->cold==&cold&&!active_,
                     "Completed loose nested borrower has another actual loan");
                check_completed();
                {std::function<void()> consume(std::forward<Consumer>(input));
                 check_completed();consume();check_completed();}
                check_completed();return;
            }catch(...){failed_=true;throw;}
        }
        need(!active_,"Completed loose borrower entered initial admission domain");
        Guard guard(state_,self_);need(guard.valid(),"Completed loose actual Stage source retired/reentered");
        try{
            const auto& epoch=require_source_epoch_current_completed(loan,cold);
            need(guard.valid()&&!failed_,"Completed loose source changed before context registration");
            const CompletedContext current(loan,cold,epoch,guard);
            struct ActiveCompleted final {
                const BnwNativeCategoryLooseAdmissionSource& source;
                const CompletedContext& context;
                ActiveCompleted(const BnwNativeCategoryLooseAdmissionSource& s,const CompletedContext& c):source(s),context(c){
                    need(!source.active_completed_,"Completed loose context registration repeated");source.active_completed_=&context;}
                ~ActiveCompleted(){if(source.active_completed_!=&context)source.failed_=true;source.active_completed_=nullptr;}
            } active(*this,current);
            check_completed();
            {std::function<void()> consume(std::forward<Consumer>(input));
             check_completed();consume();check_completed();}
            //Target destruction is INSIDE context and original Guard.
            check_completed();
        }catch(...){failed_=true;throw;}
    }
    std::filesystem::path relative_path_completed(std::string_view original)const {
        check_completed();
        native_category_patterns_detail::ascii_input(original);
        auto value=std::filesystem::path(native_loose_detail::acp_wide(std::string(original)));
        if(value.has_root_path()) {
            need(value.is_absolute(),"Category loose drive-relative/root-relative input needs actual CWD owner");
            // No canonical()/symlink rewrite, case fold or native prefix rewrite.
            // Reject '..' BEFORE lexical subtraction, then prove SAME root.
            for(const auto& part:value)need(part!=std::filesystem::path(".."),"Category loose absolute parent escape");
            auto root=active_completed_->epoch.game_root().lexically_normal();
            if(root.has_relative_path()&&root.filename().empty())root=root.parent_path();
            const auto absolute=value.lexically_normal();auto at=absolute.begin();
            for(auto expected=root.begin();expected!=root.end();++expected,++at)
                need(at!=absolute.end()&&*at==*expected,"Category loose absolute source outside SAME Original root");
            need(at!=absolute.end(),"Category loose absolute root alone needs actual root/CWD source");
            value=absolute.lexically_relative(root);
            need(!value.empty()&&!value.has_root_path(),"Category loose absolute IO affinity subtraction failed");
        }
        (void)confined_original_stat_path(active_completed_->epoch.game_root(),value);
        return value; // ONLY IO/stat/transport; receipt.query_/prefix_ are unchanged.
    }
    std::filesystem::path original_path_completed(std::string_view original)const {
        return native_category_source_detail::source_path(active_completed_->epoch.game_root(),relative_path_completed(original));
    }
    template<class Folder,class File>
    std::vector<Event> observe_completed(const Operation& op,Folder&& folder,File&& file)const {
        std::vector<Event> events;
        const auto emit=[&](Event event){events.push_back(std::move(event));return events.size()-1;};
        if(!op.directory) {
            //83AA80 direct lane;82A7A0 wildcard dispatch remains the caller's
            // actual FindA operation. A '?' without '*' is sent here directly.
            auto request=op.request;native_category_patterns_detail::ascii_input(request);
            std::replace(request.begin(),request.end(),'/','\\');
            need(request.find('*')==std::string::npos,"Category explicit File wildcard needs actual82A7A0 FindA dispatcher");
            const auto slash=request.rfind('\\');
            need(slash!=std::string::npos,"Category direct File bare/CWD prefix needs genuine CWD operand owner");
            const auto query=request.substr(0,slash+1),name=request.substr(slash+1);
            native_category_patterns_detail::ascii_input(name);
            check_completed();const auto io=native_loose_detail::acp_path(original_path_completed(request));
            const auto attributes=GetFileAttributesA(io.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();
                need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Category direct File attributes failure unknown");
                emit({request,{},0,0,0,0,error});return events;}
            need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Category direct File reparse source unsupported");
            const auto event=emit({query,name,attributes,0,0,6,0});
            const auto prefix=folder(query,event);check_completed(); //83A570 precedes actual stat.
            file(query,name,prefix,event);check_completed();return events;
        }
        const auto visit=[&](auto&& recurse,const std::string& query)->void {
            check_completed();const auto io=native_loose_detail::acp_path(original_path_completed(query));
            const auto attributes=GetFileAttributesA(io.c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();
                need(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND,"Category loose attributes probe has unknown failure");
                emit({query,{},0,0,0,0,error});return;}
            need(!(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"Category loose query reparse target owner absent");
            if(op.directory&&native_category_patterns_detail::directory_excluded_ascii(query,op.provider_rows)){
                emit({query,{},attributes,0,0,1,0});check_completed();return;}
            const auto directory_event=emit({query,{},attributes,0,0,2,0});
            const auto prefix=folder(query,directory_event);check_completed(); //83A570 BEFORE FindFirstFileA.
            native_loose_detail::NativeEnumeration enumeration;
            auto cursor=enumeration.open(io+(io.ends_with('\\')?"":"\\")+"*.*");
            if(!cursor.valid()){const auto error=GetLastError();
                need(error==ERROR_FILE_NOT_FOUND,"Category loose FindA first failed outside empty-source branch");
                emit({query,{},0,0,0,5,error});check_completed();return;}
            do {
                check_completed();const auto& data=cursor.data;const std::string name=data.cFileName;
                const auto attr=data.dwFileAttributes;
                if((attr&6)!=0)continue;
                if((attr&FILE_ATTRIBUTE_DIRECTORY)&&(name=="."||name==".."))continue;
                native_category_patterns_detail::ascii_input(name);
                need(!(attr&FILE_ATTRIBUTE_REPARSE_POINT),"Category loose FindA reparse target owner absent");
                const auto bytes=(std::uint64_t(data.nFileSizeHigh)<<32)|data.nFileSizeLow;
                const auto written=(std::uint64_t(data.ftLastWriteTime.dwHighDateTime)<<32)|data.ftLastWriteTime.dwLowDateTime;
                const auto event=emit({query,name,attr,bytes,written,(attr&FILE_ATTRIBUTE_DIRECTORY)?4:3,0});
                if(attr&FILE_ATTRIBUTE_DIRECTORY)recurse(recurse,query+name+'\\');
                else file(query,name,prefix,event); // native stat → filter → SAME pool admission NOW.
                check_completed();
            }while(cursor.next());
            need(cursor.error()==ERROR_NO_MORE_FILES,"Category loose FindNextA traversal incomplete");check_completed();
        };
        visit(visit,native_category_directory_detail::normalized_query_ascii(op.request));
        return events;
    }
    void validate_operation_completed(const Operation& op)const {
        check_completed();
        need(op.complete&&GetACP()==op.acp,"Completed loose operation incomplete/CP_ACP changed");
        const auto replay=observe_completed(op,[](const std::string&,std::size_t){return 0u;},
            [](const std::string&,const std::string&,std::uint32_t,std::size_t){});
        check_completed();
        need(replay==op.events,"Completed loose actual FindA sequence/query changed after admission");
    }
    void require_receipt_current_completed(const Receipt& receipt,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold)const {
        with_current_completed(loan,cold,[&]{
            need(receipt.operation_<operations_.size(),"Completed loose receipt lost actual operation");
            const auto& operation=*operations_[receipt.operation_];
            need(operation.complete&&std::any_of(operation.receipts.begin(),operation.receipts.end(),
                [&](const auto& p){return p.get()==&receipt;}),"Completed loose receipt is unreturned/copied/foreign");
            validate_operation_completed(operation);
            need(receipt.event_<operation.events.size()&&operation.events[receipt.event_].query==receipt.query_&&
                 operation.events[receipt.event_].name==receipt.basename_&&
                 (operation.events[receipt.event_].kind==3||operation.events[receipt.event_].kind==6),
                 "Completed loose receipt lost real FindA/direct-file event");
            receipt.stat_->require_same_service_completed(*stat_service_,loan);check_completed();
            const auto& fields=receipt.stat_->current_completed(loan);check_completed();
            const auto& relative=receipt.stat_->relative_source_completed(loan);check_completed();
            need(fields.return_value==0&&fields.size&&fields.mtime&&*fields.size==receipt.length_&&*fields.mtime==receipt.mtime_&&
                 relative==receipt.relative_&&loose_crc(receipt.basename_)==receipt.crc_,
                 "Completed loose SAME retained Original stat/node changed");
            check_completed();
            const auto prefix=prefixes_->read_registered_loose_prefix_completed(receipt.prefix_index_,loan,cold);
            check_completed();need(prefix==receipt.prefix_,"Completed loose genuine original query prefix changed");
            const auto original=runtime::identify_file(original_path_completed(receipt.query_+receipt.basename_));check_completed();
            const auto mapped_path=native_category_source_detail::source_path(active_completed_->epoch.canonical_payload_root(),receipt.relative_);
            check_completed();const auto mapped=runtime::identify_file(mapped_path);check_completed();
            need(original.bytes==receipt.original_bytes_.bytes&&original.sha256==receipt.original_bytes_.sha256&&
                 mapped.bytes==receipt.canonical_bytes_.bytes&&mapped.sha256==receipt.canonical_bytes_.sha256,
                 "Completed loose retained Original/canonical payload changed");
        });
    }
    void require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold)const {
        with_current_completed(loan,cold,[&]{
            prefixes_->require_same_input_completed(cold,loan);check_completed();
            const auto arena=arena_.lock();need(arena&&arena.use_count()>1,"Completed loose SAME Stage Arena retired");
            cache_->require_current_completed(loan,cold,*arena);check_completed();
            for(const auto& op:operations_){
                need(op&&op->complete,"Completed loose universe contains unreturned admission");
                validate_operation_completed(*op);
                for(const auto& receipt:op->receipts){need(bool(receipt),"Completed loose operation contains missing receipt");
                    require_receipt_current_completed(*receipt,loan,cold);check_completed();}
            }
            check_completed();
        });
    }
  public:
    virtual ~BnwNativeCategoryLooseAdmissionSource()=default;
    BnwNativeCategoryLooseAdmissionSource(const BnwNativeCategoryLooseAdmissionSource&)=delete;
};
inline void BnwNativeCategoryLooseAdmissionReceipt::require_current_completed(
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
    if(completed_thread_!=std::this_thread::get_id())throw std::runtime_error("Completed loose receipt foreign creator thread");
    const auto source=source_.lock();
    if(!source||source.use_count()==1)throw std::runtime_error("Completed loose actual Stage source expired");
    source->require_receipt_current_completed(*this,loan,cold);
}
inline void BnwNativeCategoryLooseAdmissionReceipt::require_current()const {
    const auto source=source_.lock();
    if(!source||source.use_count()==1)throw std::runtime_error("Category loose actual Stage source expired");
    source->require_receipt_current(*this);
}
}
