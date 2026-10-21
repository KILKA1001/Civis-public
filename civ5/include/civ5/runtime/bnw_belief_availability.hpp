#pragma once

#include "civ5/content/rules_database.hpp"
#include "civ5/saves/game_religion_culture.hpp"

namespace civ5::runtime {

// Read-only CvGameReligions::GetAvailable*Beliefs quote for a fully decoded
// BNW religion roster and the selected, activated RulesDatabase.
// A quote is not a live popup event, a localized row list, or permission to
// adopt a belief. The caller must discard it after religion/rule changes.
enum class BnwBeliefCategory {
    pantheon, founder, follower, enhancer, bonus, reformation
};
struct BnwBeliefAvailability {
    bool resolved = false;
    std::vector<std::int64_t> belief_ids;
    std::string blocker;
};
using BnwPantheonBeliefAvailability = BnwBeliefAvailability;

BnwBeliefAvailability quote_bnw_available_beliefs(
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const saves::GameReligionCultureObservation &religions,
    BnwBeliefCategory category);

BnwPantheonBeliefAvailability quote_bnw_available_pantheon_beliefs(
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const saves::GameReligionCultureObservation &religions);

} // namespace civ5::runtime
