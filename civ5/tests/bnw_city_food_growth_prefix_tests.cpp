#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
using namespace civ5;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refused(F&& f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}need(failed,"Invalid/stale growth accepted");}
const auto& named(const auto& rows,const char* key){for(const auto& r:rows)if(r.name&&std::string_view(r.name)==key)return r;throw std::runtime_error("Fixture native field absent");}
std::int64_t scalar(const auto& raw,const char* key){return named(raw.core.fields,key).values.front();}
void arithmetic(){
 using namespace runtime::detail;
 need(bnw_growth_add_food(99,-150)==-51,"Native fractional food addition differs");
 need(bnw_growth_add_kept(8,-199)==7&&bnw_growth_add_kept(8,-99)==8,"Kept must truncate delta before addition");
 need(bnw_growth_clamp_kept(-3,8,50)==0&&bnw_growth_clamp_kept(9,8,50)==4,"Native range limits differ");
 need(bnw_growth_clamp_kept(-1,8,-100)==0&&bnw_growth_clamp_kept(1,8,-100)==-8,"Native range low-first Release branch differs");
 need(bnw_growth_food_change(-151,1)==-51,"Starvation whole-food reset must retain fractional remainder");
 refused([]{(void)bnw_growth_add_food(INT32_MAX,1);});
 refused([]{(void)bnw_growth_add_kept(INT32_MIN,-100);});
 refused([]{(void)bnw_growth_clamp_kept(0,INT32_MAX,2);});
 // Multiplication must refuse before a mathematically cancelling final sum.
 refused([]{(void)bnw_growth_food_change(INT32_MIN,INT32_MAX);});
}
void selected(const std::filesystem::path& path,const std::filesystem::path& install){
 const auto save_sha=runtime::identify_file(path).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(path,install,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(boot.preview.host));need(bool(host),"Genuine growth selected Host absent");
 auto& session=host->session();const auto checkpoint=session.checkpoint();
 const auto& world=session.state().world;need(world.all_players.has_value(),"Genuine roster absent");
 std::int64_t owner=-1,id=-1;
 for(const auto& player:world.all_players->players)if(!player.cities.cities.empty()){owner=player.saved_player_id;id=scalar(player.cities.cities.front(),"m_iID");break;}
 need(owner>=0,"Genuine city absent");
 const auto& raw=world.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities.front();
 const auto initial_food=scalar(raw,"m_iFood"),initial_kept=scalar(raw,"m_iFoodKept"),initial_population=scalar(raw,"m_iPopulation");
 const auto before_revision=session.state().ui_source_revision;const auto razing=scalar(raw,"m_iRazingTurns");
 std::optional<std::int32_t> actual_difference;
 if(razing<=0){
  auto database=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
  auto rules=std::make_shared<const runtime::BnwCityFoodRateRules>(runtime::BnwCityFoodRateRules::load(database,host->identity().rules_sha256));
  auto lease=std::shared_ptr<const runtime::LiveSession>(host,&session);
  actual_difference=runtime::inspect_bnw_city_current_food_difference(lease,rules,owner,id).value_times100();
 }
 const auto pending=session.begin_bnw_city_food_growth_prefix(host,owner,id);need(bool(pending),"Actual pending absent");
 pending->require_current();const auto progress=pending->progress();
 need(progress.owner==owner&&progress.city==id,"Canonical growth identity differs");
 need(scalar(raw,"m_iPopulation")==initial_population,"Missing population effect was fabricated");
 if(razing>0){
  need(progress.phase==runtime::BnwCityFoodGrowthPhase::razing_returned&&progress.returned&&!progress.difference_observed&&progress.native_scalar_assignments==0,"Razing read later growth operands");
 }else{
  need(progress.difference_observed&&progress.difference_times100==*actual_difference,"Genuine same-owner difference was not used");
  const auto has=named(named(world.all_players->players.at(static_cast<std::size_t>(owner)).administration.blocks,"notifications").fields,"has_notifications").value;
  if(*actual_difference<0&&has!=0){
   need(progress.phase==runtime::BnwCityFoodGrowthPhase::starvation_notification&&!progress.returned&&progress.native_scalar_assignments==0,"Notification frontier skipped or food stored early");
   need(scalar(raw,"m_iFood")==initial_food&&scalar(raw,"m_iFoodKept")==initial_kept,"Notification refusal wrote food");
  }else{
   need(progress.native_scalar_assignments>=2,"Actual kept calls did not execute");
   need(session.state().ui_source_revision==before_revision+progress.native_scalar_assignments,"Immediate frame invalidation differs");
   // Threshold is unaffected by food stock, and this independent read is
   // performed on the actual AFTER-prefix owner, not a supplied cached value.
   const auto threshold=runtime::inspect_bnw_city_current_growth_threshold(host,owner,id).value();
   const auto percent=scalar(raw,"m_iMaxFoodKeptPercent");
   const auto upper=std::int64_t(threshold)*percent;need(upper>=INT32_MIN&&upper<=INT32_MAX,"Actual fixture exceeds native bound");
   const auto added=initial_kept+(*actual_difference/100);const auto cap=upper/100;
   const auto expected=added<0?0:(added>cap?cap:added);
   need(scalar(raw,"m_iFoodKept")==expected&&progress.food_kept==expected,"Actual kept source clamp differs");
   need(progress.food_times100==scalar(raw,"m_iFood"),"Actual canonical food mirror stale");
   need(progress.phase!=runtime::BnwCityFoodGrowthPhase::prepared&&progress.phase!=runtime::BnwCityFoodGrowthPhase::failed,"Actual food prefix did not advance");
   if(progress.phase==runtime::BnwCityFoodGrowthPhase::population_growth||progress.phase==runtime::BnwCityFoodGrowthPhase::population_starvation)need(!progress.returned,"Population frontier claimed completion");
  }
 }
 refused([&]{(void)session.begin_bnw_city_food_growth_prefix(host,owner,id&8191);});
 bool foreign_refused=false;std::thread thread([&]{try{pending->require_current();}catch(const std::exception&){foreign_refused=true;}});thread.join();need(foreign_refused,"Growth foreign-thread read accepted");
 need(session.bnw_city_food_growth_pending(owner,id)==pending,"Session failed to retain genuine once owner");
 // This pair encodes components only. Refuse the actual food once owner
 // BEFORE any world/codec access, including razing or pre-store frontiers.
 const auto pair_rules=runtime::BnwPlotTerrainRulesSnapshot::from_rules(host->database());
 const auto pair_refused=[&](auto&& call,const char* fragment){
  bool exact=false;try{call();}catch(const std::exception& e){exact=std::string_view(e.what()).find(fragment)!=std::string_view::npos;}
  need(exact,"Filepair lost actual food once history or refused too late");
 };
 pair_refused([&]{(void)runtime::make_bnw_plot_terrain_checkpoint_pair(session,host->identity(),pair_rules);},"cannot export food-growth once history");
 pair_refused([&]{(void)runtime::restore_bnw_plot_terrain_checkpoint_pair(runtime::BnwPlotTerrainCheckpointPair{},session,host->identity(),pair_rules);},"cannot restore over food-growth once history");

 session.restore(checkpoint);refused([&]{pending->require_current();});
 refused([&]{(void)session.begin_bnw_city_food_growth_prefix(host,owner,id);});
 need(runtime::identify_file(path).sha256==save_sha,"Growth changed original save");
 std::cout<<"PASS genuine imported food difference, source-ordered food/kept prefix or exact notification frontier, once/thread/checkpoint retirement; full city turn unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodGrowthPending>);
int main(int argc,char** argv){try{need(argc==3,"Expected genuine SAVE and install root");arithmetic();selected(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
