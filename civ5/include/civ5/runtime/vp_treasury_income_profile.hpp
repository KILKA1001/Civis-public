#pragma once
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
// VP's saved income inputs are owned by its GameCore module. They are not
// common player state and are absent from a BNW treasury import.
struct VpTreasuryIncomeProfile {
    std::int64_t internal_trade=0,incoming_espionage=0,jfd_minors=0;
    std::int64_t annexed_minors_times100=0,gold_per_export=0,gold_per_import=0;
    YieldValues exported,imported,from_minors,from_alliances,siphoned;
};
struct VpTreasuryIncomeQuote {
    std::int64_t internal_trade=0,vassal_taxes=0,minors=0,jfd_minors=0,annexed_minors=0,espionage=0;
};
inline VpTreasuryIncomeQuote &ensure_vp_treasury_income_quote(TreasuryIncomeQuote &quote){
    if(auto *state=quote.profile_extensions.find<VpTreasuryIncomeQuote>())return *state;
    return quote.profile_extensions.emplace<VpTreasuryIncomeQuote>();
}
inline const VpTreasuryIncomeQuote &vp_treasury_income_quote(const TreasuryIncomeQuote &quote){
    const auto *state=quote.profile_extensions.find<VpTreasuryIncomeQuote>();
    if(!state)throw TreasuryUnavailable("Missing VP treasury income quote");
    return *state;
}
inline VpTreasuryIncomeProfile &ensure_vp_treasury_income_profile(TreasuryPlayer &player){
    if(auto *state=player.profile_extensions.find<VpTreasuryIncomeProfile>())return *state;
    return player.profile_extensions.emplace<VpTreasuryIncomeProfile>();
}
inline const VpTreasuryIncomeProfile &vp_treasury_income_profile(const TreasuryPlayer &player){
    const auto *state=player.profile_extensions.find<VpTreasuryIncomeProfile>();
    if(!state)throw TreasuryUnavailable("Missing VP treasury income profile");
    return *state;
}
} // namespace civ5::runtime
