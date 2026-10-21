#pragma once
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {
class BnwPolicyHappinessRules {
  public:
    struct Policy {
        std::int32_t extra=0,per_city=0;
        std::int64_t blocked_branch=-1;
    };
    static BnwPolicyHappinessRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return sha256_;}
    const auto &policies() const noexcept {return policies_;}
    std::vector<bool> flags(const saves::TeamHashedArray &,bool branches) const;
  private:
    std::string sha256_;
    content::InfoTypeCatalog types_;
    std::uint64_t policy_slots_=0,branch_slots_=0;
    std::map<std::int64_t,Policy> policies_;
};
struct BnwPolicyCityHappinessTerm {
    std::int64_t city=-1;
    std::int32_t population=0,happiness=0;
};
// Native zero-initialized policy ownership followed by the saved hashed array.
// Restore all imported players atomically into the shared live policy owner.
// Already resolved owners cannot be reset by this import-only operation.
void restore_bnw_policy_membership(const saves::SavedSessionSnapshot &,
    ResearchContext &,const BnwPolicyHappinessRules &);
struct BnwPolicyHappinessQuote {
    std::int32_t owned_policies=0,active_policies=0,cities=0;
    std::int32_t flat=0,per_city_modifier=0,from_city_count=0;
    std::int32_t population_divisor=0,from_population=0,total=0;
    std::vector<BnwPolicyCityHappinessTerm> population_terms;
};
// CvPlayer::GetHappinessFromPolicies and CvPlayerPolicies numeric modifiers.
// Native free branch openers inherit the first branch containing FreePolicy;
// branch finishers without a PolicyBranchType do not inherit that mapping.
// Population division uses the current city owner and excludes puppets.
// Read-only: policy activation and aggregate happiness stores remain separate.
BnwPolicyHappinessQuote quote_bnw_happiness_from_policies(
    const LiveSessionState &,const BnwPolicyHappinessRules &,
    const SessionCheckpointIdentity &,std::int64_t player);
}
