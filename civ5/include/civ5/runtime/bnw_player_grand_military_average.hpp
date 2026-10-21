#pragma once
#include "civ5/runtime/bnw_player_do_turn_military.hpp"
namespace civ5::runtime {
struct BnwGrandMilitaryCachedOperand {
    std::int64_t player=-1,team=-1,might=0,cache_turn=-1;
};
// Only a completed source military-average query; NOT GrandStrategy completion.
class BnwGrandMilitaryAverageJournal {
 public:
    std::int64_t player()const noexcept{return player_;}
    std::int64_t elapsed()const noexcept{return elapsed_;}
    std::int64_t sum()const noexcept{return sum_;}
    std::int64_t average()const noexcept{return average_;}
    const auto &participants()const noexcept{return participants_;}
    const auto &input_revision()const noexcept{return input_;}
 private:
    friend BnwGrandMilitaryAverageJournal stage_bnw_grand_military_average(const LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,const BnwPlayerDoTurnMilitaryJournal &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    friend BnwGrandMilitaryAverageJournal query_bnw_grand_current_military_average(const LiveSessionState &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    BnwGrandMilitaryAverageJournal()=default;
    std::int64_t player_=-1,elapsed_=0,sum_=0,average_=0;
    std::vector<BnwGrandMilitaryCachedOperand> participants_;
    std::string input_;
};
inline constexpr std::array kBnwGrandMilitaryAverageSources{
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvGrandStrategyAI.cpp","1f815b0d1adde6735a5600535f2040e54bbae284bd0931489bfdeb6b4a5fec9e"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.h","a08fbe82bb6a6709529e163467ff018c962dd76b61628b0c820946d9fa1284b4"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvPreGame.h","be6d9d3923cddf517273fb9c779ab4baac821b452c0e9394043c1407ef5ac635"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvPreGame.cpp","6fabb3a68f6eea348d0a232998ee6b4f81aa3f4a7b58aa0ae2f9ec32d9383946"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvTeam.cpp","4aacb3b0c3b96e97d80d215155341752417ce962a102f6bc8916e6d04bb723cc"}
};
// Bind the current actual query inputs at accepted ResetCounters. Unknown shape
// is hashed as unknown, never turned into a usable default or query receipt.
std::string bnw_grand_military_inputs_revision(const LiveSessionState &);
// Reused actual current cached query, independent of the earlier Reset stage.
// This private-constructor query result gives no phase/Grand/RNG authority.
// Stale GetMilitaryMight caches still require BOTH native calculators.
BnwGrandMilitaryAverageJournal query_bnw_grand_current_military_average(
 const LiveSessionState &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
// CvGrandStrategyAI:870 -> CvGame:6698 (includeMe=true/includeOnlyKnown=true).
// Read-only cached GetMilitaryMight branch; stale cache requires BOTH native
// calculators and throws before a receipt. No implied empty roster/met flags.
BnwGrandMilitaryAverageJournal stage_bnw_grand_military_average(
 const LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,
 const BnwPlayerDoTurnMilitaryJournal &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
}
