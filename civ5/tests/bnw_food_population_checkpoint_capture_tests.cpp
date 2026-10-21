#include "civ5/runtime/bnw_food_population_checkpoint_capture.hpp"
#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
namespace {
using namespace civ5;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refused(F&& fn){bool no=false;try{fn();}catch(const std::exception&){no=true;}need(no,"Unsupported Food capture accepted");}
const auto& named(const auto& rows,std::string_view n){for(const auto& r:rows)if(r.name&&n==r.name)return r;throw std::runtime_error("Natural Food field missing");}
std::int64_t scalar(const auto& c,std::string_view n){return named(c.core.fields,n).values.at(0);}
void natural(const std::filesystem::path& save,const std::filesystem::path& install){
 const auto original_sha=runtime::identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,install,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));need(bool(host),"Genuine Food Host unavailable");
 auto& session=host->session();need(session.source().source.container==saves::SaveContainerKind::civ5,"Use original BNW growth scenario");
 const auto checkpoint=session.checkpoint();const auto original=session.source().snapshot;
 const auto database=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
 const auto rules=std::make_shared<const runtime::BnwCityFoodRateRules>(runtime::BnwCityFoodRateRules::load(database,host->identity().rules_sha256));
 const auto lease=std::shared_ptr<const runtime::LiveSession>(host,&session);
 std::int64_t actor=-1,id=-1;
 for(const auto& p:session.state().world.all_players->players){for(const auto& c:p.cities.cities){
  if(scalar(c,"m_iRazingTurns")>0||named(named(c.management.blocks,"citizens").fields,"m_bForceAvoidGrowth").value!=0)continue;
  const auto full=scalar(c,"m_iID");const auto diff=runtime::inspect_bnw_city_current_food_difference(lease,rules,p.saved_player_id,full).value_times100();
  if(diff<0)continue;const auto threshold=runtime::inspect_bnw_city_current_growth_threshold(host,p.saved_player_id,full).value();
  if((scalar(c,"m_iFood")+diff)/100>=threshold){actor=p.saved_player_id;id=full;break;}
 }if(actor>=0)break;}
 need(actor>=0,"No naturally eligible growth city; do not synthesize Food population phase");
 const auto before=session.state();const auto map_rng=before.random.map.state(),game_rng=before.random.gameplay.state();
 const auto food=session.begin_bnw_city_food_growth_prefix(host,actor,id);
 need(food->progress().phase==runtime::BnwCityFoodGrowthPhase::population_growth,"Genuine Food growth prefix missing");
 const auto child=food->continue_population();
 need(child->progress().phase==runtime::BnwCityPopulationPhase::plot_yield_child_required&&!food->progress().returned,"Genuine pre-plot return missing");
 const auto revision=session.state().ui_source_revision;
 const auto capture=food->capture_population_checkpoint_before_plot();capture->require_current();
 const auto& b=capture->components();const auto decoded=runtime::restore_bnw_population_totals_checkpoint_bundle(b,original,host->identity());
 const auto& player=decoded.all_players->players.at(static_cast<std::size_t>(actor));
 const auto& original_player=before.world.all_players->players.at(static_cast<std::size_t>(actor));
 const saves::SavedCityObservation* old_city=nullptr;const saves::SavedCityObservation* restored_city=nullptr;
 for(const auto& c:original_player.cities.cities)if(scalar(c,"m_iID")==id)old_city=&c;
 for(const auto& c:player.cities.cities)if(scalar(c,"m_iID")==id)restored_city=&c;
 need(old_city&&restored_city&&scalar(*restored_city,"m_iPopulation")==scalar(*old_city,"m_iPopulation")+1,"Captured actual population delta differs");
 need(scalar(player,"m_iTotalPopulation")==scalar(original_player,"m_iTotalPopulation")+1,"Captured player total differs");
 const auto stores=capture->ordered_stores();need(!stores.empty(),"Capture omitted genuine ordered stores");
 const auto& first=stores.front();need(first.kind==runtime::BnwFoodPopulationStoreKind::food&&first.full_object==id&&
  first.before==scalar(*old_city,"m_iFood")&&first.after==first.before+food->progress().difference_times100,
  "Food first recorded semantic before/after does not match independent actual operands");
 bool city_store=false;
 for(const auto& e:stores){need(e.returned&&e.actor==actor,"Capture retained incomplete/foreign store");
  if(e.kind==runtime::BnwFoodPopulationStoreKind::city_population){need(!city_store&&e.full_object==id&&e.before==scalar(*old_city,"m_iPopulation")&&
   e.after==e.before+1&&e.projection&&e.projection_before==e.before,"Actual population raw/owned semantic pair differs");city_store=true;}}
 const auto& last=stores.back();need(city_store&&last.kind==runtime::BnwFoodPopulationStoreKind::game_population&&last.full_object==10&&
  last.before==std::get<saves::Expansion2PrefixObservation>(before.world.game_state.prefix).scalars.at(10)&&last.after==last.before+1,
  "Actual last Game10 assignment differs from independently observed before value");
 need(!runtime::BnwFoodPopulationCheckpointCapture::restores_food_once_owner()&&!session.readiness().can_continue(),"Capture minted save/continuation readiness");
 refused([&]{(void)food->capture_population_checkpoint_before_plot();});capture->require_current();
 bool foreign=false;std::thread worker([&]{try{capture->require_current();}catch(const runtime::BnwCityFoodGrowthUnavailable&){foreign=true;}});worker.join();need(foreign,"Foreign-thread capture query accepted");capture->require_current();
 need(session.state().ui_source_revision==revision&&session.state().random.map.state()==map_rng&&session.state().random.gameplay.state()==game_rng,
  "Capture/repeat/thread guard mutated live world or RNG");
 // A real next native call, whether it earns its partial prefix or reaches an
 // explicit missing supplier, permanently invalidates this pre-plot capture.
 try{child->continue_plot();}catch(const std::exception&){
  need(child->progress().phase==runtime::BnwCityPopulationPhase::failed,"Actual plot failure did not retain failed stage");
 }
 refused([&]{capture->require_current();});refused([&]{(void)capture->components();});refused([&]{(void)food->capture_population_checkpoint_before_plot();});
 session.restore(checkpoint);refused([&]{capture->require_current();});
 need(runtime::identify_file(save).sha256==original_sha,"Food capture changed original save");
 std::cout<<"PASS ACTUAL_FOOD_POPULATION_PREPLOT_CAPTURE; runtime once history only, restore admission unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<civ5::runtime::BnwFoodPopulationCheckpointCapture>);
static_assert(!std::is_copy_constructible_v<civ5::runtime::BnwFoodPopulationCheckpointCapture>);
int main(int argc,char** argv){try{need(argc==3,"Use original BNW growth save and canonical civ5-assets");natural(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL Food capture: "<<e.what()<<'\n';return 1;}}
