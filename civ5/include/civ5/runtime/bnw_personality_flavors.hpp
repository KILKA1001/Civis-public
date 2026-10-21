#pragma once
#include "civ5/saves/all_players.hpp"
#include "civ5/content/rules_database.hpp"
namespace civ5::runtime {
// Current serialized native FlavorManager arrays; never leader defaults.
// First-contact retains its saved/current equality checks through this helper.
const std::vector<std::int64_t> &checked_bnw_personality_flavors(
 const saves::SavedPlayerObservation &,const saves::SavedPlayerObservation &,
 const content::RulesDatabase &);
}
