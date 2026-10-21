#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/live_session.hpp"
#include <functional>
#include <memory>
#include <span>

namespace civ5::ui {
enum class BnwResourceArtVariant { normal,heavy,alt,alt_heavy };
enum class BnwResourceArtInfoStatus { info,no_resource,unresolved_identity,null_info_slot,absent_type };
enum class BnwResourceLayoutReadBlocker {
    missing_current_debug_owner,current_city_lookup_not_bound,missing_current_globals_cache_owner };
// Status is a current checked Game cache read, not a caller bool. An
// unavailable reset/Read owner preserves other scalar/catalog read facts.
enum class BnwResourceDebugCacheReadStatus { not_queried,unavailable,checked_cache };
struct BnwResourceDebugCacheRead {
    BnwResourceDebugCacheReadStatus status=BnwResourceDebugCacheReadStatus::not_queried;
    std::optional<bool> cached_mode;
    bool operator==(const BnwResourceDebugCacheRead &) const = default;
};
// A scalar selector is useful for source contract tests; it authenticates no
// revealed resource, layout event, registry resolution or renderer state.
struct BnwResourceArtScalarInput {
    std::int32_t resource=-1,quantity=0,threshold=0,feature=-1;
    bool hills=false,water=false;
    std::array<std::int16_t,9> cached_resource_types{};
    std::int16_t cached_marsh=-1;
};
std::optional<BnwResourceArtVariant> select_bnw_resource_art_scalar_variant(
    const BnwResourceArtScalarInput &) noexcept;
struct BnwResourceArtTagReadFacts {
    std::uint64_t plot_index=0,x=0,y=0;
    std::optional<std::int64_t> wire_resource;
    std::optional<std::uint64_t> wire_resource_hash,info_slot;
    std::optional<std::int64_t> authored_id;
    std::optional<std::string> type;
    // Source order: normal/heavy/alt/alt-heavy. NULL and empty remain distinct.
    std::array<std::optional<std::string>,4> art_define_tags;
    std::optional<std::int32_t> logical_resource,feature;
    // Heavy is the final immutable selected GlobalDefines read fact. It is
    // not a receipt for original GC cache initialization order or live cache
    // mutation; matching this owner authorizes only the sealed read scope.
    std::int32_t quantity=0,plot_type=0,heavy_threshold=0;
    // revealed is the current raw/projected team bit, not the debug-aware
    // CvPlot::isRevealed/getVisibleResourceState query. Debug never overwrites it.
    bool hills=false,water=false,revealed=false;
    std::int64_t visibility_count=0;
    std::optional<std::uint8_t> first_cached_resource_match;
    std::optional<bool> cached_resource_predicate;
    bool quantity_exceeds_heavy_threshold=false;
    std::optional<saves::SavedPlotCityReference> plot_city;
    // Current source GetAt lookup facts only. Index-only handles can resolve
    // a different full ID; absence is not inferred from cell.is_city.
    std::optional<bool> plot_is_city;
    std::optional<std::int32_t> resolved_city_id;
    std::optional<std::pair<std::uint64_t,std::uint64_t>> resolved_city_source_core;
    BnwResourceArtInfoStatus info_status=BnwResourceArtInfoStatus::unresolved_identity;
    // No renderer key or conditional revealed-resource candidate is published.
    std::vector<BnwResourceLayoutReadBlocker> layout_blockers{
        BnwResourceLayoutReadBlocker::missing_current_debug_owner,
        BnwResourceLayoutReadBlocker::missing_current_globals_cache_owner};
    bool operator==(const BnwResourceArtTagReadFacts &) const = default;
};
struct BnwResourceArtTagReadStamp {
    runtime::SessionCheckpointIdentity source;
    std::uint64_t state_generation=0,frame_number=0,width=0,height=0;
    std::int64_t active_player=-1,active_team=-1,turn=0;
    bool wrap_x=false,wrap_y=false;
    std::array<std::uint8_t,16> map_guid{};
    // No reveal/policy bypass is inferred by changing raw revealed_teams.
    BnwResourceDebugCacheRead debug_cache;
};
class BnwResourceArtTagReadOwner;
class BnwResourceArtTagReadReceipt {
  public:
    const BnwResourceArtTagReadStamp &stamp() const noexcept { return stamp_; }
    std::span<const BnwResourceArtTagReadFacts> plots() const noexcept { return plots_; }
  private:
    friend class BnwResourceArtTagReadOwner;
    BnwResourceArtTagReadReceipt(BnwResourceArtTagReadStamp,
        std::vector<BnwResourceArtTagReadFacts>,std::weak_ptr<const void>);
    BnwResourceArtTagReadStamp stamp_;
    std::vector<BnwResourceArtTagReadFacts> plots_;
    std::weak_ptr<const void> owner_;
};
// Same sealed Rules Impl, immutable Info snapshots and borrowed session facade.
// The caller's retained host must keep LiveSession alive across all callbacks.
// An owned state lease does not retain the session facade. No drawing readiness
// or layout/worker event is inferred from these reads.
class BnwResourceArtTagReadOwner {
  public:
    BnwResourceArtTagReadOwner(const runtime::LiveSession &,const content::RulesDatabase &,
        const content::InfoTypeCatalog &,const runtime::SessionCheckpointIdentity &,
        std::int64_t active_player,std::uint64_t maximum_definitions=1024*1024);
    ~BnwResourceArtTagReadOwner();
    BnwResourceArtTagReadOwner(const BnwResourceArtTagReadOwner &)=delete;
    BnwResourceArtTagReadOwner &operator=(const BnwResourceArtTagReadOwner &)=delete;
    std::shared_ptr<const BnwResourceArtTagReadReceipt> capture(const runtime::LiveSession &,
        std::span<const std::uint64_t> plot_indices,std::uint64_t frame_number);
    bool matches(const runtime::LiveSession &,
        const std::shared_ptr<const BnwResourceArtTagReadReceipt> &,std::uint64_t frame_number) const noexcept;
    bool visit_current_resource_facts(const runtime::LiveSession &,
        const std::shared_ptr<const BnwResourceArtTagReadReceipt> &,std::uint64_t frame_number,
        const std::function<void(std::span<const BnwResourceArtTagReadFacts>)> &) noexcept;
  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::ui
