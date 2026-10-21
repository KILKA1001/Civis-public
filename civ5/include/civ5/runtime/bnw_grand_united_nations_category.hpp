#pragma once
#include "civ5/runtime/bnw_grand_united_nations_priority.hpp"
#include "civ5/runtime/bnw_grand_first_roll.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
struct BnwGrandUnitedNationsCategoryProgress {
 BnwGrandFirstPriorityQuote base;
 BnwGrandUnitedNationsPriorityQuote category;
 std::int32_t before_random=0;
};
// Genuine source slot1 GetBase + category, BEFORE its native random draw.
// No public constructor or caller-authored completion/permission boolean.
class BnwGrandUnitedNationsPriorityJournal {
 public:
 BnwGrandUnitedNationsPriorityJournal(BnwGrandUnitedNationsPriorityJournal&&) noexcept=default;
 BnwGrandUnitedNationsPriorityJournal &operator=(BnwGrandUnitedNationsPriorityJournal&&) noexcept=default;
 BnwGrandUnitedNationsPriorityJournal(const BnwGrandUnitedNationsPriorityJournal&)=delete;
 BnwGrandUnitedNationsPriorityJournal &operator=(const BnwGrandUnitedNationsPriorityJournal&)=delete;
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &progress()const noexcept{return progress_;}
 const auto &input_revision()const noexcept{return revision_;}
 private:
 friend BnwGrandUnitedNationsPriorityJournal stage_bnw_grand_united_nations_category(const BnwGameActivationHappinessPending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandUnitedNationsCategoryProgress &checked_bnw_grand_united_nations_category(const BnwGrandUnitedNationsPriorityJournal&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandUnitedNationsPriorityJournal()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandUnitedNationsCategoryProgress progress_;std::string revision_;
 const LiveSessionState *world_=nullptr,*source_=nullptr;const LuaScriptHost *host_=nullptr;
};
// Requires actual private first-roll journal in its exact retained suffix VM.
// Disabled/no-active source branch completes. Active votes refusal performs no
// native/VM/RNG write and creates no category journal or partial permission.
BnwGrandUnitedNationsPriorityJournal stage_bnw_grand_united_nations_category(
 const BnwGameActivationHappinessPending&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
// Retained raw/owned input seals only; consumed source getters are not replayed.
const BnwGrandUnitedNationsCategoryProgress &checked_bnw_grand_united_nations_category(
 const BnwGrandUnitedNationsPriorityJournal&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
