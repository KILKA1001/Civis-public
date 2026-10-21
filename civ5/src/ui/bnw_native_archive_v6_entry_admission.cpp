#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_archive_v6_entry_admission.hpp"
#include "bnw_native_installed_late_default_operation.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include "bnw_native_category_loose_admission_source.hpp"
#include "bnw_native_common13_source.hpp"
#include "bnw_native_package_category_source_owner.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "bnw_native_default_namespace_policy.hpp"
#include "bnw_native_current_source_control.hpp"
#include "bnw_native_primary32_record_snapshot.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include <clocale>
#include <unordered_map>
namespace civ5::ui::detail {
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
inline void category_v6_entry_inputs(const std::vector<assets::FpkEntry>& entries,
                                      std::uint32_t declared,std::uint64_t directory_end,
                                      std::uint64_t bytes) {
    if(declared>1000000 || entries.size()!=declared)throw std::runtime_error("Native finite directory declared/read count differs");
    std::uint64_t cursor=14;
    for(std::size_t n=0;n<entries.size();++n) {
        const auto& e=entries[n];(void)bnw_native_admission_ascii_crc(e.original_name);
        const auto length=e.original_name.size();const auto padding=static_cast<std::uint32_t>((4-(length&3))&3);
        if(e.ordinal!=n ||
           e.directory_record_offset!=cursor || e.padding_length!=padding ||
           e.metadata_offset!=cursor+4+length+padding || e.metadata_offset>bytes ||
           bytes-e.metadata_offset<16 || e.payload_offset<=12 || e.payload_offset>UINT32_MAX ||
           e.payload_length>UINT32_MAX || e.payload_offset>=bytes || e.payload_length>=bytes ||
           e.payload_length>bytes-e.payload_offset)
            throw std::runtime_error("FPK parser compatibility layout differs from finite native entry read branch");
        cursor=e.metadata_offset+16;
    }
    if(cursor!=directory_end || cursor>bytes)throw std::runtime_error("Native directory cursor differs from parser snapshot");
}
void ascii_locale(){const auto* value=std::setlocale(LC_CTYPE,nullptr);need(value&&std::string_view(value)=="C","Native admission _strlwr requires actual C CRT locale");}
}
struct BnwNativeArchiveEntryAdmissionArena::Impl {
    const std::thread::id constructor_thread=std::this_thread::get_id();
    void require_thread_before_snapshot()const {
        need(constructor_thread==std::this_thread::get_id(),"Category arena foreign snapshot thread");
    }
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveEntryAdmissionArena>;
    Guard guard()const noexcept{return Guard(state,self);}
    std::weak_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    std::shared_ptr<const BnwNativeDefaultNamespacePolicyOwner> initial_policy;
    bool initial_policy_attempted=false;
    mutable BnwNativeArchiveEntryKernelDiagnostic kernel;
    mutable std::map<std::uint32_t,assets::AssetHandle> canonical_selected;
    // Captured AT the positive source admission store, not reconstructed later
    // from equal-sized handle metadata. Source vector nodes remain immutable.
    struct SelectedInput {
        std::shared_ptr<const assets::FpkSource> source;
        const assets::FpkEntry* entry=nullptr;
        const BnwNativeArchiveV6StreamHeaderOwner* header=nullptr;
        const BnwNativeGlobalArchivePrefixRecord* prefix=nullptr;
        std::size_t archive_ordinal=0,entry_ordinal=0;
        std::uint32_t prefix_index=0;
    };
    mutable std::map<std::uint32_t,SelectedInput> selected_inputs;
    // Weak reverse edge: actual owned Stage retains the receipt/source.
    mutable std::map<std::uint32_t,std::weak_ptr<const BnwNativeCategoryLooseAdmissionReceipt>> selected_loose;
    mutable bool current_default_source_claimed=false;
    mutable std::weak_ptr<const BnwNativeCurrentDefaultSource> current_default_source;
    mutable bool default_projection_capture_attempted=false;
    mutable bool snapshot_capture_attempted=false;
    mutable std::size_t next_primary_ordinal=0;
    mutable std::size_t next_common_ordinal=0;
    mutable bool failed=false;
    mutable bool cache_claim_attempted=false;
    std::shared_ptr<const BnwNativeCategoryNamespaceChannel> category_namespace;
    mutable std::optional<std::size_t> last_category_ordinal;
    mutable bool category_admission_active=false;
    mutable std::optional<std::size_t> last_late_default_ordinal;
    mutable const BnwNativeInstalledLateDefaultArchiveAttempt* active_late_default_attempt=nullptr;
    mutable const Guard* active_late_default_fence=nullptr;
    mutable std::uint64_t category_node_epoch=0;
    void check(const Guard& fence,long pins)const {
        need(fence.valid()&&bnw_prefix_persistent_facade_diagnostic(self.use_count(),pins),
             "Native admission arena lost/reentered/foreign thread");
        need(!failed,"Native admission arena has terminal incomplete effects");
    }
    void require_category_selected_witness_without_callbacks(std::uint32_t key)const {
        const auto r=kernel.records().find(key);need(r!=kernel.records().end(),"Category selected row lacks SAME native pool node");
        if(r->second.offset==0){const auto w=selected_loose.find(key);
            need(w!=selected_loose.end()&&!w->second.expired(),"Category archive follows expired/unissued loose witness");}
        else need(selected_inputs.contains(key),"Category archive selected source witness absent");
    }
    void require_category_selected_witnesses_without_callbacks(const NativeResourceRecordIndex& records)const {
        for(const auto& [key,incoming]:records.records)require_category_selected_witness_without_callbacks(key);
    }
    std::shared_ptr<const BnwNativeColdFilesystemOwner> validate(const Guard& fence,long pins)const {
        check(fence,pins);
        if(category_namespace) {
            category_namespace->require_current();check(fence,pins);
            const auto actual=category_namespace->cold_owner();check(fence,pins);
            auto owner=cold.lock();need(owner && owner.get()==actual.get(),"Category admission has another Cold");
            if(installation_input){need(!lease,"Installed category cannot import saved lease");
                owner->require_same_input(*installation_input);check(fence,pins);
            }else {owner->require_same_input(*lease);check(fence,pins);}
            const auto& records=category_namespace->record_index();check(fence,pins);
            need(records.records.size()==records.record_names.size() &&
                 records.records.size()==kernel.records().size() &&
                 canonical_selected.size()==kernel.records().size() &&
                 selected_inputs.size()+selected_loose.size()==kernel.records().size() &&
                 records.bucket_order.live_nodes()==records.records.size() &&
                 kernel.bucket_order().live_nodes()==records.records.size(),"Category SAME selected native namespace/order/witness differs");
            const auto projection_order=records.bucket_order.ordered_nodes();
            const auto native_order=kernel.bucket_order().ordered_nodes();
            for(std::size_t n=0;n<native_order.size();++n)
                need(projection_order[n].key==native_order[n].key,"Category selected/native bucket traversal order differs");
            need(fence.valid_parent(owner),"Category admission persistent Cold lost");ascii_locale();return owner;
        }
        need(initial_policy_attempted&&initial_policy,"Native admission initial namespace policy absent");
        // Policy takes ONE Cold weak-lock pin. Finish that scope BEFORE this
        // Arena's own pin; two internal pins must not hide persistent loss.
        initial_policy->require_current();check(fence,pins);
        auto owner=cold.lock();need(bool(owner),"Native admission cold epoch expired");
        if(installation_input){need(!lease,"Installed resource source cannot carry imported save lease");
            owner->require_same_input(*installation_input);check(fence,pins);
        }else {owner->require_same_input(*lease);check(fence,pins);}
        need(fence.valid_parent(owner),"Native admission cold persistent owner lost");
        owner->require_fresh_direct_original_branch();check(fence,pins);
        need(fence.valid_parent(owner),"Native admission cold persistent owner lost");ascii_locale();
        need(kernel.records().size()==canonical_selected.size() &&
             kernel.records().size()==selected_inputs.size() &&
             kernel.bucket_order().live_nodes()==kernel.records().size(),"Native admitted namespace/payload state differs");
        return owner;
    }
    std::shared_ptr<const BnwNativeColdFilesystemOwner> validate_common(const Guard& fence,long pins,BnwNativeCommonSourceValidationContext& context)const {
        check(fence,pins);need(lease&&!installation_input,"Mandatory installed common Arena source before late use");
        need(!category_namespace,"Common scope requires actual initial admission arena");
        need(initial_policy_attempted&&initial_policy,"Native admission initial namespace policy absent");
        // Policy takes ONE Cold weak-lock pin. Finish that scope BEFORE this
        // Arena's own pin; two internal pins must not hide persistent loss.
        initial_policy->require_common_source_current(context);check(fence,pins);
        auto owner=cold.lock();need(bool(owner),"Native admission cold epoch expired");
        owner->require_same_input_common_source(*lease,context);check(fence,pins);
        need(fence.valid_parent(owner),"Native admission cold persistent owner lost");
        owner->require_fresh_direct_original_branch_common_source(context);check(fence,pins);
        need(fence.valid_parent(owner),"Native admission cold persistent owner lost");ascii_locale();
        need(kernel.records().size()==canonical_selected.size() &&
             kernel.records().size()==selected_inputs.size() &&
             kernel.bucket_order().live_nodes()==kernel.records().size(),"Native admitted namespace/payload state differs");
        return owner;
    }
    void validate_completed(const Guard& fence,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& actual_cold)const {
        require_thread_before_snapshot();check(fence,1);need(lease&&!installation_input,"Mandatory installed completed Arena source before late use");
        actual_cold.require_same_completed_owner_control(cold,loan);check(fence,1);loan.require_same_early_input(*lease);check(fence,1);
        if(category_namespace) {
            // Concrete Category producer authenticates SAME terminal Stage/
            // namespace/Cold/source epoch. No initial live Skin replay here.
            category_namespace->require_current_completed(loan,actual_cold);check(fence,1);
            const auto& records=category_namespace->record_index_completed(loan,actual_cold);check(fence,1);
            need(records.records.size()==records.record_names.size()&&records.records.size()==kernel.records().size()&&
                 canonical_selected.size()==kernel.records().size()&&selected_inputs.size()+selected_loose.size()==kernel.records().size()&&
                 records.bucket_order.live_nodes()==records.records.size()&&kernel.bucket_order().live_nodes()==records.records.size(),
                 "Completed category actual selected namespace/order/witness differs");
            const auto projection_order=records.bucket_order.ordered_nodes();const auto native_order=kernel.bucket_order().ordered_nodes();
            for(std::size_t n=0;n<native_order.size();++n)need(projection_order[n].key==native_order[n].key,
                "Completed category actual native bucket order differs");
            category_namespace->require_current_completed(loan,actual_cold);check(fence,1);
        }else {
            need(initial_policy_attempted&&initial_policy,"Completed admission initial policy absent");
            initial_policy->require_current_completed(loan,actual_cold);check(fence,1);
            need(kernel.records().size()==canonical_selected.size()&&kernel.records().size()==selected_inputs.size()&&
                 kernel.bucket_order().live_nodes()==kernel.records().size(),"Completed admitted namespace/payload state differs");
        }
        ascii_locale();actual_cold.require_same_completed_owner_control(cold,loan);check(fence,1);
        loan.require_same_early_input(*lease);check(fence,1);
    }
};
struct BnwNativeArchiveV6EntryAdmissionOwner::Impl {
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> self;
    using Guard=BnwNativeCallbackOwnerGuard<BnwNativeArchiveV6EntryAdmissionOwner>;
    Guard guard()const noexcept{return Guard(state,self);}
    std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena> arena;
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix;
    std::shared_ptr<const assets::FpkSource> source;
    std::uint32_t declared=0,admitted=0;
    std::size_t ordinal=0;
    std::weak_ptr<const BnwNativeCategoryArchiveChannel> category_channel;
    std::weak_ptr<const BnwNativeInstalledLateDefaultArchiveAttempt> installed_late_attempt;
    std::vector<assets::AssetHandle> category_handles;
    std::vector<BnwNativeArchiveV6EntryAdmissionOwner::CategoryAdmittedRecord> category_nodes;
    std::vector<std::uint32_t> entry_returns;
    mutable bool default_cache_stage_failed=false;
    mutable std::uint64_t default_cache_stage_epoch=0;
    mutable const DefaultCachePublicationStagingLoan* active_default_cache_stage=nullptr;
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> validate(const Guard& fence)const {
        const auto check=[&]{need(fence.valid()&&!default_cache_stage_failed,"Native admission receipt lost/reentered/foreign thread");};
        check();auto owner=arena.lock();need(bool(owner),"Native admission arena expired");
        owner->require_current();check();need(fence.valid_parent(owner),"Native admission persistent arena lost");
        header->require_current();check();prefix->require_current();check();
        // SAME Prefix just completed FULL validation; its Header link is immutable.
        prefix->require_header_identity_without_callbacks(*header);check();
        need(admitted<=declared && declared==entry_returns.size(),"Native admission computed count differs from actual loop");
        std::uint32_t count=0;for(const auto result:entry_returns){need(result<=1,"Native admission result unsupported");count+=result;}
        need(count==admitted,"Native admission accumulated83B2A0 returns differ");
        need(fence.valid_parent(owner),"Native admission arena lost after Header callbacks");return owner;
    }
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> validate_common(const Guard& fence,BnwNativeCommonSourceValidationContext& context,
        const BnwNativeCommonSourceVisitLedger::Visit& actual_visit)const {
        const auto check=[&]{need(fence.valid()&&!default_cache_stage_failed,"Native admission receipt lost/reentered/foreign thread");};
        check();auto owner=arena.lock();need(bool(owner),"Native admission arena expired");
        owner->require_common_source_current(context);check();need(fence.valid_parent(owner),"Native admission persistent arena lost");
        header->require_common_source_current(context);check();prefix->require_common_source_current(context);check();
        const auto& actual=prefix->header_owner_common_source(context);check();need(&actual==header.get(),"Native admission prefix has another Header");
        need(admitted<=declared && declared==entry_returns.size(),"Native admission computed count differs from actual loop");
        // The actual Parent-minted visit authenticates this same immutable
        // receipt once in each PRE/POST phase. Every source borrower above
        // and every fence/parent/count check still runs on identity visits.
        if(actual_visit.work()==BnwNativeCommonSourceVisitLedger::Work::validate_source){
            std::uint32_t count=0;for(const auto result:entry_returns){need(result<=1,"Native admission result unsupported");count+=result;}
            need(count==admitted,"Native admission accumulated83B2A0 returns differ");
        }
        need(fence.valid_parent(owner),"Native admission arena lost after Header callbacks");return owner;
    }
    void validate_completed(const Guard& fence,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& actual_arena,
        const BnwNativeGlobalArchivePrefixOwner& actual_registry)const {
        const auto check=[&]{need(fence.valid()&&!default_cache_stage_failed,"Completed admission receipt lost/reentered/foreign thread");};
        check();actual_arena.require_same_completed_owner_control(arena,loan,cold);check();
        need(header&&prefix&&source,"Completed admission real Header/Prefix/source absent");
        header->require_current_completed(loan,cold);check();prefix->require_current_completed(loan,cold,actual_registry);check();
        const auto& actual=prefix->header_owner_completed(loan,cold,actual_registry);check();
        need(&actual==header.get(),"Completed admission Prefix has another actual Header");
        need(admitted<=declared&&declared==entry_returns.size(),"Completed admitted count differs from true loop");
        std::uint32_t count=0;for(const auto result:entry_returns){need(result<=1,"Completed native admission result unsupported");count+=result;}
        need(count==admitted,"Completed accumulated83B2A0 returns differ");
        actual_arena.require_same_completed_owner_control(arena,loan,cold);check();
    }
};
#include "bnw_native_installed_late_default_entry_admission.inc"
BnwNativeArchiveEntryAdmissionArena::BnwNativeArchiveEntryAdmissionArena(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeArchiveEntryAdmissionArena::~BnwNativeArchiveEntryAdmissionArena()=default;
std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> BnwNativeArchiveEntryAdmissionArena::acquire(
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& source) {
    need(bool(cold),"Native admission arena requires genuine Cold");
    auto impl=std::make_unique<Impl>();
    {
        BnwNativeCallbackConstructionGuard fence(impl->state);
        const auto check=[&] {
            need(fence.valid()&&cold.use_count()>1,"Native initial arena lost persistent Cold");
            source.require_archive_namespace_construction_current(*cold);
        };
        check();cold->require_current();check();
        cold->claim_initial_entry_admission_arena();check();
        // Bind the actual parent lineage, never recreate FIRST per namespace.
        impl->kernel.bind_shared_first(source.shared_first_for_arena(*cold));check();
        impl->cold=cold;impl->installation_input=cold->installation_input_for_source();check();
        if(!impl->installation_input){impl->lease=cold->early_input_lease();}check();ascii_locale();
        cold->require_before_any_archive_request_and_header_without_callbacks();check();
        // Cold's Arena claim is already consumed. A failed constructor cannot
        // publish/recreate a late empty policy or make this epoch replayable.
        impl->initial_policy_attempted=true;
        auto policy=BnwNativeDefaultNamespacePolicyOwner::acquire(cold);check();
        cold->require_before_any_archive_request_and_header_without_callbacks();check();
        impl->initial_policy=std::move(policy); // No callbacks after final checks.
    } // Unpublished guard ends before throwing shared control-block allocation.
    auto out=std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>(new BnwNativeArchiveEntryAdmissionArena(std::move(impl)));
    out->impl_->self=out;return out;
}
std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> BnwNativeArchiveEntryAdmissionArena::acquire_category(
    std::shared_ptr<const BnwNativeCategoryNamespaceChannel> channel) {
    need(bool(channel),"Category arena requires genuine namespace channel");
    channel->claim_initial_arena(); // one-way genuine namespace claim, before allocations/callbacks
    channel->require_current();
    auto impl=std::make_unique<Impl>();impl->category_namespace=channel;
    const auto cold=channel->cold_owner();need(bool(cold),"Category arena Cold unavailable");
    impl->cold=cold;
    if(channel->installation_input_){
        impl->installation_input=cold->installation_input_for_source();
        need(impl->installation_input&&impl->installation_input.get()==channel->installation_input_.get()&&
             !impl->installation_input.owner_before(channel->installation_input_)&&!channel->installation_input_.owner_before(impl->installation_input),
             "Installed category Arena has another actual Cold/input source");
    }else impl->lease=cold->early_input_lease();
    const auto& records=channel->record_index();
    need(records.records.empty() && records.record_names.empty() && records.bucket_order.live_nodes()==0,
         "Category arena constructor must precede every namespace admission");
    channel->shared_first()->require_current();
    impl->kernel.bind_shared_first(channel->shared_first());
    {BnwNativeCallbackConstructionGuard fence(impl->state);
     need(fence.valid() && cold.use_count()>1,"Category arena lost persistent Cold during construction");
     channel->require_current();need(fence.valid(),"Category arena construction reentered");}
    auto out=std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena>(new BnwNativeArchiveEntryAdmissionArena(std::move(impl)));
    out->impl_->self=out;return out;
}
std::uint32_t BnwNativeArchiveEntryAdmissionArena::store_category_raw_record_without_fence(
    std::string_view raw,const assets::AssetHandle& handle,
    std::shared_ptr<const assets::FpkSource> source,const assets::FpkEntry& entry,
    const BnwNativeArchiveV6StreamHeaderOwner& header,const BnwNativeGlobalArchivePrefixRecord& prefix,
    std::uint32_t prefix_index,std::size_t archive_ordinal)const {
    need(impl_->category_namespace && impl_->category_admission_active,
         "Category raw store outside owned active entry loop");
    need(handle.candidate().provenance.kind==assets::SourceKind::fpk && handle.candidate().original_name==raw,
         "Category raw store payload differs from actual directory entry");
    auto& records=impl_->category_namespace->record_index();
    const auto first=impl_->category_namespace->shared_first();
    const auto crc=bnw_native_admission_ascii_crc(raw);const auto old=records.records.find(crc);
    need(old!=records.records.end() || records.records.size()<NativeResourceBucketOrder::source_initial_pool,
         "Category finite initial record pool exhausted; native growth not owned");
    //The native words are produced at this SAME real positive mode2 entry
    //store, before its selected projection. Future lookup never synthesizes
    //them from a copied AssetHandle or a declared count.
    need(source&&entry.ordinal<source->entries().size()&&&source->entries()[entry.ordinal]==&entry&&
         entry.original_name==raw&&entry.payload_length==handle.candidate().payload_size&&
         prefix_index<4096&&prefix.source_index()==prefix_index,
         "Category native mode2 store lost actual source entry/prefix");
    need(records.records.size()==impl_->kernel.records().size() &&
         impl_->selected_inputs.size()+impl_->selected_loose.size()==impl_->kernel.records().size(),
         "Category archive follows unmatched native pool/witness allocation");
    // SAME Warm namespace writers are fenced by this actual category entry
    // loop. With no weak loose witnesses, only this entry's CRC can change;
    // validate every row at the actual first/last stores, not at every store.
    // Legacy and loose-populated namespaces retain the original full pass.
    if(impl_->category_namespace->owned_warm_ && impl_->selected_loose.empty() && entry.ordinal!=0) {
        if(old!=records.records.end())impl_->require_category_selected_witness_without_callbacks(crc);
        else need(!impl_->kernel.records().contains(crc)&&!impl_->selected_inputs.contains(crc)&&
                  !impl_->canonical_selected.contains(crc),"Category new RAW CRC already has native/source/transport allocation");
    }else impl_->require_category_selected_witnesses_without_callbacks(records);
    need(impl_->kernel.admit(raw,prefix_index,entry.payload_length,entry.payload_offset,2)==1,
         "Category native mode2 record store did not return positive");
    impl_->selected_loose.erase(crc);
    impl_->canonical_selected.insert_or_assign(crc,handle);
    impl_->selected_inputs.insert_or_assign(crc,Impl::SelectedInput{
        source,&entry,&header,&prefix,archive_ordinal,entry.ordinal,prefix_index});
    if(old!=records.records.end())records.bucket_order.erase_first(crc);
    need(records.bucket_order.insert(crc,crc)==1,"Category RAW mode2 retained duplicate selected CRC");
    first->remember(crc,raw); // SAME global RAW FIRST, not normalized index insertion
    records.first_record_names.try_emplace(crc,first->names().at(crc));
    records.records.insert_or_assign(crc,handle);records.record_names.insert_or_assign(crc,std::string(raw));
    ++impl_->category_node_epoch;
    need(records.bucket_order.live_nodes()==records.records.size(),"Category RAW selected order/store differs");
    if(impl_->category_namespace->owned_warm_ && impl_->selected_loose.empty()) {
        const auto native=impl_->kernel.records().find(crc);
        const auto selected=impl_->selected_inputs.find(crc);
        const auto canonical=impl_->canonical_selected.find(crc);
        const auto projected=records.records.find(crc);
        const auto* projected_handle=projected==records.records.end()?nullptr:std::get_if<assets::AssetHandle>(&projected->second);
        const auto name=records.record_names.find(crc);
        need(native!=impl_->kernel.records().end()&&selected!=impl_->selected_inputs.end()&&
             canonical!=impl_->canonical_selected.end()&&projected_handle&&name!=records.record_names.end()&&
             native->second.crc==crc&&native->second.length==entry.payload_length&&native->second.offset==entry.payload_offset&&
             native->second.packed_prefix==(prefix_index<<20)&&selected->second.source.get()==source.get()&&
             selected->second.entry==&entry&&selected->second.header==&header&&selected->second.prefix==&prefix&&
             selected->second.archive_ordinal==archive_ordinal&&selected->second.entry_ordinal==entry.ordinal&&
             selected->second.prefix_index==prefix_index&&canonical->second.candidate().original_name==raw&&
             canonical->second.candidate().payload_size==entry.payload_length&&projected_handle->candidate().original_name==raw&&
             projected_handle->candidate().payload_size==entry.payload_length&&name->second==raw&&
             records.records.size()==impl_->kernel.records().size()&&impl_->canonical_selected.size()==impl_->kernel.records().size()&&
             impl_->selected_inputs.size()==impl_->kernel.records().size(),"Category RAW selected/native/source store differs");
        if(entry.ordinal+1==source->entries().size())impl_->require_category_selected_witnesses_without_callbacks(records);
    }
    return crc;
}
// This entry point is callable only by the genuine private Source. All stat,
// cache/header/prefix callbacks precede the Arena fence. The held actual Stage
// operation prevents another admission; epoch/row identities are rechecked
// inside the fence before the native replacement/FIRST/pool effects.
std::uint32_t BnwNativeArchiveEntryAdmissionArena::store_category_loose_record(
    std::shared_ptr<const BnwNativeCategoryLooseAdmissionReceipt> receipt,unsigned flags)const {
    auto& i=*impl_;i.require_thread_before_snapshot(); // BEFORE callbacks/mutable snapshots.
    need(receipt&&(flags==7||flags==2),"Category loose issuer/actual caller mode absent");
    receipt->require_current();const auto source=receipt->source_.lock();
    need(source&&source->arena_.lock().get()==this&&impl_->category_namespace.get()==source->channel_.get(),
         "Category loose receipt belongs to another actual Stage/Arena");
    need(i.category_namespace&&!i.category_admission_active,"Category loose admission during active archive entry loop");
    const auto before_epoch=i.category_node_epoch;
    const auto before=i.kernel.records().find(receipt->crc_);
    const std::optional<BnwNativeArchiveRecordDiagnostic> old=
        before==i.kernel.records().end()?std::nullopt:std::optional{before->second};
    bool replace=true;
    std::string cache_key;
    std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> cached;
    std::optional<std::int64_t> cached_mtime;
    if(old&&flags==7&&old->offset!=0) {
        //83B130 flags1 only overrides an old loose node. flags7 &14 ==6
        //requires actual archive cache inspection using that node's prefix.
        const auto input=i.selected_inputs.find(receipt->crc_);
        need(input!=i.selected_inputs.end()&&input->second.prefix&&input->second.header&&input->second.source&&input->second.entry,
             "Category mode7 old archive lacks genuine selected native source");
        input->second.prefix->require_same_owner(*source->prefixes_);
        const auto& raw=input->second.prefix->raw_prefix();
        std::string bytes;bytes.reserve(raw.size());
        for(auto c:raw){need(c&&c<128,"Category mode7 native cached prefix ACP branch unsupported");bytes.push_back(char(c));}
        cache_key=archive_cache_basename_ascii(bytes);
        const auto found=i.category_namespace->cache_storage().lookup(cache_key);
        if(found){cached=*found;need(bool(cached),"Category archive cache has null selected publication");
            cached_mtime=cached->checked_original_mtime(); // BEFORE ArenaGuard; its validation enters SAME Arena.
            replace=receipt->mtime_>=*cached_mtime; // signed high DWORD then unsigned low DWORD.
        } // actual inspected cache MISS allows replacement, not an invented zero mtime.
    } else if(old&&old->offset==0) {
        const auto selected=i.selected_loose.find(receipt->crc_);
        need(selected!=i.selected_loose.end(),"Category old loose has no native Source receipt");
        const auto retained=selected->second.lock();need(bool(retained),"Category old loose Source receipt expired");
        retained->require_current(); // BEFORE ArenaGuard, no recursive native record borrow.
    }
    receipt->require_current();
    std::uint32_t result=0;
    {
        auto fence=i.guard();i.validate(fence,1);
        try {
            need(before_epoch==i.category_node_epoch,"Category native namespace mutated during source callbacks");
            const auto selected=i.kernel.records().find(receipt->crc_);
            need((!old&&selected==i.kernel.records().end())||
                 (old&&selected!=i.kernel.records().end()&&selected->second==*old),
                 "Category mode7 old native node changed before replacement");
            auto& records=i.category_namespace->record_index();
            need(records.records.size()==i.kernel.records().size()&&
                 i.selected_inputs.size()+i.selected_loose.size()==i.kernel.records().size()&&
                 bool(old)==records.records.contains(receipt->crc_),
                 "Category selected projection/native pool allocation differs");
            if(!cache_key.empty()){
                const auto current=i.category_namespace->cache_storage().lookup(cache_key);
                need((!cached&&!current)||(cached&&current&&current->get()==cached.get()),
                     "Category native old-prefix cache lookup changed before replacement");
            }
            if(replace) {
                need(i.kernel.admit_loose(receipt->basename_,receipt->crc_,receipt->prefix_index_,
                     static_cast<std::uint32_t>(receipt->length_))==1,"Category actual83B1E0 loose store did not return1");
                // SAME native erase → free-head pop reuses the old pool slot;
                // the lower20 allocation identity stays in the kernel only.
                i.selected_inputs.erase(receipt->crc_);
                i.selected_loose.insert_or_assign(receipt->crc_,receipt);
                i.canonical_selected.insert_or_assign(receipt->crc_,receipt->transport_);
                if(old)records.bucket_order.erase_first(receipt->crc_);
                need(records.bucket_order.insert(receipt->crc_,receipt->crc_)==1,"Category loose duplicate projection node");
                const auto first=i.category_namespace->shared_first();
                records.first_record_names.try_emplace(receipt->crc_,first->names().at(receipt->crc_));
                records.records.insert_or_assign(receipt->crc_,receipt->transport_);
                records.record_names.insert_or_assign(receipt->crc_,receipt->basename_);
                ++i.category_node_epoch;result=1;
                need(records.bucket_order.live_nodes()==i.kernel.bucket_order().live_nodes()&&
                     records.records.size()==i.kernel.records().size(),"Category loose native/selected order differs");
            } // native preserve returns0 BEFORE FIRST registration/allocation.
            i.validate(fence,1);i.check(fence,1);
        }catch(...){i.failed=true;throw;}
    }
    // Cache publication currentness itself enters Arena: perform it only now.
    if(cached)need(cached->checked_original_mtime()==*cached_mtime,"Category old archive cache mtime changed after admission");
    receipt->require_current();return result;
}

BnwNativeArchiveEntryAdmissionArena::GlobalDefaultRecordBorrow
BnwNativeArchiveEntryAdmissionArena::borrow_current_category_record_for_manager_query(
    const BnwNativeCategoryNamespaceChannel& channel,std::uint32_t crc)const {
    auto& i=*impl_;i.require_thread_before_snapshot(); // BEFORE epoch/selected_loose reads.
    need(i.category_namespace.get()==&channel,"Category native node has another live manager namespace");
    // Any loose currentness may enter source/stat owners. Do that outside the
    // Arena fence; record/pool/witness identity is checked again inside it.
    const auto before_epoch=i.category_node_epoch;
    std::shared_ptr<const BnwNativeCategoryLooseAdmissionReceipt> loose;
    if(const auto found=i.selected_loose.find(crc);found!=i.selected_loose.end()){
        loose=found->second.lock();need(bool(loose),"Category native loose witness expired");loose->require_current();}
    const auto loose_source=loose?loose->source_.lock():std::shared_ptr<const BnwNativeCategoryLooseAdmissionSource>{};
    need(!loose||(loose_source&&loose_source.use_count()>1),"Category loose Stage lost persistent actual Source");
    GlobalDefaultRecordBorrow result;
    {
        auto fence=i.guard();i.check(fence,1);
        try {
            i.validate(fence,1);channel.require_current();i.check(fence,1);
            need(before_epoch==i.category_node_epoch,"Category native query crossed an admission effect");
            const auto& projected=channel.record_index();const auto row=projected.records.find(crc);
            if(row!=projected.records.end()) {
                const auto* current=std::get_if<assets::AssetHandle>(&row->second);
                const auto r=i.kernel.records().find(crc);const auto h=i.canonical_selected.find(crc);
                need(current&&r!=i.kernel.records().end()&&h!=i.canonical_selected.end(),
                     "Category selected row lacks actual native node/transport");
                const auto& a=current->candidate();const auto& b=h->second.candidate();
                need(a.original_name==b.original_name&&a.normalized_name==b.normalized_name&&a.payload_size==b.payload_size&&
                     a.provenance.kind==b.provenance.kind&&a.provenance.source_id==b.provenance.source_id&&
                     a.provenance.archive_relative_path==b.provenance.archive_relative_path&&
                     a.provenance.source_relative_path==b.provenance.source_relative_path,
                     "Category native/transport SAME selected row identity differs");
                if(r->second.offset==0){
                    const auto selected=i.selected_loose.find(crc);
                    need(loose&&selected!=i.selected_loose.end()&&selected->second.lock().get()==loose.get()&&
                         loose->crc_==crc&&loose_source->channel_.get()==&channel&&
                         r->second.length==static_cast<std::uint32_t>(loose->length_)&&
                         r->second.packed_prefix==(loose->prefix_index_<<20)&&
                         a.provenance.kind==assets::SourceKind::loose&&a.payload_size==std::uint64_t(loose->length_),
                         "Category loose node lost actual stat/query/prefix/pool witness");
                    result={&r->second,&h->second,{},{},loose};
                }else{
                    const auto input=i.selected_inputs.find(crc);
                    need(input!=i.selected_inputs.end()&&input->second.source&&input->second.entry&&
                         input->second.header&&input->second.prefix,"Category archive lacks positive native mode2 source");
                    const auto& src=input->second;const auto& entry=*src.entry;
                    need(src.entry_ordinal<src.source->entries().size()&&&src.source->entries()[src.entry_ordinal]==src.entry&&
                         entry.ordinal==src.entry_ordinal&&r->second.crc==crc&&r->second.offset==entry.payload_offset&&
                         r->second.length==entry.payload_length&&r->second.packed_prefix==(src.prefix_index<<20),
                         "Category archive node lost SAME Header/entry/prefix");
                    result={&r->second,&h->second,src.source,src.entry,{}};
                }
            }else need(!i.kernel.records().contains(crc)&&!loose,"Category apparent MISS hides an actual native row");
            i.validate(fence,1);channel.require_current();i.check(fence,1);
        }catch(...){i.failed=true;throw;}
    }
    if(loose)loose->require_current();
    need(!loose||loose_source.use_count()>1,"Category loose Stage Source disappeared during currentness callback");
    need(before_epoch==i.category_node_epoch,"Category native result changed after source callback");
    return result;
}

BnwNativeArchiveEntryAdmissionArena::GlobalDefaultRecordBorrow
BnwNativeArchiveEntryAdmissionArena::borrow_current_category_record_for_manager_query_completed(
    const BnwNativeCategoryNamespaceChannel& channel,std::uint32_t crc,const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {
    auto& i=*impl_;i.require_thread_before_snapshot(); // BEFORE epoch/selected_loose reads.
    loan.require_same_cold(cold);
    need(i.category_namespace.get()==&channel,"Category native node has another live manager namespace");
    // Any loose currentness may enter source/stat owners. Do that outside the
    // Arena fence; record/pool/witness identity is checked again inside it.
    const auto before_epoch=i.category_node_epoch;
    std::shared_ptr<const BnwNativeCategoryLooseAdmissionReceipt> loose;
    if(const auto found=i.selected_loose.find(crc);found!=i.selected_loose.end()){
        loose=found->second.lock();need(bool(loose),"Category native loose witness expired");loose->require_current_completed(loan,cold);}
    const auto loose_source=loose?loose->source_.lock():std::shared_ptr<const BnwNativeCategoryLooseAdmissionSource>{};
    need(!loose||(loose_source&&loose_source.use_count()>1),"Category loose Stage lost persistent actual Source");
    GlobalDefaultRecordBorrow result;
    {
        auto fence=i.guard();i.check(fence,1);
        try {
            i.validate_completed(fence,loan,cold);channel.require_current_completed(loan,cold);i.check(fence,1);
            need(before_epoch==i.category_node_epoch,"Category native query crossed an admission effect");
            const auto& projected=channel.record_index_completed(loan,cold);const auto row=projected.records.find(crc);
            if(row!=projected.records.end()) {
                const auto* current=std::get_if<assets::AssetHandle>(&row->second);
                const auto r=i.kernel.records().find(crc);const auto h=i.canonical_selected.find(crc);
                need(current&&r!=i.kernel.records().end()&&h!=i.canonical_selected.end(),
                     "Category selected row lacks actual native node/transport");
                const auto& a=current->candidate();const auto& b=h->second.candidate();
                need(a.original_name==b.original_name&&a.normalized_name==b.normalized_name&&a.payload_size==b.payload_size&&
                     a.provenance.kind==b.provenance.kind&&a.provenance.source_id==b.provenance.source_id&&
                     a.provenance.archive_relative_path==b.provenance.archive_relative_path&&
                     a.provenance.source_relative_path==b.provenance.source_relative_path,
                     "Category native/transport SAME selected row identity differs");
                if(r->second.offset==0){
                    const auto selected=i.selected_loose.find(crc);
                    need(loose&&selected!=i.selected_loose.end()&&selected->second.lock().get()==loose.get()&&
                         loose->crc_==crc&&loose_source->channel_.get()==&channel&&
                         r->second.length==static_cast<std::uint32_t>(loose->length_)&&
                         r->second.packed_prefix==(loose->prefix_index_<<20)&&
                         a.provenance.kind==assets::SourceKind::loose&&a.payload_size==std::uint64_t(loose->length_),
                         "Category loose node lost actual stat/query/prefix/pool witness");
                    result={&r->second,&h->second,{},{},loose};
                }else{
                    const auto input=i.selected_inputs.find(crc);
                    need(input!=i.selected_inputs.end()&&input->second.source&&input->second.entry&&
                         input->second.header&&input->second.prefix,"Category archive lacks positive native mode2 source");
                    const auto& src=input->second;const auto& entry=*src.entry;
                    need(src.entry_ordinal<src.source->entries().size()&&&src.source->entries()[src.entry_ordinal]==src.entry&&
                         entry.ordinal==src.entry_ordinal&&r->second.crc==crc&&r->second.offset==entry.payload_offset&&
                         r->second.length==entry.payload_length&&r->second.packed_prefix==(src.prefix_index<<20),
                         "Category archive node lost SAME Header/entry/prefix");
                    result={&r->second,&h->second,src.source,src.entry,{}};
                }
            }else need(!i.kernel.records().contains(crc)&&!loose,"Category apparent MISS hides an actual native row");
            i.validate_completed(fence,loan,cold);channel.require_current_completed(loan,cold);i.check(fence,1);
        }catch(...){i.failed=true;throw;}
    }
    if(loose)loose->require_current_completed(loan,cold);
    need(!loose||loose_source.use_count()>1,"Category loose Stage Source disappeared during currentness callback");
    need(before_epoch==i.category_node_epoch,"Category native result changed after source callback");
    return result;
}

void BnwNativeArchiveEntryAdmissionArena::require_same_category_channel(const BnwNativeCategoryNamespaceChannel& expected)const {
    auto fence=impl_->guard();impl_->validate(fence,1);
    need(impl_->category_namespace.get()==&expected,"Category arena has another namespace channel");
}
std::shared_ptr<const BnwNativeColdFilesystemOwner>
BnwNativeArchiveEntryAdmissionArena::claim_category_archive_basename_cache(const BnwNativeCategoryNamespaceChannel& expected)const {
    auto fence=impl_->guard();impl_->check(fence,1);
    need(impl_->category_namespace.get()==&expected && !impl_->cache_claim_attempted && !impl_->last_category_ordinal,
         "Category cache claim foreign/repeated/late");
    impl_->cache_claim_attempted=true; // BEFORE callbacks
    expected.require_initialization();impl_->check(fence,1);
    auto cold=impl_->validate(fence,1);
    const auto& records=expected.record_index();impl_->check(fence,1);
    need(records.records.empty() && records.bucket_order.live_nodes()==0 && fence.valid_parent(cold),
         "Category initial cache must precede namespace effects");return cold;
}
void BnwNativeArchiveEntryAdmissionArena::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();impl_->check(fence,1);
    auto visit=context.enter(*this,impl_->self);
    impl_->validate_common(fence,1,context);impl_->check(fence,1);visit.complete();
}
void BnwNativeArchiveEntryAdmissionArena::require_same_input_common_source(const BnwNativeColdFilesystemOwner& cold,BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);
    auto fence=impl_->guard();impl_->check(fence,1);
    context.require_borrowed(*this,impl_->self);
    auto actual=impl_->cold.lock();need(actual&&actual.get()==&cold&&fence.valid_parent(actual),"Common arena has another/retired Cold");
    impl_->check(fence,1);context.check();
}
void BnwNativeArchiveEntryAdmissionArena::require_current()const{auto fence=impl_->guard();impl_->validate(fence,1);}
void BnwNativeArchiveEntryAdmissionArena::require_same_input(const BnwNativeColdFilesystemOwner& cold)const{
    auto fence=impl_->guard();const auto actual=impl_->validate(fence,1);need(actual.get()==&cold,"Native admission has another Cold");
}
void BnwNativeArchiveEntryAdmissionArena::capture_primary32_record_nodes(
    BnwNativePrimary32RecordSourceBindings& b,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& root)const {
    auto& i=*impl_;auto fence=i.guard();i.check(fence,1);
    root.require_primary32_snapshot_capture_current(*this);
    need(!i.snapshot_capture_attempted && b.selected.empty() && b.arena.get()==this,
         "Native primary32 record capture repeated or has another source Arena");
    i.snapshot_capture_attempted=true; // Consume BEFORE first parent callback.
    { const auto cold=i.validate(fence,1);
      root.require_primary32_snapshot_capture_current(*this);
      need(fence.valid_parent(cold) && b.cold.get()==cold.get() && b.lease.get()==i.lease.get() && b.installation_input.get()==i.installation_input.get() && (static_cast<bool>(i.lease)!=static_cast<bool>(i.installation_input)),
           "Native primary32 capture lost SAME Cold/lease"); }
    need(i.next_primary_ordinal==32 && !i.failed && i.selected_inputs.size()==i.kernel.records().size(),
         "Native primary32 record capture has no executed complete source map");
    b.initial_policy=i.initial_policy;
    for(const auto& [crc,record]:i.kernel.records()) {
        const auto input=i.selected_inputs.find(crc);const auto first=i.kernel.first_names().find(crc);
        const auto handle=i.canonical_selected.find(crc);
        need(input!=i.selected_inputs.end() && first!=i.kernel.first_names().end() && handle!=i.canonical_selected.end(),
             "Native selected source record/FIRST/payload node absent");
        BnwNativePrimary32RecordSourceBindings::Row row;
        row.actual_record_node=&record;row.actual_handle_node=&handle->second;
        row.record=record;row.first_raw_name=first->second;
        row.archive_ordinal=input->second.archive_ordinal;row.canonical_handle=handle->second;
        need(b.selected.emplace(crc,std::move(row)).second,"Native selected source capture duplicated CRC");
    }
    root.require_primary32_snapshot_capture_current(*this);i.check(fence,1);
    require_same_primary32_record_nodes(b); // callback-free, no nested Arena guard.
}
void BnwNativeArchiveEntryAdmissionArena::require_same_primary32_record_nodes(
    const BnwNativePrimary32RecordSourceBindings& b)const {
    const auto& i=*impl_;
    // No callbacks, allocation, hashing or current/guard recursion. Root owns
    // the actual operation fence and calls this between child callback scopes.
    need((static_cast<bool>(i.lease)!=static_cast<bool>(i.installation_input)) && i.lease.get()==b.lease.get() &&
         i.installation_input.get()==b.installation_input.get() && b.arena.get()==this && !i.failed &&
         i.snapshot_capture_attempted && i.next_primary_ordinal==32 &&
         i.initial_policy.get()==b.initial_policy.get() &&
         i.kernel.records().size()==b.selected.size() && i.canonical_selected.size()==b.selected.size() &&
         i.selected_inputs.size()==b.selected.size(),"Native snapshot source map/epoch differs");
    const auto cold=i.cold.lock(); // Cold lifetime only; ZERO Package pins.
    need(cold && cold.get()==b.cold.get(),"Native snapshot has another source Cold");
    // These uint32 maps are ordered. Seek once across their current nodes;
    // every FIRST getter stays at its existing row/error boundary.
    auto r=i.kernel.records().cbegin();auto h=i.canonical_selected.cbegin();
    auto input=i.selected_inputs.cbegin();
    for(const auto& [crc,row]:b.selected) {
        while(r!=i.kernel.records().cend() && r->first<crc)++r;
        while(h!=i.canonical_selected.cend() && h->first<crc)++h;
        const auto first=i.kernel.first_names().find(crc);
        while(input!=i.selected_inputs.cend() && input->first<crc)++input;
        need(r!=i.kernel.records().end() && r->first==crc &&
             h!=i.canonical_selected.end() && h->first==crc &&
             first!=i.kernel.first_names().end() && input!=i.selected_inputs.end() &&
             input->first==crc,"Native snapshot selected node retired");
        const auto& selected=input->second;
        need(selected.source && selected.entry_ordinal<selected.source->entries().size() &&
             &selected.source->entries()[selected.entry_ordinal]==selected.entry &&
             selected.archive_ordinal<b.archives.size(),"Native snapshot FPK entry node/ordinal differs");
        const auto& e=*selected.entry;const auto& a=b.archives[selected.archive_ordinal];
        need(a.entries && a.entries->impl_->source.get()==selected.source.get() &&
             a.entries->impl_->header.get()==selected.header && a.entries->impl_->prefix.get()==selected.prefix &&
             a.entries->impl_->ordinal==selected.archive_ordinal &&
             selected.entry_ordinal<a.entries->impl_->entry_returns.size() &&
             a.entries->impl_->entry_returns[selected.entry_ordinal]==1,
             "Native snapshot FPK node has no actual positive source-entry receipt");
        need(row.actual_record_node==&r->second && row.actual_handle_node==&h->second &&
             row.record==r->second && row.first_raw_name==first->second &&
             row.archive_ordinal==selected.archive_ordinal && row.canonical_handle &&
             a.header.get()==selected.header && a.prefix.get()==selected.prefix &&
             e.ordinal==selected.entry_ordinal && e.original_name==first->second &&
             e.payload_offset==r->second.offset && e.payload_length==r->second.length &&
             selected.prefix_index==(r->second.packed_prefix>>20) && (r->second.packed_prefix&0xfffffu)==0,
             "Native snapshot actual record/FPK/FIRST/frame binding differs");
        const auto& actual=h->second.candidate();const auto& copy=row.canonical_handle->candidate();
        need(actual.original_name==e.original_name && actual.payload_size==e.payload_length &&
             copy.original_name==actual.original_name && copy.normalized_name==actual.normalized_name &&
             copy.payload_size==actual.payload_size && copy.provenance.kind==actual.provenance.kind &&
             copy.provenance.source_id==actual.provenance.source_id &&
             copy.provenance.source_relative_path==actual.provenance.source_relative_path &&
             copy.provenance.archive_relative_path==actual.provenance.archive_relative_path &&
             row.canonical_handle->requested_key().original_name()==h->second.requested_key().original_name() &&
             row.canonical_handle->requested_key().normalized_name()==h->second.requested_key().normalized_name(),
             "Native snapshot copied handle differs from actual admitted source handle node");
    }
}
void BnwNativeArchiveEntryAdmissionArena::capture_default_projection_nodes(
    BnwNativePrimary32RecordSourceBindings& b,
    NativeResourceRecordIndex& records,const ::civ5::ui::BnwNativePackageDiscoveryOwner& root)const {
    auto& i=*impl_;auto fence=i.guard();i.check(fence,1);
    root.require_default_projection_capture_current(*this);
    need(!i.default_projection_capture_attempted && b.selected.empty() && b.arena.get()==this,
         "Native primary32 record capture repeated or has another source Arena");
    i.default_projection_capture_attempted=true; // Consume BEFORE first parent callback.
    { const auto cold=i.validate(fence,1);
      root.require_default_projection_capture_current(*this);
      need(fence.valid_parent(cold) && b.cold.get()==cold.get() && b.lease.get()==i.lease.get() && b.installation_input.get()==i.installation_input.get() && (static_cast<bool>(i.lease)!=static_cast<bool>(i.installation_input)),
           "Native primary32 capture lost SAME Cold/lease"); }
    need(i.next_primary_ordinal==32 && !i.failed && i.selected_inputs.size()==i.kernel.records().size(),
         "Native primary32 record capture has no executed complete source map");
    b.initial_policy=i.initial_policy;
    for(const auto& [crc,record]:i.kernel.records()) {
        const auto input=i.selected_inputs.find(crc);const auto first=i.kernel.first_names().find(crc);
        const auto handle=i.canonical_selected.find(crc);
        need(input!=i.selected_inputs.end() && first!=i.kernel.first_names().end() && handle!=i.canonical_selected.end(),
             "Native selected source record/FIRST/payload node absent");
        BnwNativePrimary32RecordSourceBindings::Row row;
        row.actual_record_node=&record;row.actual_handle_node=&handle->second;
        row.record=record;row.first_raw_name=first->second;
        row.archive_ordinal=input->second.archive_ordinal;row.canonical_handle=handle->second;
        need(b.selected.emplace(crc,std::move(row)).second,"Native selected source capture duplicated CRC");
    }
    // Every projected handle is copied from the actual admitted handle node,
    // not a fresh parser/search or scalar DTO. Keep authentic kernel bucket
    // ordering and shared FIRST; do not re-admit/recompute record selection.
    // bind_shared_first captured the SAME global FIRST after the actual
    // primary32 admissions. Its inherited names are legitimately nonempty;
    // this new projection must still have no selected rows or bucket nodes.
    need(records.records.empty() && records.record_names.empty() && records.collisions.empty() &&
         records.bucket_order.live_nodes()==0,"Production projection received a prefilled staged index");
    need(records.first_record_names==i.kernel.first_names(),
         "Production projection inherited FIRST differs from actual source lineage");
    for(const auto& [crc,row]:b.selected) {
        need(row.actual_handle_node && NativeResourceRecordIndex::source_hash(row.first_raw_name)==crc,
             "Production projection raw ASCII/FIRST CRC differs");
        need(records.records.emplace(crc,*row.actual_handle_node).second,
             "Production projection duplicated actual admitted handle node");
        records.record_names.emplace(crc,assets::resource_basename(row.first_raw_name));
    }
    records.first_record_names=i.kernel.first_names();
    records.bucket_order=i.kernel.bucket_order();
    root.require_default_projection_capture_current(*this);i.check(fence,1);
    require_same_default_projection_nodes(b,records); // callback-free, no nested Arena guard.
}
void BnwNativeArchiveEntryAdmissionArena::require_same_default_projection_nodes(
    const BnwNativePrimary32RecordSourceBindings& b,const NativeResourceRecordIndex& records)const {
    const auto& i=*impl_;
    // No callbacks or current/guard recursion; bucket comparison may allocate. Root owns
    // the actual operation fence and calls this between child callback scopes.
    need((static_cast<bool>(i.lease)!=static_cast<bool>(i.installation_input)) && i.lease.get()==b.lease.get() &&
         i.installation_input.get()==b.installation_input.get() && b.arena.get()==this && !i.failed &&
         i.default_projection_capture_attempted && i.next_primary_ordinal==32 &&
         i.initial_policy.get()==b.initial_policy.get() &&
         i.kernel.records().size()==b.selected.size() && i.canonical_selected.size()==b.selected.size() &&
         i.selected_inputs.size()==b.selected.size(),"Native snapshot source map/epoch differs");
    const auto cold=i.cold.lock(); // Cold lifetime only; ZERO Package pins.
    need(cold && cold.get()==b.cold.get(),"Native snapshot has another source Cold");
    // These uint32 maps are ordered. Seek once across their current nodes;
    // every FIRST getter stays at its existing row/error boundary.
    auto r=i.kernel.records().cbegin();auto h=i.canonical_selected.cbegin();
    auto input=i.selected_inputs.cbegin();
    for(const auto& [crc,row]:b.selected) {
        while(r!=i.kernel.records().cend() && r->first<crc)++r;
        while(h!=i.canonical_selected.cend() && h->first<crc)++h;
        const auto first=i.kernel.first_names().find(crc);
        while(input!=i.selected_inputs.cend() && input->first<crc)++input;
        need(r!=i.kernel.records().end() && r->first==crc &&
             h!=i.canonical_selected.end() && h->first==crc &&
             first!=i.kernel.first_names().end() && input!=i.selected_inputs.end() &&
             input->first==crc,"Native snapshot selected node retired");
        const auto& selected=input->second;
        need(selected.source && selected.entry_ordinal<selected.source->entries().size() &&
             &selected.source->entries()[selected.entry_ordinal]==selected.entry &&
             selected.archive_ordinal<b.archives.size(),"Native snapshot FPK entry node/ordinal differs");
        const auto& e=*selected.entry;const auto& a=b.archives[selected.archive_ordinal];
        need(a.entries && a.entries->impl_->source.get()==selected.source.get() &&
             a.entries->impl_->header.get()==selected.header && a.entries->impl_->prefix.get()==selected.prefix &&
             a.entries->impl_->ordinal==selected.archive_ordinal &&
             selected.entry_ordinal<a.entries->impl_->entry_returns.size() &&
             a.entries->impl_->entry_returns[selected.entry_ordinal]==1,
             "Native snapshot FPK node has no actual positive source-entry receipt");
        need(row.actual_record_node==&r->second && row.actual_handle_node==&h->second &&
             row.record==r->second && row.first_raw_name==first->second &&
             row.archive_ordinal==selected.archive_ordinal && row.canonical_handle &&
             a.header.get()==selected.header && a.prefix.get()==selected.prefix &&
             e.ordinal==selected.entry_ordinal && e.original_name==first->second &&
             e.payload_offset==r->second.offset && e.payload_length==r->second.length &&
             selected.prefix_index==(r->second.packed_prefix>>20) && (r->second.packed_prefix&0xfffffu)==0,
             "Native snapshot actual record/FPK/FIRST/frame binding differs");
        const auto& actual=h->second.candidate();const auto& copy=row.canonical_handle->candidate();
        need(actual.original_name==e.original_name && actual.payload_size==e.payload_length &&
             copy.original_name==actual.original_name && copy.normalized_name==actual.normalized_name &&
             copy.payload_size==actual.payload_size && copy.provenance.kind==actual.provenance.kind &&
             copy.provenance.source_id==actual.provenance.source_id &&
             copy.provenance.source_relative_path==actual.provenance.source_relative_path &&
             copy.provenance.archive_relative_path==actual.provenance.archive_relative_path &&
             row.canonical_handle->requested_key().original_name()==h->second.requested_key().original_name() &&
             row.canonical_handle->requested_key().normalized_name()==h->second.requested_key().normalized_name(),
             "Native snapshot copied handle differs from actual admitted source handle node");
    }
    need(records.records.size()==i.canonical_selected.size() && records.record_names.size()==b.selected.size() &&
         records.first_record_names==i.kernel.first_names() &&
         records.bucket_order.ordered_nodes()==i.kernel.bucket_order().ordered_nodes() &&
         records.bucket_order.pool_size()==i.kernel.bucket_order().pool_size() &&
         records.bucket_order.source_count_field()==i.kernel.bucket_order().source_count_field() &&
         records.collisions.empty(),"Production staged index differs from actual Arena/FIRST/bucket nodes");
    for(const auto& [crc,row]:b.selected) {
        const auto staged=records.records.find(crc);const auto name=records.record_names.find(crc);
        need(staged!=records.records.end() && name!=records.record_names.end() &&
             name->second==assets::resource_basename(row.first_raw_name),"Production staged selected name differs");
        const auto* handle=std::get_if<assets::AssetHandle>(&staged->second);
        need(handle && row.actual_handle_node,"Production staged handle node missing");
        const auto& actual=row.actual_handle_node->candidate();const auto& copy=handle->candidate();
        need(copy.original_name==actual.original_name && copy.normalized_name==actual.normalized_name &&
             copy.payload_size==actual.payload_size && copy.provenance.kind==actual.provenance.kind &&
             copy.provenance.source_id==actual.provenance.source_id &&
             copy.provenance.source_relative_path==actual.provenance.source_relative_path &&
             copy.provenance.archive_relative_path==actual.provenance.archive_relative_path &&
             handle->requested_key().original_name()==row.actual_handle_node->requested_key().original_name() &&
             handle->requested_key().normalized_name()==row.actual_handle_node->requested_key().normalized_name(),
             "Production staged canonical handle differs from actual admitted payload node");
    }
}
void BnwNativeArchiveEntryAdmissionArena::require_retained_primary_default_projection_origin_without_callbacks(
    const BnwNativePrimary32RecordSourceBindings& b,const NativeResourceRecordIndex& records)const {
    const auto& i=*impl_;i.require_thread_before_snapshot(); // BEFORE mutable maps/epoch/control reads.
    // This is the late ORIGINAL-primary subset proof, not the capture-time
    // whole-Arena validator. Actual Common mode0 appends; it cannot replace
    // Primary nodes. Category replacement/loose storage has a separate Arena.
    need(!i.failed&&!i.category_namespace&&i.selected_loose.empty()&&
         i.default_projection_capture_attempted&&i.next_primary_ordinal==32&&
         b.actual_snapshot_brand&&b.arena&&b.arena.get()==this&&
         !b.arena.owner_before(i.self)&&!i.self.owner_before(b.arena)&&
         ((i.installation_input&&!i.lease&&!b.lease&&b.installation_input&&
           i.installation_input.get()==b.installation_input.get()&&
           !i.installation_input.owner_before(b.installation_input)&&!b.installation_input.owner_before(i.installation_input))||
          (!i.installation_input&&!b.installation_input&&i.lease&&b.lease&&i.lease.get()==b.lease.get()&&
           !i.lease.owner_before(b.lease)&&!b.lease.owner_before(i.lease)))&&
         i.initial_policy&&b.initial_policy&&i.initial_policy.get()==b.initial_policy.get()&&
         !i.initial_policy.owner_before(b.initial_policy)&&!b.initial_policy.owner_before(i.initial_policy)&&
         b.cold&&!b.cold.owner_before(i.cold)&&!i.cold.owner_before(b.cold),
         "Late primary origin lost SAME actual Arena/lease/Cold/policy controls");
    // b retains actual Cold and all original archive receipts/FPK Sources.
    // No additional Cold weak-lock, provider callback, source reparse or new
    // guard occurs. Shared FIRST's existing identity gate owns its temporary
    // Parent weak-lock internally; it is not a new outside facade owner.
    need(i.kernel.records().size()==i.canonical_selected.size()&&
         i.kernel.records().size()==i.selected_inputs.size()&&
         i.kernel.records().size()>=b.selected.size()&&
         i.kernel.bucket_order().live_nodes()==i.kernel.records().size(),
         "Late primary origin current Arena maps are incoherent");
    // Retain actual FIRST storage through the existing identity gate once.
    // All comparisons below are callback-free over this same map reference.
    const auto& actual_first=i.kernel.first_names();
    // These uint32 maps are ordered. Seek once across their current nodes;
    // every FIRST getter stays at its existing row/error boundary.
    auto r=i.kernel.records().cbegin();auto h=i.canonical_selected.cbegin();
    auto input=i.selected_inputs.cbegin();
    for(const auto& [crc,row]:b.selected) {
        while(r!=i.kernel.records().cend() && r->first<crc)++r;
        while(h!=i.canonical_selected.cend() && h->first<crc)++h;
        const auto first=actual_first.find(crc);
        while(input!=i.selected_inputs.cend() && input->first<crc)++input;
        need(r!=i.kernel.records().end() && r->first==crc &&
             h!=i.canonical_selected.end() && h->first==crc &&
             first!=actual_first.end() && input!=i.selected_inputs.end() &&
             input->first==crc,"Native snapshot selected node retired");
        const auto& selected=input->second;
        need(selected.source && selected.entry_ordinal<selected.source->entries().size() &&
             &selected.source->entries()[selected.entry_ordinal]==selected.entry &&
             selected.archive_ordinal<b.archives.size(),"Native snapshot FPK entry node/ordinal differs");
        const auto& e=*selected.entry;const auto& a=b.archives[selected.archive_ordinal];
        need(a.entries && a.entries->impl_->source.get()==selected.source.get() &&
             a.entries->impl_->header.get()==selected.header && a.entries->impl_->prefix.get()==selected.prefix &&
             a.entries->impl_->ordinal==selected.archive_ordinal &&
             selected.entry_ordinal<a.entries->impl_->entry_returns.size() &&
             a.entries->impl_->entry_returns[selected.entry_ordinal]==1,
             "Native snapshot FPK node has no actual positive source-entry receipt");
        need(row.actual_record_node==&r->second && row.actual_handle_node==&h->second &&
             row.record==r->second && row.first_raw_name==first->second &&
             row.archive_ordinal==selected.archive_ordinal && row.canonical_handle &&
             a.header.get()==selected.header && a.prefix.get()==selected.prefix &&
             e.ordinal==selected.entry_ordinal && e.original_name==first->second &&
             e.payload_offset==r->second.offset && e.payload_length==r->second.length &&
             selected.prefix_index==(r->second.packed_prefix>>20) && (r->second.packed_prefix&0xfffffu)==0,
             "Native snapshot actual record/FPK/FIRST/frame binding differs");
        const auto& actual=h->second.candidate();const auto& copy=row.canonical_handle->candidate();
        need(actual.original_name==e.original_name && actual.payload_size==e.payload_length &&
             copy.original_name==actual.original_name && copy.normalized_name==actual.normalized_name &&
             copy.payload_size==actual.payload_size && copy.provenance.kind==actual.provenance.kind &&
             copy.provenance.source_id==actual.provenance.source_id &&
             copy.provenance.source_relative_path==actual.provenance.source_relative_path &&
             copy.provenance.archive_relative_path==actual.provenance.archive_relative_path &&
             row.canonical_handle->requested_key().original_name()==h->second.requested_key().original_name() &&
             row.canonical_handle->requested_key().normalized_name()==h->second.requested_key().normalized_name(),
             "Native snapshot copied handle differs from actual admitted source handle node");
    }
    need(records.records.size()==b.selected.size()&&records.record_names.size()==b.selected.size()&&
         records.bucket_order.live_nodes()==b.selected.size()&&records.collisions.empty()&&
         records.bucket_order.pool_size()==i.kernel.bucket_order().pool_size()&&
         records.bucket_order.source_count_field()==records.bucket_order.live_nodes()&&
         i.kernel.bucket_order().source_count_field()==i.kernel.bucket_order().live_nodes(),
         "Late primary origin sealed projection shape differs");
    // FIRST is genuinely shared and grows. Authenticate EVERY captured name,
    // including inherited entries outside selected rows; never equate sizes.
    for(const auto& [crc,name]:records.first_record_names) {
        const auto found=actual_first.find(crc);
        need(found!=actual_first.end()&&found->second==name,
             "Late primary origin sealed FIRST entry changed/retired");
    }
    const auto sealed_order=records.bucket_order.ordered_nodes();
    const auto current_order=i.kernel.bucket_order().ordered_nodes();
    std::vector<NativeResourceBucketOrder::Node> retained_order;retained_order.reserve(b.selected.size());
    for(const auto& node:current_order) {
        const auto retained=b.selected.find(node.key);
        if(retained==b.selected.end())continue; // Only authentic new Common mode0 rows are outside this historical subset.
        need(retained->second.record.crc==node.key&&retained->second.record.symbolic_slot==node.value,
             "Late primary origin current bucket changed original native key/slot");
        retained_order.push_back(node);
    }
    need(retained_order==sealed_order,"Late primary origin retained bucket order differs");
    for(const auto& [crc,row]:b.selected) {
        const auto staged=records.records.find(crc);const auto name=records.record_names.find(crc);
        need(staged!=records.records.end() && name!=records.record_names.end() &&
             name->second==assets::resource_basename(row.first_raw_name),"Production staged selected name differs");
        const auto* handle=std::get_if<assets::AssetHandle>(&staged->second);
        need(handle && row.actual_handle_node,"Production staged handle node missing");
        const auto current=i.canonical_selected.find(crc);
        need(current!=i.canonical_selected.end()&&row.actual_handle_node==&current->second,
             "Late primary origin staged handle no longer has actual current node");
        const auto& actual=current->second.candidate();const auto& copy=handle->candidate();
        need(copy.original_name==actual.original_name && copy.normalized_name==actual.normalized_name &&
             copy.payload_size==actual.payload_size && copy.provenance.kind==actual.provenance.kind &&
             copy.provenance.source_id==actual.provenance.source_id &&
             copy.provenance.source_relative_path==actual.provenance.source_relative_path &&
             copy.provenance.archive_relative_path==actual.provenance.archive_relative_path &&
             handle->requested_key().original_name()==current->second.requested_key().original_name() &&
             handle->requested_key().normalized_name()==current->second.requested_key().normalized_name(),
             "Production staged canonical handle differs from actual admitted payload node");
    }
}
void BnwNativeArchiveEntryAdmissionArena::require_retained_common_projection_origin_without_callbacks(
    const BnwNativePrimary32RecordSourceBindings& primary,const NativeResourceRecordIndex& original,
    const std::vector<std::unique_ptr<const BnwNativeCommon13Step>>& steps,const NativeResourceRecordIndex& staged)const {
    const auto& i=*impl_;i.require_thread_before_snapshot();
    require_retained_primary_default_projection_origin_without_callbacks(primary,original);
    need(i.installation_input&&!i.lease&&!i.failed&&!i.category_namespace&&i.selected_loose.empty()&&
         i.next_primary_ordinal==32&&!steps.empty()&&steps.size()<=bnw_common13_descriptors.size(),
         "Installed Common projection lacks genuine retained original-order source journal");
    std::uint64_t admitted=0;std::size_t actual_next_common=0;
    std::map<std::uint32_t,const BnwNativeArchiveV6EntryAdmissionOwner*> common_positive;
    for(std::size_t n=0;n<steps.size();++n){
        need(bool(steps[n]),"Installed Common retained step absent");const auto& step=*steps[n];
        need(step.channel&&step.request&&step.lookup&&step.phase==BnwNativeCommon13Step::Phase::terminal,
             "Installed Common retained source did not return normally");
        const auto& channel=*step.channel;
        need(channel.ordinal_==n&&channel.installation_input_.get()==i.installation_input.get()&&!channel.lease_&&
             channel.arena_.get()==this&&!channel.arena_.owner_before(i.self)&&!i.self.owner_before(channel.arena_),
             "Installed Common returned source has another real ordinal/Arena/installation");
        if(step.absent){need(!step.construction&&!step.header&&!step.prefix&&!step.entries&&!step.publication,
            "Installed Common original absence fabricated constructor/admission/cache receipt");continue;}
        need(step.construction&&step.header&&step.prefix&&step.entries,"Installed Common present source receipt missing");
        const auto& r=*step.entries->impl_;
        need(r.ordinal==32+n&&r.header.get()==step.header.get()&&r.prefix.get()==step.prefix.get()&&r.source&&r.admitted<=r.declared&&
             r.declared==r.entry_returns.size()&&r.source->entries().size()==r.declared&&
             !r.arena.owner_before(i.self)&&!i.self.owner_before(r.arena)&&
             (r.admitted?bool(step.publication):!step.publication),
             "Installed Common returned receipt lost actual Header/Prefix/entry-return/cache branch");
        actual_next_common=n+1;std::uint32_t returned=0;
        for(const auto& entry:r.source->entries()){
            const auto result=r.entry_returns.at(entry.ordinal);need(result<=1,"Common unsupported native entry return");
            if(!result)continue;
            ++returned;const auto crc=bnw_native_admission_ascii_crc(entry.original_name);
            const auto actual=i.kernel.records().find(crc);const auto input=i.selected_inputs.find(crc);const auto handle=i.canonical_selected.find(crc);
            need(actual!=i.kernel.records().end()&&input!=i.selected_inputs.end()&&handle!=i.canonical_selected.end()&&
                 !primary.selected.contains(crc)&&common_positive.emplace(crc,step.entries.get()).second&&
                 input->second.source.get()==r.source.get()&&input->second.entry==&entry&&
                 input->second.header==r.header.get()&&input->second.prefix==r.prefix.get()&&
                 input->second.archive_ordinal==r.ordinal&&input->second.entry_ordinal==entry.ordinal&&
                 actual->second.crc==crc&&actual->second.offset==entry.payload_offset&&actual->second.length==entry.payload_length&&
                 actual->second.packed_prefix==(input->second.prefix_index<<20)&&
                 handle->second.candidate().original_name==entry.original_name&&handle->second.candidate().payload_size==entry.payload_length,
                 "Common positive return differs from actual native record/handle/source entry");
        }
        need(returned==r.admitted,"Common actual per-entry return accumulation differs");admitted+=returned;
    }
    need(i.next_common_ordinal==actual_next_common&&common_positive.size()==admitted&&
         i.kernel.records().size()==primary.selected.size()+admitted&&staged.records.size()==i.kernel.records().size()&&
         staged.record_names.size()==staged.records.size()&&staged.bucket_order.live_nodes()==staged.records.size()&&staged.collisions.empty(),
         "Installed Common projected namespace is not exactly original32 plus actual journal positive returns");
    const auto& first=i.kernel.first_names();
    for(const auto& [crc,record]:i.kernel.records()) {
        const auto selected=staged.records.find(crc);const auto name=staged.record_names.find(crc);
        const auto actual=i.canonical_selected.find(crc);const auto input=i.selected_inputs.find(crc);
        const auto raw=first.find(crc);const auto projected_first=staged.first_record_names.find(crc);
        need(selected!=staged.records.end()&&name!=staged.record_names.end()&&actual!=i.canonical_selected.end()&&
             input!=i.selected_inputs.end()&&raw!=first.end()&&projected_first!=staged.first_record_names.end()&&
             projected_first->second==raw->second&&name->second==assets::resource_basename(raw->second)&&
             (primary.selected.contains(crc)||common_positive.contains(crc)),
             "Common staged row lacks genuine original or positive Common receipt/FIRST");
        const auto* copied=std::get_if<assets::AssetHandle>(&selected->second);need(copied,"Common staged row replaced archive by unissued loose source");
        const auto& a=actual->second.candidate();const auto& c=copied->candidate();
        need(a.original_name==c.original_name&&a.normalized_name==c.normalized_name&&a.payload_size==c.payload_size&&
             a.provenance.kind==c.provenance.kind&&a.provenance.source_id==c.provenance.source_id&&
             a.provenance.source_relative_path==c.provenance.source_relative_path&&a.provenance.archive_relative_path==c.provenance.archive_relative_path&&
             actual->second.requested_key().original_name()==copied->requested_key().original_name()&&
             actual->second.requested_key().normalized_name()==copied->requested_key().normalized_name()&&record.crc==crc,
             "Common staged archive payload differs from actual admitted handle");
    }
    const auto native=i.kernel.bucket_order().ordered_nodes();const auto projected=staged.bucket_order.ordered_nodes();
    need(native.size()==projected.size(),"Common projected traversal size differs");
    for(std::size_t n=0;n<native.size();++n) {
        const auto old=primary.selected.find(native[n].key);
        need(native[n].key==projected[n].key&&
             projected[n].value==(old==primary.selected.end()?projected[n].key:old->second.record.symbolic_slot),
             "Common projected traversal is not actual original slots plus appended native CRC nodes");
    }
}
void BnwNativeArchiveEntryAdmissionArena::bind_fresh_current_default_source(
    const BnwNativeCurrentDefaultSource& source,const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
    auto& i=*impl_;auto fence=i.guard();i.check(fence,1);
    parent.require_current_default_source_operation_current(source);
    need(!i.current_default_source_claimed && !i.next_primary_ordinal && i.kernel.records().empty() &&
         source.arena_.get()==this && source.cache_ && !source.actual_record_storage_,
         "Fresh current Default Source assignment is late/repeated/foreign");
    i.current_default_source_claimed=true;
    // SAME actual native record storage identity; no caller table/snapshot.
    source.actual_record_storage_=&i.kernel;
    i.current_default_source=source.self_;
    parent.require_current_default_source_operation_current(source);i.check(fence,1);
}
void BnwNativeArchiveEntryAdmissionArena::require_same_current_default_source(
    const BnwNativeCurrentDefaultSource& source)const {
    const auto& i=*impl_;
    // Root holds source and its operation fence: no extra child/Parent weak
    // lock, guard recursion, callbacks or allocations at this identity check.
    need(i.current_default_source_claimed && !i.failed && source.arena_.get()==this &&
         source.actual_record_storage_==&i.kernel &&
         !i.current_default_source.owner_before(source.self_) && !source.self_.owner_before(i.current_default_source),
         "Current Default Source no longer denotes SAME actual Arena record storage");
}
BnwNativeArchiveEntryAdmissionArena::GlobalDefaultRecordBorrow
BnwNativeArchiveEntryAdmissionArena::borrow_current_default_record_for_global_query(
    const BnwNativeCurrentDefaultSource& source,std::uint32_t crc)const {
    auto& i=*impl_;auto fence=i.guard();i.check(fence,1);
    try {
        require_same_current_default_source(source);i.validate(fence,1);
        require_same_current_default_source(source);i.check(fence,1);
        GlobalDefaultRecordBorrow out;
        const auto record=i.kernel.records().find(crc);
        if(record!=i.kernel.records().end()) {
            const auto handle=i.canonical_selected.find(crc);const auto input=i.selected_inputs.find(crc);
            need(handle!=i.canonical_selected.end()&&input!=i.selected_inputs.end()&&input->second.source&&
                 input->second.entry&&input->second.header&&input->second.prefix,
                 "Global Default actual record lacks SAME admitted source nodes");
            const auto& selected=input->second;const auto& entry=*selected.entry;
            need(record->second.crc==crc&&record->second.offset==entry.payload_offset&&
                 record->second.length==entry.payload_length&&record->second.packed_prefix==(selected.prefix_index<<20)&&
                 selected.entry_ordinal==entry.ordinal&&selected.entry_ordinal<selected.source->entries().size()&&
                 &selected.source->entries()[selected.entry_ordinal]==selected.entry,
                 "Global Default actual kernel/source entry shape changed");
            out={&record->second,&handle->second,selected.source,selected.entry};
        }
        //No copied maps/nodes, payload read, archive reopen or artificial miss.
        i.validate(fence,1);require_same_current_default_source(source);i.check(fence,1);return out;
    }catch(...){i.failed=true;throw;}
}
BnwNativeDefaultNamespaceInitialReceiptDiagnostic
BnwNativeArchiveEntryAdmissionArena::initial_namespace_policy_receipt()const {
    auto fence=impl_->guard();impl_->validate(fence,1);
    need(!impl_->category_namespace,"Category arena cannot issue default initial policy receipt");
    const auto receipt=impl_->initial_policy->initial_receipt();impl_->check(fence,1);
    return receipt; // INITIAL immutable fields only, never current activation.
}
std::shared_ptr<const BnwNativeColdFilesystemOwner>
BnwNativeArchiveEntryAdmissionArena::claim_initial_archive_basename_cache()const {
    auto fence=impl_->guard();impl_->check(fence,1);
    need(!impl_->cache_claim_attempted,"Native initial cache already attempted");
    {
        auto cold=impl_->cold.lock();need(bool(cold),"Native initial cache Cold expired");
        cold->require_before_any_archive_request_and_header_without_callbacks();
        impl_->cache_claim_attempted=true; // BEFORE first source/provider callback.
    } // No extra Cold pin across policy validation.
    auto actual=impl_->validate(fence,1);impl_->check(fence,1);
    actual->require_before_any_archive_request_and_header_without_callbacks();
    need(fence.valid_parent(actual),"Native initial cache Cold replaced/lost");
    return actual;
}
BnwNativeArchiveV6EntryAdmissionOwner::BnwNativeArchiveV6EntryAdmissionOwner(std::unique_ptr<Impl> i):impl_(std::move(i)){}
BnwNativeArchiveV6EntryAdmissionOwner::~BnwNativeArchiveV6EntryAdmissionOwner()=default;
std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> BnwNativeArchiveV6EntryAdmissionOwner::acquire(
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header,
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& source_root) {
    need(arena&&header&&prefix,"Native admission requires genuine Arena/Header/Prefix");
    source_root.require_archive_entry_operation_current(*arena,*header,*prefix);
    auto& state=*arena->impl_;auto fence=state.guard();state.check(fence,2);
    // Failure is terminal. Prefix/FIRST/earlier per-entry effects cannot be
    // silently reset or retried as though the native partial loop were atomic.
    struct Failure {BnwNativeArchiveEntryAdmissionArena::Impl& state;bool done=false;
        explicit Failure(BnwNativeArchiveEntryAdmissionArena::Impl& s):state(s){}
        ~Failure(){if(!done)state.failed=true;}} failure(state);
    const auto cold=state.validate(fence,2);
    long prefix_internal_pins=1; // Factory argument; receipt adds one below.
    const auto check=[&]{state.check(fence,2);need(fence.valid_parent(cold),"Native admission Cold lost before effects");
        source_root.require_archive_entry_operation_current(*arena,*header,*prefix);
        need(prefix.use_count()>prefix_internal_pins,"Native admission Prefix lost persistent facade during callback");};
    header->require_current();check();prefix->require_current();check();
    prefix->require_header_identity_without_callbacks(*header);check();
    const auto& request=header->source_request();check();request.require_same_owner(*cold);check();
    const auto ordinal=request.source_ordinal();check();
    need(ordinal==state.next_primary_ordinal && ordinal<bnw_primary_archive_descriptors.size(),
         "Native admission primary source ordinal repeated/skipped");
    const auto& construction=header->construction();check();
    const auto source=construction.canonical_payload_source();check();
    const auto declared=header->header_fields().declared_entry_count;check();
    const auto bytes=header->original_byte_length();check();
    const auto prefix_index=prefix->source_index();check();
    need(source->filename_scrambling()==0 && source->extra_header_byte()==0,
         "Native admission requires finite v6 transform0 branch");
    bnw_native_v6_entry_inputs(source->entries(),declared,source->directory_end(),bytes);
    auto receipt=std::make_unique<Impl>();receipt->arena=arena;receipt->header=header;receipt->prefix=prefix;
    prefix_internal_pins=2;
    receipt->source=source;receipt->ordinal=ordinal;receipt->declared=declared;receipt->entry_returns.reserve(declared);
    // Prepare every canonical ordinal handle before any admission effect.
    // Handles are payload inputs, never used as native selected-record truth.
    std::vector<assets::AssetHandle> handles;handles.reserve(declared);
    std::unordered_map<std::string,std::size_t> occurrences;
    for(const auto& e:source->entries()) {
        const auto all=source->find_all(assets::AssetKey(e.original_name));auto& next=occurrences[e.normalized_name];
        need(next<all.size(),"Native admission source ordinal payload handle absent");
        const auto& handle=all[next++];need(handle.candidate().original_name==e.original_name&&
             handle.candidate().payload_size==e.payload_length,"Native admission canonical ordinal differs from prepared metadata");
        handles.push_back(handle);
    }
    // Last callbacks BEFORE native loop. Pins/immutable snapshot keep each
    // input stable; loop has no UI callbacks and implements source mode0.
    header->require_current();check();prefix->require_current();check();ascii_locale();
    for(const auto& e:source->entries()) {
        check();const auto result=state.kernel.admit(e.original_name,prefix_index,
            static_cast<std::uint32_t>(e.payload_length),static_cast<std::uint32_t>(e.payload_offset),0);
        if(result) {
            const auto key=bnw_native_admission_ascii_crc(e.original_name);
            need(state.canonical_selected.emplace(key,handles[e.ordinal]).second,
                 "Native admitted canonical payload already selected");
            need(state.selected_inputs.emplace(key,BnwNativeArchiveEntryAdmissionArena::Impl::SelectedInput{
                source,&e,header.get(),prefix.get(),ordinal,e.ordinal,prefix_index}).second,
                "Native selected FPK source node already admitted");
            ++receipt->admitted;
        }
        receipt->entry_returns.push_back(result); // reserved; no callback.
    }
    check();need(state.kernel.records().size()==state.canonical_selected.size() &&
        state.kernel.records().size()==state.selected_inputs.size(),"Native completed admission projection differs");
    auto out=std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>(new BnwNativeArchiveV6EntryAdmissionOwner(std::move(receipt)));
    out->impl_->self=out;
    ++state.next_primary_ordinal;failure.done=true;return out;
}
std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> BnwNativeArchiveV6EntryAdmissionOwner::acquire_common(
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header,
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix,
    const ::civ5::ui::BnwNativePackageDiscoveryOwner& source_root,
    const BnwNativeCommon13ArchiveChannel& channel) {
    need(arena&&header&&prefix,"Native admission requires genuine Arena/Header/Prefix");
    channel.require_active();
    source_root.require_archive_entry_operation_current(*arena,*header,*prefix);
    auto& state=*arena->impl_;auto fence=state.guard();state.check(fence,2);
    // Failure is terminal. Prefix/FIRST/earlier per-entry effects cannot be
    // silently reset or retried as though the native partial loop were atomic.
    struct Failure {BnwNativeArchiveEntryAdmissionArena::Impl& state;bool done=false;
        explicit Failure(BnwNativeArchiveEntryAdmissionArena::Impl& s):state(s){}
        ~Failure(){if(!done)state.failed=true;}} failure(state);
    const auto cold=state.validate(fence,2);
    long prefix_internal_pins=1; // Factory argument; receipt adds one below.
    const auto check=[&]{state.check(fence,2);need(fence.valid_parent(cold),"Native admission Cold lost before effects");
        source_root.require_archive_entry_operation_current(*arena,*header,*prefix);
        need(prefix.use_count()>prefix_internal_pins,"Native admission Prefix lost persistent facade during callback");};
    header->require_current();check();prefix->require_current();check();
    const auto& actual_header=prefix->header_owner_after_validation_without_callbacks();check();need(&actual_header==header.get(),"Native admission prefix belongs to another Header");
    const auto& request=header->source_request();check();request.require_same_owner(*cold);check();
    const auto ordinal=request.source_ordinal();check();
    channel.require_active();
    need(state.next_primary_ordinal==32 && channel.ordinal_>=state.next_common_ordinal &&
         ordinal==32+channel.ordinal_ && channel.arena_.get()==arena.get(),
         "Common admission source ordinal/Default arena repeated/skipped");
    const auto& construction=header->construction();check();
    const auto source=construction.canonical_payload_source();check();
    const auto declared=header->header_fields().declared_entry_count;check();
    const auto bytes=header->original_byte_length();check();
    const auto prefix_index=prefix->source_index();check();
    need(source->filename_scrambling()==0 && source->extra_header_byte()==0,
         "Native admission requires finite v6 transform0 branch");
    bnw_native_v6_entry_inputs(source->entries(),declared,source->directory_end(),bytes);
    auto receipt=std::make_unique<Impl>();receipt->arena=arena;receipt->header=header;receipt->prefix=prefix;
    prefix_internal_pins=2;
    receipt->source=source;receipt->ordinal=ordinal;receipt->declared=declared;receipt->entry_returns.reserve(declared);
    // Prepare every canonical ordinal handle before any admission effect.
    // Handles are payload inputs, never used as native selected-record truth.
    std::vector<assets::AssetHandle> handles;handles.reserve(declared);
    std::unordered_map<std::string,std::size_t> occurrences;
    for(const auto& e:source->entries()) {
        const auto all=source->find_all(assets::AssetKey(e.original_name));auto& next=occurrences[e.normalized_name];
        need(next<all.size(),"Native admission source ordinal payload handle absent");
        const auto& handle=all[next++];need(handle.candidate().original_name==e.original_name&&
             handle.candidate().payload_size==e.payload_length,"Native admission canonical ordinal differs from prepared metadata");
        handles.push_back(handle);
    }
    // Last callbacks BEFORE native loop. Pins/immutable snapshot keep each
    // input stable; loop has no UI callbacks and implements source mode0.
    header->require_current();check();prefix->require_current();check();ascii_locale();
    for(const auto& e:source->entries()) {
        check();const auto result=state.kernel.admit(e.original_name,prefix_index,
            static_cast<std::uint32_t>(e.payload_length),static_cast<std::uint32_t>(e.payload_offset),0);
        if(result) {
            const auto key=bnw_native_admission_ascii_crc(e.original_name);
            need(state.canonical_selected.emplace(key,handles[e.ordinal]).second,
                 "Native admitted canonical payload already selected");
            need(state.selected_inputs.emplace(key,BnwNativeArchiveEntryAdmissionArena::Impl::SelectedInput{
                source,&e,header.get(),prefix.get(),ordinal,e.ordinal,prefix_index}).second,
                "Native selected FPK source node already admitted");
            ++receipt->admitted;
        }
        receipt->entry_returns.push_back(result); // reserved; no callback.
    }
    check();need(state.kernel.records().size()==state.canonical_selected.size() &&
        state.kernel.records().size()==state.selected_inputs.size(),"Native completed admission projection differs");
    auto out=std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>(new BnwNativeArchiveV6EntryAdmissionOwner(std::move(receipt)));
    out->impl_->self=out;
    state.next_common_ordinal=channel.ordinal_+1;failure.done=true;return out;
}
std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> BnwNativeArchiveV6EntryAdmissionOwner::acquire_category(
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena,
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header,
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix,
    std::shared_ptr<const BnwNativeCategoryArchiveChannel> channel) {
    need(arena&&header&&prefix&&channel,"Category entry admission requires genuine retained inputs");
    channel->require_active_request();
    const auto ns=channel->namespace_channel();arena->require_same_category_channel(*ns);
    auto& state=*arena->impl_;auto fence=state.guard();state.check(fence,2);
    struct Failure {BnwNativeArchiveEntryAdmissionArena::Impl& state;bool done=false;
        ~Failure(){if(!done)state.failed=true;}} failure{state};
    const auto cold=state.validate(fence,2);
    const auto check=[&]{state.check(fence,2);channel->require_active_request();
        need(fence.valid_parent(cold),"Category entry lost persistent Cold");};
    need(state.cache_claim_attempted,"Category entry precedes initial namespace cache");
    const auto& request=header->source_request();check();request.require_same_category_channel(*channel);check();
    request.require_same_owner(*cold);check();
    const auto& bound=prefix->header_owner();check();need(&bound==header.get(),"Category entry prefix has another Header");
    const auto ordinal=request.source_ordinal();check();
    need(!state.last_category_ordinal || ordinal>*state.last_category_ordinal,"Category entry attempt repeated/out of source order");
    state.last_category_ordinal=ordinal; // authentic request journal may contain genuine skipped/miss/hit attempts
    const auto& construction=header->construction();check();
    const auto source=construction.canonical_payload_source();check();
    const auto declared=header->header_fields().declared_entry_count;check();
    const auto bytes=header->original_byte_length();check();
    const auto prefix_index=prefix->source_index();check();
    need(source->filename_scrambling()==0 && source->extra_header_byte()==0,"Category finite v6 transform0 required");
    category_v6_entry_inputs(source->entries(),declared,source->directory_end(),bytes);
    auto receipt=std::make_unique<Impl>();receipt->arena=arena;receipt->header=header;receipt->prefix=prefix;
    receipt->source=source;receipt->ordinal=ordinal;receipt->declared=declared;receipt->category_channel=channel;
    receipt->entry_returns.reserve(declared);receipt->category_nodes.reserve(declared);receipt->category_handles.reserve(declared);
    std::unordered_map<std::string,std::size_t> occurrences;
    for(const auto& e:source->entries()) {
        const auto all=source->find_all(assets::AssetKey(e.original_name));auto& next=occurrences[e.normalized_name];
        need(next<all.size(),"Category canonical directory ordinal payload absent");
        const auto& handle=all[next++];need(handle.candidate().original_name==e.original_name &&
            handle.candidate().payload_size==e.payload_length,"Category canonical directory ordinal metadata differs");
        receipt->category_handles.push_back(handle);
    }
    header->require_current();check();prefix->require_current();check();ascii_locale();
    struct Active {BnwNativeArchiveEntryAdmissionArena::Impl& state;
        explicit Active(BnwNativeArchiveEntryAdmissionArena::Impl& s):state(s) {
            need(!state.category_admission_active,"Category entry loop reentered");state.category_admission_active=true;}
        ~Active(){state.category_admission_active=false;}} active(state);
    for(const auto& e:source->entries()) {
        check();const auto crc=arena->store_category_raw_record_without_fence(e.original_name,receipt->category_handles[e.ordinal],source,e,*header,*prefix,prefix_index,ordinal);check();
        receipt->category_nodes.push_back({crc,prefix_index,e.ordinal,source,&e,&receipt->category_handles[e.ordinal]});
        receipt->entry_returns.push_back(1);++receipt->admitted; // actual mode2 effect, not declared count
    }
    check();auto out=std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner>(new BnwNativeArchiveV6EntryAdmissionOwner(std::move(receipt)));
    out->impl_->self=out;failure.done=true;return out;
}
std::vector<BnwNativeArchiveV6EntryAdmissionOwner::CategoryAdmittedRecord>
BnwNativeArchiveV6EntryAdmissionOwner::category_admitted_record_nodes()const {
    auto fence=impl_->guard();impl_->validate(fence);
    const auto channel=impl_->category_channel.lock();
    need(channel && impl_->category_nodes.size()==impl_->admitted,
         "Default/scalar admission cannot issue category native nodes");
    channel->require_current();need(fence.valid(),"Category native node projection reentered/retired");
    return impl_->category_nodes;
}
std::vector<BnwNativeArchiveV6EntryAdmissionOwner::CommonAdmittedRecord>
BnwNativeArchiveV6EntryAdmissionOwner::common_admitted_record_nodes(const BnwNativeCommon13ArchiveChannel& channel)const {
    channel.require_active();auto fence=impl_->guard();const auto arena=impl_->validate(fence);
    const auto& state=*arena->impl_;
    need(!state.failed && impl_->ordinal==32+channel.ordinal_ &&
         state.next_primary_ordinal==32 && state.next_common_ordinal==channel.ordinal_+1 &&
         channel.arena_.get()==arena.get(),"Common node borrow has another returned loop/arena");
    std::vector<CommonAdmittedRecord> result;result.reserve(impl_->admitted);
    for(const auto& entry:impl_->source->entries())if(impl_->entry_returns[entry.ordinal]) {
        const auto crc=bnw_native_admission_ascii_crc(entry.original_name);
        const auto r=state.kernel.records().find(crc);
        const auto input=state.selected_inputs.find(crc);
        const auto h=state.canonical_selected.find(crc);
        need(r!=state.kernel.records().end() && input!=state.selected_inputs.end() && h!=state.canonical_selected.end() &&
             input->second.source.get()==impl_->source.get() && input->second.entry==&entry &&
             input->second.header==impl_->header.get() && input->second.prefix==impl_->prefix.get() &&
             input->second.archive_ordinal==impl_->ordinal && input->second.entry_ordinal==entry.ordinal &&
             r->second.length==entry.payload_length && r->second.offset==entry.payload_offset &&
             r->second.packed_prefix==(input->second.prefix_index<<20) &&
             h->second.candidate().original_name==entry.original_name && h->second.candidate().payload_size==entry.payload_length,
             "Common projection lost actual admitted entry/handle node");
        result.push_back({&r->second,&h->second,entry.original_name});
    }
    need(result.size()==impl_->admitted,"Common projected actual positive returns differ");
    (void)impl_->validate(fence);channel.require_active();return result;
}
void BnwNativeArchiveV6EntryAdmissionOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();need(fence.valid(),"Common entry receipt lost/reentered/foreign thread");
    auto visit=context.enter(*this,impl_->self);
    impl_->validate_common(fence,context,visit);
    need(fence.valid(),"Common entry receipt changed");visit.complete();
}
std::uint32_t BnwNativeArchiveV6EntryAdmissionOwner::admitted_count_common_source(BnwNativeCommonSourceValidationContext& context)const {
    require_common_source_current(context);
    auto fence=impl_->guard();need(fence.valid(),"Common entry count lost/reentered/foreign thread");
    context.require_borrowed(*this,impl_->self);return impl_->admitted;
}
void BnwNativeArchiveV6EntryAdmissionOwner::require_current()const{auto fence=impl_->guard();impl_->validate(fence);}
void BnwNativeArchiveV6EntryAdmissionOwner::require_same_arena(const BnwNativeArchiveEntryAdmissionArena& arena)const{
    auto fence=impl_->guard();const auto actual=impl_->validate(fence);need(actual.get()==&arena,"Native admission receipt has another Arena");
}
std::pair<std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,
          std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>>
