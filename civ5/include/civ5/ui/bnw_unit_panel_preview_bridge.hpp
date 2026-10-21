#pragma once
#include "civ5/ui/live_session_frame.hpp"
#include "civ5/runtime/bnw_unit_movement_query.hpp"

namespace civ5::ui {
// Stable selected-host borrowing. No selection, command, turn, model or cache
// mutation is performed. A replaced LiveSessionState requires explicit rebind.
class BnwUnitPanelPreviewBridge {
  public:
    BnwUnitPanelPreviewBridge(const runtime::LiveSession &, const content::RulesDatabase &,
        const content::InfoTypeCatalog &, const runtime::SessionCheckpointIdentity &, std::int64_t active_player);
    std::optional<LiveWorldViewUnitReadStatsFeed> capture(const runtime::LiveSession &,
        std::int64_t selected_unit, std::uint64_t frame) const;
    bool matches(const runtime::LiveSession &, const LiveWorldViewUnitReadStatsFeed &,
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
