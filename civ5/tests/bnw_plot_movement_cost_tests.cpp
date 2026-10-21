#include "civ5/runtime/bnw_plot_movement_cost.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const BnwPlotMovementCostUnavailable&){rejected=true;}need(rejected,"Movement missing/generic/stale operand accepted");}
using Math=std::map<std::string,std::int64_t,std::less<>>;
Math basic(){return {{"faster_river",0},{"faster_hills",0},{"ignore",0},{"denominator",60},{"crossing",0},{"feature",-1},{"terrain",0},{"terrain_move",1},{"hills",0},{"discount",0},{"rough",0},{"terrain_double",0},{"base",2},{"from_route",0},{"own_target",0},{"has_team",0}};}
void pure(){
 auto m=basic();need(bnw_plot_movement_cost_math(m)==60,"Detached flat cost not60");
 m["hills"]=1;m["city"]=0;m["hill_extra"]=1;m["rough"]=1;m["rough_ends"]=0;m["hill_double"]=0;
 need(bnw_plot_movement_cost_math(m)==120,"Detached hills cost not120");m["hill_double"]=1;need(bnw_plot_movement_cost_math(m)==60,"Hill double movement applied after clamp");
 m["terrain_move"]=INT32_MAX;m["hill_extra"]=1;refuses([&]{(void)bnw_plot_movement_cost_math(m);});
 m=basic();m["crossing"]=1;m["river_extra"]=1;need(bnw_plot_movement_cost_math(m)==120,"Native river addition/order differs");
 m["faster_river"]=1;m["river"]=0;m.erase("river_extra");need(bnw_plot_movement_cost_math(m)==60,"FasterRiver failed to suppress crossing cost even on nonriver destination");
 m=basic();m["feature"]=5;m["feature_move"]=2;m["feature_double"]=1;m.erase("terrain_move");m.erase("terrain_double");need(bnw_plot_movement_cost_math(m)==60,"Feature double branch read terrain numeric suffix");
 m=basic();m["ignore"]=1;m.erase("terrain_move");m.erase("discount");need(bnw_plot_movement_cost_math(m)==60,"Ignore terrain branch demanded skipped terrain cost/discount");
 m=basic();m["rough"]=1;m["rough_ends"]=1;m.erase("terrain_double");need(bnw_plot_movement_cost_math(m)==120,"Rough ends did not clamp INT_MAX to actual base*denominator");
 m=basic();m["from_route"]=1;m["to_route"]=1;m["bridge"]=1;
 for(const char* n:{"from_route_move","to_route_move"})m[n]=30;
 for(const char* n:{"from_route_change","to_route_change"})m[n]=0;
 for(const char* n:{"from_route_flat","to_route_flat"})m[n]=10;
 need(bnw_plot_movement_cost_math(m)==20,"Route flat native base product/min differs");m["from_route_move"]=INT32_MAX;m["from_route_change"]=1;refuses([&]{(void)bnw_plot_movement_cost_math(m);});
 m=basic();m["own_target"]=1;m["feature"]=1;m["feature_move"]=2;m["feature_double"]=0;m["woods_road"]=1;m["road_move"]=30;m["road_flat"]=10;
 need(bnw_plot_movement_cost_math(m)==20,"Friendly jungle road fallback differs");
 m=basic();m["has_team"]=1;m["team_border"]=1;m["same_team"]=0;m["open_borders"]=0;need(bnw_plot_movement_cost_math(m)==120,"Great Wall addition lost");
 m["same_team"]=1;m.erase("open_borders");m.erase("player_border");need(bnw_plot_movement_cost_math(m)==60,"Great Wall short circuits read skipped player/borders getter");
 m=basic();m["terrain_move"]=INT32_MAX;refuses([&]{(void)bnw_plot_movement_cost_math(m);});
 m=basic();m["terrain_move"]=0;m.erase("discount");need(bnw_plot_movement_cost_math(m)==1,"Zero movement/minimum1/discount short circuit differs");
 m=basic();m["ignore"]=2;refuses([&]{(void)bnw_plot_movement_cost_math(m);});m=basic();m.erase("rough");refuses([&]{(void)bnw_plot_movement_cost_math(m);});
}
std::int64_t id(const saves::SavedUnitObservation& u){for(const auto& f:u.core.fields)if(f.name&&std::string_view(f.name)=="m_iID"&&f.values.size()==1)return f.values[0];throw std::runtime_error("Actual movement fullID absent");}
}
int main(int argc,char** argv){try{
 need(argc==3,"Expected SAVE and installation");pure();
 static_assert(!std::is_default_constructible_v<BnwPlotMovementCostQuery>);
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);need(selected.preview.host&&!selected.preview.blocker,"Actual selected Host missing");
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));const auto& s=host->session().state();need(s.world.all_players&&s.world.map_projection,"Actual movement fixture roster/map missing");
 const auto& h=s.world.map_header;const MapGeometry g{h.grid,h.wrap_x,h.wrap_y,saves::GameStateLayout::expansion2_version1};
 std::size_t seen=0,supported=0,unavailable=0;std::optional<BnwPlotMovementCostQuery> retained;std::optional<std::int32_t> retained_cost;
 for(const auto& p:s.world.all_players->players){
  for(const auto& u:p.units.units){
   if(seen>=24)break;
   const BnwUnitOrderKey key{p.saved_player_id,id(u)};const auto from=bnw_unit_order_plot(s,key);const auto xy=h.grid.coordinates(from);
   for(std::int32_t d=0;d<6&&seen<24;++d){const auto to=g.direction(static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]),d);if(!to||static_cast<std::uint64_t>(*to)==from)continue;++seen;
    try{auto q=bind_bnw_plot_movement_cost(host,key,static_cast<std::uint64_t>(*to));const auto value=q.value();++supported;
     need(value==inspect_bnw_plot_movement_cost(host,key,static_cast<std::uint64_t>(*to)),"Same genuine current movement read differs");
     if(!retained){retained=q;retained_cost=value;}
    }catch(const BnwPlotMovementCostUnavailable& e){++unavailable;std::cout<<"ACTUAL_COST_FRONTIER owner="<<key.first<<" id="<<key.second<<" target="<<*to<<" reason="<<e.what()<<'\n';}
   }
  }if(seen>=24)break;
 }
 refuses([&]{(void)inspect_bnw_plot_movement_cost({}, {0,0},0);});
 if(retained){
  bool foreign=false;std::thread t([&]{try{(void)retained->value();}catch(const BnwPlotMovementCostUnavailable&){foreign=true;}});t.join();need(foreign,"Movement read accepted foreign thread");
  need(retained->value()==retained_cost,"Foreign refusal altered genuine movement read");
  std::weak_ptr<ui::SelectedBnwPreviewHost> weak=host;selected.preview.host.reset();host.reset();need(!weak.expired()&&retained->value()==retained_cost,"Movement read did not retain SAME actual Host");
  {auto writer=weak.lock();need(bool(writer),"Actual retained Host gone");(void)writer->session().mutable_state();}
  refuses([&]{(void)retained->value();});
 }else std::cout<<"GENUINE_ORDINARY_MOVEMENT_POSITIVE_UNOBSERVED\n";
 std::cout<<"PASS movement source/math; actual_supported="<<supported<<" actual_frontiers="<<unavailable<<" examined="<<seen<<" movement_permission=0\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL movement cost: "<<e.what()<<'\n';return 1;}}
