#pragma once
#include "civ5/runtime/bnw_unit_movement_query.hpp"

namespace civ5::ui {
// Detached selected-Info query; this is not a live presentation receipt.
// The optional values distinguish unavailable reads from known native zero.
// info_domain is UnitInfo.Domain; HOVER needs current-plot resolution for
// native domain-sensitive movement, which this secondary read does not grant.
struct BnwUnitPanelSecondaryInfo {
    std::int64_t unit_type=-1, info_domain=-1;
    std::optional<std::int32_t> range, base_ranged_combat, rebase_range_multiplier;
    std::string range_blocker, base_ranged_blocker, rebase_blocker;
};
BnwUnitPanelSecondaryInfo read_bnw_unit_panel_secondary_info(
    const content::RulesDatabase &, const content::InfoTypeCatalog &,
    std::uint32_t type_hash, std::int64_t current_extra_range);
struct BnwUnitPanelSecondaryStats {
    std::int64_t player_id=-1, unit_id=-1;
    std::int32_t turn=-1;
    std::uint64_t frame_number=0;
    std::string source_world_revision;
    BnwUnitPanelSecondaryInfo values;
};
// Borrows the same stable selected session/rules owner as UnitPanel. Caller
// supplies its current explicit selection; no unit is selected by this reader.
class BnwUnitPanelSecondaryStatsBridge {
  public:
    BnwUnitPanelSecondaryStatsBridge(const runtime::LiveSession &,
        const content::RulesDatabase &, const content::InfoTypeCatalog &,
        const runtime::SessionCheckpointIdentity &, std::int64_t active_player);
    std::optional<BnwUnitPanelSecondaryStats> capture(const runtime::LiveSession &,
        std::int64_t selected_unit, std::uint64_t frame) const;
    bool matches(const runtime::LiveSession &, const BnwUnitPanelSecondaryStats &,
        std::int64_t selected_unit, std::uint64_t frame) const;
  private:
    const runtime::LiveSession *session_;
    const runtime::LiveSessionState *state_;
    std::uint64_t state_generation_;
    const content::RulesDatabase *database_;
    const content::InfoTypeCatalog *types_;
    runtime::SessionCheckpointIdentity identity_;
    std::int64_t player_;
};
} // namespace civ5::ui
