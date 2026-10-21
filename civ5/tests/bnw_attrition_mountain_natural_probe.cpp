#include "bnw_attrition_mountain_natural_probe.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/detail/bnw_dynamic_turn_mode.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <tuple>
#include <vector>
#include <stdexcept>
namespace civ5::tests {
namespace {
void require(bool b,const char*m){if(!b)throw std::runtime_error(m);}
std::int64_t scalar(const saves::SavedUnitObservation&u,const char*key){const saves::UnitCoreField*f=nullptr;
 for(const auto&v:u.core.fields)if(v.name&&std::string_view(v.name)==key){require(!f,"Natural mountain duplicate actual scalar");f=&v;}
 require(f&&f->values.size()==1,"Natural mountain actual scalar absent");return f->values[0];}
}
runtime::BnwRetainedTurnView probe_bnw_attrition_mountain_natural(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>&host,
 const std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>&source,
 const std::shared_ptr<runtime::BnwRetainedTurnCursor>&cursor,
 const std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore>&core){
 using namespace runtime;require(host&&source&&cursor,"Natural mountain actual objects absent");
 const auto state=host->session().read_state_lease();const auto before=cursor->view();const auto first=cursor->attrition_first_progress();
 const auto actor=cursor->player_end_after_busy_progress().player;
 require(before.frontier==BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_owner&&first.executed&&first.unit&&actor&&
  state->world.all_players&&state->world.all_players->players.size()==64,"Natural mountain requires genuine admitted first prefix/native64");
 const saves::SavedUnitObservation*unit=nullptr;for(const auto&u:state->world.all_players->players.at(*actor).units.units)if(scalar(u,"m_iID")==*first.unit){require(!unit,"Natural mountain duplicate actual unit");unit=&u;}
 require(unit,"Natural mountain genuine first unit absent");const auto hash=scalar(*unit,"idUnitType");const auto*info=host->types().find_hash(hash);
 require(info&&info->table=="Units","Natural mountain actual UnitInfo absent");std::int32_t domain=-1;bool seen=false;
 const auto rows=host->database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{info->id}},[&](const auto&r){require(!seen&&r.text("Type")==std::optional<std::string>{info->type},"Natural mountain actual rules row differs");seen=true;
  const auto text=r.text("Domain");const auto*d=text?host->types().find(*text):nullptr;require(d&&d->table=="Domains"&&d->id>=0&&d->id<5,"Natural mountain actual domain absent");domain=static_cast<std::int32_t>(d->id);});
 require(rows.all_succeeded()&&seen,"Natural mountain original Domain source failed");const auto x=scalar(*unit,"m_iX"),y=scalar(*unit,"m_iY");
 const MapGeometry geometry{state->world.map_header.grid,state->world.map_header.wrap_x,state->world.map_header.wrap_y,saves::GameStateLayout::expansion2_version1};
 const auto at=geometry.plot(x,y);require(at&&state->world.map_plots&&state->world.map_projection,"Natural mountain actual plot unavailable");
 const auto&plot=state->world.map_plots->plots.at(*at);std::optional<std::int32_t>kind;
 for(const auto&f:plot.fields)if(std::string_view(f.name)=="plot_type"){require(!kind,"Natural mountain duplicate plot kind");kind=static_cast<std::int32_t>(f.value);}
 require(kind&&*kind>=0&&*kind<4&&state->world.map_projection->cells.at(*at).plot_type==std::optional<std::int64_t>{*kind},"Natural mountain raw/projected plot kind absent");
 if(domain==4)domain=*kind==3?0:2;
 const auto expected_kind=domain==2?kind:std::nullopt;const auto expected_count=domain==2&&*kind==0?std::optional{static_cast<std::int32_t>(scalar(*unit,"m_iCanMoveAllTerrainCount"))}:std::nullopt;
 const bool damage=expected_count&&*expected_count<=0;
 std::array<std::array<std::int64_t,3>,64>flags;std::vector<std::tuple<std::size_t,std::int64_t,std::int64_t>>damages;
 for(std::size_t n=0;n<64;++n){const auto&r=state->world.all_players->players[n];flags[n]={detail::dynamic_core(r,"m_bTurnActive").values.at(0),detail::dynamic_core(r,"m_bEndTurn").values.at(0),detail::dynamic_core(r,"m_bAutoMoves").values.at(0)};
  for(const auto&u:r.units.units)damages.emplace_back(n,scalar(u,"m_iID"),scalar(u,"m_iDamage"));}
 const auto map=state->random.map.state(),game=state->random.gameplay.state();const auto revision=state->ui_source_revision;const auto turn=state->setup.game_turn;
 const auto result=source->run_player_attrition_mountain_prefix(cursor,core);const auto actual=cursor->attrition_mountain_progress();
 require(actual.issued&&actual.executed&&actual.unit==first.unit&&actual.domain==std::optional{domain}&&actual.plot_type==expected_kind&&
  actual.all_terrain_count==expected_count&&actual.damage_required==damage&&actual.revision==revision&&state->ui_source_revision==revision&&result.operation==before.operation+1&&
  result.frontier==(damage?BnwRetainedTurnFrontier::game_turn_player_attrition_mountain_damage_owner:BnwRetainedTurnFrontier::game_turn_player_attrition_death_owner),
  "Natural original domain/HOVER/mountain/allterrain condition or mandatory frontier differs");
 require(state->random.map.state()==map&&state->random.gameplay.state()==game&&state->setup.game_turn==turn,"Mountain suffix repeated parent RNG or changed clock");
 std::vector<std::tuple<std::size_t,std::int64_t,std::int64_t>>current;for(std::size_t n=0;n<64;++n){const auto&r=state->world.all_players->players[n];
  require(flags[n]==std::array<std::int64_t,3>{detail::dynamic_core(r,"m_bTurnActive").values.at(0),detail::dynamic_core(r,"m_bEndTurn").values.at(0),detail::dynamic_core(r,"m_bAutoMoves").values.at(0)},"Mountain suffix changed native64 flags");
  for(const auto&u:r.units.units)current.emplace_back(n,scalar(u,"m_iID"),scalar(u,"m_iDamage"));}
 require(current==damages,"Mountain suffix issued damage/death before real localization/changeDamage owner");
 bool refused=false;try{(void)source->run_player_attrition_mountain_prefix(cursor,core);}catch(const std::exception&){refused=true;}
 require(refused&&cursor->attrition_mountain_progress().executed&&state->random.gameplay.state()==game&&state->random.map.state()==map&&
  state->ui_source_revision==revision&&cursor->view().operation==result.operation,"Mountain actual getter replay erased history/repeated RNG");return result;
}
}
