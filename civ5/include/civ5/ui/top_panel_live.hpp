#pragma once

#include "civ5/ui/top_panel.hpp"
#include "civ5/ui/ui_runtime.hpp"
#include "civ5/runtime/science.hpp"
#include <map>
#include <set>

namespace civ5::ui {

// Assets/DLC/Expansion2/UI/InGame/TopPanel.xml owns the
// InternationalTradeRoutes TextButton. Its matching TopPanel.lua reads
// GetNumInternationalTradeRoutesUsed and GetNumInternationalTradeRoutesAvailable
// for the label; its tooltip also
// reads available land/sea trade units, their unit descriptions and the two
// GetTrade*RoutesTTString results. The owned route list proves only "used".
// A complete quote needs a current GameCore owner for the remaining values.
// Empty route text means no routes in that direction; absent quotes never mean
// zero. Unit descriptions are localization keys from the active ruleset.
struct TopPanelTradeQuote {
    bool details_resolved = false;
    int used = 0, available = 0;
    int idle_land_units = 0, idle_sea_units = 0;
    std::string land_unit_description_key, sea_unit_description_key;
    std::string established_by_player, established_by_other;
    // The mechanic owner binds the quote to the same UI frame and player.
    std::int64_t owner_player_id = -1, owner_turn = -1;
    std::uint64_t owner_frame_number = 0;
};
struct TopPanelTourismQuote {
    // These fields can be quoted independently of the influence calculation.
    // A default zero is not evidence that the live owner resolved a field.
    bool per_turn_resolved = false, great_works_resolved = false;
    // Retained full-quote flag: it attests the rate, work slots, victory
    // option and influence counts together for the retail tooltip.
    bool influence_resolved = false;
    int per_turn = 0, great_works = 0, great_work_slots = 0;
    bool cultural_victory_enabled = false;
    int influential_civilizations = 0, required_civilizations = 0;
};
struct TopPanelFaithQuote {
    // Balance, rate and source breakdown have distinct live owners. The
    // progression quote is complete only after the religion decision tree.
    bool balance_resolved = false, per_turn_resolved = false, sources_resolved = false;
    // Retained full-quote flag: it attests all fields needed by the retail
    // faith tooltip, including anarchy, sources and progression text.
    bool progression_resolved = false;
    int balance = 0, per_turn = 0;
    int from_cities = 0, from_minors = 0, from_religion = 0;
    int anarchy_turns = 0;
    // The religion owner computes pantheon, prophet, religion-slot and
    // faith-purchasable-person branches from its current turn state.
    std::string progression_text;
};

// Values in this payload are read from the live game owner at a frame boundary.
// Each displayed component requires its own attested source; zero is never
// used to stand for missing data.
// Additional BNW tooltip inputs, bound to the same captured native queries.
struct TopPanelNativeBreakdown {
    std::int64_t city_science_without_trade_times100 = 0;
    std::int64_t city_gold_without_trade_times100 = 0;
    std::int32_t anarchy_turns = 0, technology_city_cost_modifier = 0;
    std::int64_t owner_player_id = -1;
    std::int32_t owner_turn = -1;
    std::uint64_t owner_frame_number = 0;
};

struct TopPanelLivePlayer {
    TopPanelPlayerState values;
    std::optional<int> city_count;
    // Treasury balance is owned even when the complete per-turn income and
    // expense quote is not. Keep that narrower provenance explicit.
    std::optional<int> gold_balance;
    // Counted from this session's owned CvGameTrade routes. The maximum route
    // capacity, unit details and route text have separate owners.
    std::optional<int> outgoing_trade_routes;
    std::set<TopPanelMetric> available;
    std::optional<TopPanelTradeQuote> trade;
    std::optional<TopPanelTourismQuote> tourism;
    std::optional<TopPanelFaithQuote> faith;
    std::optional<TopPanelNativeBreakdown> native_breakdown;
    std::optional<runtime::TreasuryNetQuote> native_gold;
    std::optional<runtime::ScienceQuote> native_science;
};

struct TopPanelLiveFrame {
    std::map<std::int64_t, TopPanelLivePlayer> players;
    std::optional<std::string> turn_date;
    bool expansion2_ui = true;
    bool no_science = false;
    bool no_happiness = false;
    bool no_policies = false;
    bool no_religion = false;
    bool no_basic_help = false;
    bool head_selected_city = false;
    bool city_screen_up = false;
};

// Capture the authoritative owned runtime state and completed, read-only
// mechanic quotes. Missing quotes keep their metrics unavailable. The caller
// publishes the result in UiFrameSnapshot at the same frame boundary as the
// active player and turn; a saved projection is never reapplied after turns.
TopPanelLiveFrame capture_top_panel_live(
    const runtime::ResearchContext &game, const runtime::CityYieldContext &cities,
    const runtime::TreasuryContext &treasury,
    const std::map<std::int64_t, runtime::ScienceQuote> &science,
    const std::map<std::int64_t, runtime::TreasuryNetQuote> &gold,
    std::optional<std::string> turn_date = std::nullopt);

// A live frame is selected by the active player ID, never by player position.
// Missing data remains visibly unavailable; it cannot inherit preview values.
std::optional<TopPanelPresentation> present_top_panel_frame(
    const UiFrameSnapshot &frame, const TopPanelTextLocalizer &localizer);

// Navigation uses the shared UI controller. No action is reported as performed
// until that controller accepts it. City exit closes only an open city screen.
UiCommandResult dispatch_top_panel_action(TopPanelAction action, UiScreenController &controller,
                                           const UiFrameSnapshot &frame);

} // namespace civ5::ui
