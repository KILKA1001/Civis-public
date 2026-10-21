#pragma once
#include "civ5/runtime/bnw_grand_third_roll.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
struct BnwGrandConquestCategoryProgress {
 BnwGrandFirstPriorityQuote priority;
 std::optional<BnwGrandMilitaryAverageJournal> military_average;
 std::int32_t before_random=0;
};
// Source GetBase3/category only. Fresh source439 query is retained privately;
// no caller boolean/quote or previously consumed DoGuess average authorizes it.
class BnwGrandConquestPriorityJournal {
 public:
 BnwGrandConquestPriorityJournal(BnwGrandConquestPriorityJournal&&) noexcept=default;
 BnwGrandConquestPriorityJournal &operator=(BnwGrandConquestPriorityJournal&&) noexcept=default;
 BnwGrandConquestPriorityJournal(const BnwGrandConquestPriorityJournal&)=delete;
 BnwGrandConquestPriorityJournal &operator=(const BnwGrandConquestPriorityJournal&)=delete;
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &progress()const noexcept{return progress_;}
 const auto &input_revision()const noexcept{return revision_;}
 private:
 friend BnwGrandConquestPriorityJournal stage_bnw_grand_conquest_category(const BnwGameActivationHappinessPending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandConquestCategoryProgress &checked_bnw_grand_conquest_category(const BnwGrandConquestPriorityJournal&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandConquestPriorityJournal()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandConquestCategoryProgress progress_;std::string revision_;
 const LiveSessionState *world_=nullptr,*source_=nullptr;const LuaScriptHost *host_=nullptr;
};
BnwGrandConquestPriorityJournal stage_bnw_grand_conquest_category(
 const BnwGameActivationHappinessPending&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
const BnwGrandConquestCategoryProgress &checked_bnw_grand_conquest_category(
 const BnwGrandConquestPriorityJournal&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
