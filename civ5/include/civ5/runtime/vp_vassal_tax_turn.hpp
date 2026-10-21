#pragma once
#include "civ5/runtime/treasury_turn.hpp"

namespace civ5::runtime {
// VP CvTeam::m_aiVassalTax is selected GameCore state. The shared roster and
// BNW worlds have no required tax-rate field or source array width.
struct VpVassalTaxProfileState {
    std::map<std::int64_t,std::map<std::int64_t,std::int64_t>> rates_by_master_team;
};
VpVassalTaxProfileState &ensure_vp_vassal_tax_profile(ResearchContext &world);
const VpVassalTaxProfileState *find_vp_vassal_tax_profile(const ResearchContext &world);
void restore_vp_vassal_tax_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world);

struct VpVassalTaxTurnResult {
    bool vassal=false;
    std::int64_t master=-1,gross_times100=0,rate=0,expense_times100=0;
};
// VP CvPlayer::doTurn calls this after DoGold and before doResearch. Recompute
// the live expense from gross gold, leaving other players' taxes untouched.
VpVassalTaxTurnResult do_vp_vassal_tax_after_gold(TreasuryTurnContext &context,std::int64_t player);
} // namespace civ5::runtime
