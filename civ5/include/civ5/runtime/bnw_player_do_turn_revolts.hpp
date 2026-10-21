#pragma once
#include "civ5/runtime/bnw_player_do_turn_prefix.hpp"
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
namespace civ5::runtime {
enum class BnwPlayerDoTurnRevoltStep {uprisings,city_revolts,maya};
class BnwPlayerDoTurnRevoltsJournal {
 public:
    const auto &steps()const noexcept{return steps_;}
    const auto &uprising()const noexcept{return uprising_;}
    const auto &city_revolt()const noexcept{return revolt_;}
    const auto &maya()const noexcept{return maya_;}
    const auto &unhappiness()const noexcept{return unhappiness_;}
    const auto &before_world_revision()const noexcept{return before_;}
    const auto &after_world_revision()const noexcept{return after_;}
    const auto &before_rng()const noexcept{return before_rng_;}
    const auto &after_rng()const noexcept{return after_rng_;}
    bool trade_required_next()const noexcept{return trade_required_;}
 private:
    friend BnwPlayerDoTurnRevoltsJournal stage_bnw_player_do_turn_revolts(LiveSessionState &,const BnwPlayerDoTurnPrefixJournal &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    BnwPlayerDoTurnRevoltsJournal()=default;
    std::vector<BnwPlayerDoTurnRevoltStep> steps_;
    BnwUprisingTurnStep uprising_;BnwCityRevoltTurnStep revolt_;BnwMayaTurnStep maya_;
    BnwUnhappinessQuote unhappiness_;std::string before_,after_;
    SimulationRandomState before_rng_,after_rng_;bool trade_required_=false;
};
inline constexpr std::array kBnwPlayerDoTurnRevoltsSources{
    BnwPlayerDoTurnPrefixSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
    BnwPlayerDoTurnPrefixSourceFile{"CvGameCoreDLL_Expansion2/CvTraitClasses.cpp","8e5c3fe9ad40e44e2ad9a16e9ea590ff13ced2b51925f96b280495d82fec8edc"},
    BnwPlayerDoTurnPrefixSourceFile{"CvGameCoreDLL_Expansion2/CvPreGame.cpp","6fabb3a68f6eea348d0a232998ee6b4f81aa3f4a7b58aa0ae2f9ec32d9383946"}
};
// Read-only accepted suffix guard; includes exact current trade registry/RNG.
std::string bnw_player_do_turn_revolts_revision(const LiveSessionState &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
// Existing source native helpers, exact order4062-4067, SAME prefix/epoch.
// Rebuild a read-only comparison model; NEVER reset live ignore/transient flags.
// Any unsupported spawn/transfer/Maya or malformed late operand discards ALL
// suffix counter/RNG changes; accepted scalar/cramped prefix remains untouched.
// This is not a complete scheduler phase, player turn or UI delivery receipt.
BnwPlayerDoTurnRevoltsJournal stage_bnw_player_do_turn_revolts(LiveSessionState &,
    const BnwPlayerDoTurnPrefixJournal &,const BnwHappinessSuffixRules &,
    const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
}
