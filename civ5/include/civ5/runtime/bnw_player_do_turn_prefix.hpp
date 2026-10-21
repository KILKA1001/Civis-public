#pragma once
#include "civ5/runtime/bnw_player_scalar_prefix.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <array>
#include <string>
#include <vector>
namespace civ5::runtime {
// Receipts within the incomplete player_timers_and_revolts source phase.
enum class BnwPlayerDoTurnPrefixStep {cache,human_ai_pre,culture_bomb,conversion,misc_buttons_dirty,settled_city,conscript,cramped};
struct BnwPlayerMiscButtonsReceipt {std::int64_t player=-1;std::int32_t before=0,after=0;bool dirty=true;};
// New standalone client pending-bit receiver. It is neither restored original
// UI state nor evidence of delivered stock Lua/native UI listeners.
class BnwPlayerDoTurnPrefixJournal {
 public:
    const auto &steps()const noexcept{return steps_;}
    const auto &native()const noexcept{return native_;}
    const auto &misc_buttons_receipts()const noexcept{return misc_receipts_;}
    bool misc_buttons_dirty()const noexcept{return misc_dirty_;}
    bool misc_buttons_delivery_pending()const noexcept{return misc_dirty_;}
    std::int64_t player()const noexcept{return player_;}
    std::int64_t turn()const noexcept{return turn_;}
    const auto &before_world_revision()const noexcept{return before_;}
    const auto &after_world_revision()const noexcept{return after_;}
 private:
    friend BnwPlayerDoTurnPrefixJournal stage_bnw_player_do_turn_prefix(LiveSessionState &,const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t,std::int64_t);
    BnwPlayerDoTurnPrefixJournal()=default;
    std::int64_t player_=-1,turn_=-1;std::string before_,after_;
    BnwPlayerScalarAndCrampedPrefix native_;
    std::vector<BnwPlayerDoTurnPrefixStep> steps_;
    std::vector<BnwPlayerMiscButtonsReceipt> misc_receipts_;
    bool misc_dirty_=false;
};
struct BnwPlayerDoTurnPrefixSourceFile {const char *path,*sha256;};
inline constexpr std::array kBnwPlayerDoTurnPrefixSources{
    BnwPlayerDoTurnPrefixSourceFile{"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
    BnwPlayerDoTurnPrefixSourceFile{"CvGameCoreDLL_Expansion2/CvPlayerAI.cpp","bc20577e12a534ea825e95eb40b15827f637c535f7de06f746787aaeee329a38"}
};
// Actual prefix operand/store digest, shared by the dependent retained suffix.
// Includes timer/player fields absent from a unit-order revision.
std::string bnw_player_do_turn_prefix_revision(const LiveSessionState &,
    const SessionCheckpointIdentity &,const content::RulesDatabase &);
// Called only by the actual retained activation owner at source doTurn entry.
// Source 4043-4060; uses real raw/map operations and a real private pending-bit
// receiver at setConversionTimer. Rejection rolls back all native stores/bit.
// Source DoUpdateUprisings and all later player phases remain mandatory.
BnwPlayerDoTurnPrefixJournal stage_bnw_player_do_turn_prefix(LiveSessionState &,
    const content::RulesDatabase &,const SessionCheckpointIdentity &,
    std::int64_t player,std::int64_t active_player);
}
