#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include <climits>
#include "bnw_selected_city_test_locale.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
template<class T>concept PublicUnitSelectionQuery=requires(const T& value){value.is_unit_selected(0,0);};
template<class T>concept PublicSelectionPlotQuery=requires(const T& value){value.selection_plot();};
static_assert(!PublicUnitSelectionQuery<ui::BnwClientSelectionControl>);
static_assert(!PublicSelectionPlotQuery<ui::BnwClientSelectionControl>);
static_assert(!std::is_default_constructible_v<ui::BnwClientSelectionControl>);
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}need(failed,"Missing genuine selection boundary accepted");}
ui::UiContextRegistry registry(){
 // These are actual Civis receiver contexts for the typed navigation test,
 // not a fabricated stock Lua/UI completion capability.
 return ui::UiContextRegistry::from_documents(content::read_xml_metadata(R"(<Context>
 <LuaContext ID="WorldView" FileName="WorldView"/>
 <LuaContext ID="CityList" FileName="CityList" Hidden="True"/>
 <LuaContext ID="CycleModal" FileName="UI/InGame/Popups/CycleModal" Hidden="True"/>
 <LuaContext ID="CityView" FileName="CityView" Hidden="True"/>
 </Context>)"),content::read_xml_metadata("<Context/>"));
}
void arithmetic(){
 using runtime::BnwCityDirtyKind;
 const auto selected=runtime::detail::bnw_citizens_outer_dirty_kinds(true);
 need(selected==std::vector{BnwCityDirtyKind::city_info,BnwCityDirtyKind::city_screen,
  BnwCityDirtyKind::colored_plots,BnwCityDirtyKind::city_info},"Selected notification order/repetition changed");
 need(runtime::detail::bnw_citizens_outer_dirty_kinds(false)==std::vector{BnwCityDirtyKind::city_info},
  "Unselected path lost unconditional CityInfo");
 need(static_cast<std::int32_t>(BnwCityDirtyKind::city_info)==18&&
  static_cast<std::int32_t>(BnwCityDirtyKind::city_screen)==21&&
  static_cast<std::int32_t>(BnwCityDirtyKind::colored_plots)==24,"SDK dirty identifiers changed");
}
void active_team_arithmetic(runtime::LiveSessionState state){
 // Work on a detached copy of the imported state; no Session owner is issued.
 state.world={};
 state.world.game_state.layout=saves::GameStateLayout::expansion2_version1;
 state.world.pregame.emplace().layout=state.world.game_state.layout;
 state.world.pregame->archive.integers["active_player"]=-1;
 need(runtime::detail::bnw_game_active_team_read(state)==-1,
  "NO_PLAYER read later absent roster/teamTypes instead of short circuit");
 state.world.pregame->archive.integers["active_player"]=64;
 refuses([&]{(void)runtime::detail::bnw_game_active_team_read(state);});
 // Supplied pure DTO math above does not issue a Session/selection owner.
}
void actual(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto hash=runtime::identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));
 need(bool(host),"Actual Host absent");
 active_team_arithmetic(host->session().state());
 auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
 refuses([&]{(void)ui::BnwClientFrameOwner::from_selected_host(host);});
 refuses([&]{client->select({});}); // No initialized actual LiveUiHost yet.
 auto live_ui=client->create_selected_ui_host(registry(),nullptr);
 need(!live_ui->selected_city(),"Actual Civis input constructor not empty");
 auto cycle=runtime::BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
 refuses([&]{(void)host->export_import_clock_checkpoint();});
 refuses([&]{(void)cycle->no_selection_list_cycle(*live_ui);});
 refuses([&]{cycle->execute_selection_cycle_prefix(*live_ui);});
 (void)cycle->execute_timer_prefix();cycle->execute_selection_cycle_prefix(*live_ui);
 need(!cycle->no_selection_list_cycle(*live_ui)&&cycle->view().selection_cycle_attempted,
  "Actual selection receiver cycle flag did not reset");
 refuses([&]{cycle->execute_selection_cycle_prefix(*live_ui);});
 const auto initial_dirty=client->unit_dirty_state();
 need(!initial_dirty.unit_info&&!initial_dirty.plot_list_buttons&&initial_dirty.next_occurrence==1&&
  initial_dirty.events.empty()&&initial_dirty.client_revision>0,"Actual new-client dirty constructor values differ");
 need(live_ui->unit_dirty_state()==initial_dirty,"Frame and LiveUiHost do not share actual dirty storage");
 need(client->take_unit_dirty_events().empty()&&live_ui->take_unit_dirty_events().empty()&&
  client->unit_dirty_state()==initial_dirty,"Empty delivery invented dirty/reset/revision effects");
 bool dirty_thread=false;std::thread dirty_worker([&]{try{(void)live_ui->unit_dirty_state();}catch(const std::exception&){dirty_thread=true;}});
 dirty_worker.join();need(dirty_thread,"Foreign thread read mutable canonical dirty storage");
 refuses([&]{(void)client->create_selected_ui_host(registry(),nullptr);});
 bool foreign=false;
 std::thread worker([&]{try{client->select({});}catch(const std::exception&){foreign=true;}});
 worker.join();need(foreign,"Foreign thread changed actual UI input");
 const ui::ClientViewport viewport{1024,768,1024,768,96};
 auto frame=client->capture(viewport,{});need(bool(frame),"Actual CPU frame absent");
 need(!frame->stamp.selection.plot,"New canonical client invented imported plot selection");
 const auto* published_dirty=frame->ui.find<ui::BnwClientUnitDirtySnapshot>();
 need(published_dirty&&*published_dirty==client->unit_dirty_state(),"Actual Frame omitted canonical dirty snapshot");
 const auto empty_plot_frame=frame;
 const auto input_world_revision=host->session().state().ui_source_revision;
 client->select_plot(0,frame);
 need(!client->is_current(frame)&&!client->current_frame(),"Actual plot input retained stale frame");
 const auto plot_provider=client->ui_provider();refuses([&]{(void)plot_provider();});
 frame=client->capture(viewport,{});
 need(frame->stamp.selection.plot==std::optional<std::int64_t>{0}&&
  frame->stamp.identity.selection_revision>empty_plot_frame->stamp.identity.selection_revision,
  "Actual plot input not published with canonical revision");
 const auto selected_plot_revision=frame->stamp.identity.selection_revision;
 client->select_plot(0);need(client->is_current(frame),"Repeated plot input changed current publication");
 refuses([&]{client->select_plot(1,empty_plot_frame);});
 refuses([&]{client->select_plot(-1,frame);});
 refuses([&]{client->select_plot(std::int64_t{INT32_MAX}+1,frame);});
 const auto plot_count=host->session().state().world.map_header.grid.size();
 refuses([&]{client->select_plot(static_cast<std::int64_t>(plot_count),frame);});
 need(client->is_current(frame)&&frame->stamp.identity.selection_revision==selected_plot_revision&&
  host->session().state().ui_source_revision==input_world_revision,
  "Rejected input changed canonical selection or gameplay revision");
 bool plot_foreign=false;
 std::thread plot_worker([&]{try{client->select_plot(std::nullopt,frame);}catch(const std::exception&){plot_foreign=true;}});
 plot_worker.join();need(plot_foreign&&client->is_current(frame),"Foreign plot input mutated canonical storage");
 // A real callback input change must invalidate the composed publication.
 need(plot_count>1,"Prepared actual map must contain a second plot");
 bool plot_callback=false;
 need(!client->present(frame,[&](const auto&){plot_callback=true;client->select_plot(1,frame);})&&plot_callback&&
  !client->current_frame(),"Plot input changed during callback but presentation remained current");
 frame=client->capture(viewport,{});need(frame->stamp.selection.plot==std::optional<std::int64_t>{1},"Callback plot input lost");
 client->select({});frame=client->capture(viewport,{});
 need(frame->stamp.selection.plot==std::optional<std::int64_t>{1},"Unit/city empty input cleared canonical picked plot");
 client->select_plot(std::nullopt,frame);frame=client->capture(viewport,{});
 need(!frame->stamp.selection.plot,"Explicit canonical empty plot input not stored");
 client->select_plot(0,frame);frame=client->capture(viewport,{});
 // Manual typed selection uses the genuine canonical owner and the exact
 // published frame. Its result does not certify engine selection/camera.
 need(host->activation_owner().busy_state.has_value(),"Original host lost retained busy owner");
 const auto* same_busy=&*host->activation_owner().busy_state;
 const auto moves=runtime::bind_bnw_update_moves_original_import(host->session().source(),host->identity());
 const auto cycle_command=ui::UiCommand{ui::UiGameplayCommand::make("WorldView",ui::BnwManualCycleSelectionCommand{})};
 auto cycle_ui=live_ui->begin_frame(1024,768);
 need(client->dispatch(*live_ui,cycle_ui,frame,ui::UiGameplayCommand::make("CityList",
  ui::BnwManualCycleSelectionCommand{})).status==ui::UiCommandStatus::invalid,
  "Wrong manual cycle source accepted");
 need(client->dispatch(*live_ui,cycle_ui,frame,ui::UiOpenScreen{"CycleModal"}).status==ui::UiCommandStatus::navigation,
  "Manual cycle modal fixture did not open");
 frame=client->capture(viewport,{});cycle_ui=live_ui->begin_frame(1024,768);
 need(client->dispatch(*live_ui,cycle_ui,frame,cycle_command).status==ui::UiCommandStatus::invalid&&
  client->is_current(frame),"Manual cycle bypassed modal input ownership");
 need(client->dispatch(*live_ui,cycle_ui,frame,ui::UiCloseScreen{"CycleModal"}).status==ui::UiCommandStatus::navigation,
  "Manual cycle modal fixture did not close");
 frame=client->capture(viewport,{});cycle_ui=live_ui->begin_frame(1024,768);
 const auto expected_cycle=runtime::observe_bnw_game_unit_cycle(host->session().state(),moves,*same_busy,
  host->identity(),host->active_player_id(),std::nullopt,runtime::BnwUnitCycleDirection::forward,
  runtime::BnwUnitCycleFilter::all);
 const auto cycle_generation=host->session().state_generation();
 const auto cycle_map_random=host->session().state().random.map.state();
 const auto cycle_game_random=host->session().state().random.gameplay.state();
 const auto first_cycle=client->dispatch(*live_ui,cycle_ui,frame,cycle_command);
 need(first_cycle.status==ui::UiCommandStatus::navigation,"Original manual cycle command refused");
 auto cycled=client->capture(viewport,{});
 need(cycled->stamp.selection.unit==(expected_cycle.next?std::optional{expected_cycle.next->second}:std::nullopt)&&
  cycled->stamp.selection.plot==frame->stamp.selection.plot&&!cycled->stamp.selection.city,
  "Manual cycle published a different canonical selection");
 need(host->session().state_generation()==cycle_generation&&
  host->session().state().random.map.state()==cycle_map_random&&
  host->session().state().random.gameplay.state()==cycle_game_random&&
  &*host->activation_owner().busy_state==same_busy,"Manual input replaced world/RNG/busy ownership");
 if(expected_cycle.next){
  need(!client->is_current(frame)&&client->dispatch(*live_ui,cycle_ui,frame,cycle_command).status==ui::UiCommandStatus::invalid,
   "Changed manual selection retained a replayable input frame");
  cycle_ui=live_ui->begin_frame(1024,768);
  const auto head=runtime::BnwUnitOrderKey{host->active_player_id(),expected_cycle.next->second};
  const auto expected_second=runtime::observe_bnw_game_unit_cycle(host->session().state(),moves,*same_busy,
   host->identity(),host->active_player_id(),head,runtime::BnwUnitCycleDirection::backward,
   runtime::BnwUnitCycleFilter::workers);
  const auto second=client->dispatch(*live_ui,cycle_ui,cycled,ui::UiGameplayCommand::make("WorldView",
   ui::BnwManualCycleSelectionCommand{runtime::BnwUnitCycleDirection::backward,runtime::BnwUnitCycleFilter::workers}));
  if(expected_second.send_auto_moves_required){
   need(second.status==ui::UiCommandStatus::unavailable&&client->is_current(cycled),
    "Required sendAutoMoves branch wrote selection before refusal");
  }else{
   need(second.status==ui::UiCommandStatus::navigation,"Second manual cycle failed");
   const auto after=client->capture(viewport,{});
   const auto expected=expected_second.next?std::optional{expected_second.next->second}:
    (expected_second.clear_selection_required?std::nullopt:cycled->stamp.selection.unit);
   need(after->stamp.selection.unit==expected,"Second manual cycle changed source candidate");
  }
 }else std::cout<<"Original fixture has no eligible manual unit; changed-selection branch UNOBSERVED\n";
 client->select({});frame=client->capture(viewport,{});
 const auto* cities=frame->ui.find<ui::CityScreenFrame>();
 need(cities&&!cities->cities.empty(),"Actual owned city projection absent");
 const auto chosen=cities->cities.front();
 // Only actual Frame input issues membership; no private test issuer/accessor.
 const auto army=host->session().state().maintenance.players.find(host->active_player_id());
 need(army!=host->session().state().maintenance.players.end(),"Actual active unit input roster absent");
 if(!army->second.units.empty()){
  const auto full_id=army->second.units.begin()->first;
  need(full_id>=0&&full_id<=INT32_MAX,"Actual unit fullID outside native width");
  client->select({full_id,std::nullopt});
  auto unit_frame=client->capture(viewport,{});
  need(unit_frame->stamp.selection.unit==std::optional{full_id}&&
   unit_frame->stamp.selection.plot==std::optional<std::int64_t>{0},
   "Actual unit input lost fullID or changed independently picked plot");
  client->select({full_id,std::nullopt});
  need(client->capture(viewport,{})->stamp.identity.selection_revision==unit_frame->stamp.identity.selection_revision,
   "Repeated identical actual unit input changed membership revision");
  refuses([&]{client->select({std::int64_t{INT32_MAX}+1,std::nullopt});});
  if(!army->second.units.contains(INT32_MAX))refuses([&]{client->select({INT32_MAX,std::nullopt});});
  bool unit_foreign=false;
  std::thread unit_worker([&]{try{client->select({full_id,std::nullopt});}catch(const std::exception&){unit_foreign=true;}});
  unit_worker.join();need(unit_foreign,"Foreign thread changed actual unit membership");
  // The canonical input supports simultaneous city and unit values; do not
  // invent an exclusive-selection rule or a selection-plot equivalence.
  client->select({full_id,chosen.id});
  const auto combined=client->capture(viewport,{});
  need(combined->stamp.selection.unit==std::optional{full_id}&&combined->stamp.selection.city==std::optional{chosen.id},
   "Canonical combined city/unit input was made ambiguous by inference");
  client->select({});frame=client->capture(viewport,{});
 }else std::cout<<"Actual unit input positive UNOBSERVED (empty active roster)\n";
 std::cout<<"Private IsUnitSelected/Dim and dirty setter positive mutations UNOBSERVED; canonical input/flags consumer scenarios prepared\n";
 const auto unlocalized=ui::capture_city_screen_frame(host->session().state().cities,
  host->session().state().world,host->active_player_id(),std::nullopt,{});
 const auto raw_name=std::find_if(unlocalized.cities.begin(),unlocalized.cities.end(),
  [&](const auto& c){return c.owner==chosen.owner&&c.id==chosen.id;});
 need(raw_name!=unlocalized.cities.end(),"Actual unlocalized city identity absent");
 std::cout<<"Actual city name without localizer="<<(raw_name->name?"present":"absent")
  <<"; selected EN localized="<<(chosen.name?"present":"absent")<<'\n';
 need(chosen.name&&!chosen.name->empty()&&!chosen.name->starts_with("TXT_KEY_"),
  "Actual selected EN city name unresolved before route");
 const auto original_revision=frame->stamp.identity.selection_revision;
 auto ui_frame=live_ui->begin_frame(1024,768);
 need(client->dispatch(*live_ui,ui_frame,frame,ui::UiOpenScreen{"CityList"}).status==ui::UiCommandStatus::navigation,
  "Actual CityList route failed");
 frame=client->capture(viewport,{});ui_frame=live_ui->begin_frame(1024,768);
 const ui::UiCommand city_action(ui::UiGameplayCommand::make("CityList",
  ui::WorldViewAction{ui::WorldViewActionKind::open_city,chosen.id}));
 const auto selected_result=live_ui->dispatch(city_action,ui_frame);
 if(selected_result.status!=ui::UiCommandStatus::navigation)
  throw std::runtime_error("Actual current CityList city did not select: status="+
   std::to_string(static_cast<int>(selected_result.status))+" reason="+selected_result.reason);
 need(live_ui->selected_city()==std::optional{std::pair{chosen.owner,chosen.id}},
  "UI route canonical selection differs");
 need(!client->current_frame(),"Direct actual UI route retained stale published selection");
 const auto provider=client->ui_provider();refuses([&]{(void)provider();});
 bool provider_refused=false;
 try{(void)live_ui->begin_frame(1024,768);}catch(const std::exception& e){
  provider_refused=std::string_view(e.what())=="BNW client UI frame is unpublished or stale";
 }
 need(provider_refused,"Stale direct-selection UI began assembly before provider refusal");
 auto selected_frame=client->capture(viewport,{});
 need(selected_frame->stamp.selection.city==std::optional{chosen.id}&&
  selected_frame->stamp.identity.selection_revision>original_revision,"Frame copied stale independent selection");
 need(!client->is_current(frame),"Old frame survived actual selection revision");
 client->select({std::nullopt,chosen.id&8191});
 need(live_ui->selected_city()==std::optional{std::pair{chosen.owner,chosen.id}},
  "Native index alias did not resolve SAME full city");
 // Explicit client input updates the publication's selection and unpublishes
 // its frame, even when the canonical index alias selects the same full city.
 need(!client->current_frame(),"Explicit selection retained the old publication");
 refuses([&]{(void)provider();});
 const auto alias_frame=client->capture(viewport,{});
 need(alias_frame->stamp.selection.city==std::optional{chosen.id}&&
  alias_frame->stamp.identity.selection_revision==selected_frame->stamp.identity.selection_revision&&
  client->is_current(alias_frame),"Republished index alias changed canonical selection or stayed stale");
 ui_frame=live_ui->begin_frame(1024,768);
 need(live_ui->dispatch(ui::UiCloseScreen{"CityView"},ui_frame).status==ui::UiCommandStatus::navigation,
  "Actual CityView close failed");
 need(!client->current_frame(),"Direct Close retained stale selected publication");
 refuses([&]{(void)provider();});
 need(!live_ui->selected_city()&&!client->capture(viewport,{})->stamp.selection.city,
  "Actual Close did not clear shared selection");
 need(client->take_city_dirty_events().empty(),"No native Citizens action invented notifications");
 // Real source-ordered damage-zero resets preserve state_generation, so the
 // captured frame MUST stale through actual write metadata before any UI drain.
 frame=client->current_frame();ui_frame=live_ui->begin_frame(1024,768);
 const auto generation=host->session().state_generation();
 const auto source_revision=host->session().state().ui_source_revision;
 const auto prefix=host->session().begin_bnw_city_entry_damage_zero(chosen.owner,chosen.id);
 need(prefix&&prefix->progress().native_calls==3,"Actual native three-reset prefix absent");
 need(host->session().state_generation()==generation&&
  host->session().state().ui_source_revision>source_revision,"Actual partial stores did not invalidate below framebuffer");
 need(!client->is_current(frame)&&!client->current_frame()&&!live_ui->is_current_frame(ui_frame),
  "Captured frame stayed current before notifications were consumed");
 bool composed=false;need(!client->present(frame,[&](const auto&){composed=true;})&&!composed,
  "Stale frame reached presentation callback");
 need(client->dispatch(*live_ui,ui_frame,frame,ui::UiOpenScreen{"CityList"}).status==ui::UiCommandStatus::invalid,
  "Stale source frame dispatched UI input");
 frame=client->capture(viewport,{});need(client->is_current(frame),"Rebuilt actual source frame not current");
 need(frame->stamp.selection.plot==std::optional<std::int64_t>{0},
  "Authenticated same-world gameplay UI revision erased actual plot input");
 const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
 refuses([&]{(void)live_ui->selected_city();});refuses([&]{(void)client->capture(viewport,{});});
 refuses([&]{client->select_plot(0,frame);});
 refuses([&]{(void)client->unit_dirty_state();});refuses([&]{(void)live_ui->take_unit_dirty_events();});
 // The independent retained cursor also owns the genuine Host. Release it
 // before testing only the rebuilt client/UI lifetime below.
 cycle.reset();
 client.reset();live_ui.reset();
 client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
 live_ui=client->create_selected_ui_host(registry(),nullptr);
 std::weak_ptr<ui::SelectedBnwPreviewHost> weak=host;host.reset();
 need(!weak.expired()&&!live_ui->selected_city(),"Actual client failed to retain genuine parent");
 client.reset();refuses([&]{(void)live_ui->selected_city();});
 refuses([&]{(void)live_ui->unit_dirty_state();});live_ui.reset();
 need(weak.expired(),"Selection ownership formed a Host/session cycle");
 auto other=ui::bootstrap_selected_bnw_preview(save,game,63,true);
 if(other.preview.host){
  std::shared_ptr<ui::SelectedBnwPreviewHost> diagnostic(std::move(other.preview.host));
  need(diagnostic->active_player_id()!=runtime::detail::bnw_game_active_player_read(diagnostic->session().state()),
   "Prepared wrong-preview actor fixture unexpectedly uses actual Game player");
  refuses([&]{(void)ui::BnwClientFrameOwner::from_selected_host(diagnostic);});
 }else std::cout<<"Wrong-preview actor admission UNOBSERVED\n";
 need(runtime::identify_file(save).sha256==hash,"Original save changed");
 std::cout<<"Actual selection route/close/lifetime complete; changed Citizens events UNOBSERVED here\n";
}
}
int main(int argc,char** argv){try{need(argc==3,"Expected SAVE and canonical shared installation");
 arithmetic();actual(argv[1],argv[2]);std::cout<<"bnw selection/dirty focused source scenarios passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
