#include "civ5/runtime/bnw_net_message_debug_config_owner.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <thread>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
using namespace civ5;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& fn){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}need(rejected,"Expected actual source/lifetime refusal");}
struct Window {
 HWND hwnd=CreateWindowExW(0,L"STATIC",L"Civis actual net config Boot fixture",WS_OVERLAPPEDWINDOW,
  0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 ~Window(){if(hwnd)DestroyWindow(hwnd);}
};
bool same_identity(const runtime::FileIdentity& a,const runtime::FileIdentity& b){return a.bytes==b.bytes&&a.sha256==b.sha256;}
}
int main(int argc,char** argv){try{
 // --ui-language TYPE / --civis-settings FILE are real process inputs, consumed
 // by production Boot. This fixture never writes a profile or injects a getter.
 need(argc>=4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS plus optional real product settings/language selection");
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Actual hidden creator HWND unavailable");
 std::uint64_t revision=1;std::size_t client_queries=0;
 auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.hwnd),
  "actual-net-debug-config-boot-fixture",[&]{++client_queries;return revision;});
 auto lease=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],reinterpret_cast<std::uintptr_t>(window.hwnd),client);
 auto package=ui::BnwNativePackageDiscoveryOwner::create(lease);
 package->register_native_dx9_primary_prefix();
 auto boot=ui::BnwNativeResourceBootSettingsOwner::create(package);
 auto selected=boot->selected_language_input();selected->require_current();auto selected_source=selected->source();
 auto owner=boot->selected_net_debug_config();owner->require_current();auto facts=owner->source_facts();
 auto retained=boot->selected_net_debug_config();
 need(owner.get()==retained.get()&&!owner.owner_before(retained)&&!retained.owner_before(owner),"Boot reissued/cloned actual config owner");
 need(facts.manager_bootstrap_and_registered_log_list_required,"Config observation invented manager/list completion");
 // Independently obtain the actual Shell profile path. Missing file is just an
 // unavailable input, never a manufactured native getter/manager disabled ACK.
 wchar_t personal[MAX_PATH]{};need(SHGetSpecialFolderPathW(nullptr,personal,CSIDL_PERSONAL,FALSE)!=FALSE,"Actual independent personal-directory query unavailable");
 auto original=std::filesystem::absolute(std::filesystem::weakly_canonical(std::filesystem::path(personal)/L"My Games"/L"Sid Meier's Civilization 5")/L"config.ini").lexically_normal();
 need(facts.original_profile.logical_path==original&&facts.original_profile.selected_path==std::filesystem::weakly_canonical(original),"Original profile was replaced by selected Civis override");
 std::error_code error;auto original_status=std::filesystem::symlink_status(original,error);
 need(!error||error==std::errc::no_such_file_or_directory,"Independent original profile status unavailable");
 if(original_status.type()==std::filesystem::file_type::not_found){
  need(!facts.original_profile.file&&facts.original_profile.operand==runtime::BnwNetDebugConfigOperand::unavailable_or_key_missing,"Missing original became a zero/disabled result");
 }else{
  need(facts.original_profile.file&&same_identity(*facts.original_profile.file,runtime::identify_file(facts.original_profile.selected_path)),"Original config file identity differs from actual bytes");
 }
 if(selected_source.kind==runtime::BnwSelectedLanguageSourceKind::selected_ini){
  need(facts.selected_civis_config&&facts.selected_civis_config->file&&same_identity(*facts.selected_civis_config->file,selected_source.file),"Selected file facts are not the actual SAME input");
  const auto actual=std::filesystem::canonical(selected_source.config_ini);
  need(facts.selected_civis_config->selected_path==actual&&same_identity(*facts.selected_civis_config->file,runtime::identify_file(actual)),"Actual selected input path/bytes changed");
  need(facts.selected_kind==(actual==facts.original_profile.selected_path?runtime::BnwNetDebugSelectedConfigKind::same_original_profile:runtime::BnwNetDebugSelectedConfigKind::civis_selected_override),"Original and selected Civis lanes conflated");
 }else need(!facts.selected_civis_config&&facts.selected_kind==runtime::BnwNetDebugSelectedConfigKind::process_language_without_config,"Language-only CLI invented a settings file");
 const auto before_foreign=client_queries;bool foreign_owner=false,foreign_boot=false;
 std::thread foreign([&]{try{(void)owner->source_facts();}catch(const std::exception&){foreign_owner=true;}try{(void)boot->selected_net_debug_config();}catch(const std::exception&){foreign_boot=true;}});foreign.join();
 need(foreign_owner&&foreign_boot&&client_queries==before_foreign,"Foreign creator borrowed source/first-issue anchor or touched actual HWND provider");
 owner->require_current();selected->require_current();
 std::cout<<"ACTUAL_CONFIG original_exists="<<facts.original_profile.file.has_value()
  <<" original_operand="<<static_cast<int>(facts.original_profile.operand)
  <<" selected_kind="<<static_cast<int>(facts.selected_kind)
  <<" selected_exists="<<(facts.selected_civis_config&&facts.selected_civis_config->file)
  <<" manager_bootstrap_required=1 native_getter_executed=0 sink_delivered=0\n";
 // Actual Package lifetime ends while the physical input lease and Boot facade
 // remain alive. Config currentness must refuse; no guessed profile recovery.
 package.reset();lease->require_current();refuses([&]{owner->require_current();});
 refuses([&]{(void)boot->selected_net_debug_config();});
 std::cout<<"PASS actual hidden HWND/EarlyInput/primary/Boot/SAME config source, once issuer, creator and parent expiry; manager/sink/game clock remain mandatory\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
