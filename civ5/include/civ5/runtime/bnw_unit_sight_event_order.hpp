#pragma once
#include "civ5/runtime/bnw_unit_sight_exploration.hpp"

namespace civ5::runtime {

enum class BnwSightOrderedEffect : std::uint8_t {
    set_revealed,
    exploration_player_dirty,
    adjacent_owner_refresh,
    unit_meet_candidate,
    city_meet_candidate,
};

struct BnwSightOrderedEvent {
    BnwSightOrderedEffect effect=BnwSightOrderedEffect::set_revealed;
    std::uint64_t plot=0;
    // For exploration, player is the recipient; for a unit/city candidate it
    // is the object's owner. NO_PLAYER for the other event kinds.
    std::int64_t player=-1,object_id=-1;
    // Only adjacent_owner_refresh carries a second plot index.
    std::uint64_t adjacent_plot=0;
    bool first_reveal=false;
};

struct BnwSightEventOrderResult {
    bool calculated=false;
    std::string reason;
    // The only additional owned effect is EconomicAI exploration dirty from
    // the preceding detached stage. This remains a diagnostic snapshot.
    BnwSightExplorationResult exploration;
    std::vector<BnwSightOrderedEvent> events;
    std::size_t unit_meet_candidates=0,city_meet_candidates=0;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvPlot::changeAdjacentSight ring order and
// CvPlot::changeVisibilityCount -> setRevealed -> exploration -> six adjacent
// owner updates -> unit roster meets -> city meet. The trace records call
// candidates, not completed diplomacy or Lua. Natural-wonder first reveals
// fail closed in the preceding stage because their hook/rewards occur inside
// setRevealed before exploration. No new side effects are published here.
BnwSightEventOrderResult trace_bnw_unit_sight_event_order(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
