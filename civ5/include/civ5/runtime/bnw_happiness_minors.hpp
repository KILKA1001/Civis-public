#pragma once
#include "civ5/runtime/bnw_happiness_resources.hpp"
#include <cstddef>
namespace civ5::runtime {
class BnwMinorHappinessRules {
 public:
    static BnwMinorHappinessRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return resources_.rules_sha256();}
    const BnwResourceHappinessRules &resources() const noexcept {return resources_;}
    const auto &minor_traits() const noexcept {return minors_;}
    std::int32_t trait(std::string_view type) const;
    std::int32_t friend_threshold() const noexcept {return friend_threshold_;}
    std::int32_t war_influence() const noexcept {return war_influence_;}
    std::int32_t era_band(std::int32_t era) const;
    std::int32_t flat(bool ally,std::int32_t band) const {return (ally?ally_flat_:friend_flat_).at(static_cast<std::size_t>(band));}
    std::int32_t luxury(bool ally,std::int32_t band) const {return (ally?ally_luxury_:friend_luxury_).at(static_cast<std::size_t>(band));}
 private:
    BnwResourceHappinessRules resources_;
    std::map<std::string,std::int32_t,std::less<>> minors_;
    std::int32_t industrial_=-1,medieval_=-1,friend_threshold_=0,war_influence_=0;
    std::array<std::int32_t,3> friend_flat_{},ally_flat_{},friend_luxury_{},ally_luxury_{};
};
struct BnwMinorHappinessTerm {
    std::int64_t minor=-1;std::int32_t trait=-1,era=-1,base_influence_times100=0,effective_influence=0;
    bool at_war=false,allied=false,friends=false;
    std::int32_t luxuries=0,flat=0,per_luxury=0,total=0;
};
struct BnwMinorHappinessQuote {std::vector<BnwMinorHappinessTerm> minors;std::int32_t total=0;};
// Native ID domain 22..62, alive player checks, cached ally, effective war
// influence, >= friend threshold and additive ally+friend terms. Native luxury
// availability is counted even if league-banned; no influence inference of ally.
BnwMinorHappinessQuote quote_bnw_happiness_from_minors(const LiveSessionState &,
    const BnwMinorHappinessRules &,const SessionCheckpointIdentity &,std::int64_t player);
}
