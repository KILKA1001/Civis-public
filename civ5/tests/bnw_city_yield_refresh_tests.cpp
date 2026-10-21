#include "civ5/runtime/bnw_city_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <array>
#include <iostream>
#include <set>
#include <thread>
#include <type_traits>
#include <stdexcept>
#include <string_view>
using namespace civ5;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F f,std::string_view expected){bool caught=false;try{f();}catch(const std::exception& e){
 caught=true;need(std::string_view(e.what()).find(expected)!=std::string_view::npos,"Unexpected city-refresh refusal");}
 need(caught,"Invalid city-refresh source operation admitted");}
const auto& named(const auto& rows,std::string_view name){
 const typename std::remove_cvref_t<decltype(rows)>::value_type* found=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Duplicate fixture field");found=&r;}
 need(found,"Actual fixture field absent");return *found;
}
std::int32_t scalar(const auto& rows,std::string_view name){const auto& f=named(rows,name);need(f.values.size()==1&&
 f.values[0]>=INT32_MIN&&f.values[0]<=INT32_MAX,"Actual fixture native scalar absent");return static_cast<std::int32_t>(f.values[0]);}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){
 auto result=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(result.preview.blocker)throw std::runtime_error(result.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.preview.host));need(bool(host),"Actual immutable Host absent");return host;
}
void geometry(){
 // Detached geometry preserves NULL slots and repeated calls, not authority.
 runtime::MapGeometry small{{2,2},true,true,saves::GameStateLayout::expansion2_version1};
 // Native coordRange(-2,2,true) returns2, not0; CvMap::plot then
 // returns NULL. Wrap flags do NOT promise every native slot is nonnull.
 // Golden addresses come from SDK Globals37 + hex conversion + coordRange,
 // independently of the production helper, for BOTH row parities.
 constexpr auto expected_even=std::to_array<std::optional<std::int64_t>>({0,2,1,2,3,1,3,1,3,0,3,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::nullopt,1,0,3,0,2,1,2,std::nullopt,3,2,3,std::nullopt,std::nullopt,3,1,3,std::nullopt,std::nullopt,3,2});
 constexpr auto expected_odd=std::to_array<std::optional<std::int64_t>>({2,1,3,1,0,3,0,3,0,2,0,3,2,3,1,std::nullopt,1,3,2,0,2,1,3,1,2,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::nullopt,3,std::nullopt,std::nullopt,1,0,1});
 static_assert(expected_even.size()==37&&expected_odd.size()==37);
 const auto wrapped=runtime::detail::bnw_citizen_plot_positions(small,0,0);
 const auto odd=runtime::detail::bnw_citizen_plot_positions(small,0,1);
 need(wrapped==expected_even&&odd==expected_odd,"Wrapped native37 NULL/address order differs from SDK");
 need(!small.plot(0,-2)&&small.plot(0,2)==0&&small.plot(0,-1)==2,
  "Native negative exact-multiple NULL must differ from positive and negative nonmultiple wrap");
 std::set<std::int64_t> addresses;std::size_t nonnull=0;
 for(const auto& p:wrapped)if(p){addresses.insert(*p);++nonnull;}
 need(addresses.size()==4&&nonnull>addresses.size(),"Wrapped native37 call multiplicity lost");
 small.wrap_x=false;small.wrap_y=false;const auto edges=runtime::detail::bnw_citizen_plot_positions(small,0,0);
 std::size_t nulls=0;for(const auto& p:edges)if(!p)++nulls;
 need(edges[0]&&nulls>0,"Native HOME/edge NULL order differs");
}
void actual(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=runtime::identify_file(save).sha256;
 auto host=boot(save,game);const auto source_hash=runtime::identify_bytes(host->session().source().source.bytes).sha256;
 const auto& raw=host->session().state().world.all_players->players;
 const saves::SavedCityObservation* city=nullptr;std::int32_t owner=-1;
 for(const auto& p:raw)if(!p.cities.cities.empty()){owner=static_cast<std::int32_t>(p.saved_player_id);city=&p.cities.cities.front();break;}
 need(city,"Actual selected city roster empty");const auto full=scalar(city->core.fields,"m_iID");
 const auto x=scalar(city->core.fields,"m_iX"),y=scalar(city->core.fields,"m_iY");
 need(runtime::resolve_bnw_city_registry(host->session().state().world,host->session().state().cities,owner,full&8191)==city,
  "Actual full/index city alias differs");
 const auto pair_rules=runtime::BnwPlotTerrainRulesSnapshot::from_rules(host->database());
 const auto before=host->session().checkpoint();
 auto refresh=runtime::BnwCityYieldRefresh::from_selected_host(host,owner,full&8191);
 const auto first=refresh->progress();need(first.owner==owner&&first.city==full&&first.next_slot==0&&first.operation,
  "Actual source registration changed city/native slot");
 refuses([&]{(void)runtime::BnwCityYieldRefresh::from_selected_host(host,owner,full);},"already owns");
 // Actual city op is registered BEFORE its first plot. Invalid byte-component
 // input cannot bypass that once history: both APIs must reject before codecs.
 refuses([&]{(void)runtime::make_bnw_plot_terrain_checkpoint_pair(host->session(),host->identity(),pair_rules);},"city-yield once history");
 const runtime::BnwPlotTerrainCheckpointPair not_a_checkpoint{};
 refuses([&]{(void)runtime::restore_bnw_plot_terrain_checkpoint_pair(not_a_checkpoint,host->session(),host->identity(),pair_rules);},"city-yield once history");
 bool foreign=false;std::thread worker([&]{try{(void)refresh->progress();}catch(const std::exception& e){foreign=std::string_view(e.what()).find("foreign")!=std::string_view::npos;}});worker.join();
 need(foreign,"Foreign city-refresh read touched actual world");
 // Drop the external Host alias: the actual query handle must retain it.
 std::weak_ptr<ui::SelectedBnwPreviewHost> weak_host=host;host.reset();
 need(!weak_host.expired()&&refresh->progress().operation==first.operation,"External source handle did not retain real parent");
 host=weak_host.lock();const auto& map=*host->session().state().world.map_projection;
 const runtime::MapGeometry geom{map.grid,map.wrap_x,map.wrap_y,saves::GameStateLayout::expansion2_version1};
 const auto positions=runtime::detail::bnw_citizen_plot_positions(geom,x,y);need(positions[0].has_value(),"Actual native HOME absent");
 refresh->advance();const auto next=refresh->progress();
 need(next.attempts>first.attempts&&next.next_slot<=1&&next.null_skips==0,"Actual first native call order differs");
 if(next.next_slot==0){
  need(next.phase==runtime::BnwCityYieldRefreshPhase::plot_call&&next.current_child_operation,
   "Actual child blocked without retained operation");
  const auto child=host->session().bnw_plot_yield_refresh_pending(*positions[0])->progress();
  need(child.operation==*next.current_child_operation&&child.plot==*positions[0],"Actual root latest pointer/op mismatch");
  need(child.phase==runtime::BnwPlotYieldRefreshPhase::city_screen_predicate||child.phase==runtime::BnwPlotYieldRefreshPhase::symbols,
   "Actual first child did not reach precise mandatory effect");
 }else need(next.returned_calls==1,"Actual first source call skipped child return");
 const auto& current=host->session().state().world;
 const auto& cache=named(current.map_plots->plots.at(static_cast<std::size_t>(*positions[0])).arrays,"yields").values;
 const auto& projection=current.map_projection->cells.at(static_cast<std::size_t>(*positions[0]));
 need(cache.size()==6&&projection.cached_yields,"Actual native/projected six short cache absent");
 for(std::size_t yi=0;yi<6;++yi)need(projection.cached_yields->at(static_cast<std::int64_t>(yi))==cache[yi],
  "Actual first source cache store lost owned mirror");
 host->session().restore(before);need(refresh->progress().phase==runtime::BnwCityYieldRefreshPhase::quarantined,
  "Rollback resurrected native city call");
 refuses([&]{refresh->advance();},"retired/quarantined");
 refuses([&]{(void)runtime::make_bnw_plot_terrain_checkpoint_pair(host->session(),host->identity(),pair_rules);},"city-yield once history");
 refuses([&]{(void)runtime::restore_bnw_plot_terrain_checkpoint_pair(not_a_checkpoint,host->session(),host->identity(),pair_rules);},"city-yield once history");
 refuses([&]{(void)runtime::BnwCityYieldRefresh::from_selected_host(host,owner,full);},"registry retired");
 need(runtime::identify_bytes(host->session().source().source.bytes).sha256==source_hash&&runtime::identify_file(save).sha256==original,
  "Actual source/original save changed");
 refresh.reset();host.reset();need(weak_host.expired(),"Session -> Pending -> Host cycle leaked genuine owner");
 // A real facade assigned generic restored state cannot issue a fresh Game.
 auto generic=boot(save,game);auto detached=generic->session().state();
 generic->session()=runtime::LiveSession::from_restored_bnw(generic->session().source(),std::move(detached));
 refuses([&]{(void)runtime::BnwCityYieldRefresh::from_selected_host(generic,owner,full);},"SAME actual Host");
 std::cout<<"Actual native city child: owner="<<owner<<" full="<<full<<" first-op="<<first.operation
  <<" next-slot="<<unsigned(next.next_slot)<<" calls="<<unsigned(next.returned_calls)<<"\n";
 std::cout<<"Full native37 / wrapped duplicate issuing / Entry religion return UNOBSERVED by first-call fixture\n";
}
}
int main(int argc,char** argv){try{
 static_assert(!std::is_default_constructible_v<runtime::BnwCityYieldRefreshPending>);
 geometry();need(argc==3,"Expected SAVE INSTALLATION");actual(argv[1],argv[2]);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
