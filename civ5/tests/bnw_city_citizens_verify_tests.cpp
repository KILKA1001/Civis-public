#include "civ5/runtime/bnw_player_happiness_prefix_owner.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "bnw_selected_city_test_locale.hpp"
#include <array>
#include <climits>
#include <filesystem>
#include <iostream>
#include <map>
#include <thread>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Missing Citizens producer accepted");}
void math(){
 std::array<std::int16_t,6> cache{};std::array<std::int32_t,6> terrain{};
 auto a=detail::bnw_citizens_unwork_prefix(false,2,3,cache,terrain);
 need(a.working==1&&a.unassigned==4&&!a.first_terrain,"Native unwork count arithmetic differs");
 a=detail::bnw_citizens_unwork_prefix(true,INT32_MIN,INT32_MAX,cache,terrain);
 need(a.working==INT32_MIN&&a.unassigned==INT32_MAX&&!a.first_terrain,"Home plot reached count arithmetic");
 cache[2]=INT16_MIN;terrain[2]=-17;cache[3]=1;terrain[3]=INT32_MIN;
 a=detail::bnw_citizens_unwork_prefix(false,1,0,cache,terrain);
 need(a.first_terrain&&a.first_terrain->first==2&&a.first_terrain->second==32751,
  "Signed short delta/native first UI boundary differs"); // Later overflow not reached.
 terrain[2]=INT32_MAX;refuses([&]{(void)detail::bnw_citizens_unwork_prefix(false,1,0,cache,terrain);});
 cache.fill(0);refuses([&]{(void)detail::bnw_citizens_unwork_prefix(false,0,0,cache,terrain);});
 refuses([&]{(void)detail::bnw_citizens_unwork_prefix(false,1,INT32_MAX,cache,terrain);});
 cache.fill(0);terrain.fill(0);cache[0]=2;terrain[0]=7;
 a=detail::bnw_citizens_work_prefix(false,true,0,1,cache,terrain);
 need(a.working==1&&a.unassigned==0&&a.first_terrain&&a.first_terrain->second==9,"Real work direction scalar/terrain math differs");
 a=detail::bnw_citizens_work_prefix(true,true,INT32_MAX,INT32_MIN,cache,terrain);
 need(a.working==INT32_MAX&&a.unassigned==INT32_MIN&&a.first_terrain->second==9,"Home assignment changed source counts");
 refuses([&]{(void)detail::bnw_citizens_work_prefix(false,true,INT32_MAX,1,cache,terrain);});
 refuses([&]{(void)detail::bnw_citizens_work_prefix(false,true,0,0,cache,terrain);});
 terrain[0]=INT32_MAX;refuses([&]{(void)detail::bnw_citizens_work_prefix(false,true,0,1,cache,terrain);});
 saves::SavedMapProjection map;map.grid.width=12;map.grid.height=12;map.wrap_x=false;map.wrap_y=false;
 const MapGeometry g{map.grid,false,false,saves::GameStateLayout::expansion2_version1};
 for(const auto cy:{6,5}){const auto positions=detail::bnw_citizen_plot_positions(g,6,cy);
  for(std::size_t i=0;i<37;++i){need(positions[i].has_value(),"Interior native37 forward absent");const auto xy=map.grid.coordinates(static_cast<std::uint64_t>(*positions[i]));
   need(detail::bnw_citizen_plot_slot(map,6,cy,static_cast<std::int32_t>(xy[0]),static_cast<std::int32_t>(xy[1]))==static_cast<std::int32_t>(i),"Native37 inverse differs from even/odd interior forward");}}
 // SDK wrap operations act on each axis independently; asymmetric maps are
 // intentionally wider than the native ring so inverse uniqueness is known.
 for(const bool horizontal:{true,false}){
  saves::SavedMapProjection asymmetric;asymmetric.grid.width=horizontal?7:11;asymmetric.grid.height=horizontal?11:8;asymmetric.wrap_x=horizontal;asymmetric.wrap_y=!horizontal;
  const std::int32_t cx=horizontal?0:5,cy=horizontal?5:0;
  const MapGeometry wrapped_axis{asymmetric.grid,horizontal,!horizontal,saves::GameStateLayout::expansion2_version1};
  const auto positions=detail::bnw_citizen_plot_positions(wrapped_axis,cx,cy);
  for(std::size_t i=0;i<37;++i){need(positions[i].has_value(),"Asymmetric native wrap lost a plot");const auto xy=asymmetric.grid.coordinates(static_cast<std::uint64_t>(*positions[i]));
   need(detail::bnw_citizen_plot_slot(asymmetric,cx,cy,static_cast<std::int32_t>(xy[0]),static_cast<std::int32_t>(xy[1]))==static_cast<std::int32_t>(i),"Native37 independent-axis wrap inverse differs");}
 }
 map.grid.width=2;map.grid.height=2;map.wrap_x=true;map.wrap_y=true;
 const MapGeometry small{map.grid,true,true,saves::GameStateLayout::expansion2_version1};
 const auto duplicate=detail::bnw_citizen_plot_positions(small,0,0);
 std::size_t inverse_differs=0;for(std::size_t i=0;i<37;++i){if(!duplicate[i])continue;const auto xy=map.grid.coordinates(static_cast<std::uint64_t>(*duplicate[i]));
  const auto inverse=detail::bnw_citizen_plot_slot(map,0,0,static_cast<std::int32_t>(xy[0]),static_cast<std::int32_t>(xy[1]));
  if(inverse!=static_cast<std::int32_t>(i))++inverse_differs;}
 need(inverse_differs>0,"Wrapped fixture did not distinguish scan versus native inverse index");
}
std::int64_t scalar(const saves::SavedCityObservation& c,const char* name){for(const auto& f:c.core.fields)if(f.name&&std::string_view(f.name)==name){need(f.values.size()==1,"Actual core scalar absent");return f.values.front();}throw std::runtime_error("Actual native city field absent");}
struct CitizensFact {
 std::array<std::int64_t,37> forced{},working{};
 std::int64_t forced_count=0,working_count=0,unassigned=0,defaults=0,forced_defaults=0;
 std::map<std::int64_t,std::int64_t> specialists;
};
CitizensFact citizens_fact(const LiveSession& session,std::int64_t owner,std::int64_t city,const content::InfoTypeCatalog& types){
 const auto& live=session.state();const saves::SavedCityObservation* actual=nullptr;
 for(const auto& player:live.world.all_players->players)for(const auto& c:player.cities.cities)
  if(player.saved_player_id==owner&&scalar(c,"m_eOwner")==owner&&scalar(c,"m_iID")==city)actual=&c;
 need(actual,"Actual reallocation canonical city absent");
 CitizensFact result;bool found=false;
 for(const auto& block:actual->management.blocks)if(block.name&&std::string_view(block.name)=="citizens"){
  need(!found,"Duplicate actual Citizens block");found=true;
  const auto value=[&](const char* name){for(const auto& f:block.fields)if(f.name&&std::string_view(f.name)==name){
   need(f.value>=INT32_MIN&&f.value<=INT32_MAX,"Actual Citizens native scalar width differs");return f.value;}
   throw std::runtime_error("Actual Citizens native scalar absent");};
  result.forced_count=value("m_iNumForcedWorkingPlots");result.working_count=value("m_iNumCitizensWorkingPlots");result.unassigned=value("m_iNumUnassignedCitizens");result.defaults=value("m_iNumDefaultSpecialists");result.forced_defaults=value("m_iNumForcedDefaultSpecialists");
  const auto flags=[&](const char* name){std::array<std::int64_t,37> out{};bool present=false;
   for(const auto& a:block.arrays)if(a.name&&std::string_view(a.name)==name){need(!present&&a.columns==1&&a.values.size()==37,"Actual Citizens flag shape differs");present=true;
    for(std::size_t i=0;i<37;++i){need(a.values[i]==0||a.values[i]==1,"Actual Citizens flag is not native bool");out[i]=a.values[i];}}
   need(present,"Actual Citizens flag array absent");return out;};
  result.forced=flags("m_pabForcedWorkingPlot");result.working=flags("m_pabWorkingPlot");
  for(const auto& array:block.hashes)if(array.name&&std::string_view(array.name)=="m_aiSpecialistCounts"){
   need(array.wire_count>=0&&static_cast<std::uint64_t>(array.wire_count)==array.rows.size(),"Actual specialist ordered wire count differs");
   for(const auto& row:array.rows){if(!row.type_hash){need(row.values.empty(),"Actual null specialist hash carried value");continue;}
    need(row.values.size()==1&&row.values.front()>=INT32_MIN&&row.values.front()<=INT32_MAX,"Actual specialist signed count width differs");
    const auto* remap=types.find_hash(row.type_hash);if(!remap)continue;
    need(remap->table=="Specialists"&&remap->id>=0,"Actual specialist count remap is not native Specialists");result.specialists[remap->id]=row.values.front();
   }
  }

 }
 need(found,"Actual Citizens block absent");
 bool terrain=false;for(const auto& f:actual->core.fields)if(f.name&&std::string_view(f.name)=="m_aiBaseYieldRateFromTerrain"){
  need(f.values.size()==6,"Actual reallocation terrain shape differs");terrain=true;
  const auto& yields=live.cities.players.at(owner).cities.at(city).yields;
  for(std::size_t i=0;i<6;++i){const auto item=yields.find(static_cast<std::int64_t>(i));need((item==yields.end()?0:item->second.terrain)==f.values[i],"Actual removal lost paired raw/owned terrain mirror");}
 }
 need(terrain,"Actual reallocation terrain field absent");
 const auto& mirror=live.unhappiness.players.at(owner).cities.at(city).specialists;
 for(const auto& [id,value]:result.specialists){const auto found=mirror.find(id);need((found==mirror.end()?0:found->second)==value,"Actual signed specialist raw/owned count mirror stale");}
 return result;
}
void actual(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto input=identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Actual Host absent");
 auto& session=host->session();std::int64_t owner=-1,city=-1;
 for(const auto& p:session.state().world.all_players->players){for(const auto& c:p.cities.cities)if(scalar(c,"m_iDamage")==0){owner=p.saved_player_id;city=scalar(c,"m_iID");break;}if(owner!=-1)break;}
 need(owner!=-1,"Actual damage-zero city absent");
 auto pending=session.begin_bnw_city_entry_damage_zero(owner,city);
 refuses([&]{(void)session.continue_bnw_city_citizens_verify(host,pending);});
 pending=session.continue_bnw_city_entry_feature_surrounded(owner,city,BnwCityEntryFeatureRules::from_rules(host->database()));
 refuses([&]{(void)session.continue_bnw_city_citizens_verify(host,pending);});
 for(int i=0;i<4096;++i){pending=session.continue_bnw_city_entry_strategy(host,pending);if(pending->progress().strategy_frontier!=BnwCityStrategyLoopFrontier::none)break;}
 const auto stop=pending->progress();
 if(stop.strategy_frontier!=BnwCityStrategyLoopFrontier::loop_end_before_remaining_city_ai){
  refuses([&]{(void)session.continue_bnw_city_citizens_verify(host,pending);});
  need(!pending->progress().citizens_started,"Mandatory strategy frontier entered Citizens");
  std::cout<<"Actual native Citizens predecessor UNOBSERVED; strategy frontier="<<static_cast<int>(stop.strategy_frontier)<<'\n';
 }else{
  auto client=ui::BnwClientFrameOwner::from_selected_host(host,
   test_detail::selected_city_frame_sources(host));
  auto registry=ui::UiContextRegistry::from_documents(content::read_xml_metadata(
   R"(<Context><LuaContext ID="WorldView" FileName="Assets/UI/InGame/WorldView/WorldView"/></Context>)"),
   content::read_xml_metadata("<Context/>"));
  auto client_ui=client->create_selected_ui_host(std::move(registry),nullptr);
  need(bool(client_ui),"Actual empty Civis selection owner absent");
  const auto cp=session.checkpoint();bool foreign=false;
  std::thread worker([&]{try{(void)session.continue_bnw_city_citizens_verify(host,pending);}catch(const std::exception&){foreign=true;}});worker.join();need(foreign,"Foreign thread entered Citizens");
  std::uint64_t actual_reassigned=0,specialist_steps=0,specialist_writes=0,outer_returns=0;bool genuine_zero_return=false;
  for(int i=0;i<2048;++i){
   const auto before=pending->progress();pending=session.continue_bnw_city_citizens_verify(host,pending);const auto after=pending->progress();
   if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::outer_city_selected)++outer_returns;
   if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::reassignment&&after.citizens_frontier==BnwCityCitizensVerifyFrontier::none)genuine_zero_return=true;
   if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::reassignment&&after.citizens_native_writes>before.citizens_native_writes)++actual_reassigned;
   if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_process){
    ++specialist_steps;specialist_writes+=after.citizens_native_writes-before.citizens_native_writes;
    const auto& live=session.state();const saves::SavedCityObservation* current=nullptr;
    for(const auto& player:live.world.all_players->players)for(const auto& candidate:player.cities.cities)
     if(scalar(candidate,"m_eOwner")==owner&&scalar(candidate,"m_iID")==city)current=&candidate;
    need(current,"Actual specialist canonical city absent");
    for(const auto& f:current->core.fields)if(f.name&&std::string_view(f.name)=="m_aiBaseYieldRateFromSpecialists"){
     need(f.values.size()==6,"Actual specialist core six yield shape differs");
     for(std::size_t yi=0;yi<6;++yi){const auto& yields=live.cities.players.at(owner).cities.at(city).yields;
      const auto v=yields.find(static_cast<std::int64_t>(yi));need((v==yields.end()?0:v->second.specialists)==f.values[yi],
       "Actual specialist writer lost canonical raw/owned yield mirror");}
    }
   }
   if(after.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_update_religion||
      after.citizens_frontier==BnwCityCitizensVerifyFrontier::focus_and_specialists)break;
  }
  bool religion_quarantined=false;std::uint64_t religion_calls=0,happiness_stores=0;
  struct HappinessFact {std::int64_t before=0,after=0;std::uint64_t writes=0,revision_before=0,revision_after=0;};
  std::vector<HappinessFact> recent_happiness;
  const auto happy=[&](){const auto& live=session.state();const auto& rows=live.world.all_players->players.at(static_cast<std::size_t>(owner)).core.fields;
   for(const auto& f:rows)if(f.name&&std::string_view(f.name)=="m_iHappiness"){
    need(f.values.size()==1&&f.values.front()==live.unhappiness.players.at(owner).happiness,"Actual Player happiness raw/owned mirror stale");return f.values.front();}
   throw std::runtime_error("Actual Player happiness scalar absent");};
  if(pending->progress().citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_update_religion){
   for(int i=0;i<2048;++i){
    const auto before=pending->progress();const auto happiness_before=happy();const auto revision_before=session.state().ui_source_revision;
    try{
     pending=session.continue_bnw_city_citizens_verify(host,pending);++religion_calls;
     const auto after=pending->progress();
     recent_happiness.push_back({happiness_before,happy(),after.citizens_native_writes-before.citizens_native_writes,
      revision_before,session.state().ui_source_revision});
     if(recent_happiness.size()>6)recent_happiness.erase(recent_happiness.begin());
     need(after.citizens_attempts>before.citizens_attempts,
      "Genuine Religion continuation did not retain source progress");
     const auto& live=session.state();const saves::SavedCityObservation* current=nullptr;
     for(const auto& player:live.world.all_players->players)for(const auto& candidate:player.cities.cities)
      if(scalar(candidate,"m_eOwner")==owner&&scalar(candidate,"m_iID")==city)current=&candidate;
     need(current,"Genuine Religion current city absent");
     for(const auto& field:current->core.fields)if(field.name&&std::string_view(field.name)=="m_aiBaseYieldRateFromReligion"){
      need(field.values.size()==6,"Genuine Religion six-yield shape differs");
      const auto& owned=live.cities.players.at(owner).cities.at(city).yields;
      for(std::size_t yi=0;yi<6;++yi){const auto value=owned.find(static_cast<std::int64_t>(yi));
       need((value==owned.end()?0:value->second.religion)==field.values[yi],"Genuine Religion raw/owned mirror stale");}
     }
    }catch(const BnwPlayerHappinessPrefixUnavailable& error){
     const std::string_view reason=error.what();
     constexpr std::string_view lua="Player happiness mandatory genuine founder-Lua/SAME script-world epoch unavailable; native six stores retained, GameData fanout pending";
     constexpr std::string_view operand="Player happiness reached native operand unavailable: ";
     need(reason==lua||reason.starts_with(operand),"Unexpected genuine Player happiness frontier");
     religion_quarantined=true;const auto stopped=pending->progress();
     need(stopped.phase==BnwCityEntryPhase::quarantined&&stopped.citizens_native_writes==before.citizens_native_writes&&
      happy()==happiness_before&&session.state().ui_source_revision==revision_before,
      "Player happiness late refusal changed or discarded preceding native stores");
     if(reason==lua){need(recent_happiness.size()==6,"Actual six happiness assignments were not retained");
      for(const auto& fact:recent_happiness)need(fact.writes==1&&fact.revision_before<UINT64_MAX&&fact.revision_after==fact.revision_before+1,
       "Actual happiness assignment lost once count or immediate UI revision");
      const auto rules=BnwHappinessPrefixRules::load(host->database());
      need(recent_happiness.front().after==rules.default_happiness(session.state().setup.players.at(owner).handicap),
       "Actual first native happiness assignment differs from reached Handicap Info");happiness_stores=6;
     }else std::cout<<"Actual Player happiness reached operand frontier: "<<reason<<'\n';
     refuses([&]{(void)session.continue_bnw_city_citizens_verify(host,pending);});break;
    }
   }
   need(religion_quarantined,"Genuine Religion bounded supported city prefix failed to reach mandatory Player tail");
  }
  bool reallocation_refused=false,signed_religion_quarantined=false;std::uint64_t demotion_calls=0,forced_stores=0,removal_calls=0,working_removals=0,signed_steps=0,default_decrements=0,count_decrements=0;
  if(pending->progress().citizens_frontier==BnwCityCitizensVerifyFrontier::focus_and_specialists){
   for(int i=0;i<2048;++i){
    const auto before=pending->progress();const auto facts=citizens_fact(session,owner,city,host->types());const auto revision=session.state().ui_source_revision;
    try{pending=session.continue_bnw_city_citizens_verify(host,pending);}
    catch(const BnwPlayerHappinessPrefixUnavailable& error){
     const std::string_view why=error.what();need(why=="Player happiness mandatory genuine founder-Lua/SAME script-world epoch unavailable; native six stores retained, GameData fanout pending"||
      why.starts_with("Player happiness reached native operand unavailable: "),"Unexpected signed building Religion/Player frontier");
     reallocation_refused=true;signed_religion_quarantined=true;need(pending->progress().phase==BnwCityEntryPhase::quarantined,"Signed building late Player refusal lost quarantine");
     std::cout<<"Signed building retained mandatory Player tail: "<<why<<'\n';break;
    }
    catch(const std::invalid_argument& error){
     const std::string_view why=error.what();
     need(why=="Citizens puppet SetNoAutoAssignSpecialists/SetForcedAvoidGrowth requires their ordered nested continuations"||
      why=="Citizens nonhuman focus requires actual CitySpecializationAI/economic strategy owners"||
      why=="Citizens reallocation requires actual captured DoAddBestCitizen/GP continuation after signed removals",
      "Unexpected actual reallocation source refusal");
     reallocation_refused=true;std::cout<<"Actual reallocation reached mandatory frontier: "<<why<<'\n';break;
    }
    const auto after=pending->progress();const auto current=citizens_fact(session,owner,city,host->types());
    need(after.citizens_attempts>before.citizens_attempts,"Genuine reallocation did not retain actual source-call history");
    if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::reallocation_forced_plot||
       after.citizens_frontier==BnwCityCitizensVerifyFrontier::reallocation_forced_plot)++demotion_calls;
    std::size_t forced_changes=0,working_changes=0,changed_working=0;
    for(std::size_t slot=0;slot<37;++slot){
     if(facts.forced[slot]!=current.forced[slot]){need(facts.forced[slot]==1&&current.forced[slot]==0,"Actual demotion fabricated a forced flag");++forced_changes;}
     if(facts.working[slot]!=current.working[slot]){need(facts.working[slot]==1&&current.working[slot]==0,"Actual worst removal assigned a plot");++working_changes;changed_working=slot;}
    }
    need(forced_changes<=1&&working_changes<=1,"Native single continuation changed multiple inverse flags");
    if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_process||after.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_process)++signed_steps;
    if(current.defaults!=facts.defaults){need(current.defaults==facts.defaults-1,"Signed default scalar did not decrement once");++default_decrements;}
    if(current.forced_defaults!=facts.forced_defaults)need(current.forced_defaults==facts.forced_defaults-1,"Signed forced-default prefix order/value differs");
    std::size_t counts_changed=0;for(const auto& [id,value]:current.specialists){const auto old=facts.specialists.find(id);const auto previous=old==facts.specialists.end()?0:old->second;
     if(value!=previous){need(value==previous-1,"Signed native specialist count did not decrement once");++counts_changed;}}
    need(counts_changed<=1,"One signed source continuation changed multiple native specialist counts");count_decrements+=counts_changed;

    if(forced_changes){++forced_stores;need(current.forced_count==facts.forced_count-1&&current.working_count==facts.working_count&&current.unassigned==facts.unassigned,
      "Conditional forced setter lost ordered counter semantics");need(after.citizens_native_writes>=before.citizens_native_writes+2&&session.state().ui_source_revision>revision,
      "Actual forced stores lost once count or immediate UI revision");}
    if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::reallocation_remove_working)++removal_calls;
    if(working_changes){++working_removals;
     need(current.forced==facts.forced,"DoReallocate false remove-forced argument changed forced flags");
     need(changed_working==0?(current.working_count==facts.working_count&&current.unassigned==facts.unassigned):
      (current.working_count==facts.working_count-1&&current.unassigned==facts.unassigned+1),
      "Actual working removal lost native inverse-home/count semantics");
     need(session.state().ui_source_revision>revision,"Working flag/count/terrain store failed immediate UI invalidation");}
    if(before.citizens_frontier==BnwCityCitizensVerifyFrontier::outer_city_selected)++outer_returns;
   }
   need(reallocation_refused,"Actual bounded reallocation did not stop at mandatory allocation/GP or Player/puppet/nonhuman source tail");
  }
  std::cout<<"actual signed specialist continuations="<<signed_steps<<" default decrements="<<default_decrements<<" native count decrements="<<count_decrements<<" building Player quarantine="<<signed_religion_quarantined<<'\n';
  if(!signed_steps)std::cout<<"Actual processSpecialist(-1) UNOBSERVED\n";
  std::cout<<"actual forced-frontier continuations="<<demotion_calls<<" conditional forced stores="<<forced_stores<<" removal-frontier continuations="<<removal_calls<<" working removals="<<working_removals<<'\n';
  if(!forced_stores)std::cout<<"Actual forced-flag mutation UNOBSERVED\n";
  if(!working_removals)std::cout<<"Actual worst-working SetWorkingPlot(false) UNOBSERVED\n";
  std::cout<<"actual Player happiness stores="<<happiness_stores<<" (zero means complete six-store branch UNOBSERVED)\n";
  std::cout<<"actual Religion calls="<<religion_calls<<" mandatory Player tail="<<religion_quarantined
   <<" (zero means genuine branch UNOBSERVED)\n";
  const auto p=pending->progress();need(p.citizens_started&&p.citizens_next_plot<=37&&
   (p.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_update_religion||reallocation_refused),
   "Bounded actual Citizens prefix lost mandatory gameplay frontier");
  if(outer_returns){
   const auto events=client->take_city_dirty_events();
   need(!events.empty()&&events.back().kind==BnwCityDirtyKind::city_info,
    "Actual unselected Citizens path lost unconditional outer CityInfo notification");
   std::uint64_t previous=0;
   for(const auto& event:events){need(event.occurrence>previous&&event.generation==session.state_generation()&&
    event.owner==owner&&event.city==city&&event.value,"Actual dirty notification payload/order differs");previous=event.occurrence;}
   need(client->take_city_dirty_events().empty(),"Ordered notification drain duplicated actual occurrences");
  }
  for(int i=0;i<2;++i){if(religion_quarantined||reallocation_refused){const auto facts=citizens_fact(session,owner,city,host->types());const auto revision=session.state().ui_source_revision;refuses([&]{(void)session.continue_bnw_city_citizens_verify(host,pending);});const auto current=citizens_fact(session,owner,city,host->types());const auto again=pending->progress();need(current.forced==facts.forced&&current.working==facts.working&&current.forced_count==facts.forced_count&&current.working_count==facts.working_count&&current.unassigned==facts.unassigned&&current.defaults==facts.defaults&&current.forced_defaults==facts.forced_defaults&&current.specialists==facts.specialists&&session.state().ui_source_revision==revision&&again.citizens_attempts==p.citizens_attempts&&again.citizens_native_writes==p.citizens_native_writes,"Mandatory frontier replay repeated or discarded preceding stores");continue;}need(session.continue_bnw_city_citizens_verify(host,pending).get()==pending.get(),"Replay replaced native cursor");const auto after=pending->progress();need(after.citizens_attempts==p.citizens_attempts&&after.citizens_native_writes==p.citizens_native_writes&&after.citizens_next_plot==p.citizens_next_plot,"Citizens replay repeated native stores");}
  std::cout<<"actual Citizens next="<<static_cast<int>(p.citizens_next_plot)<<" native_writes="<<p.citizens_native_writes<<" frontier="<<static_cast<int>(p.citizens_frontier)<<'\n';
  std::cout<<"actual reassignment stores="<<actual_reassigned<<" zero-unassigned return="<<genuine_zero_return<<'\n';
  std::cout<<"actual specialist continuation steps="<<specialist_steps<<" native stores="<<specialist_writes<<'\n';
  if(!specialist_steps)std::cout<<"Actual specialist/default assignment UNOBSERVED\n";
  if(!actual_reassigned)std::cout<<"Actual best-plot SetWorkingPlot(true) UNOBSERVED\n";
  if(!p.citizens_native_writes)std::cout<<"Actual invalid-worked plot mutation UNOBSERVED\n";
  session.restore(cp);need(pending->progress().phase==BnwCityEntryPhase::quarantined&&pending->progress().citizens_attempts>=p.citizens_attempts,"Rollback rewound Citizens once ledger");
  refuses([&]{(void)session.continue_bnw_city_citizens_verify(host,pending);});
 }
 need(!BnwCityEntryPending::growth_ready()&&!BnwCityEntryPending::full_city_turn_ready(),"Citizens prefix granted later phases");
 need(identify_file(save).sha256==input,"Original save changed");
}
}
int main(int argc,char** argv){try{need(argc==3,"Expected SAVE and canonical shared installation");math();actual(argv[1],argv[2]);std::cout<<"bnw Citizens verify prefix tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
