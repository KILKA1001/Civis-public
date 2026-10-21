#pragma once
#include "civ5/runtime/bnw_sight_diplomacy_preflight.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include "civ5/runtime/research_selection_checkpoint.hpp"
#include "civ5/runtime/research_turn_checkpoint.hpp"
#include <utility>

namespace civ5::runtime {

struct BnwSightResearchClearResult {
    bool calculated=false;
    std::string reason;
    std::optional<LiveSessionState> detached;
    std::vector<std::int64_t> cleared_players;
    // All three companion sections must be written together if this branch
    // ever becomes publishable. They are only diagnostics here.
    std::optional<ResearchCheckpointSections> research_checkpoint;
    std::optional<saves::CivisSaveSection> turn_checkpoint,selection_checkpoint;
    // CvPlayer::setResearchingTech marks Popup dirty once per changed tech
    // when this player is active, before clearResearchQueue's three team bits.
    std::vector<std::pair<std::int64_t,std::int64_t>> popup_dirty_intents;
    std::vector<std::int64_t> active_team_dirty_intents;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvTeam::makeHasMet invokes this after its first-contact AI loop
// when the other team is human and an own living player is AI. CvPlayer.cpp
// 18791 clears queue, then all researching flags, then active-team UI bits.
// The earlier AI calls and later minor/UI/Lua effects are still mandatory;
// this detached effect cannot be published as a finished contact or setXY.
BnwSightResearchClearResult apply_bnw_sight_research_clear_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime
