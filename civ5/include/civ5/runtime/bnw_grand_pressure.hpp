#pragma once
#include "civ5/runtime/bnw_grand_fourth_roll.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
struct BnwGrandPressureMajorCheck {
 std::int64_t player=-1;
 bool alive=false;
 std::optional<std::int64_t> team;
 std::optional<bool> met;
};
struct BnwGrandPressureProgress {
 std::vector<BnwGrandPressureMajorCheck> inspected;
 std::int32_t alive_met=0,multiplier=0,age_before=0,age_after=0,active_strategy=-1;
 std::array<std::int32_t,4> before_priorities{},adopted{},log_changes{},after_priorities{};
};
// Only actual source302..352. No best/adoption/specialization/logger permission.
class BnwGrandPressureJournal {
 public:
 BnwGrandPressureJournal(BnwGrandPressureJournal&&) noexcept=default;
 BnwGrandPressureJournal &operator=(BnwGrandPressureJournal&&) noexcept=default;
 BnwGrandPressureJournal(const BnwGrandPressureJournal&)=delete;
 BnwGrandPressureJournal &operator=(const BnwGrandPressureJournal&)=delete;
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &progress()const noexcept{return progress_;}
 const auto &before_grand_revision()const noexcept{return before_grand_;}
 const auto &after_grand_revision()const noexcept{return after_grand_;}
 private:
 friend BnwGrandPressureJournal stage_bnw_grand_pressure(const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandPressureProgress &checked_bnw_grand_pressure(const BnwGrandPressureJournal&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandPressureJournal()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandPressureProgress progress_;std::string before_grand_,after_grand_,roster_;
 std::array<SimulationRandomState,2> raw_{},prefix_{},live_{};
 const LiveSessionState *world_=nullptr,*source_=nullptr;const LuaScriptHost *host_=nullptr;
};
BnwGrandPressureJournal stage_bnw_grand_pressure(
 const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
const BnwGrandPressureProgress &checked_bnw_grand_pressure(
 const BnwGrandPressureJournal&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
