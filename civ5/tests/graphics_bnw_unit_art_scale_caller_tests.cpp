#include "civ5/graphics/bnw_unit_art_scale_caller.hpp"
#include "civ5/graphics/bnw_unit_art_scale_source_fields.hpp"
#include "civ5/graphics/bnw_unit_vis_ini_source_serializer.hpp"
#include "civ5/ui/bnw_native_user_directory_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <bit>
#include <thread>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class F>void refuse(F f,unsigned& n){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Scale owner accepted missing/foreign source");++n;}
static_assert(!std::is_invocable_v<decltype(&graphics::BnwUnitArtScaleCaller::create),std::filesystem::path,float,unsigned>);
int main(int argc,char** argv){try{
 check(argc==4,"Usage: ScaleCaller SAVE GAME_ROOT SHARED_ROOT");unsigned negative=0;
 using graphics::unit_art_scale_detail::read_user_scale_fields;
 auto defaults=read_user_scale_fields("");check(defaults.scale==1.f&&defaults.multiplier==1.f&&defaults.scale_defaulted&&defaults.multiplier_defaulted,"Native empty-INI getterdefaults/percent order");
 auto declared=read_user_scale_fields("[Debugging]\nGlobalUnitScale=200\nGlobalUnitMemberMultiplier=100\n");check(declared.scale==2.f&&!declared.scale_defaulted&&declared.multiplier==1.f&&!declared.multiplier_defaulted,"Controlled declared source fields");
 auto partial=read_user_scale_fields("[Debugging]\nGlobalUnitScale=50\n");check(partial.scale==.5f&&partial.multiplier_defaulted,"Source field-level default");
 refuse([]{read_user_scale_fields("[Debugging]\nGlobalUnitScale=0\n");},negative);refuse([]{read_user_scale_fields("[Debugging]\nGlobalUnitScale=NaN\n");},negative);refuse([]{read_user_scale_fields("[Debugging]\nGlobalUnitScale=100\nGlobalUnitScale=200\n");},negative);refuse([]{read_user_scale_fields("[Debugging]\nGlobalUnitScale=100x\n");},negative);refuse([]{read_user_scale_fields("[Debugging]\nbroken\n");},negative);
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(bool(selected.preview),"Actual selected BNW host unavailable");auto host=std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(selected.preview.host));
 unsigned tableRows=0,warriorRows=0;auto table=host->database().query_rows({"SELECT count(*) AS n FROM ArtDefine_UnitMemberInfos",{}},[&](const content::RulesRowView& r){tableRows=unsigned(r.integer("n"));},1);check(table.all_succeeded(),"Actual unit-art engine SQL table absent");
 auto rows=host->database().query_rows({"SELECT Type,Scale,ZOffset,Model FROM ArtDefine_UnitMemberInfos WHERE Type IN ('ART_DEF_UNIT_MEMBER_WARRIOR','ART_DEF_UNIT_MEMBER_WARRIOR_V2','ART_DEF_UNIT_MEMBER_WARRIOR_V4')",{}},[&](const content::RulesRowView& r){++warriorRows;std::cout<<"ACTUAL_ART_ROW "<<*r.text("Type")<<" scale="<<r.real("Scale")<<" model="<<r.text("Model").value_or("<NULL>")<<'\n';},3);check(rows.all_succeeded(),"Actual unit-art selected row query failed");std::cout<<"ACTUAL_ART_TABLE rows="<<tableRows<<" selected_warriors="<<warriorRows<<std::endl;
 struct Window{HWND value=nullptr;~Window(){if(value)DestroyWindow(value);}} window;window.value=CreateWindowExW(0,L"STATIC",L"",WS_OVERLAPPEDWINDOW,0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);check(window.value&&!IsWindowVisible(window.value),"Actual hidden CPU client HWND unavailable");std::uint64_t revision=1;auto client=runtime::bnw_stock_lua_win32_client_extent(reinterpret_cast<std::uintptr_t>(window.value),"actual-unit-scale-caller",[&]{return revision;});auto colors=ui::BnwNativeUiColorOwner::create(host,reinterpret_cast<std::uintptr_t>(window.value),client);auto catalog=content::discover_packages({std::filesystem::path(argv[2])/"Assets/DLC"});check(catalog.diagnostics.empty(),"Original package catalog unavailable");auto packages=ui::BnwNativePackageDiscoveryOwner::create(argv[2],argv[3],catalog,selected.profile,colors);auto directory=ui::BnwNativeUserDirectoryOwner::create(packages);directory->require_same_resources(*packages);packages->require_same_selected_host(*host);packages->register_native_dx9_primary_prefix();auto boot=ui::BnwNativeResourceBootSettingsOwner::create(packages);if(!boot->current().adapter_caps_source_closed)boot->capture_native_dx9_adapter_source();packages->register_native_dx9_texture_quality_branch(boot);if(boot->current().effective_texture_quality&&*boot->current().effective_texture_quality!=0)packages->register_native_common_high_quality_prefix();else std::cout<<"PRODUCTION_SCALE_BLOCKED actual low Common source predecessor unavailable\n";
 refuse([&]{graphics::BnwUnitArtScaleCaller::create({},directory,host);},negative);refuse([&]{graphics::BnwUnitArtScaleCaller::create(packages,{},host);},negative);refuse([&]{graphics::BnwUnitArtScaleCaller::create(packages,directory,{});},negative);
 const auto ini=packages->resources().resolve(assets::AssetKey("Assets/Units/UnitVisConfig.ini"));const auto xml=packages->resources().resolve(assets::AssetKey("Assets/Units/Civ5ArtDefines_UnitMembers.xml"));const bool sources=std::holds_alternative<assets::AssetHandle>(ini.asset)&&std::holds_alternative<assets::AssetHandle>(xml.asset);
 if(!sources){refuse([&]{graphics::BnwUnitArtScaleCaller::create(packages,directory,host);},negative);std::cout<<"PRODUCTION_SCALE_BLOCKED initial common INI/member XML registration not owned by EARLY VFS stage\n";}
 else {
  auto caller=graphics::BnwUnitArtScaleCaller::create(packages,directory,host);check(!caller->current().caller_fields_owned,"Factory invented boot caller mode");refuse([&]{caller->compile_member("ART_DEF_UNIT_MEMBER_WARRIOR");},negative);caller->apply_reload_unit_system_scale_stage();const auto before=caller->current();check(before.caller_fields_owned&&before.publish_global&&before.current_global_scale==(before.source_transition_applied?before.caller_scale:before.initial_asset_global_scale)&&before.caller_configuration.source_path==directory->current()/L"UnitVisConfig.ini","Real normal source command did not publish retained caller fields");
  if(before.caller_configuration.source_ini_flush_requests!=0){refuse([&]{caller->compile_member("ART_DEF_UNIT_MEMBER_WARRIOR");},negative);std::cout<<"PRODUCTION_SCALE_BLOCKED source user INI writeback transition pending\n";} else if(warriorRows!=3){refuse([&]{caller->compile_member("ART_DEF_UNIT_MEMBER_WARRIOR");},negative);std::cout<<"PRODUCTION_SCALE_BLOCKED actual unit-art SQL compilation producer missing\n";}
  else {
   auto quote=caller->compile_member("ART_DEF_UNIT_MEMBER_WARRIOR");caller->validate_quote(quote);check(quote.placement_after_resource_conversion().scale==1.f,"Resource placement must not reapply source compiled scale");caller->reload_unit_libraries();refuse([&]{quote.compiled_rules();},negative);auto next=caller->compile_member("ART_DEF_UNIT_MEMBER_WARRIOR");caller->validate_quote(next);std::cout<<"PRODUCTION_COMPILED_SCALE source_quote="<<next.compiled_rules().scale<<" model="<<next.model()<<'\n';
  }
  auto current=caller->current();if(current.source_transition_applied){caller->reload_unit_libraries();check(caller->current().caller_scale==current.caller_scale&&caller->current().current_global_scale==current.current_global_scale,"Source reload reread/replaced frozen caller fields");}else {refuse([&]{caller->reload_unit_libraries();},negative);check(current.caller_configuration.pending_effects.size()==4&&current.caller_configuration.pending_effects[0].target==directory->current()/L"UnitVisConfig.ini"&&current.caller_configuration.pending_effects[1].serialized_bytes==graphics::unit_art_scale_detail::missing_unit_vis_scale_ini(2),"Pure pending source write plan lost native path/bytes");}++revision;refuse([&]{caller->current();},negative);
 }
 std::cout<<"PASS controlled numeric fields + actual retained host/art-row/VFS inventory; "<<negative<<" refusals; no arbitrary path/flags/float input or drawable claim\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}


