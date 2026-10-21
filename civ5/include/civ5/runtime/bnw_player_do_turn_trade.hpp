#pragma once
#include "civ5/runtime/bnw_player_trade_turn.hpp"
#include "civ5/runtime/live_session.hpp"
namespace civ5::runtime {
class BnwPlayerDoTurnRevoltsJournal;class BnwHappinessSuffixRules;
enum class BnwPlayerDoTurnTradeStep {clear_expired,update_connection_values,age_plundered,move_units};
class BnwPlayerDoTurnTradeJournal {
 public:
    const auto &steps()const noexcept{return steps_;}
    const auto &native()const noexcept{return native_;}
    const auto &military_input_revision()const noexcept{return military_;}
    const auto &before_world_revision()const noexcept{return before_;}
    const auto &after_world_revision()const noexcept{return after_;}
    const auto &before_rng()const noexcept{return before_rng_;}
    const auto &after_rng()const noexcept{return after_rng_;}
 private:
    friend BnwPlayerDoTurnTradeJournal stage_bnw_player_do_turn_trade(LiveSessionState &,const BnwPlayerDoTurnRevoltsJournal &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    BnwPlayerDoTurnTradeJournal()=default;
    std::vector<BnwPlayerDoTurnTradeStep> steps_;BnwPlayerTradeTurnStep native_;
    std::string before_,after_,military_;SimulationRandomState before_rng_,after_rng_;
};
struct BnwPlayerDoTurnTradeSourceFile {std::string_view path,sha256;};
inline constexpr std::array kBnwPlayerDoTurnTradeSources{
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
 BnwPlayerDoTurnTradeSourceFile{"CvGameCoreDLL_Expansion2/CvTradeClasses.cpp","6d5d287c04e8ca1b0afb86d9d907ed12dbfb65d29c4594dad213770e0cd0c5c8"}
};
// Full parsed global routes and selected current history, including metadata,
// fields/children/arrays/blobs. Hashing never invents a missing Read/default.
std::string bnw_player_trade_registry_revision(const LiveSessionState &,std::int64_t);
// Original DoTurn four native stages, SAME accepted source/RNG epoch. Own
// origins (EVEN coordinate-empty) require MoveUnit/visual/expiry owners; every
// nonempty own destination needs real yield query. They remain explicit gates.
// No player phase, outer publication, dirty delivery or readiness receipt.
BnwPlayerDoTurnTradeJournal stage_bnw_player_do_turn_trade(LiveSessionState &,
 const BnwPlayerDoTurnRevoltsJournal &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
}
