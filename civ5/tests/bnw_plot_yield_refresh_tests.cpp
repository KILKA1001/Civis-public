#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f,std::string_view reason={}){
 bool refused=false;try{f();}catch(const std::exception& e){refused=true;check(reason.empty()||std::string_view(e.what()).find(reason)!=std::string_view::npos,"Wrong refresh refusal/source order");}
 check(refused,"Invalid plot refresh accepted");
}
auto& named(auto& rows,std::string_view name){for(auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Selected source field missing");}
struct Pick {std::int64_t player=-1,city=-1,plot=-1;std::size_t player_slot=0,city_slot=0;};
Pick choose(const runtime::LiveSessionState& s){
 for(std::size_t p=0;p<s.world.all_players->players.size();++p){const auto& player=s.world.all_players->players[p];
  if(player.cities.cities.empty())continue;const auto& city=player.cities.cities.front();
  const auto x=named(city.core.fields,"m_iX").values.front(),y=named(city.core.fields,"m_iY").values.front();
  check(x>=0&&y>=0,"Selected city coordinates negative");
  return {player.saved_player_id,named(city.core.fields,"m_iID").values.front(),static_cast<std::int64_t>(s.world.map_projection->grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y))),p,0};
 }throw std::runtime_error("Selected founded city unavailable");
}
auto& plot(auto& s,const Pick& p){return s.world.map_plots->plots.at(static_cast<std::size_t>(p.plot));}
auto& cell(auto& s,const Pick& p){return s.world.map_projection->cells.at(static_cast<std::size_t>(p.plot));}
auto& city(auto& s,const Pick& p){return s.world.all_players->players.at(p.player_slot).cities.cities.at(p.city_slot);}
auto& terrain(auto& s,const Pick& p){return named(city(s,p).core.fields,"m_aiBaseYieldRateFromTerrain").values;}
std::array<std::int64_t,6> cache(const auto& s,const Pick& p){
 std::array<std::int64_t,6> result{};const auto& raw=named(plot(s,p).arrays,"yields").values;
 for(std::size_t y=0;y<6;++y){result[y]=raw.at(y);check(cell(s,p).cached_yields->at(static_cast<std::int64_t>(y))==result[y],"Raw/projected cache mismatch");}return result;
}
void cached(runtime::LiveSessionState& s,const Pick& p,const std::array<std::int64_t,6>& values){
 auto& raw=named(plot(s,p).arrays,"yields").values;check(raw.size()==6,"Native short yield span differs");
 for(std::size_t y=0;y<6;++y){raw[y]=values[y];cell(s,p).cached_yields->at(static_cast<std::int64_t>(y))=values[y];}
}
void early(runtime::LiveSessionState& s,const Pick& p){
 // Transparent finite native input fixture on the actual imported map/city.
 // This is NO_TERRAIN source calculation, never saved-cache freshness or UI ack.
 named(plot(s,p).fields,"terrain_type").value=-1;cell(s,p).terrain_type=-1;
 named(plot(s,p).fields,"potential_city_work").value=1;cell(s,p).potential_city_work=true;
}
void worked(runtime::LiveSessionState& s,const Pick& p){
 named(plot(s,p).fields,"working_city_owner").value=p.player;named(plot(s,p).fields,"working_city_id").value=p.city;
 check(cell(s,p).yield_city.has_value(),"Working-city projection absent");cell(s,p).yield_city->owner=p.player;cell(s,p).yield_city->id=p.city;
 auto& flags=named(named(city(s,p).management.blocks,"citizens").arrays,"m_pabWorkingPlot").values;check(flags.size()==37,"Native citizen slot span differs");flags.at(0)=1;
 // Remaining production yield has no sparse mirror node, a supported zero
 // before-image. Checkpoint restoration must prepare its future insertion.
 terrain(s,p).at(1)=0;s.cities.players.at(p.player).cities.at(p.city).yields.erase(1);
}
void unworked(runtime::LiveSessionState& s,const Pick& p){
 named(plot(s,p).fields,"working_city_owner").value=-1;named(plot(s,p).fields,"working_city_id").value=-1;
 check(cell(s,p).yield_city.has_value(),"Working-city projection absent");cell(s,p).yield_city->owner=-1;cell(s,p).yield_city->id=-1;
}
void selected(const std::filesystem::path& path,const std::filesystem::path& game_root){
 const auto file_hash=runtime::identify_file(path).sha256;
 auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
 if(bootstrap.preview.blocker)throw std::runtime_error("Selected BNW bootstrap refused: "+bootstrap.preview.blocker->reason);
 check(bootstrap.preview.host!=nullptr,"Selected host absent");const auto& host=*bootstrap.preview.host;
 const auto source_payload=runtime::identify_bytes(host.session().source().source.bytes).sha256;
 const auto& base=host.session().state();check(base.world.map_projection&&base.world.map_plots&&base.world.map_regions&&base.world.all_players,"Complete selected map/city import unavailable");
 const auto p=choose(base);const auto original_cache=cache(base,p);
 auto fresh=[&](runtime::LiveSessionState s){return runtime::LiveSession::from_restored_bnw(host.session().source(),std::move(s));};
 auto worked_state=[&]{auto s=base;early(s,p);worked(s,p);cached(s,p,{3,5,0,0,0,0});return s;};
 {
  auto s=worked_state();const auto food=terrain(s,p).at(0);auto session=fresh(std::move(s));
  const auto before=cache(session.state(),p);auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto progress=pending->progress();
  auto second=p;second.plot=static_cast<std::int64_t>((static_cast<std::uint64_t>(p.plot)+1)%session.state().world.map_plots->plots.size());const auto second_before=cache(session.state(),second);
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(second.plot);});check(cache(session.state(),second)==second_before,"Second plot wrote before previous mandatory effect");
  refuses([&]{(void)session.bnw_plot_yield_refresh_pending(second.plot);},"no issued cursor");
  check(progress.phase==runtime::BnwPlotYieldRefreshPhase::city_screen_predicate&&progress.next_yield==0&&progress.plot_writes==1&&progress.terrain_writes==1&&!progress.effect_started&&!progress.fresh_six_yields,"Worked refresh crossed mandatory source UI predicate");
  check(cache(session.state(),p)==std::array<std::int64_t,6>{0,5,0,0,0,0}&&before[0]==3,"First source plot store/projection differs");
  check(terrain(session.state(),p).at(0)==food-3&&session.state().cities.players.at(p.player).cities.at(p.city).yields.at(0).terrain==food-3&&terrain(session.state(),p).at(1)==0,"Worked source terrain delta/order differs");
  check(session.bnw_plot_yield_refresh_pending(p.plot).get()==pending.get(),"Session discarded its actual cursor");
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});check(cache(session.state(),p)[1]==5,"Duplicate continued source writes");
  auto old_view=session.read_state_lease();const auto cp=session.checkpoint();const auto generation=session.state_generation();session.restore(cp);
  check(session.state_generation()==generation+1&&session.read_state_lease().get()!=old_view.get(),"Checkpoint did not replace actual allocation");
  refuses([&]{(void)pending->progress();},"retired/expired");
  auto restored=session.bnw_plot_yield_refresh_pending(p.plot);check(restored.get()!=pending.get()&&restored->progress().phase==progress.phase&&restored->progress().next_yield==progress.next_yield&&restored->progress().operation==progress.operation,"Checkpoint lost source cursor/once identity");
  check(cache(session.state(),p)==cache(*old_view,p)&&terrain(session.state(),p)==terrain(*old_view,p),"Actual raw/projected/terrain checkpoint roundtrip differs");
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});
  bool thread_begin=false,thread_get=false,thread_progress=false,thread_cp=false;
  std::thread worker([&]{try{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot+1);}catch(const std::exception&){thread_begin=true;}try{(void)session.bnw_plot_yield_refresh_pending(p.plot);}catch(const std::exception&){thread_get=true;}try{(void)restored->progress();}catch(const std::exception&){thread_progress=true;}try{(void)session.checkpoint();}catch(const std::exception&){thread_cp=true;}});worker.join();
  check(thread_begin&&thread_get&&thread_progress&&thread_cp,"Foreign thread obtained refresh/current state");
  check(!session.readiness().can_continue()&&!restored->full_city_refresh_ready(),"Prefix claimed growth/turn readiness");
 }
 {
  auto s=base;early(s,p);unworked(s,p);cached(s,p,{3,5,-2,0,0,0});const auto old_terrain=terrain(s,p);auto session=fresh(std::move(s));
  auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto progress=pending->progress();
  check(cache(session.state(),p)==std::array<std::int64_t,6>{0,0,0,0,0,0}&&terrain(session.state(),p)==old_terrain,"Unworked short stores changed a city");
  check(progress.phase==runtime::BnwPlotYieldRefreshPhase::symbols&&progress.plot_writes==3&&progress.terrain_writes==0&&progress.next_yield==6&&!progress.fresh_six_yields,"Changed unworked plot skipped mandatory symbols");
  const auto cp=session.checkpoint();session.restore(cp);check(session.bnw_plot_yield_refresh_pending(p.plot)->progress().phase==runtime::BnwPlotYieldRefreshPhase::symbols,"Checkpoint skipped symbols");
  refuses([&]{(void)pending->progress();});
 }
 {
  auto s=base;early(s,p);cached(s,p,{0,0,0,0,0,0});
  // All unchanged: source never reads the working-city/citizen inputs.
  cell(s,p).yield_city.reset();named(plot(s,p).fields,"working_city_owner").name="unreachable_working_city";
  auto session=fresh(std::move(s));auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);
  const auto progress=pending->progress();check(progress.phase==runtime::BnwPlotYieldRefreshPhase::complete&&progress.fresh_six_yields&&progress.plot_writes==0&&progress.terrain_writes==0&&progress.next_yield==6,"Unchanged zero plot read unreachable city or issued effects");
  const auto cp=session.checkpoint();session.restore(cp);check(session.bnw_plot_yield_refresh_pending(p.plot)->progress().fresh_six_yields,"Completed zero checkpoint lost fresh receipt");
 }
 {
  auto s=base;named(plot(s,p).fields,"area").value=-1;cell(s,p).cached_yields.reset();cell(s,p).terrain_type.reset();cell(s,p).potential_city_work.reset();s.world.map_regions.reset();named(plot(s,p).arrays,"yields").values.clear();
  auto session=fresh(std::move(s));auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto progress=pending->progress();
  check(progress.phase==runtime::BnwPlotYieldRefreshPhase::skipped_no_area&&!progress.fresh_six_yields&&progress.plot_writes==0,"Native NULL area did not short-circuit before caches/calculation");
 }
 {
  auto s=base;named(plot(s,p).fields,"terrain_type").value=0;cell(s,p).terrain_type=0;named(plot(s,p).fields,"potential_city_work").value=1;cell(s,p).potential_city_work=true;auto session=fresh(std::move(s));const auto before=cache(session.state(),p);
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);},"calculateYield producer unavailable");check(cache(session.state(),p)==before,"Unsupported normal calculation partially stored");
  refuses([&]{(void)session.bnw_plot_yield_refresh_pending(p.plot);},"no issued cursor");
 }
 for(int stale_kind=0;stale_kind<3;++stale_kind){
  auto session=fresh(worked_state());auto& alias=session.mutable_state();const auto earlier=session.checkpoint();auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto native_cache=cache(session.state(),p);
  if(stale_kind==0)++alias.world.map_header.guid[0];else if(stale_kind==1)++cell(alias,p).cached_yields->at(1);else named(named(city(alias,p).management.blocks,"citizens").arrays,"m_pabWorkingPlot").values.at(0)=0;
  refuses([&]{(void)pending->progress();});refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});
  // CP before issuance must not erase a later failed/irreversible once slot.
  session.restore(earlier);check(cache(session.state(),p)==std::array<std::int64_t,6>{3,5,0,0,0,0}&&native_cache[0]==0,"Explicit old CP did not restore raw before-image");
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});refuses([&]{(void)session.bnw_plot_yield_refresh_pending(p.plot);},"terminal failure");
  const auto quarantined_cp=session.checkpoint();session.restore(quarantined_cp);refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});
 }
 {
  auto session=fresh(worked_state());auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto cp=session.checkpoint();(void)session.mutable_state();
  refuses([&]{(void)pending->progress();});refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});session.restore(cp);check(session.bnw_plot_yield_refresh_pending(p.plot)->progress().plot_writes==1,"Explicit CP failed to restore retired non-dispatched prefix");
 }
 {
  auto s=base;early(s,p);cached(s,p,{0,0,0,0,0,0});auto session=fresh(std::move(s));auto pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);
  check(pending->progress().phase==runtime::BnwPlotYieldRefreshPhase::complete,"Completed retirement fixture did not reach source end");
  (void)session.mutable_state();const auto retired_cp=session.checkpoint();session.restore(retired_cp);
  refuses([&]{(void)pending->progress();});refuses([&]{(void)session.bnw_plot_yield_refresh_pending(p.plot);},"terminal failure");refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});
  const auto terminal_cp=session.checkpoint();session.restore(terminal_cp);refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);});
 }
 {
  auto a=fresh(worked_state());auto pending=a.begin_bnw_plot_early_zero_yield_refresh(p.plot);auto moved=std::move(a);const auto retired_cp=moved.checkpoint();moved.restore(retired_cp);
  refuses([&]{(void)pending->progress();});refuses([&]{(void)moved.bnw_plot_yield_refresh_pending(p.plot);},"terminal failure");refuses([&]{(void)moved.begin_bnw_plot_early_zero_yield_refresh(p.plot);});
 }
 {
  auto a=fresh(worked_state());auto a_pending=a.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto cp=a.checkpoint();auto moved=std::move(a);
  refuses([&]{(void)a_pending->progress();});refuses([&]{(void)a.begin_bnw_plot_early_zero_yield_refresh(p.plot);},"facade");refuses([&]{a.restore(cp);},"facade");refuses([&]{(void)moved.begin_bnw_plot_early_zero_yield_refresh(p.plot);});moved.restore(cp);
  auto moved_pending=moved.bnw_plot_yield_refresh_pending(p.plot);auto b=fresh(worked_state());auto b_pending=b.begin_bnw_plot_early_zero_yield_refresh(p.plot);const auto b_cp=b.checkpoint();moved=std::move(b);
  refuses([&]{(void)moved_pending->progress();});refuses([&]{(void)b_pending->progress();});refuses([&]{moved.restore(cp);},"another session");moved.restore(b_cp);check(moved.bnw_plot_yield_refresh_pending(p.plot)->progress().terrain_writes==1,"Move assignment lost genuine CP cursor");
 }
 {
  std::shared_ptr<const runtime::BnwPlotYieldRefreshPending> pending;std::weak_ptr<const runtime::LiveSessionState> retained;
  {auto session=fresh(worked_state());pending=session.begin_bnw_plot_early_zero_yield_refresh(p.plot);retained=session.read_state_lease();}
  check(!retained.expired(),"Pending did not retain state allocation");refuses([&]{(void)pending->progress();},"retired/expired");pending.reset();check(retained.expired(),"Expired pending retained allocation forever");
 }
 {
  auto a=fresh(worked_state()),b=fresh(worked_state());const auto cp=a.checkpoint();const auto before=cache(b.state(),p);refuses([&]{b.restore(cp);},"another session");check(cache(b.state(),p)==before,"Foreign checkpoint partially wrote map");
 }
 {
  auto s=worked_state();cell(s,p).cached_yields->at(0)=4;auto session=fresh(std::move(s));const auto before=named(plot(session.state(),p).arrays,"yields").values;
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);},"cache stale");check(named(plot(session.state(),p).arrays,"yields").values==before,"Stale projection partially wrote raw cache");
 }
 {
  auto s=worked_state();terrain(s,p).at(0)=INT32_MIN;s.cities.players.at(p.player).cities.at(p.city).yields.at(0).terrain=INT32_MIN;auto session=fresh(std::move(s));const auto before=cache(session.state(),p);
  refuses([&]{(void)session.begin_bnw_plot_early_zero_yield_refresh(p.plot);},"int overflow");check(cache(session.state(),p)==before,"Terrain overflow wrote plot before preflight");
 }
 check(cache(host.session().state(),p)==original_cache&&runtime::identify_bytes(host.session().source().source.bytes).sha256==source_payload&&runtime::identify_file(path).sha256==file_hash,"Source host/save changed");
 check(!host.session().readiness().can_continue(),"Plot prefix opened full continuation");
 std::cout<<"PASS actual LiveSession early-zero plot stores/worked terrain/UI frontier, source seals/once/thread/lifetime/move/foreignCP and cursor/raw-mirror checkpoint preservation; mandatory UI not acknowledged\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwPlotYieldRefreshIssuer>);
static_assert(!std::is_copy_constructible_v<runtime::BnwPlotYieldRefreshIssuer>);
static_assert(!std::is_default_constructible_v<runtime::BnwPlotYieldRefreshPending>);
static_assert(!std::is_copy_constructible_v<runtime::BnwPlotYieldRefreshPending>);
static_assert(!std::is_default_constructible_v<runtime::BnwPlotYieldRefreshCheckpoint>);
int main(int argc,char** argv){try{check(argc==3,"Supply selected BNW save and installed game root");selected(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL BNW plot refresh: "<<e.what()<<'\n';return 1;}}
