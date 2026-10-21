#pragma once
#include "bnw_native_archive_basename_cache.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <span>
#include <vector>
namespace civ5::ui::detail {
class BnwNativeColdFilesystemOwner;
enum class BnwPrimary32SourceEvent {lookup,constructor_stat,header,prefix,metadata,entry,cache};
inline constexpr std::array<BnwPrimary32SourceEvent,7> bnw_primary32_source_order={
    BnwPrimary32SourceEvent::lookup,BnwPrimary32SourceEvent::constructor_stat,
    BnwPrimary32SourceEvent::header,BnwPrimary32SourceEvent::prefix,
    BnwPrimary32SourceEvent::metadata,BnwPrimary32SourceEvent::entry,BnwPrimary32SourceEvent::cache};
// Source order/control policy ONLY. Diagnostic inputs cannot issue a journal.
class BnwPrimary32JournalPolicyDiagnostic final {
    std::size_t next_=0;std::uint64_t total_=0;bool active_=false,failed_=false;
  public:
    void begin_attempt() {
        if(active_){failed_=true;active_=false;throw std::runtime_error("Primary32 caught nested attempt poisoned journal");}
        if(failed_||next_==32)throw std::runtime_error("Primary32 step retired/reentered/complete");
        active_=true;
    }
    void complete_diagnostic(std::size_t observed_ordinal,std::span<const BnwPrimary32SourceEvent> events,
                             std::uint32_t executed_positive_count) {
        if(!active_||failed_||observed_ordinal!=next_||events.size()!=bnw_primary32_source_order.size()||
           !std::equal(events.begin(),events.end(),bnw_primary32_source_order.begin())||executed_positive_count==0)
            {failed_=true;active_=false;throw std::runtime_error("Primary32 actual source order/positive executed result incomplete");}
        total_+=executed_positive_count;++next_;active_=false;
    }
    void fail_attempt()noexcept{failed_=true;active_=false;}
    std::size_t next()const noexcept{return next_;}
    std::uint64_t total()const noexcept{return total_;}
    bool complete()const noexcept{return next_==32&&!failed_&&!active_;}
    bool failed()const noexcept{return failed_;}
};
class BnwNativePrimary32LoopJournalOwner;
class BnwNativePrimary32PostpublishedJournalOperation;
// CLOSED Root-issued source capabilities, not a public aggregate/step factory.
// Root fills only from SAME privately retained source step and actual checkpoints.
class BnwNativePrimary32SourceBindings final {
  public:
    ~BnwNativePrimary32SourceBindings()=default;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativePrimary32LoopJournalOwner;
    BnwNativePrimary32SourceBindings()=default;
    BnwNativePrimary32SourceBindings(const BnwNativePrimary32SourceBindings&)=default;
    BnwNativePrimary32SourceBindings(BnwNativePrimary32SourceBindings&&)noexcept=default;
    BnwNativePrimary32SourceBindings& operator=(BnwNativePrimary32SourceBindings&&)noexcept=default;
    std::shared_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixOwner> prefixes;
    std::shared_ptr<const BnwNativeArchiveEntryAdmissionArena> arena;
    std::shared_ptr<const BnwNativeArchiveBasenameCacheOwner> cache;
    std::shared_ptr<const BnwNativeColdArchiveRequest> request;
    std::shared_ptr<const BnwNativeArchiveConstruction> construction;
    std::shared_ptr<const BnwNativeArchiveV6StreamHeaderOwner> header;
    std::shared_ptr<const BnwNativeGlobalArchivePrefixRecord> prefix;
    std::shared_ptr<const BnwNativeArchiveCacheLookupOwner> lookup;
    std::shared_ptr<const BnwNativeArchiveV6EntryAdmissionOwner> entries;
    std::shared_ptr<const BnwNativeArchiveCachePublicationOwner> publication;
    // Root-owned identity of the actual executed source frame. Copied pointer
    // tuples cannot create another source checkpoint or journal permission.
    std::shared_ptr<const std::uint8_t> source_step_brand;
    std::array<BnwPrimary32SourceEvent,7> events{};
    std::size_t event_count=0;
};
struct BnwNativePrimary32JournalFrameDiagnostic {
    std::size_t completed_archives=0;
    std::uint64_t executed_admitted_total=0;
    bool positive_primary32_complete=false;
};
template<class> class BnwNativeCallbackOwnerGuard;
class BnwNativePrimary32LoopJournalOwner final {
  public:
    ~BnwNativePrimary32LoopJournalOwner();
    BnwNativePrimary32LoopJournalOwner(const BnwNativePrimary32LoopJournalOwner&)=delete;
    void require_current()const;
    BnwNativePrimary32JournalFrameDiagnostic current()const;
  private:
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativePrimary32PostpublishedJournalOperation;
    BnwNativePrimary32JournalFrameDiagnostic current_for_postpublished_prefix(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,
        const BnwNativeCallbackOwnerGuard<::civ5::ui::BnwNativePackageDiscoveryOwner>&)const;
    static std::shared_ptr<const BnwNativePrimary32LoopJournalOwner> acquire(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&);
    void advance_one_step(const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    void require_new_step_acceptance_frame_current(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,const BnwNativePrimary32SourceBindings&,
        const BnwNativePrimary32SourceBindings&,const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>&,
        std::size_t)const;
    std::pair<std::size_t,std::uint32_t> validate_new_step_for_installation_acceptance(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,const BnwNativePrimary32SourceBindings&,
        const BnwNativePrimary32SourceBindings&,const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>&,
        std::size_t)const;
    void require_current_for_default_publication(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const;
    BnwNativePrimary32JournalFrameDiagnostic current_for_owned_primary_completion(
        const ::civ5::ui::BnwNativePackageDiscoveryOwner&,
        const BnwNativeCallbackOwnerGuard<::civ5::ui::BnwNativePackageDiscoveryOwner>&)const;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit BnwNativePrimary32LoopJournalOwner(std::unique_ptr<Impl>);
};
// Only Journal constructs this synchronous stack kernel; Root alone invokes it.
// parent_ aliases the existing local parent.lock, never another owning copy.
class BnwNativePrimary32PostpublishedJournalOperation final {
    friend class BnwNativePrimary32LoopJournalOwner;
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    const BnwNativePrimary32LoopJournalOwner& journal_;
    const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>& fence_;
    const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>& parent_;
    BnwNativePrimary32PostpublishedJournalOperation(
        const BnwNativePrimary32LoopJournalOwner&,
        const BnwNativeCallbackOwnerGuard<BnwNativePrimary32LoopJournalOwner>&,
        const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>&)noexcept;
    BnwNativePrimary32PostpublishedJournalOperation(const BnwNativePrimary32PostpublishedJournalOperation&)=delete;
    BnwNativePrimary32PostpublishedJournalOperation& operator=(const BnwNativePrimary32PostpublishedJournalOperation&)=delete;
    void require_current(const ::civ5::ui::BnwNativePackageDiscoveryOwner&,
                         const BnwNativePrimary32LoopJournalOwner&)const;
    BnwNativePrimary32JournalFrameDiagnostic execute()const;
};
}
