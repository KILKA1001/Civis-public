#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/bnw_city_current_land_improvement_strategy.hpp"
#include "civ5/runtime/bnw_city_current_capital_strategy.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Invalid ordered city strategy continuation accepted");}
std::int64_t scalar(const saves::SavedCityObservation& c,std::string_view name){const saves::CityCoreField* out=nullptr;for(const auto& f:c.core.fields)if(f.name&&name==f.name){need(!out,"Fixture duplicate core field");out=&f;}need(out&&out->values.size()==1,"Actual fixture core scalar absent");return out->values.front();}
bool same(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto r=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(r.preview.blocker)throw std::runtime_error(r.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> h(std::move(r.preview.host));need(bool(h),"Actual immutable selected Host absent");return h;}
bool known_new_source_frontier(std::string_view message){
 return message=="Land improvement military using requires genuine native Init/Reset/Read owner"||
  message=="Land improvement missing using slot requires native Reset/Read owner"||
  message=="WantTileImprovers reached genuine Player::canBuild/Plot::canBuild owner unavailable; no false inference"||
  message=="WantTileImprovers reached genuine native calculateGoldRate owner unavailable; wide treasury quote is not current source readiness"||
  message=="Capital strategy omitted reached using slot requires actual native Reset/Read owner"||
  message=="Capital strategy missing/unsafe Military using prefix requires native Reset/Read owner"||
  message=="Capital strategy finite native Military Info allocation has unsupported hole/order";
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto input=identify_file(save).sha256;
 std::shared_ptr<const BnwCityEntryPending> retained;
 std::weak_ptr<ui::SelectedBnwPreviewHost> weak;
 {
 auto host=boot(save,game);weak=host;auto& session=host->session();
 std::int64_t owner=-1,city=-1;
 for(const auto& p:session.state().world.all_players->players){for(const auto& c:p.cities.cities)if(scalar(c,"m_iDamage")==0){owner=p.saved_player_id;city=scalar(c,"m_iID");break;}if(owner!=-1)break;}
 need(owner!=-1,"Actual selected fixture has no damage-zero city");
 const auto feature=BnwCityEntryFeatureRules::from_rules(host->database());
 retained=session.begin_bnw_city_entry_damage_zero(owner,city);
 refuses([&]{(void)session.continue_bnw_city_entry_strategy(host,retained);}); // actual predecessor incomplete
 retained=session.continue_bnw_city_entry_feature_surrounded(owner,city,feature);
 const auto before_strategy=session.checkpoint();
 const auto before=make_bnw_city_management_checkpoint(session.state().world,session.source().snapshot,host->identity(),true);
 refuses([&]{(void)session.continue_bnw_city_entry_strategy(host,{});});
 auto generic=LiveSession::from_restored_bnw(session.source(),session.state());
 refuses([&]{(void)generic.continue_bnw_city_entry_strategy(host,retained);});
 bool denied=false;std::thread worker([&]{try{(void)session.continue_bnw_city_entry_strategy(host,retained);}catch(const std::exception&){denied=true;}});worker.join();need(denied,"Foreign source thread entered strategy loop");
 std::uint32_t advances=0;std::optional<BnwCityStrategyLoopFrontier> stop;
 for(std::uint32_t bounded=0;bounded<4096;++bounded){
  const auto previous=retained->progress();
  bool reached_source_failure=false;
  try{retained=session.continue_bnw_city_entry_strategy(host,retained);}
  catch(const BnwCityLandImprovementUnavailable& e){need(known_new_source_frontier(e.what()),e.what());reached_source_failure=true;std::cout<<"Actual mandatory Land source frontier: "<<e.what()<<'\n';}
  catch(const BnwCityCapitalStrategyUnavailable& e){need(known_new_source_frontier(e.what()),e.what());reached_source_failure=true;std::cout<<"Actual mandatory Capital source frontier: "<<e.what()<<'\n';}
  const auto current=retained->progress();
  if(reached_source_failure)need(current.strategy_frontier==BnwCityStrategyLoopFrontier::source_inputs_unavailable&&
   current.strategy_next_id==previous.strategy_next_id&&current.strategy_steps==previous.strategy_steps&&
   current.strategy_trigger_calls==previous.strategy_trigger_calls+1,
   "Actual failed helper advanced the cursor or lost its once attempt/history");
  need(current.phase==BnwCityEntryPhase::citystrategy_pending&&current.strategy_started&&
   current.strategy_next_id>=0&&current.strategy_next_id<=current.strategy_slots&&
   current.strategy_trigger_calls>=previous.strategy_trigger_calls&&
   current.strategy_trigger_calls<=previous.strategy_trigger_calls+1,
   "Ordered cursor lost phase/domain or replayed trigger calls");
  need(!BnwCityEntryPending::full_city_turn_ready(),"Partial loop claimed full city turn");
  need(current.strategy_frontier!=BnwCityStrategyLoopFrontier::missing_trigger,"One of the44 SDK Types remained an unconnected dispatcher frontier");
  if(current.strategy_frontier==BnwCityStrategyLoopFrontier::none){
   need(current.strategy_next_id==previous.strategy_next_id+1&&current.strategy_steps==previous.strategy_steps+1,
    "Closed no-effect branch skipped/replayed native IDs");++advances;
  }else{need(current.strategy_next_id==previous.strategy_next_id&&current.strategy_steps==previous.strategy_steps,
    "Mandatory trigger/Lua/effect frontier advanced native ID");stop=current.strategy_frontier;break;}
 }
 need(stop.has_value(),"Actual strategy loop exceeded bounded fixture budget");
 const auto stopped=retained->progress();
 for(int i=0;i<2;++i){auto alias=retained;need(session.continue_bnw_city_entry_strategy(host,alias).get()==retained.get(),"Registered continuation replaced actual Pending");const auto p=alias->progress();need(p.strategy_next_id==stopped.strategy_next_id&&p.strategy_steps==stopped.strategy_steps&&p.strategy_trigger_calls==stopped.strategy_trigger_calls&&p.strategy_frontier==stopped.strategy_frontier,"Frontier replay consumed an ID/trigger");}
 if(!stopped.strategy_publications)need(same(before,make_bnw_city_management_checkpoint(session.state().world,session.source().snapshot,host->identity(),true)),"Blocked/no-effect prefix changed management without publication history");
 else need(stopped.strategy_revision==stopped.strategy_publications&&stopped.strategy_effect_attempts==stopped.strategy_publications,"Actual end effects lost paired revision/history");
 std::cout<<"actual owner="<<owner<<" fullcity="<<city<<" slots="<<stopped.strategy_slots<<" next="<<stopped.strategy_next_id<<" closed_advances="<<advances<<" trigger_calls="<<stopped.strategy_trigger_calls<<" frontier="<<static_cast<int>(*stop)<<'\n';
 if(!advances)std::cout<<"Actual none/minor/falseSTART advance UNOBSERVED; first mandatory source frontier retained\n";
 // Old checkpoint must not replay an already evaluated helper or closed IDs.
 session.restore(before_strategy);const auto after=retained->progress();
 need(after.phase==BnwCityEntryPhase::quarantined&&after.strategy_next_id>=stopped.strategy_next_id&&
  after.strategy_trigger_calls>=stopped.strategy_trigger_calls&&after.strategy_steps>=stopped.strategy_steps,
  "Checkpoint rollback rewound actual ordered once history");
 refuses([&]{(void)session.continue_bnw_city_entry_strategy(host,retained);});
 refuses([&]{retained->require_current();});
 // No VM/query/Host owner is retained by Session -> Pending. Releasing the sole
 // external Host leaves the retained metadata cursor quarantined, not a cycle.
 host.reset();
 }
 need(weak.expired()&&retained->progress().phase==BnwCityEntryPhase::quarantined,
  "Ordered Pending formed Host/session backedge or revived world after facade release");
 retained.reset();need(identify_file(save).sha256==input,"Original selected save changed");
}
}
int main(int argc,char** argv){try{need(argc==3,"Expected SAVE and canonical shared installation");run(argv[1],argv[2]);std::cout<<"bnw ordered city strategy prefix tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
