#pragma once
#include "civ5/runtime/bnw_grand_guess_empty.hpp"
#include "civ5/runtime/bnw_personality_flavors.hpp"
#include <functional>
namespace civ5::runtime {
class BnwGrandPriorityRules {
 public:
    static BnwGrandPriorityRules load(const content::RulesDatabase &);
    struct Strategy {std::int64_t id=-1;std::string type;std::vector<std::int64_t> flavors;};
    const auto &strategies()const noexcept{return strategies_;}
    std::size_t flavor_slots()const noexcept{return flavors_;}
    const auto &identity()const noexcept{return identity_;}
 private:
    BnwGrandPriorityRules()=default;
    std::vector<Strategy> strategies_;std::size_t flavors_=0;std::string identity_;
};
struct BnwGrandFirstPriorityQuote {
    bool base_quoted=false,category_quoted=false;
    bool conquest_victory_disabled=false,culture_victory_disabled=false;
    bool culture_prefix_quoted=false;
    std::string strategy_type;
    std::optional<std::int32_t> culture_initial,culture_cities,own_culture;
    std::size_t culture_cities_visited=0;
    bool total_culture_option_anarchy_checked=false;
    std::int64_t strategy=-1,player=-1,elapsed=-1;
    std::optional<std::int32_t> base,category,before_random;
    std::size_t flavor_slots_visited=0,nonzero_flavors=0,war_teams_visited=0;
    std::size_t cramped_plots_visited=0,nuke_major_slots_visited=0;
    bool nobody_met_query=false,military_ratio_query=false;
    std::string blocker;
};
// Actual first native GetBase + its category, read-only before JonRand.
// Enabled Culture retains only its verified victory/era/flavor prefix and
// refuses the mandatory full GetTotalJONSCulturePerTurn dependency.
// A completed base/prefix is diagnostic progress, not a category receipt.
// No source priorities/RNG/decisions/Grand receipt or caller completion bool.
// Military branch uses a genuine previously quoted SAME current input owner;
// missing/stale calculators remain explicit mandatory dependencies.
BnwGrandFirstPriorityQuote quote_bnw_grand_first_priority(const LiveSessionState &,
 const BnwGrandPriorityRules &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t,
 const BnwGrandMilitaryAverageJournal *military_average=nullptr,
 const BnwGrandWorldAverageRules *culture_city_rules=nullptr);
// Reusable native GetBase for an actual requested Info slot. Read-only quote,
// not permission to skip earlier source slots/category or consume RNG.
BnwGrandFirstPriorityQuote quote_bnw_grand_base_priority(const LiveSessionState &,
 const BnwGrandPriorityRules &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t player,
 std::int64_t strategy);
// Independent source Conquest query at its actual native Info ID.
// This never grants completion of the earlier Culture/UN/Spaceship slots.
BnwGrandFirstPriorityQuote quote_bnw_grand_conquest_priority(const LiveSessionState &,
 const BnwGrandPriorityRules &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t,
 const BnwGrandMilitaryAverageJournal *military_average=nullptr);
// Actual GetBase once, then source Conquest prefix, then the fresh average
// provider ONLY when the enabled elapsed military branch reaches source439.
// The returned quote itself never grants an enclosing phase/RNG permission.
BnwGrandFirstPriorityQuote quote_bnw_grand_conquest_priority_at_source(
 const LiveSessionState &,const BnwGrandPriorityRules &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t,
 const std::function<const BnwGrandMilitaryAverageJournal &()> &);
struct BnwGrandPrioritySourceFile {std::string_view path,sha256;};
inline constexpr std::array kBnwGrandPrioritySources{
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvGrandStrategyAI.cpp","1f815b0d1adde6735a5600535f2040e54bbae284bd0931489bfdeb6b4a5fec9e"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvFlavorManager.cpp","81d22b0d0dbb9bb3ea4f2088313950b40903e42d0d8fccfb10b3a94bd5e1c5c4"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvDatabaseUtility.cpp","8595f8d425e816534214f68a578b4f2d537f80d02168d06c0eaad221a868fe30"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvPreGame.cpp","6fabb3a68f6eea348d0a232998ee6b4f81aa3f4a7b58aa0ae2f9ec32d9383946"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvTeam.cpp","4aacb3b0c3b96e97d80d215155341752417ce962a102f6bc8916e6d04bb723cc"},
 BnwGrandPrioritySourceFile{"CvGameCoreDLL_Expansion2/CvDiplomacyAIEnums.h","b3ef40ab7f65cba30dc4be1b7f9d22693fb5836882be072812741693b664171b"}
};
}
