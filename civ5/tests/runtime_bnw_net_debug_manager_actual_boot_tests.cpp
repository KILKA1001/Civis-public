#include "civ5/runtime/bnw_net_message_debug_manager_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <fstream>
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
 HWND hwnd=CreateWindowExW(0,L"STATIC",L"Civis actual manager Boot fixture",WS_OVERLAPPEDWINDOW,
  0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 ~Window(){if(hwnd)DestroyWindow(hwnd);}
};
std::string actual_bytes(const std::filesystem::path& path){
 if(!std::filesystem::exists(path))return {};
 need(std::filesystem::file_size(path)<=16*1024*1024,"Actual fixture log exceeds independent read budget");
 std::ifstream in(path,std::ios::binary);need(bool(in),"Actual fixture stream cannot be independently read");
 std::string bytes;char block[4096];while(in.read(block,sizeof(block))||in.gcount())bytes.append(block,static_cast<std::size_t>(in.gcount()));
 need(in.eof()&&!in.bad(),"Actual independent log read failed");return bytes;
}
}
int main(int argc,char** argv){try{
 need(argc>=4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS plus real selected language/settings arguments");
 Window window;need(window.hwnd&&!IsWindowVisible(window.hwnd),"Actual hidden creator HWND unavailable");
 std::uint64_t revision=1;std::size_t client_queries=0;
 auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.hwnd),
  "actual-net-debug-manager-boot-fixture",[&]{++client_queries;return revision;});
 auto lease=ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],reinterpret_cast<std::uintptr_t>(window.hwnd),client);
 auto package=ui::BnwNativePackageDiscoveryOwner::create(lease);package->register_native_dx9_primary_prefix();
 auto boot=ui::BnwNativeResourceBootSettingsOwner::create(package);
 auto config=boot->selected_net_debug_config();const auto config_facts=config->source_facts();
 auto manager=boot->selected_net_debug_manager();auto retained=boot->selected_net_debug_manager();
 need(manager.get()==retained.get()&&!manager.owner_before(retained)&&!retained.owner_before(manager),"Actual Boot cloned/reissued manager");
 const auto before=manager->source_facts();
 need(before.own_manager_constructed&&!before.original_manager_state_proven&&!before.original_path_and_format_parity_proven,"Own diagnostic backend invented original native manager/parity");
 const auto operand=config_facts.selected_civis_config?config_facts.selected_civis_config->operand:config_facts.original_profile.operand;
 need(before.operand==operand&&before.config_lane==config_facts.selected_kind,"Actual selected config lane/operand changed");
 need(before.enabled==(operand!=runtime::BnwNetDebugConfigOperand::explicit_zero),"Missing config did not preserve actual fresh default01");
 wchar_t local[MAX_PATH]{};need(SHGetSpecialFolderPathW(nullptr,local,CSIDL_LOCAL_APPDATA,FALSE)!=FALSE,"Actual independent LocalAppData source unavailable");
 const auto expected=std::filesystem::absolute(std::filesystem::path(local)/L"Civis"/L"Logs"/std::to_wstring(GetCurrentProcessId())/L"net_message_debug.log").lexically_normal();
 need(before.actual_own_log_path==expected&&before.registrations==0&&before.written_bytes==0,"Actual fresh manager path/registry/write state differs");
 const bool existed=std::filesystem::exists(expected);const auto initial=actual_bytes(expected);
 auto first=manager->write_bootstrap_diagnostic();auto second=manager->write_bootstrap_diagnostic();const auto after=manager->source_facts();
 if(before.enabled){
  const std::string message="Civis net-message logger bootstrap\n";
  need(first==runtime::BnwNetDebugOwnDelivery::actual_file_write_and_flush&&second==first,"Actual enabled sink did not return both writes/flushes");
  need(after.registrations==1&&after.write_calls==2&&after.flush_calls==2&&after.written_bytes==message.size()*2,"Actual sink registered/logged/flushed wrong count");
  need(actual_bytes(expected)==initial+message+message,"Independent OS file bytes do not match both actual appends");
 }else {
  need(first==runtime::BnwNetDebugOwnDelivery::actual_disabled_return&&second==first,"Actual disabled branch entered enabled producer");
  need(after.registrations==0&&after.write_calls==0&&after.written_bytes==0&&after.flush_calls==0&&std::filesystem::exists(expected)==existed&&actual_bytes(expected)==initial,"Actual disabled return touched stream/registry/file");
 }
 const auto before_foreign=client_queries;bool foreign_manager=false,foreign_boot=false;
 std::thread foreign([&]{try{manager->write_bootstrap_diagnostic();}catch(const std::exception&){foreign_manager=true;}try{(void)boot->selected_net_debug_manager();}catch(const std::exception&){foreign_boot=true;}});foreign.join();
 need(foreign_manager&&foreign_boot&&client_queries==before_foreign,"Foreign creator borrowed actual registry/Boot/provider");
 manager->require_current();need(manager->source_facts().written_bytes==after.written_bytes,"Foreign refusal still wrote bytes");
 package.reset();lease->require_current();refuses([&]{manager->require_current();});refuses([&]{manager->write_bootstrap_diagnostic();});refuses([&]{(void)boot->selected_net_debug_manager();});
 std::cout<<"PASS actual hidden HWND/EarlyInput/primary/Boot/SAME manager, source/default01, OS registered file bytes+flush or selected disabled return, creator/parent expiry; original format parity=0 game clock advanced=0\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
