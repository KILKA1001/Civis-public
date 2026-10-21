#pragma once

#include "civ5/runtime/live_session.hpp"
#include <map>

namespace civ5::runtime {

struct BnwSettlerUnitRule {
    std::uint32_t hash=0;
    std::int64_t default_ai=-1;
};
struct BnwSettlerDemandRules {
    bool resolved=false;
    std::int64_t settle_ai=-1;
    std::map<std::int64_t,BnwSettlerUnitRule> units;
};
struct BnwSettlerDemandEvidence {
    // No original BNW save can attest freshness after load hooks or actions.
    bool unit_roster_current=false;
    bool city_roster_current=false;
    bool production_queues_current=false;
};
struct BnwSettlerDemand {
    std::int64_t cities=0, existing_settlers=0, being_trained=0;
    std::int64_t total_settlers=0;
};

// Expansion2 CvMilitaryAI.cpp:2153 calls getNumCities and
// GetNumUnitsWithUnitAI(UNITAI_SETTLE,true). Bind UNITAI_SETTLE and each
// UnitEntry::DefaultUnitAI to the selected active BNW SQL/catalog.
BnwSettlerDemandRules load_bnw_settler_demand_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

// Cross-check the owned city and unit collections against their decoded
// identities and imported production queues. Reject absent freshness
// evidence; this only supplies two UpdateBaseData inputs, not the reserve.
BnwSettlerDemand bind_bnw_settler_demand(
    const LiveSessionState &state,std::int64_t player,
    const BnwSettlerDemandRules &rules,
    const BnwSettlerDemandEvidence &evidence);

} // namespace civ5::runtime
