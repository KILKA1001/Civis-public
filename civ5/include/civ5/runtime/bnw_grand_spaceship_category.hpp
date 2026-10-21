#pragma once
#include "civ5/runtime/bnw_grand_spaceship_priority.hpp"
#include "civ5/runtime/bnw_grand_second_roll.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
struct BnwGrandSpaceshipCategoryProgress {
 BnwGrandFirstPriorityQuote base;
 BnwGrandSpaceshipPriorityQuote category;
 std::int32_t before_random=0;
};
// Genuine source slot2 GetBase + category, BEFORE its native random draw.
// No public constructor or caller-authored completion/permission boolean.
class BnwGrandSpaceshipPriorityJournal {
 public:
 BnwGrandSpaceshipPriorityJournal(BnwGrandSpaceshipPriorityJournal&&) noexcept=default;
 BnwGrandSpaceshipPriorityJournal &operator=(BnwGrandSpaceshipPriorityJournal&&) noexcept=default;
 BnwGrandSpaceshipPriorityJournal(const BnwGrandSpaceshipPriorityJournal&)=delete;
 BnwGrandSpaceshipPriorityJournal &operator=(const BnwGrandSpaceshipPriorityJournal&)=delete;
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &progress()const noexcept{return progress_;}
 const auto &input_revision()const noexcept{return revision_;}
 private:
 friend BnwGrandSpaceshipPriorityJournal stage_bnw_grand_spaceship_category(const BnwGameActivationHappinessPending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandSpaceshipCategoryProgress &checked_bnw_grand_spaceship_category(const BnwGrandSpaceshipPriorityJournal&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandSpaceshipPriorityJournal()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandSpaceshipCategoryProgress progress_;std::string revision_;
 const LiveSessionState *world_=nullptr,*source_=nullptr;const LuaScriptHost *host_=nullptr;
};
// Requires actual private second-roll journal in its exact retained suffix VM.
// Reuses actual Space leaf and requires native Apollo counter when Info exists.
// No native/VM/RNG write or fabricated technology/project completion.
BnwGrandSpaceshipPriorityJournal stage_bnw_grand_spaceship_category(
 const BnwGameActivationHappinessPending&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
// Retained raw/owned input seals only; consumed source getters are not replayed.
const BnwGrandSpaceshipCategoryProgress &checked_bnw_grand_spaceship_category(
 const BnwGrandSpaceshipPriorityJournal&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
