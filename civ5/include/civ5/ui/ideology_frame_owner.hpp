#pragma once

#include "civ5/ui/ideology_layout.hpp"
#include "civ5/ui/live_session_frame.hpp"

namespace civ5::ui {

// The actual GameCore/Lua popup owner publishes ProgressionIdeologyPopupEvent
// through LiveSessionUiSources. Only while that event is active does this
// reader count alive players by their current GetLateGamePolicyTree branch.
// It must not infer a popup or adopter counts from a save or policy table.
using IdeologyAdopterReader = std::function<std::optional<std::vector<IdeologyAdopterCount>>(
    const runtime::LiveSession &, const ProgressionIdeologyPopupEvent &)>;
// Must call the equivalent of Game.GetNumFreePolicies(branch.ID) for each
// selected BNW ideology at the active popup event. Missing data is not zero.
using IdeologyFreePolicyReader = std::function<std::optional<std::vector<IdeologyFreePolicyCount>>(
    const runtime::LiveSession &, const ProgressionIdeologyPopupEvent &)>;
// This reader must enumerate the complete selected GameInfo.Policies result
// for the three ideology branches in level 1/2/3 query order and localize
// row.Help using the selected language. The presenter measures each Label.
// No reader means no tenet rows are published.
using IdeologyTenetHelpReader = std::function<std::optional<std::vector<IdeologyTenetHelpRow>>(
    const runtime::LiveSession &, const ProgressionIdeologyPopupEvent &)>;

// Wraps the ordinary live frame provider without changing its contract.
// The session must outlive the provider; the selected XML identity and reader
// are captured by value. Every
// frame revalidates session/player/turn/event/three activated branch IDs;
// missing or conflicting evidence leaves IdeologyLayoutEvidence absent.
LiveUiHost::FrameProvider make_bnw_ideology_layout_frame_provider(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    LiveSessionUiSources sources, const ProgressionScreenSource &selected_source,
    IdeologyAdopterReader adopters, IdeologyTenetHelpReader tenet_help = {},
    IdeologyFreePolicyReader free_policies = {});

} // namespace civ5::ui