BnwNativeArchiveV6EntryAdmissionOwner::bound_archive_cache_sources()const {
    auto fence=impl_->guard();const auto arena=impl_->validate(fence);
    need(fence.valid()&&fence.valid_parent(arena),"Native entry cache binding lost source parent");
    return {impl_->header,impl_->prefix};
}
std::uint32_t BnwNativeArchiveV6EntryAdmissionOwner::admitted_count()const{
    auto fence=impl_->guard();impl_->validate(fence);return impl_->admitted;
}
std::uint32_t BnwNativeArchiveV6EntryAdmissionOwner::admitted_count_after_validation_without_callbacks()const{
    // All increments belong to the unpublished unique receipt's real entry
    // loop. No later writer or mutable alias survives shared publication.
    return impl_->admitted;
}
std::uint32_t BnwNativeArchiveV6EntryAdmissionOwner::declared_count()const{
    auto fence=impl_->guard();impl_->validate(fence);return impl_->declared;
}

BnwNativeArchiveV6EntryAdmissionOwner::DefaultCachePublicationInputs
BnwNativeArchiveV6EntryAdmissionOwner::borrow_default_cache_publication_inputs(
    const BnwNativeArchiveEntryAdmissionArena& arena,const BnwNativeColdArchiveRequest& request,
    const std::function<void()>& controller)const {
    auto fence=impl_->guard();
    const auto check=[&]{need(fence.valid(),"Native cache input receipt lost/reentered/foreign thread");controller();};
    check();
    // Each validate return's transient Arena pin ends in this full expression;
    // no extra Arena facade pin survives into the next provider callback.
    need(impl_->validate(fence).get()==&arena,"Native admission receipt has another Arena");check();
    need(impl_->header&&impl_->prefix,"Native entry cache sources unavailable");
    const auto count=impl_->admitted;check();
    const auto metadata=impl_->header->cache_publication_metadata(request,count,controller);check();
    need(impl_->validate(fence).get()==&arena,"Native admission receipt has another Arena");check();
    need(impl_->admitted==count,"Native cache input admitted count changed within source fence");
    // Immutable owned fields only; facade outputs copy AFTER all callbacks.
    return {impl_->header,impl_->prefix,std::get<0>(metadata),count,
            std::get<1>(metadata),std::get<2>(metadata)};
}

