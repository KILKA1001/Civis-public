#pragma once
#include "civ5/runtime/bnw_grand_culture_priority.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
struct BnwGrandFirstRollProgress {
 std::int32_t before_random=0,source_bound=0,active_strategy=-1,active_boost=0;
 std::uint16_t native_bound=0,roll=0;
 std::int32_t previous_priority=0,priority=0;
 std::array<SimulationRandomState,2> before_random_streams{},after_random_streams{};
};
// Completion of ONLY the first native source roll and priority[0] write.
// No public constructor or caller-authored completion/permission boolean.
class BnwGrandFirstRollJournal {
 public:
 BnwGrandFirstRollJournal(BnwGrandFirstRollJournal&&) noexcept=default;
 BnwGrandFirstRollJournal &operator=(BnwGrandFirstRollJournal&&) noexcept=default;
 BnwGrandFirstRollJournal(const BnwGrandFirstRollJournal&)=delete;
 BnwGrandFirstRollJournal &operator=(const BnwGrandFirstRollJournal&)=delete;
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &progress()const noexcept{return progress_;}
 const auto &before_grand_revision()const noexcept{return before_grand_;}
 const auto &after_grand_revision()const noexcept{return after_grand_;}
 private:
 friend BnwGrandFirstRollJournal stage_bnw_grand_first_roll(const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandFirstRollProgress &checked_bnw_grand_first_roll(const BnwGrandFirstRollJournal&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandFirstRollJournal()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandFirstRollProgress progress_;std::string before_grand_,after_grand_;
 std::array<SimulationRandomState,2> raw_{},prefix_{};
 const LiveSessionState *world_=nullptr,*source_=nullptr;const LuaScriptHost *host_=nullptr;
};
// All genuine category/identity/schema/RNG guards and allocations precede
// prospective draw. Commit consists only of two noexcept assignments.
BnwGrandFirstRollJournal stage_bnw_grand_first_roll(
 const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
// Narrow postcondition: SAME VM/world/source, native Grand and both RNGs.
// Consumed Culture getters are intentionally not re-run after this write.
const BnwGrandFirstRollProgress &checked_bnw_grand_first_roll(
 const BnwGrandFirstRollJournal&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
inline constexpr std::array kBnwGrandFirstRollSources{
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvGrandStrategyAI.cpp","1f815b0d1adde6735a5600535f2040e54bbae284bd0931489bfdeb6b4a5fec9e"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvRandom.cpp","0245c2d6e7a46968af4e704626764afe8af0216beeb237a9977e48dc52212134"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvRandom.h","8252ad7513940ee200d77747c97a2e4f9feea2ef2febb2030b1ae4e5eeb4fb7e"}
};
}
