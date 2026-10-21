#pragma once

#include "civ5/runtime/bnw_deficit_selection.hpp"
#include "civ5/runtime/live_session.hpp"
#include <map>
#include <optional>
#include <span>

namespace civ5::runtime {

struct BnwDeficitLiveEvidence {
    // BNW CvMilitaryAI::Read does not serialize either defense state. Only an
    // already-run, current AI phase may attest these values for this player.
    bool military_ai_current=false;
    std::optional<std::int64_t> land_defense,naval_defense;
    // Saved CvPlot m_units and CvUnit coordinates are observations, not proof
    // that load hooks or subsequent actions preserved the live plot roster.
    bool plot_unit_roster_current=false;
    // CvUnit::m_combatCity is not serialized. Each occupied queried plot needs
    // an authoritative current answer for resident units unless a resolved
    // combat-unit handle already proves isFighting true.
    std::map<std::pair<std::int64_t,std::int64_t>,bool> combat_city_current;
};

// Expansion2 CvPlot.cpp:3861 isUnitFighting checks the current plot's units;
// CvUnit.cpp:9936 isFighting checks combat unit OR combat city. This binding
// verifies owned plot/unit membership and resolves the serialized combat-unit
// handle. Missing live-roster or combat-city evidence blocks a false answer.
bool bind_bnw_plot_unit_fighting(
    const LiveSessionState &state,std::int64_t x,std::int64_t y,
    const BnwDeficitLiveEvidence &evidence);

// Populate precisely the inputs consumed by choose_bnw_deficit_candidate.
// No BNW defense state is inferred from serialized strategy history or unit
// counts. Without current military AI evidence this function rejects.
BnwDeficitMilitaryContext bind_bnw_deficit_military_context(
    const LiveSessionState &state,std::int64_t player,
    std::span<const std::pair<std::int64_t,std::int64_t>> plots,
    const BnwDeficitLiveEvidence &evidence);

} // namespace civ5::runtime
