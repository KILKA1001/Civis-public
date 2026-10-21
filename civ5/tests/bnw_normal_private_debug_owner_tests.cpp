#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>

using namespace civ5;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void refuses(F&& f,std::string_view text={}){
 bool refused=false;
 try{f();}catch(const std::exception& e){refused=true;check(text.empty()||std::string_view(e.what()).find(text)!=std::string_view::npos,"Wrong private Game refusal");}
 check(refused,"Invalid private Game owner accepted");
}
template<class F>void foreign_refuses(F&& f){
 bool refused=false;std::thread worker([&]{try{f();}catch(const std::exception&){refused=true;}});
 worker.join();check(refused,"Foreign thread accessed actual Game/world");
}
using Six=std::array<std::int64_t,6>;
Six cached(const runtime::LiveSession& session,std::int32_t plot){
 const auto& cell=session.state().world.map_projection->cells.at(static_cast<std::size_t>(plot));
 check(cell.cached_yields&&cell.cached_yields->size()==6,"Actual plot six short cache absent");
 Six out{};for(std::size_t y=0;y<6;++y)out[y]=cell.cached_yields->at(static_cast<std::int64_t>(y));return out;
}
void same_progress(const runtime::BnwPlotYieldRefreshProgress& a,const runtime::BnwPlotYieldRefreshProgress& b){
 check(a.plot==b.plot&&a.operation==b.operation&&a.next_yield==b.next_yield&&a.plot_writes==b.plot_writes&&
  a.terrain_writes==b.terrain_writes&&a.phase==b.phase&&a.effect_started==b.effect_started&&
  a.fresh_six_yields==b.fresh_six_yields,"Staged actual restore changed source cursor/history");
}
void selected(const std::filesystem::path& save,const std::filesystem::path& installation){
 const auto save_hash=runtime::identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,installation,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 check(static_cast<bool>(boot.preview.host),"Genuine immutable selected host absent");
 const auto& host=*boot.preview.host;
 const auto payload_hash=runtime::identify_bytes(host.session().source().source.bytes).sha256;
 const auto rules=runtime::BnwNormalPlotYieldRules::load(host.database(),host.identity().rules_sha256);
 const auto city_rules=runtime::load_city_yield_rules(host.database(),content::TechnologyProfile::bnw);
 const auto maintenance=runtime::load_maintenance_rules(host.database(),content::TechnologyProfile::bnw,host.types());
 const auto treasury=runtime::load_treasury_rules(host.database(),content::TechnologyProfile::bnw,host.types());
 const auto unhappiness=runtime::load_bnw_unhappiness_rules(host.database());
 const runtime::LiveSessionImportRules imports{host.database(),host.types(),host.technologies(),host.defines(),
  city_rules,host.production_rules(),maintenance,treasury,host.identity(),{}};
 auto fresh=[&]{return runtime::LiveSession::restore_bnw(host.session().source(),imports);};
 constexpr std::int32_t home=632; // Actual road home, not a detached NO_ROUTE variant.
 check(host.session().state().world.map_projection->cells.at(home).plot_city.has_value(),"Actual home632 fixture changed");
 {
  auto session=fresh();check(!session.bnw_game_debug_mode(),"Actual Reset/Read does not restore false Game cache");
  foreign_refuses([&]{(void)session.bnw_game_debug_mode();});
  foreign_refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(home,rules);});
  foreign_refuses([&]{session.rebuild_bnw_unhappiness_cache(unhappiness,host.defines());});
  check(!session.bnw_game_debug_mode(),"Foreign refusal retired real owner");
  const auto pending=session.begin_bnw_plot_normal_yield_refresh(home,rules);
  const auto progress=pending->progress();const auto after=cached(session,home);
  check(!progress.effect_started,"Normal Game join fabricated UI delivery");
  check(progress.phase==runtime::BnwPlotYieldRefreshPhase::city_screen_predicate||
   progress.phase==runtime::BnwPlotYieldRefreshPhase::symbols||progress.phase==runtime::BnwPlotYieldRefreshPhase::complete,
   "Actual home632 did not reach real native normal prefix");
  if(progress.phase!=runtime::BnwPlotYieldRefreshPhase::complete)check(!progress.fresh_six_yields,"Pending mandatory UI granted freshness");
  foreign_refuses([&]{(void)pending->progress();});
  const auto cp=session.checkpoint();const auto gen=session.state_generation();
  auto other=fresh();const auto other_cp=other.checkpoint();
  refuses([&]{session.restore(other_cp);},"another session");
  same_progress(progress,pending->progress());check(session.state_generation()==gen&&!session.bnw_game_debug_mode(),"Failed staging changed actual field owner");
  session.restore(cp);
  check(session.state_generation()==gen+1&&!session.bnw_game_debug_mode()&&cached(session,home)==after,"Healthy restore lost NEW real Game/world owner");
  refuses([&]{(void)pending->progress();},"retired/expired");
  const auto restored=session.bnw_plot_yield_refresh_pending(home);same_progress(progress,restored->progress());
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(home,rules);});
  // Reusing immutable restoration DATA creates another owner/generation; it
  // does not resurrect either original retired read control.
  session.restore(cp);check(session.state_generation()==gen+2&&!session.bnw_game_debug_mode(),"Checkpoint value resurrected old control");
  refuses([&]{(void)restored->progress();},"retired/expired");
  same_progress(progress,session.bnw_plot_yield_refresh_pending(home)->progress());
  check(!session.readiness().can_continue(),"Game debug join enabled full turn");
 }
 {
  auto session=fresh();const auto pending=session.begin_bnw_plot_normal_yield_refresh(home,rules);
  const auto cp=session.checkpoint();const auto after=cached(session,home);
  auto moved=std::move(session);check(!moved.bnw_game_debug_mode(),"Noexcept move copied/lost actual Game fields");
  refuses([&]{(void)session.bnw_game_debug_mode();});
  refuses([&]{(void)pending->progress();},"retired/expired");
  refuses([&]{(void)moved.begin_bnw_plot_normal_yield_refresh(home,rules);});
  moved.restore(cp);check(!moved.bnw_game_debug_mode()&&cached(moved,home)==after,"Move restore did not stage new actual owner");
  (void)moved.bnw_plot_yield_refresh_pending(home)->progress();
 }
 {
  auto incoming=fresh();const auto cp=incoming.checkpoint();
  auto destination=fresh();const auto old_pending=destination.begin_bnw_plot_normal_yield_refresh(home,rules);
  destination=std::move(incoming);check(!destination.bnw_game_debug_mode(),"Move assignment lost incoming actual Game owner");
  refuses([&]{(void)old_pending->progress();},"retired/expired");
  refuses([&]{(void)incoming.bnw_game_debug_mode();});
  destination.restore(cp);check(!destination.bnw_game_debug_mode(),"Transferred checkpoint cannot restore actual owner");
 }
 {
  auto session=fresh();const auto pending=session.begin_bnw_plot_normal_yield_refresh(home,rules);
  const auto gen=session.state_generation();const auto after=cached(session,home);
  session.rebuild_bnw_unhappiness_cache(unhappiness,host.defines());
  check(session.state_generation()==gen+1&&!session.bnw_game_debug_mode()&&cached(session,home)==after,"Real rebuild lost actual Game storage or changed plot caches");
  refuses([&]{(void)pending->progress();},"retired/expired");
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(home,rules);});
 }
 {
  auto session=fresh();const auto pending=session.begin_bnw_plot_normal_yield_refresh(home,rules);
  const auto after=cached(session,home);(void)session.mutable_state();
  refuses([&]{(void)session.bnw_game_debug_mode();},"actual reset/Read owner");
  refuses([&]{(void)pending->progress();},"retired/expired");
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(home,rules);},"private BNW import identity");
  check(cached(session,home)==after,"Revocation changed raw short caches");
 }
 {
  auto generic=runtime::LiveSession::from_restored_bnw(host.session().source(),host.session().state());
  refuses([&]{(void)generic.bnw_game_debug_mode();},"actual reset/Read owner");
  refuses([&]{(void)generic.begin_bnw_plot_normal_yield_refresh(home,rules);},"private BNW import identity");
 }
 {
  std::shared_ptr<const runtime::BnwPlotYieldRefreshPending> retained;
  {auto session=fresh();retained=session.begin_bnw_plot_normal_yield_refresh(home,rules);}
  refuses([&]{(void)retained->progress();},"retired/expired"); // No raw facade callback.
 }
 {
  auto session=fresh();const auto pending=session.begin_bnw_plot_normal_yield_refresh(home,rules);
  const auto progress=pending->progress();const auto after=cached(session,home);const auto gen=session.state_generation();
  const auto rejected=runtime::apply_live_action(session,[](runtime::LiveSessionState&){
   return runtime::LiveActionResult{runtime::LiveActionOutcome::rejected,"actual rejected trial"};
  });
  check(rejected.outcome==runtime::LiveActionOutcome::rejected&&!session.bnw_game_debug_mode(),"Rejected trial retired real Game storage");
  same_progress(progress,pending->progress());
  refuses([&]{(void)runtime::apply_live_action(session,[](runtime::LiveSessionState&)->runtime::LiveActionResult{
   throw std::runtime_error("actual throwing trial");
  });},"actual throwing trial");
  check(session.state_generation()==gen&&!session.bnw_game_debug_mode()&&cached(session,home)==after,"Throwing trial changed actual Game/world");
  same_progress(progress,pending->progress());
  const auto committed=runtime::apply_live_action(session,[](runtime::LiveSessionState&){
   return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,"generic no-op trial, no typed Game proof"};
  });
  check(committed.outcome==runtime::LiveActionOutcome::committed&&session.state_generation()==gen+1&&cached(session,home)==after,"Committed trial lost exact transaction semantics");
  refuses([&]{(void)session.bnw_game_debug_mode();},"actual reset/Read owner");
  refuses([&]{(void)pending->progress();},"retired/expired");
  refuses([&]{(void)session.begin_bnw_plot_normal_yield_refresh(home,rules);},"private BNW import identity");
 }
 // Authentic hidden-resource discovery only. Missing scene or a distinct
 // reached producer frontier cannot be replaced by manufactured issuing state.
 std::uint32_t visited=0;std::optional<std::int32_t> hidden;bool issued_hidden=false;
 const auto& actual=*host.session().state().world.map_projection;
 for(std::size_t i=0;i<actual.cells.size()&&visited<64;++i){
  const auto& cell=actual.cells[i];
  if(i>static_cast<std::size_t>(INT32_MAX)||!cell.owner||*cell.owner<0||
   !cell.resource_type_hash||*cell.resource_type_hash==0)continue;
  ++visited;const auto plot=static_cast<std::int32_t>(i);
  try{(void)runtime::calculate_bnw_normal_plot_yield(host.session().state(),host.session().source(),host.identity(),rules,plot,0);}
  catch(const runtime::BnwNormalPlotYieldUnavailable& e){
   if(std::string_view(e.what()).find("hidden resource requires genuine debug owner join")!=std::string_view::npos){hidden=plot;break;}
  }
 }
 if(hidden){
  auto session=fresh();const auto old=cached(session,*hidden);
  try{const auto pending=session.begin_bnw_plot_normal_yield_refresh(*hidden,rules);(void)pending->progress();issued_hidden=true;}
  catch(const std::exception& e){check(cached(session,*hidden)==old,"Rejected hidden prepare changed stores");std::cout<<"Actual hidden resource "<<*hidden<<" reached separate frontier: "<<e.what()<<'\n';}
 }
 check(runtime::identify_file(save).sha256==save_hash&&runtime::identify_bytes(host.session().source().source.bytes).sha256==payload_hash,"Original selected save/source payload changed");
 std::cout<<"Actual private Game lifecycle prepared; hidden-scene="<<(hidden?"observed":"UNOBSERVED")
  <<" hidden-issued="<<issued_hidden<<"; true-debug writer/no-area/quarantine/native UI scenarios unverified\n";
}
}
static_assert(std::is_nothrow_move_constructible_v<runtime::LiveSession>);
static_assert(std::is_nothrow_move_assignable_v<runtime::LiveSession>);
static_assert(!std::is_default_constructible_v<runtime::BnwPlotYieldRefreshIssuer>);
int main(int argc,char** argv){try{check(argc==3,"Supply actual selected BNW save and shared installation view");selected(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL private Game owner: "<<e.what()<<'\n';return 1;}}
