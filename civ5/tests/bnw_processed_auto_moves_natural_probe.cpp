#include "bnw_processed_auto_moves_natural_probe.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/detail/bnw_dynamic_turn_mode.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <stdexcept>
namespace civ5::tests {
namespace {void require(bool b,const char*m){if(!b)throw std::runtime_error(m);}}
runtime::BnwRetainedTurnView probe_bnw_processed_auto_moves_natural(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>&host,
 const std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>&source,
 const std::shared_ptr<runtime::BnwRetainedTurnCursor>&cursor,
 const std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore>&core){
 using namespace runtime;require(host&&source&&cursor,"Actual natural ProcessedAutoMoves objects missing");
 const auto state=host->session().read_state_lease();const auto before=cursor->view();const auto end=cursor->player_end_after_busy_progress();
 require(before.frontier==BnwRetainedTurnFrontier::game_turn_player_end_turn_processed_auto_owner&&end.issued&&end.source_adopted&&
  end.player&&end.before_auto_moves==std::optional{true}&&!end.setter_returned,"Natural changed-AutoMoves actual false-AI predecessor missing");
 const auto actor=static_cast<std::size_t>(*end.player);require(actor<64&&state->world.all_players&&state->world.all_players->players.size()==64,
  "Natural ProcessedAutoMoves actual64 actor missing");
 std::array<std::int64_t,64>active{},ending{},automatic{};const auto processed=host->activation_owner().processed_auto_moves;
 require(processed.size()==64,"Actual SAME original64 processed owner missing");
 for(std::size_t n=0;n<64;++n){const auto&raw=state->world.all_players->players[n];active[n]=detail::dynamic_core(raw,"m_bTurnActive").values.at(0);
  ending[n]=detail::dynamic_core(raw,"m_bEndTurn").values.at(0);automatic[n]=detail::dynamic_core(raw,"m_bAutoMoves").values.at(0);require(processed.contains(n),"Processed owner native64 hole");}
 require(automatic[actor]==1,"Natural changed-AutoMoves did not have a genuine true saved flag");
 const auto revision=state->ui_source_revision;const auto turn=state->setup.game_turn;const auto map=state->random.map.state(),game=state->random.gameplay.state();
 const auto result=source->run_processed_auto_moves_after_busy(cursor,core);const auto actual=cursor->processed_auto_moves_progress();
 require(actual.attempted&&actual.issued&&actual.auto_stored&&actual.processed_stored&&actual.source_adopted&&actual.setter_returned&&
  actual.player==end.player&&actual.before_processed==std::optional{processed.at(actor)}&&actual.before_revision==revision&&
  actual.after_revision==revision+1&&state->ui_source_revision==revision+1&&actual.owner_after==actual.owner_before+1&&
  result.operation==before.operation+1&&result.frontier==BnwRetainedTurnFrontier::game_turn_player_unit_attrition_owner,
  "Native paired AutoMoves/ProcessedAutoMoves order, Source adoption or once history differs");
 for(std::size_t n=0;n<64;++n){const auto&raw=state->world.all_players->players[n];
  require(detail::dynamic_core(raw,"m_bTurnActive").values.at(0)==active[n]&&detail::dynamic_core(raw,"m_bEndTurn").values.at(0)==ending[n]&&
   detail::dynamic_core(raw,"m_bAutoMoves").values.at(0)==(n==actor?0:automatic[n])&&
   host->activation_owner().processed_auto_moves.at(n)==(n==actor?false:processed.at(n)),"Native paired store mutated another actual64 field/owner");}
 require(state->setup.game_turn==turn&&state->random.map.algorithm()==RandomAlgorithm::bnw&&state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&
  state->random.map.state()==map&&state->random.gameplay.state()==game,"Native paired store changed clock or RNG");
 bool refused=false;try{(void)source->run_processed_auto_moves_after_busy(cursor,core);}catch(const std::exception&){refused=true;}
 const auto replay=cursor->processed_auto_moves_progress();require(refused&&state->ui_source_revision==revision+1&&replay.owner_after==actual.owner_after&&
  cursor->view().operation==result.operation&&replay.auto_stored&&replay.processed_stored&&replay.source_adopted,
  "Native paired-store replay duplicated or erased genuine history");return result;
}
}
