#pragma once
#include "civ5/runtime/bnw_happiness_policies.hpp"
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwPolicyAdoptionPending;
class BnwNativeUnitAllocatorCore;
class BnwBuildingHappinessRules {
 public:
    static BnwBuildingHappinessRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return sha256_;}
    const BnwPolicyHappinessRules &policies() const noexcept {return policies_;}
    std::int32_t building_count(std::int32_t real,std::int32_t free) const;
    const CityYieldRules &replacements() const noexcept {return replacements_;}
    const auto &building_modifiers() const noexcept {return buildings_;}
    const auto &policy_modifiers() const noexcept {return policy_;}
    const auto &classes() const noexcept {return classes_;}
    const content::InfoTypeEntry *resolve(std::uint64_t hash) const {return types_.find_hash(hash);}
 private:
    BnwPolicyHappinessRules policies_;
    CityYieldRules replacements_;
    content::InfoTypeCatalog types_;
    std::string sha256_;
    std::int32_t city_max_=0;
    std::set<std::int64_t> classes_;
    std::map<std::int64_t,YieldValues> buildings_,policy_;
};
struct BnwLocalCityHappinessTerm {
    std::int64_t city=-1;
    std::int32_t base=0,policy=0,cap=0,total=0,unmodded=0;
};
struct BnwBuildingClassHappinessTerm {
    std::int64_t generator=-1,target=-1;
    std::int32_t target_count=0,per_target=0,total=0;
};
struct BnwBuildingHappinessQuote {
    std::vector<BnwLocalCityHappinessTerm> cities;
    std::vector<BnwBuildingClassHappinessTerm> class_terms;
    std::int32_t local=0,global_classes=0,global_unmodded=0,per_policies=0,global=0;
};
// SDK GetLocalHappiness/GetHappinessFromBuildings, using exact native building
// cache getters and reconciled current free/real counts. Religion with a majority
// and positive garrison modifiers require their native owners and are rejected;
// they are not treated as zero. These read-only terms never update world caches.
BnwBuildingHappinessQuote quote_bnw_happiness_from_buildings(
    const LiveSessionState &,const BnwBuildingHappinessRules &,
    const SessionCheckpointIdentity &,std::int64_t player);
namespace detail {
// Supplied arithmetic only; never a current-world issuer or stage permission.
enum class BnwBuildingHappinessReachedStage {local_cities,global_buildings};
std::int32_t bnw_happiness_buildings_reached_math(const LiveSessionState&,
 const BnwBuildingHappinessRules&,const SessionCheckpointIdentity&,std::int64_t,
 BnwBuildingHappinessReachedStage);
// Reached native local-city arithmetic for an actual in-flight policy owner.
// The private source proof retains the genuine Host and its SAME state; no
// caller garrison result, public execution receipt or store grant is accepted.
std::int32_t bnw_happiness_buildings_local_math_for_policy(const LiveSessionState&,
 const BnwBuildingHappinessRules&,const SessionCheckpointIdentity&,std::int64_t,
 const ui::SelectedBnwPreviewHost&,const std::shared_ptr<const BnwPolicyAdoptionPending>&);
// Numeric adapter to the genuine SAME newborn Core. No caller garrison bool,
// imported saved garrison ID or execution/publication capability is accepted.
std::int32_t bnw_happiness_buildings_reached_math_for_initial_unit(const LiveSessionState&,
 const BnwBuildingHappinessRules&,const SessionCheckpointIdentity&,std::int64_t,
 BnwBuildingHappinessReachedStage,const BnwNativeUnitAllocatorCore&);
}
}
