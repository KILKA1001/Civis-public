#pragma once

#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/city_screens.hpp"
#include "civ5/ui/bnw_city_banner_appearance_types.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/minimap_tile_grid.hpp"
#include "civ5/ui/bnw_city_production_backend.hpp"
#include <functional>
#include <map>
#include <memory>
#include <optional>

namespace civ5::runtime { struct BnwGameActivationOwner; struct PregameRoster; }

namespace civ5::ui {

class BnwOwnedNotificationPublicationOwner;
struct BnwEngineUiNotificationEffect;
// Closed pure value composition, never an acceptance/presentation issuer.
class LiveNotificationFrameMerge final {
 friend class BnwOwnedNotificationPublicationOwner;
 static UiFrameSnapshot merge(const UiFrameSnapshot&,
  const std::vector<BnwEngineUiNotificationEffect>&,const std::vector<std::int64_t>&);
};
class BnwNotificationPopupOwner;
struct BnwNotificationActivationServices;

struct LiveCityBannerAppearances {
    std::map<std::pair<std::int64_t,std::int64_t>, BnwCityBannerAppearance> cities;
};

struct LiveSessionUiQuotes {
    std::map<std::int64_t, runtime::ScienceQuote> science;
    std::map<std::int64_t, runtime::TreasuryNetQuote> gold;
    std::map<std::int64_t, TopPanelTradeQuote> trade;
    std::map<std::int64_t, TopPanelTourismQuote> tourism;
    std::map<std::int64_t, TopPanelFaithQuote> faith;
    std::map<std::int64_t, TopPanelNativeBreakdown> bnw_native_breakdowns;
};

// Published only by the current GameCore/Lua notification owner. The saved
// 100-slot notification array is not a post-load event queue.
struct LiveWorldViewNotificationFeed {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::vector<WorldViewNotification> events;
};

// Only the current GameCore/Lua popup owner may publish this queue. Saved
// popup slots and XML templates do not identify post-load modal events.
struct LivePopupEventFeed {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::vector<PopupEvent> events;
};

struct LiveWorldViewPlotIcon {
    std::uint64_t plot = 0;
    std::int64_t type = -1;
    std::int64_t value = 0;
    // The camera/active icon atlas owner supplies screen position and crop.
    WorldViewIcon icon;
};
struct LiveWorldViewPlotFeed {
    std::int64_t player_id = -1, team_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame_number = 0;
    std::optional<std::pair<std::uint64_t, std::string>> hovered_plot;
    std::vector<LiveWorldViewPlotIcon> resources, yields;
};
struct LiveWorldViewUnitActionFeed {
    std::int64_t player_id = -1, unit_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame_number = 0;
    std::uint64_t owner_sequence = 0;
    std::vector<WorldViewActionItem> actions;
};
// A resolved, read-only portrait from the current UI owner. The frame accepts
// it only for the same selected player/unit/turn/frame and after roster checks.
struct LiveWorldViewUnitPortraitFeed {
    std::int64_t player_id = -1, unit_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame_number = 0;
    WorldViewUnitPortrait portrait;
};
// Actual native queries for the already selected live unit. Unknown branches
// remain absent and carry their own reason; strings are never default zeros.
struct LiveWorldViewUnitReadStatsFeed {
    std::int64_t player_id=-1, unit_id=-1;
    std::int32_t turn=-1;
    std::uint64_t frame_number=0;
    std::string source_world_revision;
    std::optional<std::string> movement, strength;
    std::string movement_blocker, strength_blocker;
};
// Read-only visibility of selected UnitPanel frames. A receipt or current
// GameCore owner must bind it to the same selected unit and captured frame.
struct LiveWorldViewUnitStatBoxesFeed {
    std::int64_t player_id = -1, unit_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame_number = 0;
    WorldViewUnitStatBoxes boxes;
};
struct LiveWorldViewInfoCornerFeed {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    std::vector<WorldViewActionItem> items;
};
// Current GameMenu.lua OnShowHide result. A saved multiplayer bit or mod list
// cannot establish the selected QuickSaveButton's current visibility.
struct LiveGameMenuQuickSaveFeed {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame_number = 0, owner_sequence = 0;
    bool visible = false;
};
struct LiveGameMenuLoadGameFeed {
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t frame_number = 0, owner_sequence = 0;
    bool visible = false;
};

// Reconcile authored 2D plot events with the current logical map and fog.
// Read-only GameCore queries must confirm visible resources and current
// yields; a saved plot cache by itself is not enough to publish an icon.
WorldViewState apply_live_world_view_plot_feed(WorldViewState state,
    const saves::SavedMapProjection &map, const LiveWorldViewPlotFeed &feed,
    std::uint64_t frame_number, std::int32_t turn,
    const std::function<std::optional<std::int64_t>(std::uint64_t,
                                                   std::int64_t)> &resource_type,
    const std::function<std::optional<std::int64_t>(std::uint64_t,
                                                   std::int64_t)> &plot_yield);

struct LiveSessionUiSources {
    // The caller owns the activated BNW rules for the lifetime of the provider.
    // With either pointer absent, progression state is absent from the frame.
    const content::TechnologyRules *technologies = nullptr;
    const content::RulesDatabase *database = nullptr;
    content::InfoLocalizer localize_info;
    std::function<std::optional<std::string>(std::string_view)> localize_city;
    // Called afresh for each frame. Empty means no city selected / no completed
    // read-only quotes. A quote provider must never mutate LiveSession.
    std::function<std::optional<std::int64_t>()> selected_city;
    // The UI selection owner supplies an ID, never a decoded save default.
    // The frame resolves it against the current owned unit, maintenance and
    // plot rosters before a read-only UnitPanel is published.
    std::function<std::optional<std::int64_t>()> selected_unit;
    std::function<std::optional<LiveWorldViewUnitPortraitFeed>(
        const runtime::LiveSession &, std::uint64_t)> selected_unit_portrait;
    std::function<std::optional<LiveWorldViewUnitStatBoxesFeed>(
        const runtime::LiveSession &, std::uint64_t)> selected_unit_stat_boxes;
    // GameCore/Lua action owner supplies the current selected unit's actual
    // menu. Availability still requires the matching gameplay backend.
    // The GameCore/Lua action list must be captured for this exact UI frame.
    std::function<std::optional<LiveWorldViewUnitActionFeed>(
        const runtime::LiveSession &, std::uint64_t)>
        selected_unit_actions;
    std::function<std::optional<LiveWorldViewInfoCornerFeed>(const runtime::LiveSession &)>
        info_corner_items;
    std::function<std::optional<LiveGameMenuQuickSaveFeed>(
        const runtime::LiveSession &, std::uint64_t)> game_menu_quick_save;
    // Current GameMenu.lua OnShowHide result, not a reconstructed save flag.
    std::function<std::optional<LiveGameMenuLoadGameFeed>(
        const runtime::LiveSession &, std::uint64_t)> game_menu_load_game;
    std::function<std::optional<LiveWorldViewNotificationFeed>(const runtime::LiveSession &)>
        active_notifications;
    // Called for each frame. Its event order is the current modal queue order;
    // missing, stale or malformed owner data publishes no PopupFrame.
    std::function<std::optional<LivePopupEventFeed>(const runtime::LiveSession &)>
        active_popups;
    // Renderer/hover owner publishes only current camera positions and text
    // from the active BNW UI. These callbacks perform live gameplay queries;
    // absent callbacks keep resource/yield icons unavailable.
    std::function<std::optional<LiveWorldViewPlotFeed>(const runtime::LiveSession &,
                                                       std::uint64_t)> plot_feed;
    std::function<std::optional<std::int64_t>(const runtime::LiveSession &,
        std::uint64_t, std::int64_t)> visible_resource_type;
    std::function<std::optional<std::int64_t>(const runtime::LiveSession &,
        std::uint64_t, std::int64_t)> current_plot_yield;
    // Frame number lets the trade-capacity owner attest the exact frame.
    std::function<LiveSessionUiQuotes(const runtime::LiveSession &,
                                      std::uint64_t)> quotes;
    // The current LiveSession does not own GameCore/Lua popup events. The
    // event owner returns the active chooser only while it is actually open.
    // Its research context must borrow this same LiveSession's owned state;
    // absent or mismatched context yields an unavailable chooser, never
    // fabricated candidates.
    std::function<std::optional<ProgressionTechPopupEvent>(const runtime::LiveSession &)>
        active_tech_popup;
    // Only the GameCore/Lua popup owner can identify a current ideology
    // choice. Missing or zero-token events never become selectable UI rows.
    std::function<std::optional<ProgressionIdeologyPopupEvent>(const runtime::LiveSession &)>
        active_ideology_popup;
    // Preferred read-only candidate path for a real chooser event. The
    // detached session copy lets the BNW research checks observe current
    // queue/player data without exposing mutable published state to the UI.
    const runtime::BnwResearchChoiceRules *research_choice_rules = nullptr;
    const runtime::BnwResearchChoiceServices *research_choice_services = nullptr;
    // Legacy borrowed-context adapter for owners that already hold a live
    // ResearchSelectionContext. A mismatched context is rejected.
    std::function<runtime::ResearchSelectionContext *(const runtime::LiveSession &)>
        research_selection;
    std::function<std::optional<BnwCityBannerAppearance>(const runtime::LiveSession &,
        std::int64_t active_player, std::int64_t city_player, std::int64_t city_id,
        std::uint64_t frame)> city_banner_appearance;
    std::function<bool(const runtime::LiveSession &, const BnwCityBannerAppearance &)>
        city_banner_appearance_current;
    const content::RulesDatabase *unit_stats_database = nullptr;
    const runtime::SessionCheckpointIdentity *unit_stats_identity = nullptr;
    std::function<std::optional<LiveWorldViewUnitReadStatsFeed>(
        const runtime::LiveSession &, std::uint64_t, std::int64_t)> selected_unit_read_stats;
    std::function<bool(const runtime::LiveSession &, const LiveWorldViewUnitReadStatsFeed &,
        std::int64_t, std::uint64_t)> selected_unit_read_stats_current;
    // Optional current-read validator for captured native quotes. Source owners
    // are checked again before composition, input and presentation.
    std::function<bool(const runtime::LiveSession &,const LiveSessionUiQuotes &,
        std::uint64_t)> quotes_current;
    const content::InfoTypeCatalog *unit_stats_types = nullptr;
    // Source current selected UnitPanel reads; numeric locale composition may
    // remain unavailable. No paint/input state is established by the callback.
    std::function<std::optional<BnwUnitPanelReadRows>(const runtime::LiveSession &,
        std::uint64_t, std::int64_t)> selected_unit_stat_rows;
    std::function<bool(const runtime::LiveSession &, const BnwUnitPanelReadRows &,
        std::int64_t, std::uint64_t)> selected_unit_stat_rows_current;
    // Runs after base capture and before immutable client publication.
    std::function<UiFrameSnapshot(const runtime::LiveSession &, UiFrameSnapshot)>
        augment_snapshot;

};

// The frontend must supply the actual local active player ID. LiveSession does
// not own UI selection. Each call reads current game_turn and owned live state;
// saved source values are used only for matching city names.
UiFrameSnapshot capture_live_session_ui_frame(const runtime::LiveSession &session,
    std::uint64_t frame_number, std::int64_t active_player_id,
    const LiveSessionUiSources &sources = {});

// Clear the minimap's initial fog only for owned, revealed or currently
// visible plots. The live map must come from the current session cache;
// hidden plots retain their fog bit. This does not synthesize minimap terrain
// records or projection coordinates.
std::size_t apply_live_bnw_minimap_fog(MinimapTileGrid &grid,
                                      const WorldViewMapVisibility &visibility);

// A direct FrameProvider for LiveUiHost. The session, rules and any captured
// callbacks must outlive it; frame numbers increase with each invocation.
LiveUiHost::FrameProvider make_live_session_frame_provider(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    LiveSessionUiSources sources = {});

// Borrow a restored BNW session and its activated production rules for a real
// preview run. The binding owns the typed backend and frame provider; the game
// owner must keep the session, rules and callback captures alive throughout it.
struct LiveBnwPreviewInput {
    runtime::LiveSession &session;
    std::int64_t active_player_id;
    const runtime::ProductionProgressRules &production_rules;
    LiveSessionUiSources frame_sources;
    runtime::BnwProductionOrderServices production_services;
    BnwProductionPreview production_preview;
    BnwNotificationPopupOwner *notification_owner = nullptr;
    std::shared_ptr<BnwNotificationActivationServices> notification_services;
    // Identifies the restored world and activated rules for native scene owners.
    // Synthetic UI inputs may omit it; they cannot establish a fog baseline.
    std::optional<runtime::SessionCheckpointIdentity> scene_identity;
    // Same enclosing source transaction owner as the borrowed live session.
    // A renderer installs its actual native journal here; no readiness is granted.
    runtime::BnwGameActivationOwner *activation_owner = nullptr;
    // Actual persistent roster shared with the host's Lua PreGame services.
    const runtime::PregameRoster *pregame_roster = nullptr;
};
struct LiveBnwPreviewBinding {
    LiveUiHost::FrameProvider frames;
    std::shared_ptr<BnwCityProductionUiBackend> backend;
};
LiveBnwPreviewBinding bind_live_bnw_preview(LiveBnwPreviewInput input);

} // namespace civ5::ui
