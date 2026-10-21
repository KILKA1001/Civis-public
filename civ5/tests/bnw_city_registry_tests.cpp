#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
using namespace civ5;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void refuses(F&& operation){bool failed=false;try{operation();}catch(const std::exception&){failed=true;}need(failed,"Invalid city registry accepted");}
auto& field(auto& rows,std::string_view name){for(auto& row:rows)if(row.name&&name==row.name)return row;throw std::runtime_error("Actual registry source field missing");}
}
int main(int argc,char** argv){try{
 need(argc==3,"Requires actual selected save and shared installation");
 const auto save_sha=runtime::identify_file(argv[1]).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);
 need(static_cast<bool>(boot.preview),"Actual selected city registry unavailable");
 const auto& base=boot.preview.host->session().state();
 std::size_t rows=0;
 std::int32_t picked_owner=-1,picked_id=-1;
 for(const auto& player:base.world.all_players->players){
  const auto owner=static_cast<std::int32_t>(player.saved_player_id);
  for(const auto& raw:player.cities.cities){
   const auto id=static_cast<std::int32_t>(field(raw.core.fields,"m_iID").values.at(0));
   const auto* existing=runtime::resolve_bnw_current_city(base,owner,id);
   need(existing==&raw,"Existing current source lookup differs");
   need(runtime::resolve_bnw_city_registry(base.world,base.cities,owner,id)==existing,"Extracted full-ID lookup differs");
   need(runtime::resolve_bnw_city_registry(base.world,base.cities,owner,id&8191)==existing,"Native index-only lookup lost its generation binding");
   const auto other_generation=(id&~8191)==8192?16384:8192;
   need(runtime::resolve_bnw_city_registry(base.world,base.cities,owner,(id&8191)|other_generation)==nullptr,"Stale full generation accepted");
   const auto* owned=runtime::resolve_bnw_city_yield_registry(base.world,base.cities,owner,id&8191);
   need(owned==&base.cities.players.at(owner).cities.at(id),"Actual raw-to-owned city identity differs");
   if(!rows){picked_owner=owner;picked_id=id;}
   ++rows;
  }
 }
 need(rows>0,"Actual imported cities absent");
 // Independent malformed copies exercise native metadata and raw/owned joins;
 // none is a load, command, freshness or continuation receipt.
 const auto check_changed=[&](auto&& change){auto state=base;change(state);refuses([&]{(void)runtime::resolve_bnw_city_registry(state.world,state.cities,picked_owner,picked_id);});};
 check_changed([&](auto& state){++state.world.all_players->players.at(static_cast<std::size_t>(picked_owner)).cities.header.wire_city_count;});
 check_changed([&](auto& state){++field(state.world.all_players->players.at(static_cast<std::size_t>(picked_owner)).cities.header.fields,"num_slots").value;});
 check_changed([&](auto& state){++field(state.world.all_players->players.at(static_cast<std::size_t>(picked_owner)).cities.header.fields,"free_list_count").value;});
 check_changed([&](auto& state){auto& city=state.cities.players.at(picked_owner).cities.at(picked_id);city.source_core->second+=1;});
 check_changed([&](auto& state){auto& cities=state.world.all_players->players.at(static_cast<std::size_t>(picked_owner)).cities.cities;for(auto& city:cities)if(field(city.core.fields,"m_iID").values.at(0)==picked_id)field(city.core.fields,"m_iID").values.at(0)=std::int64_t(INT32_MAX)+1;});
 check_changed([&](auto& state){state.world.all_players->players.at(static_cast<std::size_t>(picked_owner)).saved_player_id=(picked_owner+1)%64;});
 check_changed([&](auto& state){auto& cities=state.world.all_players->players.at(static_cast<std::size_t>(picked_owner)).cities.cities;for(auto& city:cities)if(field(city.core.fields,"m_iID").values.at(0)==picked_id)field(city.core.fields,"m_eOwner").values.at(0)=(picked_owner+1)%64;});
 saves::SavedSessionSnapshot absent;runtime::CityYieldContext unbound;
 need(!runtime::resolve_bnw_city_registry(absent,unbound,-1,0)&&!runtime::resolve_bnw_city_registry(absent,unbound,64,0)&&!runtime::resolve_bnw_city_registry(absent,unbound,0,-1),"Native absent sentinel borrowed unknown registry");
 refuses([&]{(void)runtime::resolve_bnw_city_registry(absent,unbound,0,0);});
 need(runtime::identify_file(argv[1]).sha256==save_sha,"Original save changed");
 std::cout<<"PASS actual raw/owned city registry full/index-only IDs, stale generations and native metadata guards; no world mutation\n";
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
