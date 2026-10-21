#include "civ5/runtime/bnw_city_current_project_production_difference.hpp"
#include "civ5/runtime/bnw_city_production_turn.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool n,const char* why){if(!n)throw std::runtime_error(why);}
template<class F>std::string refusal(F&& f){try{f();}catch(const std::exception& e){return e.what();}throw std::runtime_error("Expected current production refusal");}
const auto& field(const auto& c,std::string_view name){for(const auto& f:c.core.fields)if(f.name&&name==f.name)return f;throw std::runtime_error("Actual production field absent");}
void run(const std::filesystem::path& save,const std::filesystem::path& installation){
 const auto sha=identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,installation,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));auto& session=host->session();
 // Native nested overflow before city cancellation; supplied math cannot issue.
 check(detail::bnw_project_modifier_math(1,-1,7,2)==9,"Native nested project result wrong");
 check(detail::bnw_project_modifier_math(1,-1,INT32_MIN+1,0)==INT32_MIN+1,"Valid native boundary wrong");
 (void)refusal([]{(void)detail::bnw_project_modifier_math(1,-1,INT32_MIN,0);});
 (void)refusal([]{(void)detail::bnw_project_modifier_math(-1,1,INT32_MAX,0);});
 (void)refusal([]{(void)detail::bnw_project_modifier_math(INT32_MAX,0,1,-1);});
 (void)refusal([]{(void)detail::bnw_project_modifier_math(INT32_MIN,0,0,-1);});
 // Explicit supplied arithmetic only. None of these contexts creates a writer.
 CityYieldContext pure;pure.yield_slots=6;pure.no_happiness=true;pure.players[0].cities[0].owner=0;pure.players[0].cities[0].id=0;pure.players[0].capital=-1;
 pure.players[0].cities[0].population=3;auto& values=pure.players[0].cities[0].yields[1];values.terrain=2;values.buildings=1;values.per_population=25;
 CityYieldRules rules;const auto defines=host->database().cache_global_defines(content::GlobalDefineProfile::bnw);
 const auto math=[&](std::int32_t modifier,std::int32_t overflow){return detail::bnw_native_current_production_difference_math(pure,rules,defines,0,0,modifier,overflow);};
 check(math(10,200)==612,"Native base/per-pop/modifier then overflow divide order wrong"); // 375*110/100+200
 check(math(-101,7)==7,"Extra modifier must enter native clamp before product");
 values.per_religion=INT64_MAX;values.process_modifier=INT64_MAX;
 check(math(0,0)==375,"Current difference read unused per-religion/process operands");
 values.terrain=INT32_MAX;(void)refusal([&]{(void)math(0,0);});values.terrain=2;
 values.per_population=INT32_MAX;(void)refusal([&]{(void)math(0,0);});values.per_population=25;
 (void)refusal([&]{(void)math(INT32_MAX,0);});
 (void)refusal([&]{(void)math(0,INT32_MAX);});
 saves::TradeRouteObservation route;route.origin_owner=0;route.origin_x=0;route.origin_y=0;route.destination_owner=-1;route.destination_x=1;route.destination_y=1;route.origin_yields.assign(6,0);route.destination_yields.assign(6,0);route.origin_yields[1]=13;pure.trade_routes.push_back(route);
 check(math(0,2)==390,"Native trade was not added after overflow");
 static_assert(!std::is_default_constructible_v<BnwCityCurrentProjectProductionDifference>);
 unsigned seen=0,observed=0,changed=0,ai=0,wrong_order=0,dependency=0;
 std::optional<BnwCityCurrentProjectProductionDifference> reader;std::pair<std::int64_t,std::int64_t> key{};
 for(const auto& [owner,player]:session.state().cities.players){
  for(const auto& [id,city]:player.cities){
   (void)city;if(seen==24){break;}++seen;
   try{reader=inspect_bnw_city_current_project_production_difference(host,owner,id);key={owner,id};++observed;}
   catch(const std::exception& e){const std::string text=e.what();
    if(text.find("AI_chooseProduction")!=std::string::npos)++ai;
    else if(text.find("queued item cost/modifier")!=std::string::npos||text.find("empty queue requires")!=std::string::npos)++wrong_order;
    else if(text.find("unavailable")!=std::string::npos||text.find("requires")!=std::string::npos||text.find("needs")!=std::string::npos){++dependency;std::cout<<"ACTUAL_PROJECT_FRONTIER "<<text<<'\n';}
    else throw;
   }
   if(reader){break;}
  }
  if(reader||seen==24){break;}
 }
 if(reader){
  const auto value=reader->value_times100();check(reader->value_times100()==value,"Actual current difference changed without mutation");
  bool foreign=false;std::thread thread([&]{try{(void)reader->value_times100();}catch(const std::exception&){foreign=true;}});thread.join();check(foreign,"Foreign production read accepted");
  const auto cp=session.checkpoint();const auto* raw=resolve_bnw_current_city(session.state(),static_cast<std::int32_t>(key.first),static_cast<std::int32_t>(key.second));check(raw,"Actual project city absent");
  const auto overflow_before=field(*raw,"m_iOverflowProduction").values.front(),feature_before=field(*raw,"m_iFeatureProduction").values.front();const auto revision=session.state().ui_source_revision;
  auto pending=session.begin_bnw_city_production_project(host,key.first,key.second);const auto p=pending->progress();
  check(p.difference_times100==value&&p.native_calls==1,"Actual current difference did not feed native project call");
  raw=resolve_bnw_current_city(session.state(),p.owner,p.city);check(raw,"Produced actual project city absent");
  const auto target=static_cast<std::int64_t>(p.project_before)+value;
  check(field(*raw,"m_paiProjectProduction").values.at(static_cast<std::size_t>(p.process))==target,"Raw project progress not stored");
  const auto& progress=*session.state().cities.players.at(p.owner).cities.at(p.city).production_progress;const auto it=progress.projects.find(p.process);check((it==progress.projects.end()?0:it->second)==target,"Owned project progress mirror not stored");
  if(target!=p.project_before){
   ++changed;check(p.phase==BnwCityProductionTurnPhase::city_screen_predicate_pending&&p.raw_writes==1&&session.state().ui_source_revision==revision+1,"Changed project did not stop at mandatory UI tail");
   check(field(*raw,"m_iOverflowProduction").values.front()==overflow_before&&field(*raw,"m_iFeatureProduction").values.front()==feature_before,"Native caches cleared before project UI tail");
   const auto why=refusal([&]{session.continue_bnw_city_production_process_suppressed(host,pending);});check(why.find("selection/banner UI owner")!=std::string::npos,"Project UI was skipped or falsely acknowledged");
   check(pending->progress().phase==BnwCityProductionTurnPhase::city_screen_predicate_pending,"Known frontier destroyed valid retained project tail");
   (void)refusal([&]{(void)reader->value_times100();});
  }else{
   session.continue_bnw_city_production_process_suppressed(host,pending);session.continue_bnw_city_production_process_suppressed(host,pending);
   check(pending->progress().phase==BnwCityProductionTurnPhase::complete_component||pending->progress().phase==BnwCityProductionTurnPhase::pop_order_pending,"Zero-change project native return frontier lost");
  }
  (void)refusal([&]{(void)session.begin_bnw_city_production_project(host,key.first,key.second);});
  const auto sealed=session.checkpoint();(void)sealed;
  const auto terrain_rules=BnwPlotTerrainRulesSnapshot::from_rules(host->database());const auto reason=refusal([&]{(void)make_bnw_plot_terrain_checkpoint_pair(session,host->identity(),terrain_rules);});check(reason.find("production-turn once history")!=std::string::npos,"Export lost project once history");
  session.restore(cp);(void)refusal([&]{pending->require_current();});(void)refusal([&]{(void)session.begin_bnw_city_production_project(host,key.first,key.second);});
 }
 std::cout<<"project_scan="<<seen<<" actual_difference="<<observed<<" actual_project_store="<<changed<<" ai="<<ai<<" wrong_order="<<wrong_order<<" source_frontier="<<dependency<<'\n';
 if(!observed)std::cout<<"GENUINE_CURRENT_PROJECT_DIFFERENCE_UNOBSERVED\n";
 if(!changed)std::cout<<"GENUINE_POSITIVE_PROJECT_STORE_UNOBSERVED\n";
 check(!BnwCityProductionTurnPending::full_production_ready()&&!session.readiness().can_continue(),"Numeric/project prefix falsely grants full turn");
 check(identify_file(save).sha256==sha,"Original save mutated");
}
}
int main(int argc,char** argv){try{check(argc==3,"Usage SAVE installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
