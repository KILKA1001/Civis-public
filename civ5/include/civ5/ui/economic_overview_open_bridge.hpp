#pragma once

#include "civ5/ui/economic_overview_lifecycle.hpp"

namespace civ5::ui {

struct EconomicGoldSourceProof {
    bool verified = false;
    std::string top_panel_lua_path, top_panel_lua_sha256;
    std::string top_panel_xml_path, top_panel_xml_sha256;
    std::string economic_lua_path, economic_lua_sha256;
    std::string blocker;
};

// Verify the selected TopPanel GoldPerTurn callback and its EconomicOverview
// listener through the active VFS. This proves authored source order only;
// native Lua registration and PopupShown still need their own owner.
EconomicGoldSourceProof inspect_economic_gold_source(
    const assets::ResourceMountTable &resources);

struct EconomicGoldClick {
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame = 0, owner_sequence = 0;
    std::int32_t viewport_width = 0, viewport_height = 0;
    OverviewRect gold_button;
    std::string top_panel_lua_sha256, top_panel_xml_sha256;
};

struct EconomicGoldClickResult {
    bool consumed = false;
    EconomicPopupQueueStatus status = EconomicPopupQueueStatus::blocked;
    std::string blocker;
};

// The presenter supplies a measured GoldPerTurn rectangle and current click
// sequence. This Civis-side typed route performs the authored popup event in
// EconomicOverviewLifecycle; it does not claim native Lua callback execution.
class EconomicOverviewOpenBridge {
  public:
    EconomicOverviewOpenBridge(const runtime::LiveSession &session,
        std::int64_t active_player_id, EconomicOverviewLifecycle &lifecycle,
        EconomicGoldSourceProof source);

    EconomicGoldClickResult click(const runtime::LiveSession &source_session,
        const UiFrameSnapshot &frame, const EconomicGoldClick &measurement,
        std::int32_t x, std::int32_t y);

  private:
    const runtime::LiveSession *session_;
    std::int64_t active_player_id_;
    EconomicOverviewLifecycle *lifecycle_;
    EconomicGoldSourceProof source_;
    std::uint64_t last_attempted_sequence_ = 0;
};

} // namespace civ5::ui
