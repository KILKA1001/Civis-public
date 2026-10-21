#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/live_session.hpp"
#include <functional>
#include <memory>
#include <span>

namespace civ5::ui {
enum class BnwFeatureArtTagReadStatus {
    key, no_feature, unresolved_identity, null_info_slot, absent_type, absent_art_tag
};
// Logical Info read only: not visibility, placement, a renderer selection or a
// GameplayFeatureChanged notification. Source row ID can differ from Info slot.
struct BnwFeatureArtTagReadFacts {
    std::uint64_t plot_index=0,x=0,y=0;
    std::optional<std::int64_t> wire_feature;
    std::optional<std::uint64_t> wire_feature_hash,info_slot;
    std::optional<std::int64_t> authored_id;
    std::optional<std::string> type,art_define_tag;
    std::optional<bool> natural_wonder;
    BnwFeatureArtTagReadStatus status=BnwFeatureArtTagReadStatus::unresolved_identity;
    bool operator==(const BnwFeatureArtTagReadFacts &) const = default;
};
struct BnwFeatureArtTagReadStamp {
    runtime::SessionCheckpointIdentity source;
    std::uint64_t state_generation=0,frame_number=0,width=0,height=0;
    std::int64_t active_player=-1,active_team=-1,turn=0;
    bool wrap_x=false,wrap_y=false;
    std::array<std::uint8_t,16> map_guid{};
};
class BnwFeatureArtTagReadOwner;
class BnwFeatureArtTagReadReceipt {
  public:
    const BnwFeatureArtTagReadStamp &stamp() const noexcept { return stamp_; }
    std::span<const BnwFeatureArtTagReadFacts> plots() const noexcept { return plots_; }
  private:
    friend class BnwFeatureArtTagReadOwner;
    BnwFeatureArtTagReadReceipt(BnwFeatureArtTagReadStamp,
        std::vector<BnwFeatureArtTagReadFacts>,std::weak_ptr<const void>);
    BnwFeatureArtTagReadStamp stamp_;
    std::vector<BnwFeatureArtTagReadFacts> plots_;
    std::weak_ptr<const void> owner_;
};
// Explicit immutable preview Rules scope is required before construction.
// Snapshot uses existing prefetch_info/cache_info_types, never an ID lookup or
// a second rules service. Later reads retain SAME DB Impl currentness only;
// they do not dereference the database facade or the caller's type catalog.
// The retained host must keep this borrowed LiveSession facade alive throughout
// every call (including callbacks). A state lease alone does not retain it.
class BnwFeatureArtTagReadOwner {
  public:
    BnwFeatureArtTagReadOwner(const runtime::LiveSession &,const content::RulesDatabase &,
        const content::InfoTypeCatalog &,const runtime::SessionCheckpointIdentity &,
        std::int64_t active_player,std::uint64_t maximum_definitions=1024*1024);
    ~BnwFeatureArtTagReadOwner();
    BnwFeatureArtTagReadOwner(const BnwFeatureArtTagReadOwner &)=delete;
    BnwFeatureArtTagReadOwner &operator=(const BnwFeatureArtTagReadOwner &)=delete;
    std::shared_ptr<const BnwFeatureArtTagReadReceipt> capture(const runtime::LiveSession &,
        std::span<const std::uint64_t> plot_indices,std::uint64_t frame_number);
    bool matches(const runtime::LiveSession &,
        const std::shared_ptr<const BnwFeatureArtTagReadReceipt> &,std::uint64_t frame_number) const noexcept;
    // For a later existing art-registry join. Pins input/callable/Impl/state before a
    // callback; false on a source change, owner retirement, exception or reentry.
    // A retained receipt is still readable after owner retirement, never current.
    bool visit_current_art_keys(const runtime::LiveSession &,
        const std::shared_ptr<const BnwFeatureArtTagReadReceipt> &,std::uint64_t frame_number,
        const std::function<void(std::span<const BnwFeatureArtTagReadFacts>)> &) noexcept;
  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::ui
