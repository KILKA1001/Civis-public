#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"

namespace civ5::runtime {

// Pure source numerical input, NOT a retained/session/UI completion issuer.
// Detached diagnostic callers may exercise schema/last-wins/overflow cases.
// Only the actual cursor binds this SAME implementation to actual Host rules
// and restored live players; no returned scalar can be supplied to operation8.
std::int32_t compute_bnw_deal_resource_total_diagnostic(
    const LiveSessionState&,const content::RulesDatabase&,
    const content::InfoTypeCatalog&,const content::GlobalDefineCache&,
    std::int64_t player,std::int64_t resource);

// CvGameDeals::DoTurn's source-ordered safe branch: empty current list, or
// nonempty list with no expiring item and no reachable resource-shortage
// query. DoUpdateCurrentDealsList then migrates completed deals to history.
// An expiring item requires DoEndTradedItem and UI publication; resource
// validity requires live getNumResourceTotal. Both reject before mutation.
BnwTurnHandler make_bnw_game_deals_handler();

} // namespace civ5::runtime