#include "bnw_native_default_cache_publication_stage.inc"
void BnwNativeArchiveEntryAdmissionArena::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);}
void BnwNativeArchiveEntryAdmissionArena::require_same_input_completed(const BnwNativeColdFilesystemOwner& cold,
    const BnwNativeCompletedColdSourceLoan& loan)const {require_current_completed(loan,cold);}
void BnwNativeArchiveEntryAdmissionArena::require_same_completed_owner_control(
    const std::weak_ptr<const BnwNativeArchiveEntryAdmissionArena>& expected,const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {
    impl_->require_thread_before_snapshot();loan.require_same_cold(cold);
    need(!expected.expired()&&!expected.owner_before(impl_->self)&&!impl_->self.owner_before(expected),
        "Completed admission actual Arena factory control differs");require_current_completed(loan,cold);
}
BnwNativeDefaultNamespaceInitialReceiptDiagnostic BnwNativeArchiveEntryAdmissionArena::initial_namespace_policy_receipt_completed(
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
    // Finish Arena guard BEFORE initial Policy guard. No second Arena Scope.
    require_current_completed(loan,cold);need(impl_->initial_policy_attempted&&impl_->initial_policy,
        "Completed initial namespace policy missing/category namespace is distinct");
    const auto result=impl_->initial_policy->initial_receipt_completed(loan,cold);require_current_completed(loan,cold);return result;
}
void BnwNativeArchiveV6EntryAdmissionOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena,registry);}
void BnwNativeArchiveV6EntryAdmissionOwner::require_same_arena_completed(const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold,
    const BnwNativeGlobalArchivePrefixOwner& registry)const {require_current_completed(loan,cold,arena,registry);}
