#include "civ5/runtime/bnw_fresh_game.hpp"
#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void frontier(F&& f,runtime::BnwFreshGameBoundary boundary,
 const std::shared_ptr<runtime::BnwFreshGame>& game){
 bool caught=false;try{f();}catch(const runtime::BnwFreshGameRefusal& e){
  caught=e.code()==runtime::BnwFreshGameError::native_rules_unavailable;
 }
 need(caught&&game->observe().boundary==boundary,"Expected genuine mandatory original-order frontier");
}
template<class F> void refused(F&& f,runtime::BnwFreshGameError code){
 bool caught=false;try{f();}catch(const runtime::BnwFreshGameRefusal& e){caught=e.code()==code;}
 need(caught,"Wrong Game refusal/source phase");
}
struct Window {
 HWND hwnd=CreateWindowExW(0,L"STATIC",L"Fresh Pregame actual registry component",WS_OVERLAPPEDWINDOW,
  0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 ~Window(){if(hwnd)DestroyWindow(hwnd);}
};
// Authored downstream assertions only: NOT executed until an actual whole
// ordered Globals return and SAME Game menu continuation exist. No readiness
// flag, supplied alternate Pregame realm or target-only Info cache may call it.
[[maybe_unused]] void assert_reached_actual_last_civilization_component(
 const std::shared_ptr<runtime::BnwFreshGame>& game,
 const std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner>& options,
 const std::filesystem::path& selected_ini){
 const auto original_settings=options->source_receipt();
 auto copied=*game;
 frontier([&]{game->advance_single_player_pregame_menu_prefix(selected_ini);},
  runtime::BnwFreshGameBoundary::pregame_menu_options_required,game);
 frontier([&]{game->advance_single_player_pregame_menu_options_prefix();},
  runtime::BnwFreshGameBoundary::pregame_settings_source_required,game);
 need(options->pregame_input_observation().empty(),"Menu reset prematurely read host LastCiv");
 const auto before=game->observe();
 refused([&]{copied.advance_pregame_settings_last_civilization_prefix(options);},runtime::BnwFreshGameError::parent_retired);
 bool foreign_refusal=false;std::thread foreign([&]{try{game->advance_pregame_settings_last_civilization_prefix(options);}
  catch(const runtime::BnwFreshGameRefusal& e){foreign_refusal=e.code()==runtime::BnwFreshGameError::wrong_thread;}});foreign.join();
 need(foreign_refusal&&options->pregame_input_observation().empty(),"Foreign thread reached host registry read");
 bool stopped=false;try{game->advance_pregame_settings_last_civilization_prefix(options);}
 catch(const runtime::BnwFreshGameRefusal& e){stopped=e.code()==runtime::BnwFreshGameError::native_rules_unavailable;}
 need(stopped,"Incomplete host loader reported success");
 const auto registered=options->pregame_input_observation();need(registered.size()==1,"First reached key not registered exactly once");
 const auto& slot=registered.front();need(slot.section=="UserSettings"&&slot.key=="LastCiv","Wrong native first setting key");
 if(slot.default_inserted)need(slot.value==-1,"Genuine missing-key signed default is not -1");
 const auto after=game->observe();const auto boundary=slot.value<0?
  runtime::BnwFreshGameBoundary::pregame_last_speed_required:runtime::BnwFreshGameBoundary::pregame_civilization_info_required;
 need(after.boundary==boundary&&std::get<std::int32_t>(after.scalars.at("LoadPreGameSettings.LastCiv"))==slot.value,
  "Actual saved/default signed value did not choose native short circuit");
 need(after.map_random==before.map_random&&after.gameplay_random==before.gameplay_random&&after.cache_resets==before.cache_resets,
  "Settings prefix replayed Game reset/RNG/cache");
 const auto registry_file=options->source_receipt();
 need(original_settings.selected_path==registry_file.selected_path&&original_settings.file.has_value()==registry_file.file.has_value(),
  "Actual original settings path/presence changed");
 if(original_settings.file)need(original_settings.file->bytes==registry_file.file->bytes&&original_settings.file->sha256==registry_file.file->sha256,
  "Actual original UserSettings was overwritten");
 refused([&]{game->advance_pregame_settings_last_civilization_prefix(options);},runtime::BnwFreshGameError::wrong_phase);
 refused([&]{game->advance_reset_prefix();},runtime::BnwFreshGameError::native_rules_unavailable);
 need(game->observe().order==after.order&&options->pregame_input_observation().size()==1,"Refused loader/reset replay mutated source order");
 need(options->pregame_input_observation().size()==1,"Actual LastCiv assertions did not reach one registry key");
}

}
int main(int argc,char** argv){try{
 need(argc>=5,"Expected ACTUAL_BNW_SAVE ORIGINAL_INSTALL CANONICAL_ASSETS SELECTED_INI [genuine product input args]");
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Own hidden current-process HWND absent");
 const auto hwnd=reinterpret_cast<std::uintptr_t>(window.hwnd);
 auto early=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],hwnd,
  runtime::bnw_stock_lua_win32_client_extent(hwnd,"newgame-registry-component",[]{return std::uint64_t{1};}));
 // Existing genuine imported-save resource factory is only the Boot fixture
 // predecessor of this component. No new-game early-input issuer is claimed.
 auto packages=ui::BnwNativePackageDiscoveryOwner::create(early);
 packages->register_native_dx9_primary_prefix();
 auto boot=ui::BnwNativeResourceBootSettingsOwner::create(packages);
 auto options=boot->selected_user_options();auto input=boot->selected_language_input();
 need(options->pregame_input_observation().empty(),"Boot prematurely registered unissued Pregame LastCiv");
 const auto original_settings=options->source_receipt();
 const auto& catalog=early->catalog();std::vector<std::size_t> dlc;
 for(std::size_t n=0;n<catalog.packages.size();++n){const auto& p=catalog.packages[n];
  if(p.kind==content::PackageKind::dlc){const auto id=content::canonical_guid(p.id);
   if(id=="6da07636-4123-4018-b643-6575b4ec336b"||id=="0e3751a1-f840-4e1b-9706-519bf484e59d")dlc.push_back(n);
  }
 }
 need(dlc.size()==2,"Real G&K and BNW selection missing");
 auto content=runtime::bootstrap_bnw_fresh_game_content_owner(argv[2],catalog,dlc);
 need(content->content_ready(),"Actual selected BNW rules failed activation");
 auto config=runtime::BnwCreationConfig::from_selected_rules(content,
  {"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM","GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"},input);
 auto game=runtime::BnwFreshGame::from_creation_config(config);
 const auto before=game->observe();
 refused([&]{game->advance_single_player_pregame_menu_prefix(argv[4]);},runtime::BnwFreshGameError::native_rules_unavailable);
 need(game->observe().order==before.order&&options->pregame_input_observation().empty(),
  "Pristine Boot Game bypassed whole ordered Globals or mutated menu/settings before it");
 bool stopped=false;try{game->advance_ordered_globals_info_prefix(argv[4]);}catch(const std::exception& e){
  stopped=std::string_view(e.what()).find("next actual PlayerColors retained ColorsLookup")!=std::string_view::npos;
 }
 const auto actual=game->ordered_globals_info_observation();
 need(stopped&&!actual.global_cache_complete&&actual.completed_tables==std::vector<std::string>{"Colors"}&&
  actual.mandatory_next=="PlayerColors"&&game->observe().boundary==runtime::BnwFreshGameBoundary::ordered_globals_player_colors_required,
  "Actual settings prerequisite did not stop on genuine ordered Globals PlayerColors frontier");
 refused([&]{game->advance_single_player_pregame_menu_prefix(argv[4]);},runtime::BnwFreshGameError::wrong_phase);
 refused([&]{game->advance_pregame_settings_last_civilization_prefix(options);},runtime::BnwFreshGameError::wrong_phase);
 need(options->pregame_input_observation().empty(),"Unissued actual LastCiv read escaped mandatory Globals predecessor");
 const auto registry_file=options->source_receipt();
 need(original_settings.selected_path==registry_file.selected_path&&original_settings.file.has_value()==registry_file.file.has_value(),
  "Original settings path/presence changed before loader");
 if(original_settings.file)need(original_settings.file->bytes==registry_file.file->bytes&&original_settings.file->sha256==registry_file.file->sha256,
  "Original UserSettings was overwritten before genuine loader");
 const auto after=game->observe();need(after.map_random==before.map_random&&after.gameplay_random==before.gameplay_random&&
  after.cache_resets==before.cache_resets,"Unissued loader/reset changed Game RNG/cache");
 std::cout<<"Actual Boot settings mandatory prerequisite=PlayerColors LastCiv=unissued downstream_assertions=dormant"
  <<" full_loader=0 newgame_issuer=0 full_game=0 can_continue=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
