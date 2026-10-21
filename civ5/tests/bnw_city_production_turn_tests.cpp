#include "civ5/runtime/bnw_city_production_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool n,const char* why){if(!n)throw std::runtime_error(why);}
template<class F>std::string refusal(F&& f){try{f();}catch(const std::exception& e){return e.what();}throw std::runtime_error("Expected actual production source refusal");}
std::int64_t scalar(const auto& c,std::string_view name){for(const auto& f:c.core.fields)if(f.name&&name==f.name){check(f.values.size()==1,"Actual city scalar missing");return f.values.front();}throw std::runtime_error("Actual city scalar absent");}
void run(const std::filesystem::path& save,const std::filesystem::path& installation){
 const auto sha=identify_file(save).sha256;
 // Native pure operands never issue a mutable city owner.
 check(detail::bnw_production_supply_math(0,0,0,0,0,3,1,70)==-20,"Native trade subtraction wrong");
 check(detail::bnw_production_supply_math(2,1,3,10,15,10,1,70)==-30,"Native population truncate/order wrong");
 check(detail::bnw_production_supply_math(100,0,0,0,0,1,0,70)==0,"Negative out-of-supply not clamped");
 check(detail::bnw_production_supply_math(0,0,0,0,0,8,0,70)==-70,"Native supply cap wrong");
 (void)refusal([]{(void)detail::bnw_production_supply_math(INT32_MAX,1,1,0,0,0,0,70);});
 (void)refusal([]{(void)detail::bnw_production_supply_math(0,0,0,INT32_MAX,2,0,0,70);});
 (void)refusal([]{(void)detail::bnw_production_supply_math(0,0,0,0,0,INT32_MAX,0,70);}); // before min
 (void)refusal([]{(void)detail::bnw_production_supply_math(0,0,0,0,0,0,0,INT32_MIN);}); // native negation
 static_assert(!std::is_default_constructible_v<BnwCityProductionTurnPending>);
 auto boot=ui::bootstrap_selected_bnw_preview(save,installation,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));auto& session=host->session();
 unsigned seen=0,supported=0,ai=0,empty=0,items=0,unsuppressed=0,league=0;bool mutated=false;
 std::shared_ptr<const BnwCityProductionTurnPending> pending;
 std::pair<std::int64_t,std::int64_t> key;
 const auto before=session.checkpoint();
 for(const auto& [owner,player]:session.state().cities.players){
  for(const auto& [id,c]:player.cities){
   (void)c;if(seen==24){break;}++seen;
   try{pending=session.begin_bnw_city_production_process_suppressed(host,owner,id);key={owner,id};++supported;}
   catch(const std::exception& e){const std::string text=e.what();
    if(text.find("AI_chooseProduction")!=std::string::npos)++ai;
    else if(text.find("empty queue requires")!=std::string::npos)++empty;
    else if(text.find("queued item cost/modifier")!=std::string::npos)++items;
    else if(text.find("unsuppressed producer")!=std::string::npos)++unsuppressed;
    else if(text.find("matching LeagueProject")!=std::string::npos)++league;
    else throw;
   }
   if(pending){break;}
  }
  if(pending||seen==24){break;}
 }
 if(pending){
  check(pending->progress().native_calls==1,"Process call marker not retained before setters");
  bool foreign=false;std::thread t([&]{try{pending->require_current();}catch(const std::exception&){foreign=true;}});t.join();check(foreign,"Foreign production cursor accepted");pending->require_current();
  (void)refusal([&]{(void)session.begin_bnw_city_production_process_suppressed(host,key.first,key.second);});
  session.continue_bnw_city_production_process_suppressed(host,pending);
  check(pending->progress().native_calls==2&&pending->progress().raw_writes==1,"Overflow setter prefix missing");
  auto partial=session.checkpoint();(void)partial;
  session.continue_bnw_city_production_process_suppressed(host,pending);
  check(pending->progress().phase==BnwCityProductionTurnPhase::complete_component&&pending->progress().native_calls==3&&pending->progress().raw_writes==2,"Feature setter or source return missing");
  const auto* raw=resolve_bnw_current_city(session.state(),static_cast<std::int32_t>(key.first),static_cast<std::int32_t>(key.second));check(raw,"Actual produced city absent");
  check(scalar(*raw,"m_iOverflowProduction")==0&&scalar(*raw,"m_iFeatureProduction")==0,"Native production cache stores not applied");
  const auto& owned=*session.state().cities.players.at(key.first).cities.at(key.second).production_progress;check(owned.overflow_times100==0&&owned.feature_times100==0,"Paired production cache mirrors not applied");
  mutated=pending->progress().overflow_before!=0||pending->progress().feature_before!=0;
  (void)refusal([&]{session.continue_bnw_city_production_process_suppressed(host,pending);});
  const auto rules=BnwPlotTerrainRulesSnapshot::from_rules(host->database());
  const auto reason=refusal([&]{(void)make_bnw_plot_terrain_checkpoint_pair(session,host->identity(),rules);});check(reason.find("production-turn once history")!=std::string::npos,"File export lost production once history");
  // An older CP cannot erase a genuinely issued no-change/partial operation.
  session.restore(before);(void)refusal([&]{pending->require_current();});
  (void)refusal([&]{(void)session.begin_bnw_city_production_process_suppressed(host,key.first,key.second);});
  const auto receiver_reason=refusal([&]{BnwPlotTerrainCheckpointPair bytes;(void)restore_bnw_plot_terrain_checkpoint_pair(bytes,session,host->identity(),rules);});check(receiver_reason.find("production-turn once history")!=std::string::npos,"File restore bypassed retained production history");
  // A moved facade retains the same once tombstone but revokes old pointers.
  auto moved=std::move(session);(void)refusal([&]{pending->require_current();});
  check(!moved.readiness().can_continue(),"Moved component falsely grants whole-game continuation");
 }
 std::cout<<"genuine_scan="<<seen<<" supported="<<supported<<" actual_changed="<<mutated<<" ai_frontier="<<ai<<" empty_frontier="<<empty<<" item_frontier="<<items<<" unsuppressed_frontier="<<unsuppressed<<" league_frontier="<<league<<'\n';
 if(!supported)std::cout<<"GENUINE_SUPPRESSED_PROCESS_MUTATION_UNOBSERVED\n";
 if(!mutated)std::cout<<"NONZERO_CACHE_CLEAR_UNOBSERVED\n";
 check(!BnwCityProductionTurnPending::full_production_ready(),"Component incorrectly grants full production");
 check(identify_file(save).sha256==sha,"Original save changed");
}
}
int main(int argc,char** argv){try{check(argc==3,"Usage SAVE installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
