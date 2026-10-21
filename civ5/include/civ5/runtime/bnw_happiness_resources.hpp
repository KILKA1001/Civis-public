#pragma once
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {
class BnwResourceHappinessRules {
  public:
    struct Resource { std::int32_t happiness=0,usage=0; };
    static BnwResourceHappinessRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return sha256_;}
    std::uint64_t resource_slots() const noexcept {return slots_;}
    const std::map<std::int64_t,Resource> &resources() const noexcept {return resources_;}
    const content::InfoTypeEntry *resolve(std::uint64_t hash) const {return types_.find_hash(hash);}
    std::int32_t minor_quantity_multiplier() const noexcept {return minor_quantity_;}
    std::int32_t minor_happiness_multiplier() const noexcept {return minor_happiness_;}
    std::int32_t variety_bonus() const noexcept {return variety_;}
  private:
    std::string sha256_;
    content::InfoTypeCatalog types_;
    std::uint64_t slots_=0;
    std::map<std::int64_t,Resource> resources_;
    std::int32_t minor_quantity_=0,minor_happiness_=0,variety_=0;
};
struct BnwLuxuryHappinessTerm {
    std::int64_t resource=-1;
    bool banned=false,retained_export=false;
    std::int32_t available=0,from_minors=0,base=0,after_minor_bonus=0,extra=0,total=0;
};
struct BnwResourceHappinessQuote {
    std::vector<BnwLuxuryHappinessTerm> luxuries;
    std::int32_t positive_resource_types=0,resource_happiness=0,variety=0,total=0;
};
// Source GetHappinessFromResources/GetHappinessFromResourceVariety with native
// luxury availability, minor quantity/bonus, export retention and league bans.
// Hash arrays are rebuilt on native zeroed slots, last row wins. Unknown
// mandatory resource identities refuse the quote. No happiness cache is written.
BnwResourceHappinessQuote quote_bnw_happiness_from_resources(
    const LiveSessionState &,const BnwResourceHappinessRules &,
    const SessionCheckpointIdentity &,std::int64_t player);
} // namespace civ5::runtime
