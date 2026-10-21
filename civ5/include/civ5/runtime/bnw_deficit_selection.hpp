#pragma once

#include "civ5/runtime/simulation_random.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <utility>

namespace civ5::runtime {

enum class BnwDeficitDomain { land, sea, other };
struct BnwDeficitUnitInfo {
    BnwDeficitDomain domain=BnwDeficitDomain::other;
    std::int64_t intrinsic_power=0;
};
struct BnwDeficitRules {
    // Keys are the decoded BNW idUnitType hashes resolved against active SQL.
    std::map<std::uint32_t,BnwDeficitUnitInfo> units;
    std::int64_t max_hit_points=0;
    bool unit_delete_disabled=false;
};
struct BnwDeficitMilitaryContext {
    // CvMilitaryAI defense states are not serialized in the BNW strategy
    // block. Source enum: NO=-1, ENOUGH=0, NEUTRAL=1, NEEDED=2, CRITICAL=3.
    std::optional<std::int64_t> land_defense,naval_defense;
    // CvUnit::canScrap asks live CvPlot::isUnitFighting. The saved unit alone
    // cannot prove that condition, so every evaluated plot needs a binding.
    std::map<std::pair<std::int64_t,std::int64_t>,bool> plot_unit_fighting;
};
struct BnwDeficitCandidate {
    std::int64_t unit_id=-1;
    std::uint64_t source_index=0;
    std::int64_t power=0;
    BnwDeficitDomain domain=BnwDeficitDomain::other;
};
struct BnwDeficitSelection {
    std::int64_t combat_units=0,cities=0;
    bool random_drawn=false,random_allows_disband=false;
    std::optional<BnwDeficitCandidate> land,naval,selected;
};

// Pure selection preparation over the owned decoded BNW snapshot. The only
// mutation is one gameplay RNG draw when combat units exceed cities; a local
// RNG copy is committed only after all required fields/AI inputs validate.
// Source: Expansion2 CvPlayer.cpp:16848 DoDeficit and CvMilitaryAI.cpp:3487
// FindBestUnitToScrap(true /*bDeficitForcedDisband*/). No unit is scrapped,
// transferred or notified. This helper cannot complete DoDeficit/DoGold.
BnwDeficitSelection choose_bnw_deficit_candidate(
    const saves::SavedSessionSnapshot &snapshot,std::int64_t player,
    const BnwDeficitRules &rules,const BnwDeficitMilitaryContext &military,
    SimulationRandom &gameplay_random);

} // namespace civ5::runtime
