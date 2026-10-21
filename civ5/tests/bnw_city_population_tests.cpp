#include "civ5/runtime/bnw_city_population.hpp"
#include "civ5/runtime/bnw_player_happiness_prefix_owner.hpp"
#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
using namespace civ5;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::size_t checked_index(std::int64_t value){need(value>=0&&std::in_range<std::size_t>(value),"Actual population vector index unavailable");return static_cast<std::size_t>(value);}
template<class F>void refuses(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Invalid population continuation accepted");}
const auto& named(const auto& rows,const char* key){for(const auto& r:rows)if(r.name&&std::string_view(r.name)==key)return r;throw std::runtime_error("Population actual fixture field unavailable");}
std::int64_t scalar(const auto& c,const char* key){return named(c.core.fields,key).values.at(0);}
void arithmetic(){
 const std::vector<runtime::CityYieldReligion> status{{0,4,100},{1,2,30},{2,1,20},{3,0,0}};
 need(runtime::detail::bnw_city_religion_centi_math(status,125)==2,"Religion count must exclude pantheon and zero followers before multiply/divide");
 need(runtime::detail::bnw_city_religion_centi_math(status,-1)==0,"Nonpositive perReligion must skip count arithmetic");
 refuses([&]{(void)runtime::detail::bnw_city_religion_centi_math(status,INT32_MAX);});
 need(runtime::detail::bnw_population_add(1,-1)==0,"Starvation population signed ADD differs");
 need(runtime::detail::bnw_population_add(INT32_MAX-1,1)==INT32_MAX,"Highest valid growth population differs");
 refuses([]{(void)runtime::detail::bnw_population_add(INT32_MAX,1);});
 refuses([]{(void)runtime::detail::bnw_population_add(INT32_MIN,-1);});
}
void selected(const char* save,const char* install){
 const auto original=runtime::identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,install,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));need(bool(host),"Actual population selected Host unavailable");
 auto& session=host->session();const auto checkpoint=session.checkpoint();
 need(session.state().world.all_players.has_value(),"Actual population roster unavailable");
 std::int64_t owner=-1,id=-1;
 for(const auto& p:session.state().world.all_players->players)if(!p.cities.cities.empty()){owner=p.saved_player_id;id=scalar(p.cities.cities.front(),"m_iID");break;}
 need(owner>=0,"Actual population city unavailable");
 // Readback of the real SAME session, never a supplied completed-return proof.
 // Native plot/religion prefixes may create zero yield-map slots; compare the
 // science misc value and complete raw misc vector, not unrelated map shape.
 const auto science_misc=[&]{
  const auto& state=session.state();
  const auto& city=state.world.all_players->players.at(checked_index(owner)).cities.cities.front();
  const auto& raw_misc=named(city.core.fields,"m_aiBaseYieldRateFromMisc").values;
  const auto& yields=state.cities.players.at(owner).cities.at(id).yields;
  const auto found=yields.find(3);const auto owned_misc=found==yields.end()?0:found->second.misc;
  need(raw_misc.at(3)==owned_misc,"Actual science misc raw/owned projection differs");
  return std::pair{raw_misc,owned_misc};
 };
 const auto original_science=science_misc();
 const auto science_unreached=[&]{need(science_misc()==original_science,
  "Population science executed before genuine UpdateReligion/DoUpdateHappiness return");};
 auto food=session.begin_bnw_city_food_growth_prefix(host,owner,id);food->require_current();
 const auto f=food->progress();science_unreached();
 const auto phase=runtime::BnwCityFoodGrowthPhase::population_growth;
 const auto starvation=runtime::BnwCityFoodGrowthPhase::population_starvation;
 if(f.phase!=phase&&f.phase!=starvation){
  refuses([&]{(void)food->continue_population();});
  std::cout<<"Actual growth/starvation population branch UNOBSERVED; actual non-population food return refuses child issuance\n";
  session.restore(checkpoint);refuses([&]{food->require_current();});
 }else{
  // Detached readback is an expected result witness. It never issues stores.
  const auto before=session.state();const auto delta=f.phase==phase?1:-1;
  const auto& raw=before.world.all_players->players.at(checked_index(owner)).cities.cities.front();
  const auto old=scalar(raw,"m_iPopulation"),highest=scalar(raw,"m_iHighestPopulation");
  const auto unassigned=named(named(raw.management.blocks,"citizens").fields,"m_iNumUnassignedCitizens").value;
  auto child=food->continue_population();child->require_current();const auto result=child->progress();science_unreached();
  const auto& current=session.state();const auto& actual=current.world.all_players->players.at(checked_index(owner)).cities.cities.front();
  need(result.owner==owner&&result.city==id&&result.change==delta&&result.before==old&&result.after==old+delta,"Actual population source identity/expression differs");
  if(delta<0&&unassigned!=old){
   need(result.assignments==0&&scalar(actual,"m_iPopulation")==old&&
    (result.phase==runtime::BnwCityPopulationPhase::remove_default_specialist_required||result.phase==runtime::BnwCityPopulationPhase::remove_worst_worked_or_specialist_required),"Starvation bypassed mandatory pre-store citizen removal");
   refuses([&]{child->continue_plot();});
  }else{
   need(result.phase==runtime::BnwCityPopulationPhase::plot_yield_child_required,"Native population effects stopped at an unexpected boundary");
   need(scalar(actual,"m_iPopulation")==old+delta&&current.cities.players.at(owner).cities.at(id).population==old+delta,"Actual population raw/owned projection differs");
   need(scalar(actual,"m_iHighestPopulation")==std::max(highest,old+delta),"Native highest population branch differs");
   need(result.rank_assignments==before.world.all_players->players.at(checked_index(owner)).cities.cities.size(),"Owner FLTA rank invalidation skipped a city");
   for(const auto& c:current.world.all_players->players.at(checked_index(owner)).cities.cities)
    need(named(named(c.production.blocks,"before_strategy").fields,"m_bPopulationRankValid").value==0,"Native owner rank cache not invalidated");
   const auto x=scalar(raw,"m_iX"),y=scalar(raw,"m_iY");const auto plot=before.world.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
   const auto area=named(before.world.map_plots->plots.at(plot).fields,"area").value;
   const auto* a=runtime::resolve_bnw_area_registry(before.world,static_cast<std::int32_t>(area));
   const auto* b=runtime::resolve_bnw_area_registry(current.world,static_cast<std::int32_t>(area));need(a&&b,"Actual area binding absent");
   need(named(b->fields,"population").value==named(a->fields,"population").value+delta&&
    named(b->arrays,"population_per_player").values.at(checked_index(owner))==named(a->arrays,"population_per_player").values.at(checked_index(owner))+delta,"Native area population pair differs");
   const auto before_total=named(before.world.all_players->players.at(checked_index(owner)).core.fields,"m_iTotalPopulation").values.at(0);
   need(current.cities.players.at(owner).total_population==before_total+delta&&
    named(current.world.all_players->players.at(checked_index(owner)).core.fields,"m_iTotalPopulation").values.at(0)==before_total+delta,"Actual player raw/owned total differs");
   const auto team=runtime::detail::bnw_player_team_read(before,static_cast<std::int32_t>(owner));
   need(named(current.world.teams->teams.at(checked_index(team)).fields,"population").value==named(before.world.teams->teams.at(checked_index(team)).fields,"population").value+delta,"Actual team population total differs");
   need(std::get<saves::Expansion2PrefixObservation>(current.world.game_state.prefix).scalars.at(10)==std::get<saves::Expansion2PrefixObservation>(before.world.game_state.prefix).scalars.at(10)+delta,"Actual Game population total differs");
   need(current.ui_source_revision==before.ui_source_revision+result.assignments,"Population immediate store revision differs");
   if(delta<0)need(result.unassigned_removal_returned&&named(named(actual.management.blocks,"citizens").fields,"m_iNumUnassignedCitizens").value==unassigned-1,"Actual early removal/unassigned fixup differs");
   need(!session.bnw_unhappiness_current(),"Population reused old happiness freshness");
   // This is the genuine reached population return, never a supplied plot.
   // Each call executes either the first normal prefix or its required UI tail.
   bool missing=false;std::uint64_t calls=0;
   while(child->progress().phase!=runtime::BnwCityPopulationPhase::update_religion_required&&calls<9){
    ++calls;
    try{child->continue_plot();}
    catch(const runtime::BnwNormalPlotYieldUnavailable& e){
     const std::string_view reason=e.what();
     need(reason=="Normal plot yield natural-wonder native arithmetic frontier unavailable"||
          reason=="Normal plot yield reached Improvement Info or pillage unavailable"||
          reason=="Normal plot yield reached Feature Info unavailable"||
          reason=="Normal plot yield reached Resource Info unavailable",
          "Unexpected genuine population normal-source gap");
     missing=true;std::cout<<"Actual population normal prefix reached source frontier: "<<reason<<'\n';break;
    }
    catch(const std::runtime_error& e){
     need(std::string_view(e.what())=="Plot UI tail genuine actual UI selection owner unavailable",
          "Unexpected genuine population plot child failure");
     missing=true;std::cout<<"Actual population plot prefix reached mandatory UI selection frontier\n";break;
    }
   }
   science_unreached();
   if(missing){
    need(child->progress().phase==runtime::BnwCityPopulationPhase::failed,"Missing effect did not quarantine population call");
    refuses([&]{child->require_current();});refuses([&]{child->continue_plot();});
   }else{
    child->require_current();food->require_current();
    need(calls>0&&calls<=8&&child->progress().phase==runtime::BnwCityPopulationPhase::update_religion_required,
         "Actual single plot call did not return before mandatory UpdateReligion");
    const auto latest=session.bnw_plot_yield_refresh_pending(static_cast<std::int64_t>(plot));
    need(bool(latest),"Population did not retain actual latest plot child");
    const auto trace=latest->progress();
    need(trace.phase==runtime::BnwPlotYieldRefreshPhase::complete||trace.phase==runtime::BnwPlotYieldRefreshPhase::skipped_no_area,
         "Population accepted a plot call with unfinished mandatory effects");
    need(child->progress().assignments==result.assignments&&scalar(session.state().world.all_players->players.at(checked_index(owner)).cities.cities.front(),"m_iPopulation")==old+delta,
         "Plot child replayed population stores");
    refuses([&]{child->continue_plot();});
    std::cout<<"Actual population single plot child returned; native UpdateReligion still required\n";
    const auto religion_before=session.state().world.all_players->players.at(checked_index(owner)).cities.cities.front();
    const auto native_revision=session.state().ui_source_revision;
    child->continue_religion(); // captures majority BEFORE issuing fresh city37
    need(child->progress().phase==runtime::BnwCityPopulationPhase::religion_city_call&&
     child->progress().religion_reset_assignments==0&&session.state().ui_source_revision==native_revision,
     "Religion reset preceded genuine native37 return");
    refuses([&]{child->continue_plot();});
    bool religion_gap=false;std::uint64_t religion_calls=0;
    while(child->progress().phase!=runtime::BnwCityPopulationPhase::player_update_religion_required&&religion_calls<4096){
     ++religion_calls;const auto before_reset=child->progress().religion_reset_assignments;
     try{child->continue_religion();}
     catch(const runtime::BnwNormalPlotYieldUnavailable& e){
      const std::string_view reason=e.what();
      need(reason=="Normal plot yield natural-wonder native arithmetic frontier unavailable"||
           reason=="Normal plot yield reached Improvement Info or pillage unavailable"||
           reason=="Normal plot yield reached Feature Info unavailable"||
           reason=="Normal plot yield reached Resource Info unavailable",
           "Unexpected actual religion city37 normal-source gap");
      religion_gap=true;std::cout<<"Actual UpdateReligion city37 mandatory source frontier: "<<reason<<'\n';break;
     }
     catch(const std::runtime_error& e){
      const std::string_view reason=e.what();
      need(reason=="Plot UI tail genuine actual UI selection owner unavailable"||
           reason=="Population religion actual selection owner absent",
           "Unexpected actual UpdateReligion failure");
      religion_gap=true;std::cout<<"Actual UpdateReligion mandatory selection frontier: "<<reason<<'\n';break;
     }
     child->require_current();food->require_current();const auto r=child->progress();
     need(r.religion_reset_assignments<=6&&r.religion_city_slots<=37&&
      r.religion_city_null_skips+r.religion_city_returned_calls==r.religion_city_slots,
      "Religion cursor accepted batch/unreached plot facts");
     if(r.religion_reset_assignments>before_reset){
      need(r.religion_city_slots==37&&r.religion_reset_assignments==before_reset+1,
       "Religion reset skipped source assignment or began before37 return");
      const auto& now=session.state().world.all_players->players.at(checked_index(owner)).cities.cities.front();
      const auto n=r.religion_reset_assignments;
      if(n==1)need(scalar(now,"m_iJONSCulturePerTurnFromReligion")==0&&
       scalar(now,"m_iFaithPerTurnFromReligion")==scalar(religion_before,"m_iFaithPerTurnFromReligion"),
       "Culture reset moved later faith store early");
      if(n==2)need(scalar(now,"m_iFaithPerTurnFromReligion")==0,"Faith reset native assignment differs");
      if(n>=3){const auto& values=named(now.core.fields,"m_aiBaseYieldRateFromReligion").values;
       need(values.at(n-3)==0&&session.state().cities.players.at(owner).cities.at(id).yields.at(n-3).religion==0,
        "Religion base reset raw/owned mirror differs");}
     }
    }
    science_unreached();
    if(religion_gap){
     need(child->progress().phase==runtime::BnwCityPopulationPhase::failed,"Religion partial effects were not quarantined");
     refuses([&]{child->require_current();});refuses([&]{child->continue_religion();});
    }else{
     need(religion_calls<4096&&child->progress().phase==runtime::BnwCityPopulationPhase::player_update_religion_required&&
      child->progress().religion_city_slots==37&&child->progress().religion_reset_assignments==6&&
      child->progress().religion_yield==6,"Actual religion did not reach mandatory player happiness after all native yields");
     const auto player_rules=runtime::BnwHappinessPrefixRules::load(host->database());
     const auto inherited_slots=child->progress().religion_city_slots;
     const auto inherited_resets=child->progress().religion_assignments;
     for(std::uint8_t n=0;n<6;++n){
      const auto before=session.state();
      const auto expected=runtime::detail::bnw_happiness_prefix_reached_stage_math(before,player_rules,
       host->identity(),owner,static_cast<runtime::BnwHappinessNativeStore::Stage>(n));
      child->continue_religion();child->require_current();food->require_current();
      science_unreached();
      const auto& now=session.state();const auto r=child->progress();
      need(r.player_religion_attempted&&r.player_happiness_stores==n+1&&
       now.ui_source_revision==before.ui_source_revision+1,
       "Player religion prefix skipped actual assignment/once/revision");
      need(named(now.world.all_players->players.at(checked_index(owner)).core.fields,"m_iHappiness").values.at(0)==expected.happiness&&
       now.unhappiness.players.at(owner).happiness==expected.happiness&&
       !now.cities.players.at(owner).excess_happiness,
       "Player happiness raw/authoritative/derived caches differ");
      if(n==0)need(expected.happiness==player_rules.default_happiness(before.setup.players.at(owner).handicap),
       "Player first happiness store was ADD instead of native SET");
      need(r.religion_city_slots==inherited_slots&&r.religion_assignments==inherited_resets,
       "Player UpdateReligion replayed native city37 or religion stores");
     }
     need(child->progress().phase==runtime::BnwCityPopulationPhase::player_founder_religion_required,
      "Player prefix bypassed native founder query");
     const auto before=session.state();bool refused=false;
     try{child->continue_religion();}catch(const runtime::BnwPlayerHappinessPrefixUnavailable& e){
      refused=std::string_view(e.what())=="Player happiness mandatory genuine founder-Lua/SAME script-world epoch unavailable; native six stores retained, GameData fanout pending";
     }
     need(refused&&child->progress().phase==runtime::BnwCityPopulationPhase::failed&&
      child->progress().player_happiness_stores==6&&session.state().ui_source_revision==before.ui_source_revision&&
      named(session.state().world.all_players->players.at(checked_index(owner)).core.fields,"m_iHappiness").values.at(0)==
       named(before.world.all_players->players.at(checked_index(owner)).core.fields,"m_iHappiness").values.at(0),
      "Founder refusal lost partial stores or performed a successful skip");
     science_unreached();
     refuses([&]{child->require_current();});refuses([&]{child->continue_religion();});
     science_unreached();
     std::cout<<"Actual Player.UpdateReligion six stores retained; founder-Lua and later player/UI effects still mandatory\n";
    }

   }

  }
  refuses([&]{(void)food->continue_population();});
  bool other=false;std::thread worker([&]{try{child->require_current();}catch(const std::exception&){other=true;}});worker.join();need(other,"Foreign thread population read accepted");
  session.restore(checkpoint);refuses([&]{child->require_current();});refuses([&]{food->require_current();});
  refuses([&]{(void)food->continue_population();});
 }
 science_unreached();
 need(runtime::identify_file(save).sha256==original,"Population changed original save");
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityPopulationPending>);
int main(int argc,char** argv){try{need(argc==3,"Expected actual SAVE and install root");arithmetic();selected(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
