#pragma once
#include "civ5/runtime/bnw_grand_world_averages.hpp"
#include "civ5/runtime/bnw_player_grand_military_average.hpp"
namespace civ5::runtime {
// These private factories own actual query evidence. A caller-supplied public
// BnwGrandWorldAverages/calculated=true is never accepted as completion.
class BnwGrandWorldAverageJournal {
 public:
    std::int64_t player()const noexcept{return player_;}
    const auto &averages()const noexcept{return averages_;}
    const auto &query_revision()const noexcept{return query_;}
    const auto &grand_revision()const noexcept{return grand_;}
    const auto &trade_revision()const noexcept{return trade_;}
    const auto &military_revision()const noexcept{return military_;}
    const auto &military_average_revision()const noexcept{return military_average_;}
    const auto &identity()const noexcept{return identity_;}
 private:
    friend BnwGrandWorldAverageJournal stage_bnw_grand_world_average_journal(const LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,const BnwPlayerDoTurnMilitaryJournal &,const BnwGrandMilitaryAverageJournal &,const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    BnwGrandWorldAverageJournal()=default;
    std::int64_t player_=-1;BnwGrandWorldAverages averages_;
    SessionCheckpointIdentity identity_;
    std::string query_,grand_,trade_,military_,military_average_;
};
struct BnwGrandForeignMajorCheck {
    std::int64_t player=-1;
    bool alive=false,self=false;
    // Source short circuit: dead/self slots do NOT read team/hasMet here.
    std::optional<std::int64_t> team;
    std::optional<bool> met;
};
class BnwGrandGuessEmptyJournal {
 public:
    std::int64_t player()const noexcept{return player_;}
    const auto &inspected()const noexcept{return inspected_;}
    const auto &query_revision()const noexcept{return query_;}
    const auto &grand_revision()const noexcept{return grand_;}
    // Only source DoGuess's empty eligible-target suffix, not Grand::DoTurn.
 private:
    friend BnwGrandGuessEmptyJournal stage_bnw_grand_guess_empty(const LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,const BnwPlayerDoTurnMilitaryJournal &,const BnwGrandMilitaryAverageJournal &,const BnwGrandWorldAverageJournal &,const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
    BnwGrandGuessEmptyJournal()=default;
    std::int64_t player_=-1;std::vector<BnwGrandForeignMajorCheck> inspected_;
    std::string query_,grand_;
};
// Complements existing Trade and Military input seals. Unknown structures are
// hashed as unknown and never become fake empty/default query evidence.
std::string bnw_grand_world_average_inputs_revision(const LiveSessionState &);
std::string bnw_grand_native_component_revision(const LiveSessionState &,std::int64_t);
// Pure prospective operand digest; same metadata and exact existing encoding.
std::string bnw_grand_native_component_revision(const LiveSessionState &,std::int64_t,const saves::PlayerStrategyBlock &);
BnwGrandWorldAverageJournal stage_bnw_grand_world_average_journal(
 const LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,
 const BnwPlayerDoTurnMilitaryJournal &,const BnwGrandMilitaryAverageJournal &,
 const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t);
// Does not recalculate an already accepted average. Revalidates SAME seals,
// then source22loop. First eligible target requires actual priority producers.
// Empty eligible branch preserves every current Grand field and both RNGs.
BnwGrandGuessEmptyJournal stage_bnw_grand_guess_empty(
 const LiveSessionState &,const BnwPlayerDoTurnTradeJournal &,
 const BnwPlayerDoTurnMilitaryJournal &,const BnwGrandMilitaryAverageJournal &,
 const BnwGrandWorldAverageJournal &,const BnwGrandWorldAverageRules &,
 const BnwHappinessSuffixRules &,const content::RulesDatabase &,
 const SessionCheckpointIdentity &,std::int64_t);
inline constexpr std::array kBnwGrandGuessEmptySources{
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvGrandStrategyAI.cpp","1f815b0d1adde6735a5600535f2040e54bbae284bd0931489bfdeb6b4a5fec9e"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvDiplomacyAIEnums.h","b3ef40ab7f65cba30dc4be1b7f9d22693fb5836882be072812741693b664171b"}
};
}