std::uint32_t BnwNativeArchiveV6EntryAdmissionOwner::admitted_count_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena,registry);return impl_->admitted;
}
std::uint32_t BnwNativeArchiveV6EntryAdmissionOwner::declared_count_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena,registry);return impl_->declared;
}
std::pair<std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner>,std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord>> BnwNativeArchiveV6EntryAdmissionOwner::bound_archive_cache_sources_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold,const BnwNativeArchiveEntryAdmissionArena& arena,
    const BnwNativeGlobalArchivePrefixOwner& registry)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold,arena,registry);return {impl_->header,impl_->prefix};
}

// Exact original default node/source lender under an explicit completed source loan.
BnwNativeArchiveEntryAdmissionArena::GlobalDefaultRecordBorrow
BnwNativeArchiveEntryAdmissionArena::borrow_current_default_record_for_global_query_completed(
    const BnwNativeCurrentDefaultSource& source,std::uint32_t crc,const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {
    auto& i=*impl_;i.require_thread_before_snapshot();loan.require_same_cold(cold);
    auto fence=i.guard();i.check(fence,1);
    try {
        require_same_current_default_source(source);i.validate_completed(fence,loan,cold);
        require_same_current_default_source(source);i.check(fence,1);
        GlobalDefaultRecordBorrow out;
        const auto record=i.kernel.records().find(crc);
        if(record!=i.kernel.records().end()) {
            const auto handle=i.canonical_selected.find(crc);const auto input=i.selected_inputs.find(crc);
            need(handle!=i.canonical_selected.end()&&input!=i.selected_inputs.end()&&input->second.source&&
                 input->second.entry&&input->second.header&&input->second.prefix,
                 "Global Default actual record lacks SAME admitted source nodes");
            const auto& selected=input->second;const auto& entry=*selected.entry;
            need(record->second.crc==crc&&record->second.offset==entry.payload_offset&&
                 record->second.length==entry.payload_length&&record->second.packed_prefix==(selected.prefix_index<<20)&&
                 selected.entry_ordinal==entry.ordinal&&selected.entry_ordinal<selected.source->entries().size()&&
                 &selected.source->entries()[selected.entry_ordinal]==selected.entry,
                 "Global Default actual kernel/source entry shape changed");
            out={&record->second,&handle->second,selected.source,selected.entry};
        }
        //No copied maps/nodes, payload read, archive reopen or artificial miss.
        i.validate_completed(fence,loan,cold);require_same_current_default_source(source);i.check(fence,1);return out;
    }catch(...){i.failed=true;throw;}
}
}
