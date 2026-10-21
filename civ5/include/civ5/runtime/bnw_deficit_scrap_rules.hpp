#pragma once

#include "civ5/runtime/bnw_deficit_scrap.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/content/rules_database.hpp"

namespace civ5::runtime {

// Bind CvUnitEntry::GetProductionCost (Units.Cost), the refund define and the
// selected GameSpeeds.TrainPercent to exactly the payload and activated rules
// used by this owned BNW session. Reject stale ID/hash catalogs and define
// caches. This supplies a scrap draft only; it cannot complete DoDeficit.
BnwDeficitScrapRules load_bnw_deficit_scrap_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const ResearchContext &setup,
    const saves::LoadedSave &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
