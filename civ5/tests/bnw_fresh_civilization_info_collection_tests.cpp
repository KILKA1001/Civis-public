#include "civ5/runtime/bnw_fresh_game.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <algorithm>
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}
struct Window{HWND h=CreateWindowExW(0,L"STATIC",L"Actual ordered Globals",WS_OVERLAPPEDWINDOW,0,0,841,621,
 nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(h)DestroyWindow(h);}};}
int main(int argc,char** argv){try{
 need(argc>=5,"Expected ACTUAL_BNW_SAVE ORIGINAL_INSTALL CANONICAL_ASSETS SELECTED_INI [product input args]");Window w;
 need(w.h&&!IsWindowVisible(w.h),"Actual hidden owned window missing");const auto hwnd=reinterpret_cast<std::uintptr_t>(w.h);
 auto early=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],hwnd,
  runtime::bnw_stock_lua_win32_client_extent(hwnd,"genuine-ordered-globals-component",[]{return std::uint64_t{1};}));
 auto packages=ui::BnwNativePackageDiscoveryOwner::create(early);packages->register_native_dx9_primary_prefix();
 auto boot=ui::BnwNativeResourceBootSettingsOwner::create(packages);auto input=boot->selected_language_input();
 const auto& catalog=early->catalog();std::vector<std::size_t> selected;
 for(std::size_t n=0;n<catalog.packages.size();++n){const auto& p=catalog.packages[n];
  const auto id=content::canonical_guid(p.id);if(p.kind==content::PackageKind::dlc&&
   (id=="6da07636-4123-4018-b643-6575b4ec336b"||id=="0e3751a1-f840-4e1b-9706-519bf484e59d"))selected.push_back(n);
 }
 need(selected.size()==2,"Actual G&K/BNW selection missing");auto rules=runtime::bootstrap_bnw_fresh_game_content_owner(argv[2],catalog,selected);
 need(rules->content_ready(),"Actual selected BNW rules failed activation");
 auto config=runtime::BnwCreationConfig::from_selected_rules(rules,
  {"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM","GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"},input);
 auto game=runtime::BnwFreshGame::from_creation_config(config);const auto before=game->observe();
 bool stopped=false;try{game->advance_ordered_globals_info_prefix(argv[4]);}catch(const std::exception& e){
  stopped=std::string_view(e.what()).find("next actual PlayerColors retained ColorsLookup")!=std::string_view::npos;
 }
 need(stopped,"Actual ordered cache did not reach genuine next source frontier");const auto prefix=game->ordered_globals_info_observation();
 need(prefix.completed_tables==std::vector<std::string>{"Colors"}&&prefix.mandatory_next=="PlayerColors"&&
  prefix.color_slots>0&&prefix.player_color_slots==0&&prefix.civilization_slots==0&&!prefix.global_cache_complete,
  "Ordered source skipped/forged global Info predecessor");
 const auto& order=prefix.source_order;const auto defines=std::find(order.begin(),order.end(),"actual LoadGlobalDefines return");
 const auto color=std::find(order.begin(),order.end(),"PrefetchCollection delete/clear Colors");
 const auto rgba=std::find(order.begin(),order.end(),"CvColorInfo actual RGBA CacheResults return");
 need(defines!=order.end()&&color!=order.end()&&rgba!=order.end()&&defines<color&&color<rgba,
  "Actual global definitions/base/derived Color cache order differs");
 std::uint64_t append=0;for(const auto& effect:order)if(effect=="append actual Info Colors")++append;
 const auto actual=rules->rules->prefetch_info("Colors",{},content::InfoCollectionBudget{65536});
 need(actual.all_succeeded()&&append==actual.rows&&prefix.color_slots==actual.slots,
  "Actual owned Color constructor/CacheResults/append span differs from source collection");
 const auto after=game->observe();need(after.boundary==runtime::BnwFreshGameBoundary::ordered_globals_player_colors_required&&
  after.cache_resets==before.cache_resets&&after.map_random==before.map_random&&after.gameplay_random==before.gameplay_random,
  "Ordered source reset Game/cache/RNG");
 bool replay=false;try{game->advance_ordered_globals_info_prefix(argv[4]);}catch(const runtime::BnwFreshGameRefusal& e){
  replay=e.code()==runtime::BnwFreshGameError::wrong_phase;
 }
 need(replay&&game->ordered_globals_info_observation().source_order==order,"Repeated source call replayed actual Effects");
 bool menu=false;try{game->advance_single_player_pregame_menu_prefix(argv[4]);}catch(const runtime::BnwFreshGameRefusal& e){
  menu=e.code()==runtime::BnwFreshGameError::wrong_phase;
 }
 need(menu,"Menu bypassed incomplete genuine ordered global cache");
 std::cout<<"Genuine ordered full typed definitions and Colors slots="<<prefix.color_slots
  <<" next=PlayerColors CivilizationInfo=unissued GameSpeedInfo=unissued full_game=0 can_continue=0\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
