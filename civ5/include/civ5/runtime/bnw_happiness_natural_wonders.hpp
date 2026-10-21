#pragma once
#include "civ5/runtime/live_session.hpp"
namespace civ5::runtime {
class BnwNaturalWonderHappinessRules {
  public:
    static BnwNaturalWonderHappinessRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return sha256_;}
    std::int32_t per_discovered_wonder() const noexcept {return per_wonder_;}
    std::int32_t feature_happiness(const saves::SavedMapCellProjection &) const;
  private:
    std::string sha256_;
    content::InfoTypeCatalog types_;
    std::map<std::int64_t,std::int32_t> features_;
    std::int32_t per_wonder_=0;
};
struct BnwNaturalWonderHappinessTerm {
    std::uint64_t plot=0;
    std::int32_t base=0,after_trait=0;
};
struct BnwNaturalWonderHappinessQuote {
    std::int32_t discovered_count=0,discovered_base=0,discovered_after_trait=0;
    std::int32_t owned_features=0,total=0;
    std::vector<BnwNaturalWonderHappinessTerm> owned_positive_features;
};
// CvPlayer::GetHappinessFromNaturalWonders: team discoveries use the trait's
// happiness modifier; each positive owned feature uses its separate yield
// modifier. No happiness/visibility/ownership cache is changed by this query.
BnwNaturalWonderHappinessQuote quote_bnw_happiness_from_natural_wonders(
    const LiveSessionState &,const BnwNaturalWonderHappinessRules &,
    const SessionCheckpointIdentity &,std::int64_t player);
}
