#include "bnw_native_primary32_record_snapshot.hpp"
#include "bnw_native_callback_owner_guard.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <exception>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace civ5::ui::detail {
namespace {
void need(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
// One stack aggregate per validate; samples never log, allocate or retain owners.
struct SnapshotValidationCost final {
    using Clock=std::chrono::steady_clock;
    enum Kind { authenticate,parents,journal,header_request,request_owner,request_ordinal,
        prefix_owner,prefix_header,prefix_index,entries_arena,entries_count,
        publication_current,header_construction,construction_canonical,publication_canonical,
        rows,final_input,kind_count };
    struct Slot {std::uint64_t calls=0,incomplete=0;Clock::duration elapsed{};};
    Slot slots[kind_count]{};
    Clock::time_point begin{};
    int exceptions=0;
    bool has_facade=false,complete=false;
    explicit SnapshotValidationCost(bool facade)noexcept:has_facade(facade) {
        const int saved_errno=errno;const DWORD saved_error=GetLastError();
        begin=Clock::now();exceptions=std::uncaught_exceptions();
        SetLastError(saved_error);errno=saved_errno;
    }
    struct Sample final {
        Slot& slot;Clock::time_point begin{};int exceptions=0;
        Sample(SnapshotValidationCost& cost,Kind kind)noexcept:slot(cost.slots[kind]) {
            const int saved_errno=errno;const DWORD saved_error=GetLastError();
            ++slot.calls;begin=Clock::now();exceptions=std::uncaught_exceptions();
            SetLastError(saved_error);errno=saved_errno;
        }
        ~Sample()noexcept {
            const int saved_errno=errno;const DWORD saved_error=GetLastError();
            slot.elapsed+=Clock::now()-begin;
            if(std::uncaught_exceptions()>exceptions)++slot.incomplete;
            SetLastError(saved_error);errno=saved_errno;
        }
    };
    template<class F>decltype(auto) measure(Kind kind,F&& supplier) {
        Sample sample(*this,kind);return supplier(); // Preserve void/value/reference return type.
    }
    ~SnapshotValidationCost()noexcept {
        const int saved_errno=errno;const DWORD saved_error=GetLastError();
        const auto end=Clock::now();
        const auto total_us=std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count();
        const auto steady_ms=std::chrono::duration_cast<std::chrono::milliseconds>(end.time_since_epoch()).count();
        constexpr const char* names[kind_count]={"authenticate","parents","journal","header_request",
            "request_owner","request_ordinal","prefix_owner","prefix_header","prefix_index",
            "entries_arena","entries_count","publication_current","header_construction",
            "construction_canonical","publication_canonical","rows","final_input"};
        char line[4096];std::size_t used=0;
        const auto append=[&](const char* format,auto... values)noexcept {
            const auto available=sizeof(line)-used-1;
            const int written=std::snprintf(line+used,available,format,values...);
            if(written>0){const auto count=static_cast<std::size_t>(written);used+=count<available?count:available-1;}
        };
        append("[BNW_SNAPSHOT_VALIDATE] facade=%d complete=%d unwind=%d total_us=%lld steady_ms=%lld",
            has_facade?1:0,complete?1:0,std::uncaught_exceptions()>exceptions?1:0,
            static_cast<long long>(total_us),static_cast<long long>(steady_ms));
        for(std::size_t n=0;n<kind_count;++n){
            const auto us=std::chrono::duration_cast<std::chrono::microseconds>(slots[n].elapsed).count();
            append(" %s=%llu/%lld/%llu",names[n],static_cast<unsigned long long>(slots[n].calls),
                static_cast<long long>(us),static_cast<unsigned long long>(slots[n].incomplete));
        }
        line[used++]='\n';std::fwrite(line,1,used,stderr);std::fflush(stderr);
        SetLastError(saved_error);errno=saved_errno;
    }
};
}
struct BnwNativePrimary32RecordSnapshotOwner::Impl {
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativePrimary32RecordSnapshotOwner> self;
    std::unique_ptr<const BnwNativePrimary32RecordSourceBindings> source;
    using Guard = BnwNativeCallbackOwnerGuard<BnwNativePrimary32RecordSnapshotOwner>;
    Guard guard() const noexcept { return Guard(state, self); }
    template<class Fence> void validate(const Fence& fence,
                                       const BnwNativePrimary32RecordSnapshotOwner* facade) const {
        SnapshotValidationCost cost(facade!=nullptr);
        need(bool(source), "Primary32 record capture absent");
        const auto& b = *source;
        const auto check = [&] {
            need(fence.valid() && !b.package.expired() && b.actual_snapshot_brand && b.authenticate,
                 "Primary32 record source/facade/Root brand expired or reentered");
            // No Package weak lock in this leaf: Root callback owns exactly ONE
            // transient lock, released BEFORE any child current/Journal call.
            cost.measure(SnapshotValidationCost::authenticate,[&]{b.authenticate(b, facade);});
            need(fence.valid(), "Primary32 record callback caught leaf reentry/facade loss");
        };
        check();
        need((static_cast<bool>(b.lease)!=static_cast<bool>(b.installation_input)) && b.cold && b.initial_policy && b.prefixes && b.arena && b.cache && b.journal,
             "Primary32 record parents missing");
        if(b.installation_input){
            cost.measure(SnapshotValidationCost::parents,[&]{BnwNativePrimary32RecordSnapshotOwner::require_installed_cold_source(b);});check();
            cost.measure(SnapshotValidationCost::parents,[&]{b.installation_input->require_current();});check();
        }else{
        cost.measure(SnapshotValidationCost::parents,[&]{b.cold->require_same_input(*b.lease);}); check();
        const auto& lease = cost.measure(SnapshotValidationCost::parents,[&]() -> decltype(auto) {return b.cold->early_input_lease();}); check();
        need(lease.get() == b.lease.get(), "Primary32 record capture has another lease");
        }
        cost.measure(SnapshotValidationCost::parents,[&]{b.initial_policy->require_same_input(*b.cold);}); check();
        cost.measure(SnapshotValidationCost::parents,[&]{b.prefixes->require_same_input(*b.cold);}); check();
        cost.measure(SnapshotValidationCost::parents,[&]{b.arena->require_same_input(*b.cold);}); check();
        cost.measure(SnapshotValidationCost::parents,[&]{b.cache->require_same_arena(*b.arena);}); check();
        const auto completed = cost.measure(SnapshotValidationCost::journal,[&]() -> decltype(auto) {return b.journal->current();}); check();
        need(completed.positive_primary32_complete && completed.completed_archives == 32 &&
             completed.executed_admitted_total == b.selected.size(),
             "Primary32 record set differs from real completed mode0 admissions");
        std::array<std::uint32_t,32> prefix_indices{};
        for (std::size_t n = 0; n < b.archives.size(); ++n) {
            const auto& a = b.archives[n];
            need(a.header && a.prefix && a.entries && a.publication && a.actual_step_brand,
                 "Primary32 record source archive frame absent");
            // FULL source currentness and each original Root/leaf fence remain.
            // Only sealed identities immediately following them avoid getters.
            cost.measure(SnapshotValidationCost::header_request,[&]{a.header->require_current();}); check();
            cost.measure(SnapshotValidationCost::request_owner,[&]{a.header->require_default_projection_request_identity_without_callbacks(*b.cold,n);}); check();
            check(); // Removed ordinal getter's original Root/leaf fence.
            cost.measure(SnapshotValidationCost::prefix_owner,[&]() -> decltype(auto) {return a.prefix->require_same_owner(*b.prefixes);}); check();
            cost.measure(SnapshotValidationCost::prefix_header,[&]{a.prefix->require_header_identity_without_callbacks(*a.header);}); check();
            prefix_indices[n] = cost.measure(SnapshotValidationCost::prefix_index,[&]{return a.prefix->source_index_after_validation_without_callbacks();}); check();
            cost.measure(SnapshotValidationCost::entries_arena,[&]() -> decltype(auto) {return a.entries->require_same_arena(*b.arena);}); check();
            need(cost.measure(SnapshotValidationCost::entries_count,[&]{return a.entries->admitted_count_after_validation_without_callbacks();}) != 0, "Primary32 record archive has no actual positive admissions"); check();
            cost.measure(SnapshotValidationCost::publication_current,[&]() -> decltype(auto) {return a.publication->require_current();}); check();
            cost.measure(SnapshotValidationCost::publication_canonical,[&]{a.publication->require_default_projection_payload_identity_without_callbacks(*a.header);}); check();
            // Keep both removed Construction getter boundaries, including the
            // full exact-node Root authentication; neither retains a pin.
            check();
            check();
        }
        { SnapshotValidationCost::Sample row_sample(cost,SnapshotValidationCost::rows);
        for (const auto& [crc, row] : b.selected) {
            need(row.actual_record_node && row.actual_handle_node && row.canonical_handle &&
                 row.archive_ordinal < b.archives.size() && row.record.crc == crc,
                 "Primary32 record selected source node absent");
            bnw_primary32_record_shape_diagnostic(row.record, row.first_raw_name, prefix_indices[row.archive_ordinal]);
            const auto& candidate = row.canonical_handle->candidate();
            need(candidate.original_name == row.first_raw_name && candidate.payload_size == row.record.length,
                 "Primary32 record selected handle/FIRST/length differs");
            // Actual native-node and handle-node equality is authenticated by
            // Root, never obtained by dereferencing untrusted raw pointers here.
        }
        }
        if(b.installation_input){cost.measure(SnapshotValidationCost::final_input,[&]{b.installation_input->require_current();});check();}
        else{cost.measure(SnapshotValidationCost::final_input,[&]{b.lease->require_current();}); check();}
        cost.complete=true;
    }
};
void BnwNativePrimary32RecordSnapshotOwner::require_installed_cold_source(
    const BnwNativePrimary32RecordSourceBindings& b) {
    // Called only after the closed bindings/frame/brand check, before the same
    // callback revalidation performed by the existing leaf validation body.
    b.cold->require_same_input(*b.installation_input);
}
BnwNativePrimary32RecordSnapshotOwner::BnwNativePrimary32RecordSnapshotOwner(std::unique_ptr<Impl> i)
    : impl_(std::move(i)) {}
BnwNativePrimary32RecordSnapshotOwner::~BnwNativePrimary32RecordSnapshotOwner() = default;
std::shared_ptr<const BnwNativePrimary32RecordSnapshotOwner>
BnwNativePrimary32RecordSnapshotOwner::acquire(BnwNativePrimary32RecordSourceBindings bindings) {
    auto i = std::make_unique<Impl>();
    i->source = std::unique_ptr<const BnwNativePrimary32RecordSourceBindings>(
        new BnwNativePrimary32RecordSourceBindings(std::move(bindings)));
    { BnwNativeCallbackConstructionGuard fence(i->state); i->validate(fence, nullptr); }
    auto out = std::shared_ptr<const BnwNativePrimary32RecordSnapshotOwner>(
        new BnwNativePrimary32RecordSnapshotOwner(std::move(i)));
    out->impl_->self = out;
    return out;
}
const BnwNativePrimary32RecordSourceBindings&
BnwNativePrimary32RecordSnapshotOwner::source_identity_without_callbacks()const noexcept{
    return *impl_->source; // unique_ptr<const Bindings>, closed factory; no weak lock/current/IO.
}
void BnwNativePrimary32RecordSnapshotOwner::require_current() const {
    auto fence = impl_->guard(); impl_->validate(fence, this);
}
std::size_t BnwNativePrimary32RecordSnapshotOwner::selected_record_count_diagnostic() const {
    auto fence = impl_->guard(); impl_->validate(fence, this); return impl_->source->selected.size();
}
BnwNativePrimary32RecordQueryDiagnostic
BnwNativePrimary32RecordSnapshotOwner::query_ascii_record_diagnostic(std::u16string_view raw) const {
    auto fence = impl_->guard();
    need(fence.valid(), "Primary32 borrowed query cannot be read by retired/reentered facade");
    // All reads of caller-owned memory finish BEFORE the first external callback.
    const auto query = bnw_primary32_prepare_ascii_query_diagnostic(raw);
    impl_->validate(fence, this);
    const auto* row = bnw_default_namespace_crc_selection_diagnostic(impl_->source->selected, query);
    BnwNativePrimary32RecordQueryDiagnostic result;
    result.requested_crc = query.crc;
    if (row) {
        result.matched = true; result.selected = row->record;
        result.first_raw_name = row->first_raw_name; result.archive_ordinal = row->archive_ordinal;
    }
    // Diagnostic string allocation above. Revalidate after LAST callbacks and
    // real Root authentication BEFORE returning copied facts, no read capability.
    impl_->validate(fence, this); return result;
}
}
