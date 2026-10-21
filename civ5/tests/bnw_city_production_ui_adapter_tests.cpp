#include "civ5/ui/bnw_city_production_ui_adapter.hpp"
#include "bnw_selected_city_test_locale.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_city_current_project_production_difference.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>
using namespace civ5;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void refuses(F&& f,std::string_view why={}){
 bool failed=false;try{f();}catch(const std::exception& e){failed=true;need(why.empty()||std::string_view(e.what()).find(why)!=std::string_view::npos,"Wrong production UI refusal");}
 need(failed,"Invalid production UI operation accepted");
}
ui::UiContextRegistry registry(){
 // Existing genuine Civis route. This does not issue a stock Lua epoch.
 return ui::UiContextRegistry::from_documents(content::read_xml_metadata(R"(<Context>
 <LuaContext ID="WorldView" FileName="WorldView"/>
 <LuaContext ID="CityList" FileName="CityList" Hidden="True"/>
 <LuaContext ID="CityView" FileName="CityView" Hidden="True"/>
 </Context>)"),content::read_xml_metadata("<Context/>"));
}
std::int64_t scalar(const saves::SavedCityObservation& city,std::string_view name){
 for(const auto& f:city.core.fields)if(f.name&&name==f.name){need(f.values.size()==1,"Actual cache scalar absent");return f.values.front();}
 throw std::runtime_error("Actual production cache field absent");
}

// These supplied descriptors exercise validation only. They do not issue a
// production cursor, a stock VM loan, a source revision or a BANNER return.
void descriptor_tests(){
 using P=runtime::BnwCityProductionTurnProgress;using Phase=runtime::BnwCityProductionTurnPhase;
 using E=runtime::BnwCityDirtyEvent;using Kind=runtime::BnwCityDirtyKind;
 const P before{17,0,8192,-1,30,40,125,200,900,1,1,Phase::city_screen_predicate_pending};
 auto after=before;after.native_calls=3;after.phase=Phase::banner_event_pending;
 const std::vector<E> prefix{{6,9,0,8192,-1,-1,Kind::city_screen,true}};
 auto appended=prefix;appended.push_back({7,9,0,8192,-1,-1,Kind::city_screen,true});
 const auto validate=[&](const P& b,const P& a,std::uint64_t oldrev,std::uint64_t newrev,
                         std::uint64_t oldord,std::uint64_t neword,const std::vector<E>& q0,const std::vector<E>& q1){
  ui::detail::require_production_city_screen_delta(b,a,oldrev,newrev,oldord,neword,9,q0,q1);
 };
 validate(before,after,11,12,7,8,prefix,appended);
 // Repeated identity is a second ordered occurrence, not a coalesced event.
 need(appended[0].owner==appended[1].owner&&appended[0].city==appended[1].city&&appended[0].occurrence!=appended[1].occurrence,"Ordered CityScreen descriptor test lost duplicate identity");
 auto resumed=before;resumed.phase=Phase::city_screen_dirty_pending;resumed.native_calls=2;
 validate(resumed,after,11,12,7,8,prefix,appended);
 auto skipped=before;skipped.native_calls=2;skipped.phase=Phase::banner_event_pending;
 validate(before,skipped,UINT64_MAX,UINT64_MAX,UINT64_MAX,UINT64_MAX,prefix,prefix);
 refuses([&]{validate(resumed,after,11,11,7,7,prefix,prefix);});
 refuses([&]{validate(before,after,11,13,7,8,prefix,appended);});
 refuses([&]{validate(before,after,11,12,7,9,prefix,appended);});
 refuses([&]{validate(before,after,UINT64_MAX,0,7,8,prefix,appended);});
 refuses([&]{validate(before,after,11,12,UINT64_MAX,0,prefix,appended);});
 auto queue=appended;queue.front().value=false;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.push_back(queue.back());refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().generation=10;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().occurrence=8;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().owner=1;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().city=0;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().plot=0;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().yield=0;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 queue=appended;queue.back().value=false;refuses([&]{validate(before,after,11,12,7,8,prefix,queue);});
 auto changed=after;changed.turn=18;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.owner=1;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.city=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.process=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.overflow_before=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.feature_before=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.difference_times100=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.project_before=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.production_needed=0;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.raw_writes=2;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.native_calls=2;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 changed=after;changed.phase=Phase::overflow_pending;refuses([&]{validate(before,changed,11,12,7,8,prefix,appended);});
 auto exhausted=before;exhausted.native_calls=255;changed=after;changed.native_calls=0;
 refuses([&]{validate(exhausted,changed,11,12,7,8,prefix,appended);});
 std::cout<<"SUPPLIED_CITYSCREEN_DELTA_VALIDATION_ONLY no_VM_or_production_issuer\n";
}

void run(const std::filesystem::path& save,const std::filesystem::path& installation){
 descriptor_tests();
 static_assert(!std::is_default_constructible_v<ui::BnwCityProductionUiAdapter>);
 const auto save_sha=runtime::identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,installation,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));auto& session=host->session();
 auto adapter=ui::BnwCityProductionUiAdapter::from_selected_host(host);
 refuses([]{(void)ui::BnwCityProductionUiAdapter::from_selected_host({});},"Host absent");
 refuses([&]{(void)adapter->continue_city({});},"cursor absent");
 bool foreign_factory=false,foreign_current=false;
 std::thread worker([&]{
  try{(void)ui::BnwCityProductionUiAdapter::from_selected_host(host);}catch(const std::exception&){foreign_factory=true;}
  try{adapter->require_current();}catch(const std::exception&){foreign_current=true;}
 });worker.join();need(foreign_factory&&foreign_current,"Foreign production UI source read accepted");
 auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
 auto live_ui=client->create_selected_ui_host(registry(),nullptr);
 const ui::ClientViewport viewport{1024,768,1024,768,96};
 auto frame=client->capture(viewport,{});
 const auto* city_screen=frame->ui.find<ui::CityScreenFrame>();need(city_screen,"Actual city frame absent");
 std::optional<std::pair<std::int64_t,std::int64_t>> chosen;
 unsigned seen=0,supported=0,ai=0,wrong_order=0,dependencies=0,zero_change=0,changed=0,predicate_false=0,city_screen_events=0;
 for(const auto& [owner,player]:session.state().cities.players){
  for(const auto& [id,city]:player.cities){
   (void)city;if(seen==24){break;}++seen;
   try{
    auto reader=runtime::inspect_bnw_city_current_project_production_difference(host,owner,id);
    ++supported;if(reader.value_times100()!=0){chosen=std::pair{owner,id};break;}++zero_change;
   }catch(const std::exception& e){const std::string text=e.what();
    if(text.find("AI_chooseProduction")!=std::string::npos)++ai;
    else if(text.find("queued item cost/modifier")!=std::string::npos||text.find("empty queue requires")!=std::string::npos)++wrong_order;
    else if(text.find("unavailable")!=std::string::npos||text.find("requires")!=std::string::npos||text.find("needs")!=std::string::npos){++dependencies;std::cout<<"ACTUAL_PROJECT_UI_FRONTIER "<<text<<'\n';}
    else throw;
   }
  }
  if(chosen||seen==24){break;}
 }
 if(chosen){
  bool selected=false;
  for(const auto& city:city_screen->cities)if(city.owner==chosen->first&&city.id==chosen->second){
   std::cout<<"Actual PROJECT city localized name="<<(city.name?"present":"absent")<<'\n';
   need(city.name&&!city.name->empty(),"Actual selected city EN localized name absent");selected=true;break;
  }
  if(selected){
   auto ui_frame=live_ui->begin_frame(1024,768);
   need(client->dispatch(*live_ui,ui_frame,frame,ui::UiOpenScreen{"CityList"}).status==ui::UiCommandStatus::navigation,"Actual CityList route refused");
   frame=client->capture(viewport,{});ui_frame=live_ui->begin_frame(1024,768);
   const ui::UiCommand open(ui::UiGameplayCommand::make("CityList",ui::WorldViewAction{ui::WorldViewActionKind::open_city,chosen->second}));
   const auto result=live_ui->dispatch(open,ui_frame);
   if(result.status!=ui::UiCommandStatus::navigation)throw std::runtime_error("Actual canonical city selection refused: "+result.reason);
   frame=client->capture(viewport,{});
  }
  (void)client->take_city_dirty_events(); // actual prior transfer; never a completion ack
  const auto before=session.checkpoint();
  auto pending=session.begin_bnw_city_production_project(host,chosen->first,chosen->second);const auto progress=pending->progress();
  need(progress.phase==runtime::BnwCityProductionTurnPhase::city_screen_predicate_pending,"Actual changed project did not reach UI tail");++changed;
  const auto* raw=runtime::resolve_bnw_current_city(session.state(),progress.owner,progress.city);need(raw,"Actual changed city disappeared");
  const auto overflow=scalar(*raw,"m_iOverflowProduction"),feature=scalar(*raw,"m_iFeatureProduction");
  const auto revision=session.state().ui_source_revision,ordinal=session.state().city_dirty_next_occurrence;
  const auto queue_before=session.state().city_dirty_events;
  frame=client->capture(viewport,{}); // AFTER real PROJECT store; isolate CityScreen revision effect
  need(client->is_current(frame),"Current post-store frame absent before notification");
  const auto tail=adapter->continue_city(pending);
  ui::detail::require_production_city_screen_delta(progress,tail,revision,session.state().ui_source_revision,
      ordinal,session.state().city_dirty_next_occurrence,session.state_generation(),queue_before,session.state().city_dirty_events);
  need(tail.phase==runtime::BnwCityProductionTurnPhase::banner_event_pending,"Production CityScreen falsely completed banner return");
  if(selected){
   need(!client->is_current(frame),"Actual notification failed to retire source frame");
   frame=client->capture(viewport,{});const auto events=client->take_city_dirty_events();
   need(events.size()==1,"Selected project notification duplicated or dropped");const auto& e=events.front();
   need(e.occurrence==ordinal&&e.generation==session.state_generation()&&e.owner==progress.owner&&e.city==progress.city&&e.kind==runtime::BnwCityDirtyKind::city_screen&&e.value&&e.plot==-1&&e.yield==-1,"Actual ordered CityScreen identity differs");
   need(session.state().ui_source_revision==revision+1&&session.state().city_dirty_next_occurrence==ordinal+1&&tail.native_calls==progress.native_calls+2,"Selected predicate/publication native order differs");++city_screen_events;
  }else{
   need(session.state().city_dirty_events.empty()&&session.state().ui_source_revision==revision&&session.state().city_dirty_next_occurrence==ordinal&&tail.native_calls==progress.native_calls+1,"False source predicate published a notification");++predicate_false;
  }
  raw=runtime::resolve_bnw_current_city(session.state(),progress.owner,progress.city);need(raw,"Current city disappeared after UI publication");
  need(scalar(*raw,"m_iOverflowProduction")==overflow&&scalar(*raw,"m_iFeatureProduction")==feature,"Caches cleared before authentic SpecificCityInfoDirty return");
  refuses([&]{(void)adapter->continue_city(pending);},"SpecificCityInfoDirty BANNER return unavailable");
  refuses([&]{session.continue_bnw_city_production_process_suppressed(host,pending);},"SpecificCityInfoDirty BANNER return unavailable");
  need(pending->progress().phase==runtime::BnwCityProductionTurnPhase::banner_event_pending,"Known mandatory frontier erased healthy once history");
  refuses([&]{(void)session.begin_bnw_city_production_project(host,chosen->first,chosen->second);});
  const auto after=session.checkpoint();(void)after;session.restore(before);
  refuses([&]{adapter->require_current();});refuses([&]{pending->require_current();});
  refuses([&]{(void)session.begin_bnw_city_production_project(host,chosen->first,chosen->second);});
 }else{
  const auto checkpoint=session.checkpoint();session.restore(checkpoint);refuses([&]{adapter->require_current();});
 }
 std::cout<<"project_ui_scan="<<seen<<" supported="<<supported<<" changed="<<changed<<" selected_event="<<city_screen_events<<" predicate_false="<<predicate_false<<" zero_change_reads="<<zero_change<<" ai="<<ai<<" wrong_order="<<wrong_order<<" dependencies="<<dependencies<<'\n';
 if(!changed)std::cout<<"GENUINE_PROJECT_UI_TAIL_UNOBSERVED\n";
 if(!city_screen_events)std::cout<<"GENUINE_SELECTED_PROJECT_CITYSCREEN_UNOBSERVED\n";
 if(!predicate_false)std::cout<<"GENUINE_OWNER_MISMATCH_PROJECT_PREDICATE_UNOBSERVED\n";
 need(!runtime::BnwCityProductionTurnPending::full_production_ready()&&!session.readiness().can_continue(),"UI component falsely grants production/game continuation");
 std::cout<<"GENUINE_PRIVATE_STOCK_CITYSCREEN_DISPATCH_UNOBSERVED required_graph_callsite_missing\n";
 need(runtime::identify_file(save).sha256==save_sha,"Original save mutated");
}
}
int main(int argc,char** argv){try{need(argc==3,"Usage SAVE installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
