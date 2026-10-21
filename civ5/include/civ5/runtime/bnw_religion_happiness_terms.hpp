#pragma once
#include "civ5/runtime/bnw_happiness_receipt_types.hpp"
#include "civ5/runtime/bnw_founder_religion_native.hpp"
namespace civ5::runtime {
struct BnwBeliefHappinessTerms {
    std::int32_t player_happiness=0,peaceful_foreign_divisor=0;
    float per_following_city=0;
};
class BnwReligionHappinessRules {
 public:
    static BnwReligionHappinessRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return rules_sha256_;}
    std::uint64_t belief_slots() const noexcept {return slots_;}
    std::vector<std::int64_t> resolved_beliefs(const saves::ReligionObservation &) const;
    BnwBeliefHappinessTerms terms(const saves::ReligionObservation &,bool at_peace) const;
 private:
    BnwReligionHappinessRules()=default;
    struct Entry {std::int32_t player_happiness=0,divisor=0;float following=0;bool requires_peace=false;};
    std::map<std::int64_t,Entry> entries_;
    content::InfoTypeCatalog types_;
    std::uint64_t slots_=0;
    std::string rules_sha256_;
};
// Current native roster, at-war flags and actual serialized religion status
// lists reconciled with the existing city runtime. No cached majority assumed.
class BnwReligionFollowersReadModel {
 public:
    static BnwReligionFollowersReadModel bind(const LiveSessionState &);
    bool at_peace(std::int64_t player) const;
    std::int32_t cities_following(std::int32_t religion) const;
    std::int32_t foreign_cities_following(std::int64_t player,std::int32_t religion) const;
    std::int32_t foreign_followers(std::int64_t player,std::int32_t religion) const;
    std::int32_t peaceful_foreign_followers(std::int64_t player,std::int32_t religion) const;
 private:
    BnwReligionFollowersReadModel()=default;
    struct Player {bool alive=false;std::int64_t team=-1;std::vector<std::vector<CityYieldReligion>> cities;};
    std::vector<Player> players_;
    std::vector<bool> alive_teams_;
    std::vector<std::vector<bool>> at_war_;
    bool at_war(std::int64_t team,std::int64_t other) const;
};
struct BnwReligionHappinessTermsQuote {
    BnwReligionHappinessQuote query;
    std::optional<BnwReligionHappinessTerms> terms;
};
// All three GetHappinessFromReligion terms, in SDK order, after the mandatory
// selected Lua gate. A positive foreign divisor requires a SECOND founder
// accumulator in the same mutable VM/world epoch, never reuse of its first ID.
// Rules identity and every required native input remain mandatory. No publish.
BnwReligionHappinessTermsQuote quote_bnw_happiness_from_religion_terms(
    const LiveSessionState &,const SavedContentProfile &,
    const SessionCheckpointIdentity &,BnwGameplayLuaEpochOwner &,
    const BnwReligionHappinessRules &,std::int64_t player);
}
