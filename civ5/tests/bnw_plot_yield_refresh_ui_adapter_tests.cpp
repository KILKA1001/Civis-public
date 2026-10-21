#include "civ5/ui/bnw_plot_yield_refresh_ui_adapter.hpp"
#include "bnw_selected_city_test_locale.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <iostream>
#include <set>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f,std::string_view why={}){
 bool failed=false;try{f();}catch(const std::exception& e){
  failed=true;need(why.empty()||std::string_view(e.what()).find(why)!=std::string_view::npos,"Wrong tail refusal/source order");
 }need(failed,"Invalid tail operation accepted");
}
ui::UiContextRegistry registry(){
 // Real Civis typed navigation receiver; no invented stock Lua receipt.
 return ui::UiContextRegistry::from_documents(content::read_xml_metadata(R"(<Context>
 <LuaContext ID="WorldView" FileName="WorldView"/>
 <LuaContext ID="CityList" FileName="CityList" Hidden="True"/>
 <LuaContext ID="CityView" FileName="CityView" Hidden="True"/>
 </Context>)"),content::read_xml_metadata("<Context/>"));
}
void selected(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto save_hash=runtime::identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));
 need(bool(host),"Actual immutable Host absent");
 auto adapter=ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(host);
 refuses([]{(void)ui::BnwPlotYieldRefreshUiAdapter::from_selected_host({});},"Host absent");
 bool thread_read=false,thread_factory=false;
 std::thread worker([&]{
  try{adapter->require_current();}catch(const std::exception&){thread_read=true;}
  try{(void)ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(host);}catch(const std::exception&){thread_factory=true;}
 });worker.join();need(thread_read&&thread_factory,"Foreign thread entered actual source tail");
 auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
 auto live_ui=client->create_selected_ui_host(registry(),nullptr);
 const ui::ClientViewport viewport{1024,768,1024,768,96};
 auto frame=client->capture(viewport,{});
 const auto* cities=frame->ui.find<ui::CityScreenFrame>();
 need(cities&&!cities->cities.empty(),"Actual owned city projection absent");
 const auto chosen=cities->cities.front();
 std::cout<<"Actual plot UI city owner="<<chosen.owner<<" id="<<chosen.id
  <<" localized_name="<<(chosen.name?*chosen.name:"<absent>")<<'\n';
 need(chosen.name&&!chosen.name->empty(),"Actual plot UI selected city EN localized name absent");
 auto ui_frame=live_ui->begin_frame(1024,768);
 need(client->dispatch(*live_ui,ui_frame,frame,ui::UiOpenScreen{"CityList"}).status==ui::UiCommandStatus::navigation,
  "Actual CityList route refused");
 frame=client->capture(viewport,{});ui_frame=live_ui->begin_frame(1024,768);
 const ui::UiCommand action(ui::UiGameplayCommand::make("CityList",
  ui::WorldViewAction{ui::WorldViewActionKind::open_city,chosen.id}));
 const auto route=live_ui->dispatch(action,ui_frame);
 if(route.status!=ui::UiCommandStatus::navigation)
  throw std::runtime_error("Actual canonical city route refused: "+route.reason);
 frame=client->capture(viewport,{});
 const auto& city=host->session().state().cities.players.at(chosen.owner).cities.at(chosen.id);
 need(city.x>=0&&city.y>=0&&city.x<=INT32_MAX&&city.y<=INT32_MAX,"Native current city coordinates overflow");
 const auto& map=host->session().state().world.map_header;
 const runtime::MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,map.layout};
 const auto positions=runtime::detail::bnw_citizen_plot_positions(
  geometry,static_cast<std::int32_t>(city.x),static_cast<std::int32_t>(city.y));
 const auto rules=runtime::BnwNormalPlotYieldRules::load(host->database(),host->identity().rules_sha256);
 const auto initial=host->session().checkpoint();
 std::set<std::int64_t> visited;
 std::uint32_t supported=0,frontiers=0,changed=0,predicates=0,dirty_count=0,symbols=0;
 std::uint64_t last_occurrence=0;
 std::optional<runtime::LiveSession::Checkpoint> retained_notifications;
 std::optional<std::int64_t> issued;
 // Bounded future actual observation; never synthesize cache mismatches,
 // working flags, teams, source selection or a normal-mode permission.
 for(std::size_t slot=0;slot<24;++slot){
  if(!positions[slot]||!visited.insert(*positions[slot]).second)continue;
  const auto plot=*positions[slot];const auto revision=host->session().state().ui_source_revision;
  std::shared_ptr<const runtime::BnwPlotYieldRefreshPending> pending;
  try{pending=host->session().begin_bnw_plot_normal_yield_refresh(plot,rules);}
  catch(const runtime::BnwNormalPlotYieldUnavailable& e){++frontiers;std::cout<<"Native calculation frontier: "<<e.what()<<'\n';continue;}
  ++supported;auto progress=pending->progress();
  if(progress.plot_writes){
   ++changed;issued=plot;need(host->session().state().ui_source_revision>revision,"Real short/terrain store did not stale UI before notification");
   need(!client->is_current(frame),"Old frame survived real plot/terrain write");
  }
  using Phase=runtime::BnwPlotYieldRefreshPhase;
  for(std::size_t step=0;step<8&&progress.phase!=Phase::complete&&progress.phase!=Phase::skipped_no_area;++step){
   if(progress.phase==Phase::city_screen_predicate)++predicates;
   progress=adapter->continue_plot(plot);
  }
  need(progress.phase==Phase::complete||progress.phase==Phase::skipped_no_area,"Finite native tail did not reach its exact end");
  refuses([&]{(void)adapter->continue_plot(plot);},"cannot replay completed");
  refuses([&]{(void)host->session().begin_bnw_plot_normal_yield_refresh(plot,rules);});
  if(!host->session().state().plot_gameplay_events.empty()||!host->session().state().city_dirty_events.empty())
   retained_notifications=host->session().checkpoint();
  const auto events=adapter->take_plot_events();
  need(events.size()==(progress.plot_writes?1U:0U),"Symbols call lost/duplicated or unchanged plot invented notification");
  for(const auto& e:events){
   ++symbols;need(e.occurrence>last_occurrence&&e.generation==host->session().state_generation()&&
    e.operation==progress.operation&&e.plot==plot&&e.map_guid==host->session().state().world.map_header.guid&&
    e.target_revision<=host->session().state().ui_source_revision,"Typed event source binding/order differs");
   last_occurrence=e.occurrence;
   const auto& cell=host->session().state().world.map_projection->cells.at(static_cast<std::size_t>(plot));
   need(e.x==cell.x&&e.y==cell.y&&cell.cached_yields,"Typed plot event coordinates/cache absent");
   for(std::int64_t y=0;y<6;++y)need(e.yields.at(static_cast<std::size_t>(y))==cell.cached_yields->at(y),"Typed event actual short snapshot differs");
  }
  const auto dirty=client->take_city_dirty_events();
  need(dirty.size()<=6,"Native per-yield CityScreen notifications exceeded six");
  std::int8_t previous_yield=-1;
  for(const auto& e:dirty){
   ++dirty_count;need(e.kind==runtime::BnwCityDirtyKind::city_screen&&e.value&&e.plot==plot&&
    e.owner==chosen.owner&&e.city==chosen.id&&e.yield>previous_yield&&e.generation==host->session().state_generation(),
    "Native true selection dirty ordering/source differs");previous_yield=e.yield;
  }
  need(adapter->take_plot_events().empty(),"Consumed factual queue silently reissued notification");
  frame=client->capture(viewport,{});
 }
 std::cout<<"Actual tail observations supported="<<supported<<" frontier="<<frontiers<<" changed="<<changed
  <<" predicates="<<predicates<<" dirty="<<dirty_count<<" symbols="<<symbols<<'\n';
 if(!changed)std::cout<<"Changed imported prefix/void notification UNOBSERVED\n";
 if(!dirty_count)std::cout<<"Actual selected CityScreen dirty branch UNOBSERVED\n";
 need(!host->session().readiness().can_continue(),"Tail granted whole-game continuation");
 if(retained_notifications){
  host->session().restore(*retained_notifications);
  refuses([&]{adapter->require_current();},"retired");
  refuses([&]{(void)ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(host);},"generation reconciliation");
 }
 if(issued){
  host->session().restore(initial);
  refuses([&]{(void)host->session().begin_bnw_plot_normal_yield_refresh(*issued,rules);});
 }
 // Finish lifetime test on a separate genuine import so no old-generation
 // queue is reconciled by the test or presented as renderer completion.
 client.reset();live_ui.reset();frame.reset();adapter.reset();host.reset();
 auto fresh=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(fresh.preview.blocker)throw std::runtime_error(fresh.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> parent(std::move(fresh.preview.host));
 auto retained=ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(parent);
 std::weak_ptr<ui::SelectedBnwPreviewHost> weak=parent;parent.reset();
 need(!weak.expired(),"Adapter lost actual strong parent");retained->require_current();
 auto writer=weak.lock();auto moved=std::move(writer->session());
 refuses([&]{retained->require_current();},"retired");writer.reset();retained.reset();
 need(weak.expired(),"Session/adapter formed strong Host cycle");
 need(runtime::identify_file(save).sha256==save_hash,"Original save changed");
}
}
static_assert(!std::is_default_constructible_v<ui::BnwPlotYieldRefreshUiAdapter>);
static_assert(!std::is_copy_constructible_v<ui::BnwPlotYieldRefreshUiAdapter>);
static_assert(std::is_trivially_copyable_v<runtime::BnwPlotGameplayEvent>);
int main(int argc,char** argv){try{
 need(argc==3,"Supply selected BNW save and shared installation");selected(argv[1],argv[2]);
 std::cout<<"PASS genuine typed UI/gameplay tail scope; no renderer/full city turn grant\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL plot UI tail: "<<e.what()<<'\n';return 1;}}
