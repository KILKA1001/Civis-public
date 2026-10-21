#pragma once
#include "civ5/runtime/bnw_player_do_turn_trade.hpp"
#include "civ5/runtime/bnw_player_trade_strategy_prefix.hpp"
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
namespace civ5::runtime {
class BnwPlayerDoTurnMilitaryJournal {
 public:
    const auto &native()const noexcept{return native_;}
    const auto &before_counters()const noexcept{return before_counters_;}
    const auto &before_world_revision()const noexcept{return before_;}
    const auto &after_world_revision()const noexcept{return after_;}
    const auto &grand_military_input_revision()const noexcept{return grand_;}
 private:
    friend BnwPlayerDoTurnMilitaryJournal stage_bnw_player_do_turn_military(LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    BnwPlayerDoTurnMilitaryJournal()=default;
    BnwMilitaryResetStep native_;std::optional<LiveSessionState::BnwMilitaryRequestCounters> before_counters_;
    std::string before_,after_,grand_;
};
inline constexpr std::array kBnwPlayerDoTurnMilitarySources{
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvMilitaryAI.cpp","b5df85f78088a7df45303e4330b6e93146401643907706a213a38d90cc52d622"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvMilitaryAI.h","f1d89343ec7e6ecb98cffb98096cd28d94a9407c02d228855f6ce565b29a4aba"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvGrandStrategyAI.cpp","1f815b0d1adde6735a5600535f2040e54bbae284bd0931489bfdeb6b4a5fec9e"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvDiplomacyAI.cpp","45abb0d902d0a2ff291b2fb15bbe96ffb5d8c09edf646046d0fe716d03e660cf"}
};
// Complete actual selected Military Read record plus retained counter presence.
// Absence is unknown before Reset, never evidence of constructor zero defaults.
std::string bnw_player_military_registry_revision(const LiveSessionState &,std::int64_t);
// Native source land=0, naval=0, saved army=-1, SAME accepted Trade registry.
// Next mandatory GrandStrategy runs for humans too, never an empty shortcut.
BnwPlayerDoTurnMilitaryJournal stage_bnw_player_do_turn_military(LiveSessionState &,
 const BnwPlayerDoTurnTradeJournal &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
}
