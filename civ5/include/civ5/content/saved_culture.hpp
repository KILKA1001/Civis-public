#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/game_religion_culture.hpp"

namespace civ5::content {
// BNW GreatWork versions 1/2 omit the class and look it up in the active
// rules. Preserve an explicit saved class (including -1). Missing lookup
// remains unresolved; SQL/schema errors propagate to the profile loader.
// This does not verify that rules correspond to the saved content profile.
std::optional<std::int64_t> saved_great_work_class(const RulesDatabase &rules,
                                                   const saves::GreatWorkObservation &work);
} // namespace civ5::content
