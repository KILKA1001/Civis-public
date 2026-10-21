#pragma once
#include "civ5/runtime/bnw_grand_united_nations_category.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
// Same source operand schema, separate private completion/authority type.
using BnwGrandSecondRollProgress=BnwGrandFirstRollProgress;
class BnwGrandSecondRollJournal {
 public:
 BnwGrandSecondRollJournal(BnwGrandSecondRollJournal&&) noexcept=default;
 BnwGrandSecondRollJournal &operator=(BnwGrandSecondRollJournal&&) noexcept=default;
 BnwGrandSecondRollJournal(const BnwGrandSecondRollJournal&)=delete;
 BnwGrandSecondRollJournal &operator=(const BnwGrandSecondRollJournal&)=delete;
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &progress()const noexcept{return progress_;}
 const auto &before_grand_revision()const noexcept{return before_grand_;}
 const auto &after_grand_revision()const noexcept{return after_grand_;}
 private:
 friend BnwGrandSecondRollJournal stage_bnw_grand_second_roll(const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandSecondRollProgress &checked_bnw_grand_second_roll(const BnwGrandSecondRollJournal&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandSecondRollJournal()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandSecondRollProgress progress_;std::string before_grand_,after_grand_;
 std::array<SimulationRandomState,2> raw_{},prefix_{};
 const LiveSessionState *world_=nullptr,*source_=nullptr;const LuaScriptHost *host_=nullptr;
};
// Requires the authentic private UN journal on the SAME original suffix epoch.
// Neither source category/GetBase nor already consumed first RNG is replayed.
// All fallible preparation precedes the two noexcept gameplay/priority1 writes.
BnwGrandSecondRollJournal stage_bnw_grand_second_roll(
 const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
// Narrow postcondition only; earlier source receipts become consumed values.
const BnwGrandSecondRollProgress &checked_bnw_grand_second_roll(
 const BnwGrandSecondRollJournal&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
