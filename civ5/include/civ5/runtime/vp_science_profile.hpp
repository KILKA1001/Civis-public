#pragma once
#include "civ5/runtime/science.hpp"

namespace civ5::runtime {
struct VpScienceSpy {
    bool alive=false;
    // Resolved through the plot-city relation, not a nearest-city heuristic.
    std::optional<std::pair<std::int64_t,std::int64_t>> city;
};
// Saved VP GameCore science caches. The common player holds none of these
// fields; BNW imports and queries do not require this extension.
struct VpScienceProfile {
    std::optional<std::uint64_t> source_policy_modifiers;
    std::optional<std::pair<std::uint64_t,std::uint64_t>> source_espionage;
    std::int64_t cached_happiness_total=0,last_turn_times100=0,vassal_modifier=0;
    std::int64_t per_trade_partner=0,jfd_minors=0,annexed_minors_times100=0;
    std::int64_t incoming_espionage=0,outgoing_espionage=0,passive_spy_modifier=0;
    std::vector<VpScienceSpy> spies;
    // City -> spy player -> saved science percentage.
    std::map<std::int64_t,YieldValues> passive_spy_percent;
};
// VP-specific breakdown of a science quote. The common result contains only
// components shared with BNW; VP's total still includes these components.
struct VpScienceQuote {
    std::int64_t vassals=0,jfd_minors=0,minors=0,annexed_minors=0;
    std::int64_t incoming_espionage=0,outgoing_espionage=0,passive_spies=0;
};
inline VpScienceQuote &ensure_vp_science_quote(ScienceQuote &quote){
    if(auto *state=quote.profile_extensions.find<VpScienceQuote>())return *state;
    return quote.profile_extensions.emplace<VpScienceQuote>();
}
inline const VpScienceQuote &vp_science_quote(const ScienceQuote &quote){
    const auto *state=quote.profile_extensions.find<VpScienceQuote>();
    if(!state)throw ScienceUnavailable("Missing VP science quote");
    return *state;
}
inline VpScienceProfile &ensure_vp_science_profile(SciencePlayer &player){
    if(auto *state=player.profile_extensions.find<VpScienceProfile>())return *state;
    return player.profile_extensions.emplace<VpScienceProfile>();
}
inline VpScienceProfile &vp_science_profile(SciencePlayer &player){
    auto *state=player.profile_extensions.find<VpScienceProfile>();
    if(!state)throw ScienceUnavailable("Missing VP science profile");
    return *state;
}
inline const VpScienceProfile &vp_science_profile(const SciencePlayer &player){
    const auto *state=player.profile_extensions.find<VpScienceProfile>();
    if(!state)throw ScienceUnavailable("Missing VP science profile");
    return *state;
}
} // namespace civ5::runtime
