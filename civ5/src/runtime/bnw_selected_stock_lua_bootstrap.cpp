#include "civ5/ui/civis_input_policy_owner.hpp"
#include "civ5/runtime/bnw_civis_owned_stock_source_bundle.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/top_panel.hpp"
#include "civ5/runtime/bnw_selected_unitflag_context_owner.hpp"
#include <cmath>
#include <limits>
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include "civ5/graphics/bnw_unit_should_dim_transport.hpp"
#include "civ5/ui/bnw_tech_award_input_dispatch.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/ui/bnw_worldview_renderer_projection_loan.hpp"
#include "civ5/runtime/bnw_unitflag_health_instance_loan.hpp"
#include "civ5/runtime/bnw_worldview_script_ui_adapter.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/runtime/bnw_major_tech_transport_consumer.hpp"
#include "civ5/ui/bnw_tech_award_context_owner.hpp"
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include "civ5/runtime/bnw_owned_team_set_has_tech_lua_dispatch.hpp"
#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/lua_assets.hpp"
#include "civ5/content/mod_script_registry.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/ui/selected_bnw_civilopedia_preamble.hpp"
#include "civ5/ui/bnw_civilopedia_right_tree_owner.hpp"
#include "civ5/ui/bnw_civilopedia_native_runtime_owner.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "../ui/bnw_native_completed_color_query.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <climits>
#include <array>
#include <algorithm>
#include <fstream>
#include <set>
#include <type_traits>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cerrno>
#include <exception>
#include <utility>
namespace civ5::runtime {
namespace {
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::string source(const std::filesystem::path &path,std::string *sha=nullptr){
 const auto before=identify_file(path);need(before.bytes<=64ULL*1024*1024,"Stock Lua source exceeds budget");std::ifstream input(path,std::ios::binary);need(static_cast<bool>(input),"Stock Lua source unavailable");std::string text((std::istreambuf_iterator<char>(input)),{});const auto actual=identify_bytes(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));need(before.sha256==actual.sha256,"Stock Lua source changed during read");if(sha)*sha=actual.sha256;return text;
}
const content::XmlNode *first_context(const content::XmlNode &node){if(node.name=="LuaContext")return &node;for(const auto &child:node.children)if(const auto *found=first_context(child))return found;return nullptr;}
content::Catalog mirrored_catalog(const std::filesystem::path &root,const content::Catalog &catalog,const SavedContentProfile &selected){
 auto mirrored=catalog;const auto shared=content::discover_packages({root/"Assets/DLC"});need(shared.diagnostics.empty(),"Shared asset package discovery is incomplete");std::set<std::size_t> seen;
 for(const auto index:selected.enabled_dlc){need(index<catalog.packages.size()&&seen.insert(index).second,"Selected stock DLC identity unavailable");const auto &original=catalog.packages[index];need(original.kind==content::PackageKind::dlc,"Selected stock package is not DLC");const content::PackageManifest *found=nullptr;for(const auto &candidate:shared.packages)if(candidate.kind==original.kind&&content::canonical_guid(candidate.id)==content::canonical_guid(original.id)&&candidate.version==original.version){need(!found,"Shared selected DLC manifest is ambiguous");found=&candidate;}need(found,"Shared selected DLC manifest is absent");// The selected installation descriptor is rules metadata. Shared local assets may have VP audio edits; preserve the authoritative document and remap its payload root only.
mirrored.packages[index].path=found->path;}
 return mirrored;
}
}
struct BnwSelectedStockLuaBootstrap::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 // Published weakly only after the actual SAME-VM localization receiver bind.
 std::weak_ptr<ui::BnwEngineUiTurnOwner> end_turn_engine;
 // The genuine native cache initializer and actual SAME-VM OnDimEvent
 // registration must install this owner. No source declaration enables it.
 std::shared_ptr<graphics::BnwUnitShouldDimQueue> should_dim_queue;
 BnwStockLuaBootstrapProgress progress;
 std::filesystem::path root;
 std::shared_ptr<const BnwCivisOwnedStockSourceBundle> civis_sources;
 content::Catalog catalog;
 SavedContentProfile selected;
 SessionCheckpointIdentity identity;
 std::shared_ptr<const LiveSessionState> world;
 std::shared_ptr<LiveSessionState> diagnostic_world;
 std::unique_ptr<content::RulesDatabase> database;
 content::InfoTypeCatalog types;
 std::unique_ptr<content::LooseResourcePlan> mounts;
 std::unique_ptr<content::ModScriptRegistry> mods;
 struct ModdingEntryPointQuery {std::weak_ptr<ScriptQuery> query;const content::ModScriptRegistry*registry;};
 std::vector<ModdingEntryPointQuery> modding_entrypoint_queries;
 std::unique_ptr<LuaScriptHost> host;
 ScriptMapProvider live_map;
 std::shared_ptr<content::RulesDatabase> retained_saved_database;
 std::string parent,prefix,controls;
 struct TechAwardSource {std::filesystem::path physical;assets::AssetHandle handle;std::string bytes,sha;};
 std::vector<TechAwardSource> tech_award_sources;
 bool tech_award_activation_attempted=false,tech_award_activated=false;
 void require_tech_award_sources_current()const{
  // The actual include lender accounts its own provisional failure sentinel;
  // ordinary callers still see the conservative SAME VM failure stamp.
  if(operation)operation->require_active();vm_current_provider_bookend();
  need(tech_award_sources.size()==7,"TECH_AWARD selected source closure absent");
  need(selected_source_identity(root/"Assets/DLC/Expansion2/UI/InGame/InGame.xml").sha256==progress.declaration_sha256,
       "TECH_AWARD selected InGame declaration retired");
  for(const auto& s:tech_award_sources){
   need(selected_source_identity(s.physical).sha256==s.sha&&identify_bytes(s.handle.read_bytes(64ULL*1024*1024)).sha256==s.sha,
        "TECH_AWARD selected VFS/source identity changed");
  }
  if(operation)operation->require_active();vm_current_provider_bookend();
 }
 void prepare_tech_award_sources(){
  healthy();need(operation&&!diagnostic_world&&mounts&&host&&progress.first_prefix_executed,
      "TECH_AWARD requires genuine retained executed Bootstrap/VM");
  const auto declaration_path=root/"Assets/DLC/Expansion2/UI/InGame/InGame.xml";
  need(selected_source_identity(declaration_path).sha256==progress.declaration_sha256,"TECH_AWARD InGame identity differs");
  const auto declaration=selected_source_xml(declaration_path);
  const auto* context=named_xml(declaration,"LuaContext","ID","TechAwardPopup");
  need(context&&context->attribute("FileName")=="Assets/UI/InGame/Popups/TechAwardPopup"&&context->attribute("Hidden")=="True",
       "TECH_AWARD actual selected context declaration absent");
  // No new VM, fake Controls, replacement script, or detached resolver.
  struct SourceSpec {const char* key;const char* selected_path;const char* sha;};
  // This exact known stock closure is the implemented receiver slice. A
  // prelaunch replacement OnPopup cannot return without native QueuePopup
  // and be counted as delivered merely because the file stayed current.
  const std::array<SourceSpec,7> paths{{
   {"Assets/UI/InGame/Popups/TechAwardPopup.lua","Assets/UI/InGame/Popups/TechAwardPopup.lua","55aef8cc53575b82882a81298f57a3cc07b3fbc50fe644f30e2feda6c65e8f5a"},
   {"Assets/UI/InGame/Popups/TechAwardPopup.xml","Assets/DLC/Expansion2/UI/InGame/Popups/TechAwardPopup.xml","f71128bad9a3d38bc331430c98a111a0864029fb6a05cb701ca30a622e713bac"},
   {"Assets/UI/InGame/TechTree/TechButtonInclude.lua","Assets/DLC/Expansion2/UI/InGame/TechTree/TechButtonInclude.lua","03e5be256a72fa4c80b14a4f6b046452370651cf3002093ee1d7f76f798fd86a"},
   {"Assets/UI/InGame/InfoTooltipInclude.lua","Assets/DLC/Expansion2/UI/InGame/InfoTooltipInclude.lua","405a3d677fff50acdc1155a97e713185219ca9fc2ceb61db6d298b1fc5382fa4"},
   {"Assets/UI/InstanceManager.lua","Assets/UI/InstanceManager.lua","b5439abfd50c170f411972e9c9b60e8ecd7be8a13851cfc28c7c02d7bb781a1e"},
   {"Assets/UI/IconSupport.lua","Assets/UI/IconSupport.lua","c9815b6fb6ee5d9fbba1a523b56437e8d4fa798ab8633f9cfc7d9783be5434b4"},
   {"Assets/Gameplay/Lua/FLuaVector.lua","Assets/Gameplay/Lua/FLuaVector.lua","e9170f61fc4bb070ee8d1617c547f2cb56b47937db00ccd8b184ad2a15bfb2f6"}
  }};
  for(const auto& spec:paths){
   const auto* key=spec.key;const auto* selected_path=spec.selected_path;
   const auto resolved=mounts->resources.resolve(assets::AssetKey(key));const auto* handle=std::get_if<assets::AssetHandle>(&resolved.asset);
   need(handle,"TECH_AWARD actual selected VFS handle unavailable");
   std::string sha;const auto bytes=selected_source_text(root/selected_path,&sha);
   need(sha==spec.sha,"TECH_AWARD selected original script/XML slice differs");
   if(const auto found=includes.find(key);found!=includes.end())
    need(found->second==bytes,"TECH_AWARD existing original include bytes differ");
   need(identify_bytes(handle->read_bytes(64ULL*1024*1024)).sha256==sha,"TECH_AWARD actual selected VFS winner differs");
   tech_award_sources.push_back({root/selected_path,*handle,bytes,sha});
   if(std::string_view(key).ends_with("Include.lua")){
    need(!includes.contains(key),"TECH_AWARD include source already substituted");
    includes.emplace(key,bytes);bounded_include_names.emplace_back(key);
   }
  }
  require_tech_award_sources_current();
 }

 std::vector<std::string> bounded_include_names;
 std::map<std::string,std::string> includes;
 BnwStockLuaClientExtentProvider client;
 std::optional<BnwStockLuaClientExtent> screen_snapshot;
 std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> gui_screen;
 std::shared_ptr<BnwSelectedWorldViewContextOwner> worldview_sources;
 std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5> worldview_trees;
 std::shared_ptr<ui::CivisInputPolicyOwner> worldview_input_policy;
 std::shared_ptr<ScriptUiControlGraph> action_info_graph;
 std::shared_ptr<ScriptUiControlGraph> worldview_graph;
 std::shared_ptr<BnwSelectedUnitFlagContextOwner> unitflag_native;
 bool worldview_activation_attempted=false;
 const BnwWorldViewSourceOperationLoan* worldview_operation=nullptr;
 std::vector<std::int64_t> worldview_mouse_events; // filled only by the selected native enum issuer

 std::unique_ptr<ui::BnwCivilopediaScrollBarOwner> scrollbar,resize_transaction;
 std::shared_ptr<ui::BnwCivilopediaRightTreeOwner> right_tree;
 std::shared_ptr<ui::BnwCivilopediaNativeWindowBinding> native_window;
 std::shared_ptr<ui::BnwCivilopediaNativeRuntimeOwner> native_runtime;
 std::optional<assets::ResourceMountTable> native_resources;
 bool native_attachment_attempted=false;
 bool color_attachment_attempted=false;
 std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery> completed_color;
 std::weak_ptr<const ui::BnwNativePackageDiscoveryOwner> completed_color_parent;
 std::uint64_t staged_panel_queries=0,staged_resizes=0;
 std::int64_t staged_panel_width=0,staged_panel_height=0,staged_bar_width=0,staged_bar_height=0;
 bool sibling_transaction=false;
 std::vector<BnwStockScrollOperation> expected_sibling_operations;
 std::size_t staged_sibling_mutations=0;
 std::uint64_t staged_left_panel_queries=0;
 std::int64_t staged_left_width=0,staged_left_height=0;
 bool attempted=false;
 std::optional<BnwStockLuaAttemptLease> operation;
 bool uncertain=false;
 std::exception_ptr retained_error;
 const LuaScriptHost* host_owner=nullptr;
 const void* host_vm=nullptr;
 std::uint64_t host_epoch=0;
 void fail()noexcept {
  if(std::this_thread::get_id()==thread){uncertain=true;if(!retained_error)retained_error=std::current_exception();}
  if(operation)operation->invalidate();
 }
 ~Impl(){
  // Final teardown only: error paths retain host/coroutine/native scratch.
  if(operation)operation->invalidate();
  host.reset(); // All callback captures and guards are still alive here.
 }
 FileIdentity selected_source_identity(const std::filesystem::path& path)const{
  return civis_sources?civis_sources->source_identity(path):identify_file(path);
 }
 content::XmlNode selected_source_xml(const std::filesystem::path& path)const{
  return civis_sources?civis_sources->source_xml(path):content::read_xml(path);
 }
 std::string selected_source_text(const std::filesystem::path& path,std::string* sha=nullptr)const{
  if(!civis_sources)return source(path,sha);
  const auto key=path.lexically_normal().lexically_relative(root.lexically_normal());
  if(key.empty()||key.is_absolute())throw ScriptError("Own stock source root changed");
  for(const auto& part:key)if(part=="..")throw ScriptError("Own stock source path escapes bank");
  return civis_sources->text(assets::path_utf8(key),sha);
 }
 void healthy()const {
  need(std::this_thread::get_id()==thread,"Stock VM foreign thread");
  if(operation)operation->require_active();
  vm_current();
 }
 void vm_current()const {
  need(std::this_thread::get_id()==thread,"Stock VM foreign thread");
  need(!uncertain,"Stock operation has retained uncertainty; no replay");
  if(civis_sources)civis_sources->require_current();
  if(host){
   const auto stamp=host->stock_operation_stamp();
   need(host.get()==host_owner&&stamp.vm==host_vm&&!stamp.failed&&stamp.failure_epoch==host_epoch,
        "Stock VM/failure identity changed; no replay");
  }
 }
 void vm_current_provider_bookend()const {
  need(std::this_thread::get_id()==thread,"Stock VM foreign provider thread");
  need(!uncertain,"Stock provider has retained uncertainty; no replay");
  if(civis_sources)civis_sources->require_current();
  if(host){
   std::optional<LuaScriptHost::StockOperationStamp> native;
   if(worldview_operation){
    need(static_cast<bool>(worldview_sources),"WorldView source member retired during native provider");
    const auto& actual=worldview_sources->current_operation_loan();
    need(&actual==worldview_operation,"WorldView native provider scope changed");
    native=host->stock_worldview_provider_stamp(actual);
   }
   const auto lent=native?native:host->stock_include_provider_stamp();
   const auto stamp=lent?*lent:host->stock_operation_stamp();
   need(host.get()==host_owner&&stamp.vm==host_vm&&!stamp.failed&&stamp.failure_epoch==host_epoch,
        "Stock actual include provider/VM failure identity changed; no replay");
  }
 }
 void execute(std::string_view context,std::string_view text,std::string_view name) {
  if(operation)operation->require_active();
  const auto before=host->stock_operation_stamp();
  need(!before.failed,"Stock VM was already failed");
  host->execute(context,text,name);
  const auto after=host->stock_operation_stamp();
  need(after.vm==before.vm&&!after.failed&&after.failure_epoch==before.failure_epoch,
       "Stock VM callback failed or caught reentry");
  if(operation)operation->require_active();
 }
 template<class R,class... A>std::function<R(A...)> protect(std::function<R(A...)> actual){
  if(!actual||!operation)return actual;
  const auto lease=*operation;
  return [this,actual=std::move(actual),lease](A... args)->R {
   try {
    const auto serial=lease.operation_serial();
    if constexpr(std::is_void_v<R>){actual(std::forward<A>(args)...);lease.require_operation(serial);}
    else {auto result=actual(std::forward<A>(args)...);lease.require_operation(serial);return result;}
   }catch(...){fail();throw;}
  };
 }
#include "bnw_stock_player_roster_owned.inc"
 struct OwnedSavedConnection final:ScriptSaveDataConnection {
  Impl* owner;
  std::unique_ptr<ScriptSaveDataConnection> actual;
  BnwStockLuaAttemptLease lease;
  OwnedSavedConnection(Impl* o,std::unique_ptr<ScriptSaveDataConnection> c,BnwStockLuaAttemptLease l)
      :owner(o),actual(std::move(c)),lease(std::move(l)){}
  std::unique_ptr<ScriptQuery> query(std::string_view sql,std::span<const ScriptValue> args)override;
 };
 struct OwnedQuery final:ScriptQuery {
  Impl* owner;
  std::unique_ptr<ScriptQuery> actual;
  BnwStockLuaAttemptLease lease;
  OwnedQuery(Impl* o,std::unique_ptr<ScriptQuery> q,BnwStockLuaAttemptLease l)
      :owner(o),actual(std::move(q)),lease(std::move(l)){}
  template<class F>auto run(F&& f)const->std::invoke_result_t<F> {
   try {
    const auto serial=lease.operation_serial();
    if constexpr(std::is_void_v<std::invoke_result_t<F>>){std::forward<F>(f)();lease.require_operation(serial);}
    else {auto r=std::forward<F>(f)();lease.require_operation(serial);return r;}
   }catch(...){owner->fail();throw;}
  }
  std::optional<ScriptRow> next()override{return run([&]{return actual->next();});}
  bool step()override{return run([&]{return actual->step();});}
  ScriptRow row()const override{return run([&]{return actual->row();});}
  std::pair<bool,ScriptValue> cache_key(bool id)const override{return run([&]{return actual->cache_key(id);});}
  void reset()override{run([&]{actual->reset();});}
  void bind(std::span<const ScriptValue> v)override{run([&]{actual->bind(v);});}
  void release()override{run([&]{actual->release();});}
  std::string_view error()const override{return run([&]{return actual->error();});}
 };
 Impl(std::filesystem::path assets,const content::Catalog &input_catalog,const SavedContentProfile &profile,const SessionCheckpointIdentity &id,std::shared_ptr<const LiveSessionState> state_lease,const content::RulesDatabase &rules,ScriptMapProvider map,std::shared_ptr<LiveSessionState> diagnostic,std::optional<BnwStockLuaAttemptLease> lease={},std::shared_ptr<content::RulesDatabase> saved_database={},std::shared_ptr<const BnwCivisOwnedStockSourceBundle> own_sources={}):root(std::move(assets)),civis_sources(std::move(own_sources)),selected(profile),identity(id),world(std::move(state_lease)),diagnostic_world(std::move(diagnostic)),live_map(std::move(map)),retained_saved_database(std::move(saved_database)),operation(std::move(lease)){
  try{
   if(operation){operation->require_active();need(operation->world_lease().get()==world.get(),"Stock constructor world is not SAME loan allocation");}
   need(static_cast<bool>(world),"Stock Lua actual retained world is absent");
   const auto& state=*world;
   if(civis_sources){civis_sources->require_current();need(root.lexically_normal()==civis_sources->recipe().canonical_payload_root.lexically_normal(),"Own stock root differs from captured bank");}
   else {root=std::filesystem::canonical(root);
    need(std::filesystem::is_directory(root/"Assets"),"Stock Lua bootstrap explicit shared source root has no Assets directory");}
   need(selected.can_activate&&selected.all_requirements_located&&selected.enabled_mods.empty()&&state.setup.profile==content::TechnologyProfile::bnw&&identity.payload_sha256.size()==64&&identity.rules_sha256.size()==64,"Stock Lua bootstrap selected BNW identity unavailable");
   need(state.world.pregame&&state.world.pregame->layout==saves::GameStateLayout::expansion2_version1,"Stock Lua bootstrap decoded pregame unavailable");const auto scenario=state.world.pregame->archive.flags.find("load_wb_scenario");need(scenario!=state.world.pregame->archive.flags.end(),"Stock Lua bootstrap native WB scenario flag is unknown");progress.load_wb_scenario=scenario->second;need(!scenario->second,"Stock Lua bootstrap requires a selected scenario context loader for load_wb_scenario=true");
   need(identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,"Stock Lua bootstrap rules fingerprint differs");
   if(operation)unit_rules_current=rules.immutable_preview_fingerprint_currentness();
   catalog=civis_sources?civis_sources->recipe().catalog:mirrored_catalog(root,input_catalog,selected);
   bool expansion2=false;for(const auto i:selected.enabled_dlc)if(content::canonical_guid(catalog.packages[i].id)=="6da07636-4123-4018-b643-6575b4ec336b")expansion2=true;need(expansion2,"Stock Lua bootstrap selected Expansion2 unavailable");
   if(civis_sources){
    const auto& memory=civis_sources->resources();
    mounts=std::make_unique<content::LooseResourcePlan>(content::LooseResourcePlan{
      memory,root,static_cast<std::uint64_t>(memory.keys().size())});
    civis_sources->require_current();
   }else{
   mounts=std::make_unique<content::LooseResourcePlan>(content::build_selected_loose_resource_plan(root,selected.enabled_dlc,catalog));content::mount_base_game_lua(mounts->resources,root);
   // The rules mount deliberately excludes UI. Add only this verified base
   // include closure as a fallback; this is not full UISkin activation.
   auto base_ui=std::make_shared<assets::LooseResourceIndex>(root);base_ui->add_file("Assets/UI/InGame/UnitFlagManager.xml");base_ui->add_file("Assets/DLC/Expansion2/UI/InGame/UnitFlagManager.lua");base_ui->add_file("Assets/UI/InstanceManager.lua");base_ui->add_file("Assets/UI/IconSupport.lua");base_ui->add_file("Assets/UI/Styles.xml");
   for(const auto key:{"Assets/UI/InGame/Popups/TechAwardPopup.lua","Assets/UI/InGame/Popups/TechAwardPopup.xml","Assets/UI/InGame/TechTree/TechButtonInclude.lua","Assets/UI/InGame/InfoTooltipInclude.lua"})base_ui->add_file(key);for(const auto font:{"TwCenMT16","TwCenMT18","TwCenMT20"})base_ui->add_file(std::string("Assets/UI/Fonts/Tw Cent MT/")+font+".ggxml");std::uint64_t mount_id=1;for(const auto &mount:mounts->resources.mounts())mount_id=std::max(mount_id,mount.id+1);mounts->resources.mount({mount_id,0,0,assets::resource_lookup(base_ui,false),true,false,assets::resource_lookup_all(base_ui,false),assets::resource_keys(base_ui)});
   }
   mods=std::make_unique<content::ModScriptRegistry>(catalog,selected,std::span<const content::ModRegistration>{});need(mods->planned_entry_points("InGameUIAddin").empty()&&mods->entry_points("InGameUIAddin").empty(),"Stock zero-mod entrypoint registry differs");
   const auto declaration=root/"Assets/DLC/Expansion2/UI/InGame/InGame.xml";progress.declaration_sha256=selected_source_identity(declaration).sha256;const auto document=selected_source_xml(declaration);const auto *first=first_context(document);need(document.name=="Context"&&first&&first->attribute("ID")=="CivilopediaScreen"&&first->attribute("FileName")=="Assets/UI/Civilopedia/CivilopediaScreen","Stock first context declaration differs");progress.context="CivilopediaScreen";
   parent=selected_source_text(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",&progress.parent_sha256);
   need(progress.parent_sha256=="2ee2d25004202db0f68d26cd5ae61e171bcaaeb8e15c49780d90590a1be2ce2a","Stock parent source revision unsupported by continuation boundary");
   const std::vector<std::string> names{"Assets/UI/InstanceManager.lua","Assets/UI/IconSupport.lua","Assets/Gameplay/Lua/FLuaVector.lua"};
   for(const auto &name:names){const auto text=selected_source_text(root/name);const auto resolved=mounts->resources.resolve(assets::AssetKey(name));const auto *handle=std::get_if<assets::AssetHandle>(&resolved.asset);need(handle,"Stock selected include VFS resolution unavailable");const auto bytes=handle->read_bytes(64ULL*1024*1024);need(identify_bytes(bytes).sha256==selected_source_identity(root/name).sha256,"Stock selected include VFS payload differs");includes.emplace(name,text);bounded_include_names.push_back(name);}
   const auto xml_path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";progress.xml_sha256=selected_source_identity(xml_path).sha256;const auto layout=selected_source_xml(xml_path);need(selected_source_identity(xml_path).sha256==progress.xml_sha256,"Stock selected XML changed during read");const auto managers=ui::inspect_civilopedia_manager_block(parent,layout,includes.at(names[0]));need(managers.verified&&managers.bindings.size()==33,"Stock Civilopedia manager prefix is unsupported");progress.manager_bindings=managers.bindings.size();
   constexpr std::string_view boundary="local _, screenSizeY = UIManager:GetScreenSizeVal();";const auto at=parent.find(boundary);need(at!=std::string::npos&&parent.find(boundary,at+1)==std::string::npos&&parent.find("__civis_stock_parent_continuation")==std::string::npos,"Stock continuation source boundary differs");
   constexpr std::string_view next="local scrollBarWidth = Controls.ScrollBar:GetSize().x;";const auto next_at=parent.find(next);need(next_at>at&&next_at!=std::string::npos&&parent.find(next,next_at+1)==std::string::npos,"Stock next native GetSize frontier differs");constexpr std::string_view panel_boundary="local scrollPanelSize = Controls.ScrollPanel:GetSize();";const auto panel_at=parent.find(panel_boundary);need(panel_at>next_at&&panel_at!=std::string::npos&&parent.find(panel_boundary,panel_at+1)==std::string::npos,"Stock next ScrollPanel frontier differs");constexpr std::string_view left_boundary="Controls.LeftScrollBar:SetSize( scrollBarSize );";const auto left_at=parent.find(left_boundary);need(left_at>panel_at&&left_at!=std::string::npos&&parent.find(left_boundary,left_at+1)==std::string::npos,"Stock next left ScrollBar resize frontier differs");constexpr std::string_view calculate_boundary="Controls.ScrollPanel:CalculateInternalSize();";const auto calculate_at=parent.find(calculate_boundary,left_at);need(calculate_at>left_at&&calculate_at!=std::string::npos,"Stock initial CalculateInternalSize frontier differs");constexpr std::string_view setter_boundary="Controls.ScrollBar:SetSize( scrollBarSize );";const auto setter_at=parent.find(setter_boundary);need(setter_at>panel_at&&setter_at<left_at&&parent.find(setter_boundary,setter_at+1)==std::string::npos,"Stock first mandatory native slider setter boundary differs");auto staged_parent=parent;staged_parent.insert(calculate_at,"\ncoroutine.yield('Controls.ScrollPanel.CalculateInternalSize',scrollPanelSize.x,scrollPanelSize.y,leftScrollPanelSize.x,leftScrollPanelSize.y,scrollBarSize.x,scrollBarSize.y)\n");staged_parent.insert(left_at,"\ncoroutine.yield('Controls.LeftScrollBar.SetSize',scrollPanelSize.x,scrollPanelSize.y,scrollBarSize.x,scrollBarSize.y)\n");staged_parent.insert(setter_at,"\ncoroutine.yield('Controls.ScrollBar.SetSize',scrollPanelSize.x,scrollPanelSize.y,scrollBarSize.x,scrollBarSize.y)\n");staged_parent.insert(panel_at,"\ncoroutine.yield('Controls.ScrollPanel.GetSize',scrollBarWidth)\n");staged_parent.insert(next_at,"\ncoroutine.yield('Controls.ScrollBar.GetSize',screenSizeY)\n");staged_parent.insert(at,"\ncoroutine.yield('UIManager.GetScreenSizeVal')\n");
   prefix="__civis_stock_parent_continuation = coroutine.create(function()\n"+staged_parent+"\nend)\nlocal ok,frontier=coroutine.resume(__civis_stock_parent_continuation);assert(ok,frontier);assert(frontier=='UIManager.GetScreenSizeVal')\n";
   controls="do local bindings={";std::set<std::string> seen;for(const auto &binding:managers.bindings)if(seen.insert(binding.parent_control).second)controls+=binding.parent_control+"={id='"+binding.parent_control+"',kind='"+binding.parent_kind+"'},";controls+="};for id,binding in pairs(bindings) do assert(Controls[id]==nil);Controls[id]=binding end end";
   database=rules.clone_gameplay();types=database->cache_info_types();need(types.report().all_succeeded(),"Stock Lua bootstrap InfoType catalog unavailable");progress.selected_sources_verified=true;progress.pending_api="UIManager.GetScreenSizeVal";progress.blocker="Selected parent prefix is staged; native UI owner and remaining selected context activation are required";
  }catch(const std::exception &error){fail();progress.blocker=error.what();}
 }
 // Issued only by the private actual-world constructor. Diagnostic copies
 // never acquire these providers or the selected context-source channel.
 std::function<bool()> unit_rules_current;
 std::optional<content::GlobalDefineCache> unit_defines;
 std::shared_ptr<const ScriptUnitReadProvider> unit_reads;
 struct UnitFlagSources;
 std::shared_ptr<UnitFlagSources> unitflag_sources;
 const std::vector<std::string> unitflag_events{
  "SerialEventUnitCreated","LocalMachineUnitPositionChanged","UnitActionChanged",
  "UnitGarrison","UnitEmbark","SerialEventUnitTeleportedToHex","UnitMoveQueueChanged",
  "SerialEventCityCreated","UnitVisibilityChanged","SerialEventUnitDestroyed",
  "UnitSelectionChanged","SerialEventUnitSetDamage","UnitNameChanged","HexFOWStateChanged",
  "MultiplayerGamePlayerUpdated","UnitStateChangeDetected","UnitShouldDimFlag",
  "UnitMarkThreatening","StrategicViewStateChanged","SerialEventCityDestroyed",
  "SerialEventCityCaptured","RunCombatSim","EndCombatSim"};
 static std::int32_t unit_native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Unit native int range differs");return static_cast<std::int32_t>(n);}
 static std::int32_t unit_scalar(const saves::UnitCoreObservation& core,std::string_view name){
  const saves::UnitCoreField* found=nullptr;
  for(const auto& field:core.fields)if(field.name&&std::string_view(field.name)==name){need(!found,"Unit duplicate raw scalar");found=&field;}
  need(found&&found->encoding==saves::UnitCoreEncoding::i32&&found->values.size()==1,"Unit raw scalar schema differs");
  return unit_native(found->values[0]);
 }
 static void transport_node(const saves::UnitTailNode& node,std::string_view name,const saves::UnitTailNode*& found){
  if(node.name&&std::string_view(node.name)==name){need(!found,"Unit duplicate transport scalar");found=&node;}
  for(const auto& child:node.children)transport_node(child,name,found);
 }
 static std::int32_t transport_scalar(const saves::SavedUnitObservation& unit,std::string_view name){
  need(unit.tail.layout==saves::GameStateLayout::expansion2_version1,"Unit transport layout differs");
  const saves::UnitTailNode* found=nullptr;for(const auto& block:unit.tail.blocks)transport_node(block,name,found);
  need(found&&found->encoding&&std::string_view(found->encoding)=="i32"&&found->values.size()==1,"Unit transport raw scalar absent");
  return unit_native(found->values[0]);
 }
 std::int32_t unit_max_hit_points()const{
  need(unit_defines&&unit_defines->profile()==content::GlobalDefineProfile::bnw,"Unit SAME selected GC cache absent");
  const auto* entry=unit_defines->find("MAX_HIT_POINTS");
  need(entry&&entry->kind==content::GlobalDefineKind::integer&&entry->lookup.found&&entry->lookup.error.empty()&&
       !entry->retained_previous&&std::holds_alternative<std::int64_t>(entry->lookup.value),"Unit genuine MAX_HIT_POINTS cache unavailable");
  const auto value=unit_native(std::get<std::int64_t>(entry->lookup.value));need(value>0,"Unit MAX_HIT_POINTS invalid");return value;
 }
 void unit_require_current()const{
  if(operation)operation->require_active();vm_current_provider_bookend();need(operation&&operation->world_lease().get()==world.get()&&!diagnostic_world,"Unit read requires actual SAME world loan");
  need(unit_rules_current&&unit_rules_current(),"Unit selected immutable rules owner retired");
  if(operation)operation->require_active();vm_current_provider_bookend();
  need(world->setup.profile==content::TechnologyProfile::bnw&&world->world.all_players&&
       world->world.all_players->layout==saves::GameStateLayout::expansion2_version1&&
       world->world.all_players->players.size()==64,"Unit actual BNW player roster differs");
  (void)unit_max_hit_points();
 }
 static constexpr std::int32_t unit_index_mask=8191,unit_serial_stride=8192;
 // Structural validation derived from SDK Load/GetAt and the existing roster
 // checkpoint validator. It creates no issuer: only the private actual-world
 // constructor supplies this provider. Wire record order is not slot order.
 std::map<std::int32_t,const saves::SavedUnitObservation*> unit_slot_roster(std::int32_t owner)const{
  unit_require_current();need(owner>=0&&owner<64,"Unit native roster player unavailable");
  const auto& player=world->world.all_players->players.at(owner);const auto& units=player.units;const auto& h=units.header;
  need(player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner&&
       units.layout==saves::GameStateLayout::expansion2_version1&&h.layout==units.layout&&h.fields.size()==5&&
       h.wire_unit_count>=0&&static_cast<std::uint64_t>(h.wire_unit_count)==units.units.size(),"Unit native header/owner/count differs");
  const std::array<std::string_view,5> names{"num_slots","last_index","free_list_head","free_list_count","current_id"};
  for(std::size_t i=0;i<names.size();++i)need(h.fields[i].name&&std::string_view(h.fields[i].name)==names[i],"Unit native header schema differs");
  const auto slots=unit_native(h.fields[0].value),last=unit_native(h.fields[1].value),head=unit_native(h.fields[2].value);
  const auto count=unit_native(h.fields[3].value),serial=unit_native(h.fields[4].value);
  // Exact retained power-of-two storage only; malformed/noncanonical header
  // shapes remain unavailable rather than yielding false native nil.
  need(slots>=0&&slots<=unit_serial_stride&&(slots==0||(slots&(slots-1))==0)&&
       h.source_slots==static_cast<std::uint64_t>(slots)&&h.next_free_indices.size()==h.source_slots&&
       last>=-1&&last<slots&&count>=0&&count<=last+1&&serial>=unit_serial_stride&&serial%unit_serial_stride==0&&
       ((count==0&&head==-1)||(count>0&&head>=0&&head<=last)),"Unit native free-list shape unavailable");
  std::map<std::int32_t,const saves::SavedUnitObservation*> occupied;
  for(std::size_t i=0;i<units.units.size();++i){
   const auto& unit=units.units[i];const auto id=unit_scalar(unit.core,"m_iID");const auto slot=id&unit_index_mask;
   need(unit.index==i&&unit.core.layout==units.layout&&id>=0&&slot<=last&&
        (id&~unit_index_mask)<serial&&h.next_free_indices.at(slot)==-1&&
        unit_scalar(unit.core,"m_eOwner")==owner&&occupied.emplace(slot,&unit).second,"Unit native occupied slot/generation differs");
   const auto& owned=world->maintenance.players.at(owner).units.at(id);
   need(owned.source_core==std::optional{std::pair{unit.core.start_offset,unit.core.end_offset}},"Unit native slot raw/owned full-ID provenance differs");
  }
  std::set<std::int32_t> freed;auto next=head;
  for(std::int32_t i=0;i<count;++i){
   need(next>=0&&next<=last&&!occupied.contains(next)&&freed.insert(next).second,"Unit native free-list chain differs");
   next=unit_native(h.next_free_indices.at(next));
  }
  need(next==-1&&occupied.size()+freed.size()==static_cast<std::size_t>(last+1),"Unit native occupied/free partition differs");
  return occupied;
 }
 const saves::SavedUnitObservation* unit_lookup(std::int32_t owner,std::int32_t requested_id)const{
  unit_require_current();if(owner<0||owner>=64)return nullptr;
  const auto occupied=unit_slot_roster(owner);
  if(requested_id==-1)return nullptr; // Native FFreeList::INVALID_INDEX.
  const auto found=occupied.find(requested_id&unit_index_mask);
  if(found==occupied.end())return nullptr;
  const auto actual_id=unit_scalar(found->second->core,"m_iID");
  // Masking is used only to resolve the native slot. The issued reference and
  // g_MasterList key always retain the canonical complete ID from actual core.
  if((requested_id&~unit_index_mask)==0||actual_id==requested_id)return found->second;
  return nullptr;
 }
 struct UnitRosterQuote final:ScriptUnitRosterQuote{
  Impl* owner;std::int32_t player;std::uint64_t serial,revision;
  std::vector<std::int64_t> full_ids,header_values,next_free;
  std::vector<const saves::SavedUnitObservation*> actual;
  std::vector<std::pair<std::uint64_t,std::uint64_t>> source_core;
  UnitRosterQuote(Impl* o,std::int32_t p):owner(o),player(p),serial(o->operation->operation_serial()),revision(o->world->ui_source_revision){
   const auto occupied=o->unit_slot_roster(p);need(occupied.size()<=65536,"Unit native quote row budget exceeded");
   const auto& h=o->world->world.all_players->players.at(p).units.header;
   for(const auto& field:h.fields)header_values.push_back(field.value);next_free=h.next_free_indices;
   for(const auto& [slot,unit]:occupied){(void)slot;full_ids.push_back(unit_scalar(unit->core,"m_iID"));actual.push_back(unit);source_core.emplace_back(unit->core.start_offset,unit->core.end_offset);}
   require_current(player);
  }
  void require_current(std::int64_t state)const override{
   owner->unit_require_current();need(state==player,"Unit iterator actual player state changed");
   owner->operation->require_operation(serial);need(owner->world->ui_source_revision==revision,"Unit iterator actual world revision changed");
   const auto occupied=owner->unit_slot_roster(player);const auto& h=owner->world->world.all_players->players.at(player).units.header;
   need(occupied.size()==full_ids.size()&&h.fields.size()==header_values.size()&&h.next_free_indices==next_free,"Unit iterator native roster/header changed");
   for(std::size_t i=0;i<header_values.size();++i)need(h.fields[i].value==header_values[i],"Unit iterator native free-list header changed");
   std::size_t i=0;for(const auto& [slot,unit]:occupied){(void)slot;need(unit==actual[i]&&unit_scalar(unit->core,"m_iID")==full_ids[i]&&
    std::pair{unit->core.start_offset,unit->core.end_offset}==source_core[i],"Unit iterator actual occupied allocation changed");++i;}
   owner->operation->require_operation(serial);
  }
  const std::vector<std::int64_t>& ids()const override{require_current(player);return full_ids;}
 };

#include "bnw_unitflag_unitinfo_reads.inc"
 struct UnitReference final:ScriptUnitReadReference{
  Impl* owner;std::int32_t player,id;const saves::SavedUnitObservation* actual;
  std::pair<std::uint64_t,std::uint64_t> source_core;
  UnitReference(Impl* o,std::int32_t p,std::int32_t i,const saves::SavedUnitObservation* u)
   :owner(o),player(p),id(i),actual(u),source_core(u->core.start_offset,u->core.end_offset){}
  void require_current()const override{
   owner->unit_require_current();const auto* current=owner->unit_lookup(player,id);
   need(current==actual&&current&&std::pair{current->core.start_offset,current->core.end_offset}==source_core,
        "Unit retained full-ID reference allocation retired");
  }
  ScriptValue read(std::string_view method)const override{
   require_current();const auto serial=owner->operation->operation_serial();ScriptValue value;
   if(method=="GetID")value=std::int64_t{id};
   else if(method=="GetOwner")value=std::int64_t{player};
   else if(method=="GetX")value=std::int64_t{unit_scalar(actual->core,"m_iX")};
   else if(method=="GetY")value=std::int64_t{unit_scalar(actual->core,"m_iY")};
   else if(method=="MovesLeft")value=std::int64_t{std::max(std::int32_t{0},unit_scalar(actual->core,"m_iMoves"))};
   else if(method=="GetDamage")value=std::int64_t{unit_scalar(actual->core,"m_iDamage")};
#include "bnw_unit_health_read.inc"
#include "bnw_unitflag_constructor_predicates.inc"
#include "bnw_unitflag_constructor_unitinfo_methods.inc"
   else if(method=="GetMaxHitPoints")value=std::int64_t{owner->unit_max_hit_points()};
   else if(method=="IsDead")value=unit_scalar(actual->core,"m_iDamage")>=owner->unit_max_hit_points();
   else if(method=="IsCargo"){
    const auto transport_owner=transport_scalar(*actual,"m_transportUnit.eOwner");
    const auto transport_id=transport_scalar(*actual,"m_transportUnit.iID");
    value=owner->unit_lookup(transport_owner,transport_id)!=nullptr;
   }else throw ScriptError("Unit method needs another genuine provider");
   require_current();owner->operation->require_operation(serial);return value;
  }
 };
 struct UnitReads final:ScriptUnitReadProvider{
  Impl* owner;explicit UnitReads(Impl* o):owner(o){}
  void require_current()const override{owner->unit_require_current();}
  std::shared_ptr<const ScriptUnitReadReference> find(std::int64_t p,std::int64_t id)const override{
   require_current();const auto serial=owner->operation->operation_serial();
   const auto player=unit_native(p),full_id=unit_native(id);const auto* unit=owner->unit_lookup(player,full_id);
   std::shared_ptr<const ScriptUnitReadReference> result;
   if(unit)result=std::make_shared<UnitReference>(owner,player,unit_scalar(unit->core,"m_iID"),unit);
   require_current();owner->operation->require_operation(serial);return result;
  }
  std::shared_ptr<const ScriptUnitRosterQuote> capture_roster(std::int64_t p)const override{
   require_current();const auto serial=owner->operation->operation_serial();const auto player=unit_native(p);
   need(player>=0&&player<64,"Unit iterator player unavailable");
   auto result=std::make_shared<UnitRosterQuote>(owner,player);
   require_current();owner->operation->require_operation(serial);return result;
  }
 };
 static const content::XmlNode* named_xml(const content::XmlNode& root,std::string_view kind,std::string_view key,std::string_view value){
  const content::XmlNode* found=nullptr;
  std::function<void(const content::XmlNode&)> visit=[&](const auto& node){
   if(node.name==kind&&node.attribute(std::string(key))==value){need(!found,"Selected UnitFlag XML membership ambiguous");found=&node;}
   for(const auto& child:node.children)visit(child);
  };visit(root);return found;
 }
 struct UnitFlagChild final:ScriptUiChildSource{
  Impl* owner;assets::AssetHandle handle;assets::Bytes bytes;std::string sha;
  UnitFlagChild(Impl* o,assets::AssetHandle h):owner(o),handle(std::move(h)),bytes(handle.read_bytes(64ULL*1024*1024)),sha(identify_bytes(bytes).sha256){}
  std::string_view context()const override{return "UnitFlagManager";}
  const assets::AssetHandle& asset()const override{return handle;}
  void require_current()const override{
   owner->unit_require_current();const auto serial=owner->operation->operation_serial();
   need(sha=="d2bfe09c86633e29df346c4cd2f2feacf1cdb5c747798fb34b07eb279a84572f"&&
        identify_bytes(handle.read_bytes(64ULL*1024*1024)).sha256==sha,"UnitFlag SAME selected Lua source retired");
   owner->operation->require_operation(serial);
  }
  void require_captured_bytes(std::span<const std::uint8_t> captured)const override{
   require_current();need(identify_bytes(captured).sha256==sha&&captured.size()==bytes.size(),"UnitFlag captured source substituted");require_current();
  }
 };
 struct UnitFlagSources final:ScriptUiContextSourceOwner{
  Impl* owner;std::shared_ptr<const UnitFlagChild> child;
  assets::AssetHandle xml_handle;assets::Bytes xml_bytes;
  std::string declaration_sha,xml_sha,declaration_text;
  content::XmlNode declaration,layout;
  UnitFlagSources(Impl* o,assets::AssetHandle lua,assets::AssetHandle xml):owner(o),child(std::make_shared<UnitFlagChild>(o,std::move(lua))),
   xml_handle(std::move(xml)),xml_bytes(xml_handle.read_bytes(64ULL*1024*1024)),
   xml_sha(identify_bytes(xml_bytes).sha256),
   declaration_text(o->selected_source_text(o->root/"Assets/DLC/Expansion2/UI/InGame/InGame.xml",&declaration_sha)),
   declaration(content::read_xml_metadata(declaration_text)),
   layout(content::read_xml_metadata(std::string(reinterpret_cast<const char*>(xml_bytes.data()),xml_bytes.size()))){
   need(declaration_sha==o->progress.declaration_sha256,"UnitFlag captured declaration differs from SAME selected Bootstrap");
   need(declaration_sha=="94400b55698da6b4b54aed1f6c6f477c2faa1149edf78f72a4d2430dc640a8f4"&&
        xml_sha=="b14f9bc64a1a81696d4890f39eaf0fe9fced625519c5000626350de867553363","UnitFlag selected XML revision differs");
   const auto* bulk=named_xml(declaration,"Container","ID","BulkUI");need(bulk,"UnitFlag actual BulkUI declaration absent");
   const auto* view=named_xml(*bulk,"Container","ID","WorldViewControls");need(view,"UnitFlag actual WorldView declaration absent");
   const auto* selected=named_xml(*view,"Container","ID","SelectedUnitContainer");
   const auto* flag=named_xml(*view,"LuaContext","ID","UnitFlagManager");
   need(selected&&flag&&flag->attribute("FileName")=="Assets/UI/InGame/UnitFlagManager","UnitFlag selected sibling/context declaration absent");
   need(layout.name=="Context"&&layout.attribute("ColorSet")=="Beige_Black_Alpha","UnitFlag actual ColorSet differs");
   for(const auto id:{"AirCraftFlags","CivilianFlags","MilitaryFlags","GarrisonFlags","CityContainer"})need(named_xml(layout,"Container","ID",id),"UnitFlag actual XML container absent");
   for(const auto name:{"NewUnitFlag","CityFlag","AirButton"})need(named_xml(layout,"Instance","Name",name),"UnitFlag actual XML template absent");
   child->require_captured_bytes(child->bytes);require_current("CivilopediaScreen",nullptr);
  }
  void require_current(std::string_view context,const ScriptUiChildSource* actual)const override{
   owner->unit_require_current();const auto serial=owner->operation->operation_serial();
   need(owner->selected_source_identity(owner->root/"Assets/DLC/Expansion2/UI/InGame/InGame.xml").sha256==declaration_sha&&
        owner->selected_source_identity(owner->root/"Assets/UI/InGame/UnitFlagManager.xml").sha256==xml_sha&&
        identify_bytes(xml_handle.read_bytes(64ULL*1024*1024)).sha256==xml_sha,"UnitFlag actual XML source lease retired");
   if(actual){need(actual==child.get()&&context==child->context(),"Foreign selected context descriptor");child->require_current();}
   else{
    // Root belongs to the existing first-context Bootstrap and its SAME source,
    // not an arbitrary caller-supplied context name.
    need(context=="CivilopediaScreen"&&named_xml(declaration,"LuaContext","ID",context)&&
         owner->selected_source_identity(owner->root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua").sha256==owner->progress.parent_sha256,
         "Selected context has no actual retained activation root");
   }
   owner->operation->require_operation(serial);
  }
  void require_native_activation()const{
   require_current("CivilopediaScreen",nullptr);child->require_current();
   // Existing source graph supports authored containers only. It has no
   // WorldView native context, NewUnitFlag factory, sibling lookup or native
   // alpha/cargo/city owner. Do not synthesize g_MasterList to cross this gap.
   throw ScriptError("UnitFlagManager: actual WorldView context/NewUnitFlag factory and native Anchor/HealthBar/Cargo/City alpha owner are absent");
  }
  std::shared_ptr<const ScriptUiChildSource> resolve_child(std::string_view parent,const ScriptUiChildSource* actual,std::string_view request)override{
   require_current(parent,actual);
   // UnitFlagManager is a WorldView sibling, not a child of Civilopedia.
   // A future actual WorldView activation owner must issue that parent first.
   (void)request;throw ScriptError("Selected InGame child activation requires actual WorldView context/control owner");
  }
 };
 void prepare_unitflag_owned(){
  healthy();need(operation&&!diagnostic_world&&database,"UnitFlag preparation requires actual private Bootstrap loan");
  unit_defines=database->cache_global_defines(content::GlobalDefineProfile::bnw);(void)unit_max_hit_points();unit_require_current();
  unit_reads=std::make_shared<UnitReads>(this);
  const auto lua=mounts->resources.resolve(assets::AssetKey("Assets/UI/InGame/UnitFlagManager.lua"));
  const auto xml=mounts->resources.resolve(assets::AssetKey("Assets/UI/InGame/UnitFlagManager.xml"));
  const auto* lua_handle=std::get_if<assets::AssetHandle>(&lua.asset);const auto* xml_handle=std::get_if<assets::AssetHandle>(&xml.asset);
  need(lua_handle&&xml_handle,"UnitFlag selected native VFS handles unavailable");
  unitflag_sources=std::make_shared<UnitFlagSources>(this,*lua_handle,*xml_handle);
  unitflag_sources->require_current("CivilopediaScreen",nullptr);
 }

 const BnwStockLuaBootstrapProgress &advance(){
  healthy();
  if(!progress.selected_sources_verified)return progress;
  if(attempted){
   if(!host||!progress.first_prefix_executed)return progress;
   if(progress.pending_api=="Controls.ScrollBar.GetSize"&&!progress.native_scrollbar_queries){
    try{
     const auto current=client.current?client.current():std::nullopt;
     if(!current||!screen_snapshot||current->surface_owner!=screen_snapshot->surface_owner||current->revision!=screen_snapshot->revision||current->width!=screen_snapshot->width||current->height!=screen_snapshot->height){progress.blocker="Actual client frame changed after stock screen query; coroutine remains suspended before control creation";return progress;}
     scrollbar=ui::BnwCivilopediaScrollBarOwner::create(root,mounts->resources,*current,progress.xml_sha256);
    }catch(const std::exception &error){fail();progress.blocker=std::string("Selected parsed ScrollBar creation unavailable; Lua remains suspended: ")+error.what();return progress;}
    try{
     execute(progress.context,"local ok,frontier,width=coroutine.resume(__civis_stock_parent_continuation);assert(ok,frontier);assert(frontier=='Controls.ScrollPanel.GetSize' and width=="+std::to_string(scrollbar->render_snapshot().control("ScrollBar")->rectangle.width)+");assert(coroutine.status(__civis_stock_parent_continuation)=='suspended')","=selected-stock-owned-scrollbar-resume");
     need(progress.native_scrollbar_queries==1&&progress.native_screen_queries==1&&host->source_errors().empty()&&!host->loading_complete(),"Selected native ScrollBar query did not retain incomplete stock epoch");
     progress.pending_api="Controls.ScrollPanel.GetSize";progress.blocker="Actual selected ScrollBar GetSize completed on owned parsed client layout; same coroutine paused before native ScrollPanel.GetSize";
    }catch(const std::exception &error){fail();progress.blocker=std::string("Selected stock ScrollBar resume failed; VM retained as uncertain without publication: ")+error.what();}
    return progress;
   }
   if(progress.pending_api=="Controls.ScrollPanel.GetSize"&&!progress.native_panel_queries){
    try{
     const auto current=client.current?client.current():std::nullopt;
     if(!current||!scrollbar||!screen_snapshot||!scrollbar->get_size(*current,"ScrollPanel").available){progress.blocker="Actual client changed before panel getter; original owner/coroutine retained";return progress;}
     resize_transaction=scrollbar->fork_for_frame(*current);staged_panel_queries=0;
    }catch(const std::exception &error){fail();progress.blocker=std::string("Parsed panel getter unavailable; original owner/coroutine retained: ")+error.what();return progress;}
    try{
     const auto &panel=scrollbar->render_snapshot().control("ScrollPanel")->rectangle;
     execute(progress.context,"local ok,frontier,pw,ph,bw,bh=coroutine.resume(__civis_stock_parent_continuation);assert(ok,frontier);assert(frontier=='Controls.ScrollBar.SetSize' and pw=="+std::to_string(panel.width)+" and ph=="+std::to_string(panel.height)+" and bw=="+std::to_string(progress.scrollbar_width)+" and bh=="+std::to_string(screen_snapshot->height-172)+");assert(coroutine.status(__civis_stock_parent_continuation)=='suspended')","=selected-stock-panel-getter-before-native-slider");
     need(staged_panel_queries==1&&host->source_errors().empty()&&!host->loading_complete(),"Stock panel getter did not retain mandatory native slider boundary");
     auto accepted=progress;accepted.native_panel_queries=1;accepted.panel_width=staged_panel_width;accepted.panel_height=staged_panel_height;accepted.pending_api="Controls.ScrollBar.SetSize";accepted.blocker="First native Slider.SetSize requires owned embedded Graphic track/nine-slice/UV and bounds caches; same source coroutine remains suspended BEFORE setter";
     const auto final=client.current?client.current():std::nullopt;need(final&&resize_transaction->get_size(*final).available,"Actual client changed before panel getter publication");
     static_assert(std::is_nothrow_move_assignable_v<BnwStockLuaBootstrapProgress>);scrollbar=std::move(resize_transaction);progress=std::move(accepted);
    }catch(const std::exception &error){fail();progress.blocker=std::string("Stock panel getter transaction failed; VM and native scratch retained as uncertain: ")+error.what();}
    return progress;
   }
   if(progress.pending_api=="Controls.ScrollBar.SetSize"){
    try{
     const auto current=client.current?client.current():std::nullopt;
     need(current&&scrollbar&&scrollbar->get_size(*current).available,
          "Actual client changed before native child initialization");
     if(!right_tree){
      // This scoped initialization dependency uses explicit shared UI archives.
      // It does not establish the complete selected native UISkin/VFS epoch.
      auto resources=mounts->resources;
      if(!civis_sources){
      assets::AssetEnvironment environment(root);
      std::uint64_t next_mount=1;
      for(const auto &mount:resources.mounts())next_mount=std::max(next_mount,mount.id+1);
      for(const auto archive:{"Resource/DX9/UITextures.fpk","Resource/DX9/Expansion1UITextures.fpk","Resource/DX9/Expansion2UITextures.fpk"}){
       auto opened=environment.open_fpk_archive(archive);
       const auto *source=std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
       need(source&&*source,"Native right-tree scoped UI archive unavailable");
       resources.mount({next_mount++,0,0,assets::resource_lookup(*source),true,true,
           assets::resource_lookup_all(*source),assets::resource_keys(*source)});
      }
      }else civis_sources->require_current();
      auto staged=ui::BnwCivilopediaRightTreeOwner::create(root,resources,scrollbar->render_snapshot());
      const auto final=client.current?client.current():std::nullopt;
      need(final.has_value(),"Actual client unavailable after native child creation");
      (void)staged->advance(*final,scrollbar->render_snapshot().control_revision);
      right_tree=std::move(staged);
      native_resources=std::move(resources);
     }
     const auto &tree=right_tree->advance(*current,scrollbar->render_snapshot().control_revision);
     progress.staged_right_controls=tree.controls.size();
     progress.staged_right_literal_metrics=tree.completed_literal_labels;
     progress.initial_native_dependency=tree.pending_dependency;
     progress.blocker="Initial native child dependencies before first Slider.SetSize: "+tree.pending_dependency+
         "; same source coroutine remains suspended BEFORE setter";
     if(native_window){
      need(native_resources.has_value(),"Native runtime requires SAME retained VM resource mounts");
      if(!native_runtime){
       native_runtime=ui::BnwCivilopediaNativeRuntimeOwner::create(native_window,root,*native_resources,*scrollbar,right_tree);
       if(civis_sources){
        civis_sources->require_current();
        native_runtime->bind_civis_color_snapshot(civis_sources->font_color_snapshot().retain_owner(),
            civis_sources->bank_,civis_sources->ui_snapshot().retain_owner(),civis_sources->host_);
        civis_sources->require_current();
       }
       if(completed_color)native_runtime->bind_completed_color_query(completed_color,completed_color_parent);
       if(gui_screen)native_runtime->bind_gui_screen(gui_screen);
      }
      const auto initialized=native_runtime->advance_initialization();
      progress.initial_native_dependency=initialized.pending_dependency;
      progress.blocker="Retained native source operation before first Slider.SetSize: "+initialized.pending_dependency+
          "; SAME stock coroutine remains suspended BEFORE setter";
     }
    }catch(const std::exception &error){
     fail();progress.blocker=std::string("Native right-tree dependency unavailable; same coroutine retained BEFORE setter: ")+error.what();
    }
    return progress;
   }
   if(progress.native_scrollbar_resizes||progress.native_sibling_mutations){
    progress.blocker="Legacy bounded setter observations do not verify native Slider/Graphic caches; staged VM cannot authorize native continuation";
    return progress;
   }
   if(progress.native_screen_queries||progress.pending_api!="UIManager.GetScreenSizeVal")return progress;
   if(operation&&!gui_screen){progress.blocker="Actual composed GUI screen owner unavailable; SAME VM remains before UIManager.GetScreenSizeVal";return progress;}
   if(!client.current){progress.blocker="Selected stock Lua requires attached current Civis client extent; same coroutine remains suspended";return progress;}
   try{auto current=client.current();if(!current||current->surface_owner.empty()||current->width<=0||current->height<=0||current->width>INT32_MAX||current->height>INT32_MAX){progress.blocker="Attached Civis client extent unavailable or invalid; original Lua stack retained before native screen query";return progress;}screen_snapshot=std::move(current);
    execute(progress.context,"local ok,frontier,height=coroutine.resume(__civis_stock_parent_continuation);assert(ok,frontier);assert(frontier=='Controls.ScrollBar.GetSize' and height=="+std::to_string(screen_snapshot->height)+");assert(coroutine.status(__civis_stock_parent_continuation)=='suspended')","=selected-stock-native-screen-resume");
    need(progress.native_screen_queries==1&&host->source_errors().empty()&&!host->loading_complete(),"Selected native screen query did not retain incomplete stock epoch");progress.pending_api="Controls.ScrollBar.GetSize";progress.blocker="Actual stock UIManager screen query completed on current Civis client extent; same coroutine is paused before native ScrollBar.GetSize control owner";
   }catch(const std::exception &error){fail();progress.blocker=std::string("Selected stock native screen resume failed; VM retained as uncertain without publication: ")+error.what();}
   return progress;
  }
  attempted=true;
  try{
   ScriptSourceProvider provider;provider.files=[this](std::string_view){return bounded_include_names;};provider.read=[this](std::string_view context,std::string_view key){if(context=="TechAwardPopup")require_tech_award_sources_current();const auto at=includes.find(std::string(key));need(at!=includes.end(),"Stock prefix requested source outside verified include closure");progress.includes_executed.emplace_back(key);return at->second;};ScriptUiEventProvider events;events.notifications={"AfterModsDeactivate","AfterModsActivate","SerialEventGameMessagePopup","SerialEventGameMessagePopupShown","GameplaySetActivePlayer","SerialEventEndTurnDirty"};
   if(unitflag_sources)events.notifications.insert(events.notifications.end(),unitflag_events.begin(),unitflag_events.end());
   // Exact retained WorldView.lua .Add surface. A declared registration
   // channel is not a native occurrence, successful dispatch or frame ACK.
   constexpr std::string_view worldview_notifications[]={"KeyUpEvent","UIPathFinderUpdate",
     "SerialEventMouseOverHex","ClearUnitMoveHexRange","StartUnitMoveHexRange",
     "AddUnitMoveHexRangeHex","GameplaySetActivePlayer","MultiplayerGameAbandoned","MultiplayerGameLastPlayer"};
   for(const auto name:worldview_notifications)
    if(std::find(events.notifications.begin(),events.notifications.end(),name)==events.notifications.end())
      events.notifications.emplace_back(name);
   // Exact retained ActionInfoPanel.lua registration family. The SDK UI
   // interface defines local/remote turn-end without arguments and blocking-change
   // with (previous, new) enum operands. Registration performs no occurrence,
   // delivery, native effect or readiness acknowledgement.
   constexpr std::string_view action_info_notifications[]={
     "ActivePlayerTurnEnd","RemotePlayerTurnEnd","EndTurnBlockingChanged"};
   for(const auto name:action_info_notifications)
    if(std::find(events.notifications.begin(),events.notifications.end(),name)==events.notifications.end())
      events.notifications.emplace_back(name);
   ScriptUiLayoutProvider layout;layout.context_id=progress.context;layout.controls={{"ScrollBar","ScrollBar",true},{"ScrollPanel","ScrollPanel",true},{"UpButton","UpButton",true},{"DownButton","DownButton",true},{"LeftScrollPanel","ScrollPanel",true},{"LeftScrollBar","ScrollBar",true},{"LeftUpButton","UpButton",true},{"LeftDownButton","DownButton",true}};
   layout.screen_size=[this]{
    need(screen_snapshot.has_value(),"Stock UIManager has no attached current client extent");
    std::int64_t width=screen_snapshot->width,height=screen_snapshot->height;
    if(operation){
     need(static_cast<bool>(gui_screen),"Stock UIManager requires actual compositor-issued GUI screen");
     const auto actual=gui_screen;
     actual->with_actual_screen_loan([&](const auto& loan){width=loan.width();height=loan.height();loan.require_current();});
     healthy();need(width==screen_snapshot->width&&height==screen_snapshot->height,
      "Actual GUI layout dimensions differ from SAME physical client consistency check");
    }
    ++progress.native_screen_queries;progress.screen_width=width;progress.screen_height=height;
    progress.client_extent_owner=screen_snapshot->surface_owner;progress.client_extent_revision=screen_snapshot->revision;
    return std::pair{width,height};
   };
   layout.get_size=[this](std::string_view id)->std::pair<std::int64_t,std::int64_t>{
    need(scrollbar&&screen_snapshot&&(id=="ScrollBar"||id=="ScrollPanel"||id=="LeftScrollPanel"),"Stock parsed native control GetSize owner unavailable");const auto actual=client.current?client.current():std::nullopt;need(actual.has_value(),"Stock current client extent unavailable during native control query");
    const auto *owner=resize_transaction?resize_transaction.get():scrollbar.get();const auto size=owner->get_size(*actual,id);need(size.available,size.blocker.c_str());
    if(id=="ScrollBar"){need(!resize_transaction,"Unexpected repeated ScrollBar source getter inside panel transaction");++progress.native_scrollbar_queries;progress.scrollbar_width=size.width;progress.scrollbar_height=size.height;}
    else if(id=="ScrollPanel"){need(resize_transaction&&!sibling_transaction,"Native ScrollPanel query requires its original detached UI transaction");++staged_panel_queries;staged_panel_width=size.width;staged_panel_height=size.height;}
    else{need(resize_transaction&&sibling_transaction&&staged_sibling_mutations==7&&staged_left_panel_queries==0,"Native LeftScrollPanel query requires exact sibling source order");++staged_left_panel_queries;staged_left_width=size.width;staged_left_height=size.height;}return {size.width,size.height};
   };
   layout.set_size=[](std::string_view,std::int64_t,std::int64_t){throw ScriptError("Native Slider.SetSize requires actual embedded Graphic track/cache owner; no successful bounded rectangle substitution");};
   layout.set_offset=[](std::string_view,std::int64_t,std::int64_t){throw ScriptError("Native SetOffset requires complete control/bounds cache owner");};
   layout.calculate_internal_size=[](std::string_view){throw ScriptError("Native CalculateInternalSize requires actual direct-child and slider/cache owners");};
   provider.files=protect(std::move(provider.files));provider.read=protect(std::move(provider.read));
   layout.screen_size=protect(std::move(layout.screen_size));layout.get_size=protect(std::move(layout.get_size));
   layout.set_size=protect(std::move(layout.set_size));layout.set_offset=protect(std::move(layout.set_offset));
   layout.calculate_internal_size=protect(std::move(layout.calculate_internal_size));
   auto gameplay=diagnostic_world?lua_gameplay_services(*database,diagnostic_world->setup,diagnostic_world->technologies,types):operation?gameplay_read_services_with_roster_owned():lua_gameplay_read_services(*database,world,types);
   if(operation&&gameplay.query){
    const auto actual=std::move(gameplay.query);
    gameplay.query=protect(decltype(gameplay.query){[this,actual](std::string_view sql,std::span<const ScriptValue> args)->std::unique_ptr<ScriptQuery>{
     auto query=actual(sql,args);if(!query)return {};
     return std::make_unique<OwnedQuery>(this,std::move(query),*operation);
    }});
   }
   gameplay.units=unit_reads;
   gameplay.db_statement_count=protect(std::move(gameplay.db_statement_count));
   gameplay.db_statement_sql=protect(std::move(gameplay.db_statement_sql));
   gameplay.db_memory_usage=protect(std::move(gameplay.db_memory_usage));
   gameplay.db_collect_memory_usage=protect(std::move(gameplay.db_collect_memory_usage));
   gameplay.ids=protect(std::move(gameplay.ids));
   gameplay.has_tech=protect(std::move(gameplay.has_tech));
   gameplay.player_team=protect(std::move(gameplay.player_team));
   gameplay.player_alive=protect(std::move(gameplay.player_alive));
   gameplay.player_leader=protect(std::move(gameplay.player_leader));
   gameplay.info_types=protect(std::move(gameplay.info_types));
   gameplay.game_turn=protect(std::move(gameplay.game_turn));
   gameplay.player_human=protect(std::move(gameplay.player_human));
   gameplay.game_speed=protect(std::move(gameplay.game_speed));
   gameplay.player_ever_alive=protect(std::move(gameplay.player_ever_alive));
   gameplay.player_minor=protect(std::move(gameplay.player_minor));
   gameplay.player_minor_type=protect(std::move(gameplay.player_minor_type));
   gameplay.player_civilization=protect(std::move(gameplay.player_civilization));
   auto modding=mods->provider();modding.entry_points=protect(std::move(modding.entry_points));
   modding.evaluated_file=protect(std::move(modding.evaluated_file));
   ScriptSaveDataProvider save_data;
   if(retained_saved_database){
    need(operation.has_value(),"Saved SQL production binding needs SAME active source attempt");
    auto actual=lua_saved_data_services(std::move(retained_saved_database));
    const auto lease=*operation;
    save_data.open=protect(decltype(save_data.open){[this,actual=std::move(actual),lease]()mutable->std::unique_ptr<ScriptSaveDataConnection>{
     auto connection=actual.open();
     if(!connection)return {};
     return std::make_unique<OwnedSavedConnection>(this,std::move(connection),lease);
    }});
   }
   host=std::make_unique<LuaScriptHost>(ScriptBudget{64ULL*1024*1024,2'000'000},std::move(provider),std::move(modding),std::move(gameplay),std::move(save_data),ScriptPregameProvider{},std::move(live_map),std::move(events),ScriptContentProvider{},ScriptUserDataProvider{},ScriptEnumProvider{},ScriptLocaleProvider{},std::move(layout));
   {const auto stamp=host->stock_operation_stamp();host_owner=host.get();host_vm=stamp.vm;host_epoch=stamp.failure_epoch;}
   // UnitFlag sources remain prepared/retained, but their WorldView activation
   // is explicitly unavailable. Do not register that dormant child channel for
   // this bounded Civilopedia root; its original includes use the SAME VM/fenv.
   // dispatch_same_unit_dim_owned still requires the real native activation.
   execute(progress.context,controls,"=selected-stock-owned-manager-identities");execute(progress.context,prefix,"@Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua:staged-prefix");
   need(progress.includes_executed==bounded_include_names&&host->source_errors().empty(),"Stock parent required includes did not execute in source order");
   execute(progress.context,"assert(type(__civis_stock_parent_continuation)=='thread' and coroutine.status(__civis_stock_parent_continuation)=='suspended' and type(Alphabetically)=='function' and type(IconLookup)=='function' and Events.AfterModsDeactivate.Count()==1 and Events.AfterModsActivate.Count()==1)","=selected-stock-staged-owner-check");
   progress.first_prefix_executed=true;progress.prefix_executions=1;progress.invalidation_listeners=2;need(!host->loading_complete(),"Partial stock prefix accidentally marked loading complete");
   progress.blocker="Persistent selected stock parent paused before UIManager.GetScreenSizeVal; same VM/local continuation retained. Remaining contexts, full native IncludeFileList, UI API owner and atomic epoch publication are unavailable";
  }catch(const std::exception &error){fail();progress.blocker=std::string("Selected stock bootstrap failed; staged VM retained as uncertain and not retried: ")+error.what();}
  return progress;
 }
};
std::unique_ptr<ScriptQuery> BnwSelectedStockLuaBootstrap::Impl::OwnedSavedConnection::query(
    std::string_view sql,std::span<const ScriptValue> args){
 try {
  const auto serial=lease.operation_serial();
  auto query=actual->query(sql,args);
  lease.require_operation(serial);
  if(!query)return {};
  return std::make_unique<OwnedQuery>(owner,std::move(query),lease);
 }catch(...){owner->fail();throw;}
}
BnwSelectedStockLuaBootstrap::BnwSelectedStockLuaBootstrap(std::filesystem::path root,const content::Catalog &catalog,const SavedContentProfile &selected,const SessionCheckpointIdentity &identity,const LiveSessionState &state,const content::RulesDatabase &database):BnwSelectedStockLuaBootstrap(std::move(root),catalog,selected,identity,state,database,ScriptMapProvider{}){}
BnwSelectedStockLuaBootstrap::BnwSelectedStockLuaBootstrap(std::filesystem::path root,const content::Catalog &catalog,const SavedContentProfile &selected,const SessionCheckpointIdentity &identity,const LiveSessionState &state,const content::RulesDatabase &database,ScriptMapProvider map) :impl_([&]{
    auto diagnostic=std::make_shared<LiveSessionState>(state);
    return std::make_unique<Impl>(std::move(root),catalog,selected,identity,
        diagnostic,database,std::move(map),diagnostic);
 }()){}
BnwSelectedStockLuaBootstrap::BnwSelectedStockLuaBootstrap(std::filesystem::path root,
 const content::Catalog& catalog,const SavedContentProfile& selected,
 const SessionCheckpointIdentity& identity,std::shared_ptr<const LiveSessionState> state,
 const content::RulesDatabase& database,ScriptMapProvider map,BnwStockLuaAttemptLease operation,
 std::shared_ptr<content::RulesDatabase> saved_database)
 :impl_(std::make_unique<Impl>(std::move(root),catalog,selected,identity,std::move(state),
     database,std::move(map),std::shared_ptr<LiveSessionState>{},std::move(operation),std::move(saved_database))){
 if(impl_->progress.selected_sources_verified){try{impl_->prepare_unitflag_owned();}catch(...){impl_->fail();throw;}}
}
void BnwSelectedStockLuaBootstrap::dispatch_same_unit_dim_owned(
 const BnwUnitMovePending& pending,const BnwStockLuaAttemptLease& lease){
 try{
  require_healthy_owned();const auto serial=lease.operation_serial();
  need(impl_->operation&&lease.world_lease().get()==impl_->world.get(),"Dim SAME world loan differs");
  const auto operands=pending.stock_dim_operands();
  need(impl_->host&&impl_->host.get()==impl_->host_owner&&impl_->unit_reads&&impl_->unitflag_sources,
       "Dim actual retained VM/providers have not been prepared");
  impl_->unitflag_sources->require_current("CivilopediaScreen",nullptr);
  // CvLuaUnit::MovesLeft clamps below zero. The DLL dim operand instead
  // authenticates the RAW canonical store, through the SAME provenance lookup.
  const auto* actual=impl_->unit_lookup(operands.owner,operands.full_id);
  need(actual&&Impl::unit_scalar(actual->core,"m_iMoves")==operands.moves,
       "Dim actual full-ID unit stored moves differ");
  // This boolean is the native stored-moves comparison. When activation is
  // supplied it must be pushed as a Lua boolean, never numeric zero.
  const bool dim=operands.moves<=0;(void)dim;
  lease.require_operation(serial);
  // The retained source owner checks genuine prepared XML/templates, then
  // denies activation until the actual WorldView/control effect owner exists.
  // No event dispatch, listener-count ACK or queued-intent replay occurs here.
  impl_->unitflag_sources->require_native_activation();
  throw ScriptError("UnitFlagManager activation returned without an actual registered native handler");
 }catch(...){impl_->fail();throw;}
}

// SourceOwner's actual attempt bookend may run inside the real include lender.
// healthy()/ordinary vm_current() and the general LuaHost stamp stay conservative.
void BnwSelectedStockLuaBootstrap::require_vm_current_stamp()const{impl_->vm_current_provider_bookend();}
std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>
BnwSelectedStockLuaBootstrap::retain_research_choice_gui_owned()const {
    impl_->healthy();
    need(impl_->host&&impl_->gui_screen,"Research action SAME host/composed GUI absent");
    const auto actual=impl_->gui_screen;
    const auto canonical=actual->retain_owner();
    impl_->healthy();
    need(canonical&&canonical.get()==actual.get()&&!canonical.owner_before(actual)&&
         !actual.owner_before(canonical),"Research action substituted compositor-issued GUI control");
    canonical->require_current();impl_->healthy();return canonical;
}

std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>
BnwSelectedStockLuaBootstrap::retain_research_choice_gui_storage_without_callbacks()const {
    impl_->vm_current(); // Creator first; no retired GUI/provider call.
    const auto actual=impl_->gui_screen;
    need(static_cast<bool>(actual),"Research actual temporary GUI storage absent");
    impl_->vm_current();return actual;
}

ScriptTestResult BnwSelectedStockLuaBootstrap::call_policy_adoption_hook_owned(
 std::int64_t player,std::int64_t policy,const BnwStockLuaAttemptLease& lease){
 try{
  impl_->healthy();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
   !impl_->operation->control_.owner_before(lease.control_),"PlayerAdoptPolicy foreign actual attempt");
  const auto serial=lease.operation_serial();
  need(impl_->host&&player>=0&&player<64&&policy>=0&&policy<=INT32_MAX,"PlayerAdoptPolicy actual operands unavailable");
  const ScriptValue args[]={player,policy};lease.require_operation(serial);impl_->healthy();
  const auto actual=impl_->host->call_hook("PlayerAdoptPolicy",args);
  impl_->healthy();lease.require_operation(serial);
  if(!actual.errors.empty())throw ScriptError(actual.errors.front());
  return actual; // Genuine SAME VM result, including actual false/no listeners.
 }catch(...){impl_->fail();throw;}
}

std::optional<bool> BnwSelectedStockLuaBootstrap::test_policy_adoption_owned(
 std::int64_t player,std::int64_t policy,const BnwStockLuaAttemptLease& lease) {
 try {
  impl_->healthy();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
   !impl_->operation->control_.owner_before(lease.control_),"Policy TestAll foreign actual attempt");
  const auto serial=lease.operation_serial();need(impl_->host&&player>=0&&player<64&&policy>=0&&policy<=INT32_MAX,"Policy TestAll actual operands unavailable");
  const ScriptValue args[]={player,policy};lease.require_operation(serial);impl_->healthy();
  const auto actual=impl_->host->test_all("PlayerCanAdoptPolicy",args);
  impl_->healthy();lease.require_operation(serial);
  if(!actual.errors.empty())throw ScriptError(actual.errors.front());
  return actual.value; // Actual SAME-VM no-listener result, not caller permission.
 }catch(...){impl_->fail();throw;}
}

std::optional<bool> BnwSelectedStockLuaBootstrap::test_research_choice_owned(
 BnwResearchChoiceEvent event,std::int64_t player,std::int64_t technology,const BnwStockLuaAttemptLease& lease) {
 try {
  impl_->healthy();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
       !impl_->operation->control_.owner_before(lease.control_),"Research TestAll foreign actual attempt");
  const auto serial=lease.operation_serial();
  need(impl_->host&&player>=0&&technology>=0,"Research TestAll actual operands/host unavailable");
  const char* name=nullptr;
  switch(event){
   case BnwResearchChoiceEvent::player_can_ever_research:name="PlayerCanEverResearch";break;
   case BnwResearchChoiceEvent::player_can_research:name="PlayerCanResearch";break;
   default:throw ScriptError("Research TestAll unsupported typed event");
  }
  const ScriptValue args[]={player,technology};
  lease.require_operation(serial);impl_->healthy();
  const auto actual=impl_->host->test_all(name,args);
  impl_->healthy();lease.require_operation(serial);
  if(!actual.errors.empty())throw ScriptError(actual.errors.front());
  // Preserve actual dispatch nil/no-listener, false and true distinctly.
  return actual.value;
 }catch(...){impl_->fail();throw;}
}
void BnwSelectedStockLuaBootstrap::require_owned_physical_window()const{
 require_healthy_owned();need(static_cast<bool>(impl_->native_window),"Color actual native window absent");
 impl_->native_window->require_owned_renderer_current();require_healthy_owned();
}
std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery>BnwSelectedStockLuaBootstrap::retain_completed_color_query(
 std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery> input){
 require_owned_physical_window();
 need(input&&!impl_->color_attachment_attempted,"Color source attachment absent/already attempted");
 impl_->color_attachment_attempted=true; // Before retain_owner's actual Source callbacks.
 try{
  auto canonical=input->retain_owner();
  require_owned_physical_window();
  need(canonical&&canonical.get()==input.get()&&!canonical.owner_before(input)&&!input.owner_before(canonical),
       "Color source incoming control is not SAME canonical producer");
  return canonical;
 }catch(...){impl_->fail();throw;}
}
void BnwSelectedStockLuaBootstrap::bind_completed_color_query(
 std::shared_ptr<const ui::detail::BnwNativeCompletedColorQuery> canonical,
 std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> actual_parent){
 require_owned_physical_window();
 need(impl_->color_attachment_attempted&&!impl_->completed_color&&canonical&&actual_parent,
      "Color source binding requires genuine retained canonical attempt/Parent");
 try{
  canonical->require_same_parent(*actual_parent);require_owned_physical_window();
  if(impl_->native_runtime)impl_->native_runtime->bind_completed_color_query(canonical,actual_parent);
  require_owned_physical_window();
  impl_->completed_color_parent=actual_parent;impl_->completed_color=std::move(canonical);
 }catch(...){impl_->fail();throw;}
}
void BnwSelectedStockLuaBootstrap::require_prepared_owned()const {
 need(impl_->operation.has_value(),"Diagnostic Bootstrap has no actual operation loan");
 impl_->healthy();need(impl_->progress.selected_sources_verified,"Owned stock sources did not prepare");
}
void BnwSelectedStockLuaBootstrap::require_healthy_owned()const {
 need(impl_->operation.has_value(),"Diagnostic Bootstrap cannot attest an owned return");impl_->healthy();
}
BnwSelectedStockLuaBootstrap::OwnedOutcome BnwSelectedStockLuaBootstrap::advance_owned(){
 require_prepared_owned();
 try {
  const auto actual_window=impl_->native_window;
  // Even a missing-GUI suspension must validate its retained physical source.
  // This native getter invokes no client extent provider or Lua source getter.
  if(actual_window)(void)actual_window->native_window();
  impl_->healthy();
  (void)impl_->advance();
  if(!impl_->uncertain){
   need(impl_->host&&impl_->progress.first_prefix_executed,"Owned stock VM did not reach a planned suspension");
   impl_->execute(impl_->progress.context,
    "assert(coroutine.status(__civis_stock_parent_continuation)=='suspended')",
    "=selected-stock-owned-return-suspension");
   impl_->healthy();
   impl_->healthy();
   need(impl_->native_window.get()==actual_window.get() &&
        !impl_->native_window.owner_before(actual_window) && !actual_window.owner_before(impl_->native_window),
        "Owned stock advance substituted its actual native Window binding");
   if(actual_window)(void)actual_window->native_window();
   impl_->healthy();
  }
 }catch(...){impl_->fail();}
 return {impl_->uncertain?BnwStockLuaOperationOutcome::uncertain:BnwStockLuaOperationOutcome::incomplete_suspended,
         &impl_->progress,impl_->retained_error};
}
BnwStockLuaOperationRecord BnwSelectedStockLuaBootstrap::retained_operation_record()const {
 need(std::this_thread::get_id()==impl_->thread,"Stock journal foreign thread");
 BnwStockLuaOperationRecord r;r.vm_retained=true;r.host_retained=static_cast<bool>(impl_->host);
 if(impl_->host){
  const auto stamp=impl_->host->stock_operation_stamp();
  r.host_failure_epoch=stamp.failure_epoch;r.host_failed=stamp.failed;
  r.same_vm_identity=impl_->host.get()==impl_->host_owner&&stamp.vm==impl_->host_vm;
  r.retained_lua_bytes=impl_->host->memory_bytes();
 }
 r.suspended_before=impl_->progress.pending_api;return r;
}
BnwSelectedStockLuaBootstrap::~BnwSelectedStockLuaBootstrap()=default;
bool BnwSelectedStockLuaBootstrap::attach_client_extent(BnwStockLuaClientExtentProvider provider){if(!provider.current||impl_->progress.native_screen_queries||impl_->native_attachment_attempted)return false;impl_->client=std::move(provider);return true;}
bool BnwSelectedStockLuaBootstrap::attach_native_client(std::uintptr_t actual_hwnd,BnwStockLuaClientExtentProvider provider){
 if(!actual_hwnd||!provider.current||!impl_->progress.first_prefix_executed||!impl_->host||
    impl_->progress.native_screen_queries||impl_->native_attachment_attempted)return false;
 impl_->native_attachment_attempted=true; // one attempt, even after late failure
 auto window=ui::BnwCivilopediaNativeWindowBinding::attach(actual_hwnd,std::move(provider));
 const BnwStockLuaClientExtentProvider client{[window]{const auto retained=window;return retained->current_extent();}};
 impl_->native_window=std::move(window);impl_->client=client;return true;
}
void BnwSelectedStockLuaBootstrap::bind_gui_screen(std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> incoming){
 require_prepared_owned();impl_->healthy();
 need(incoming&&impl_->native_window,"GUI binding requires SAME typed physical Window first");
 const auto canonical=incoming->retain_owner();impl_->healthy();
 need(canonical.get()==incoming.get()&&!canonical.owner_before(incoming)&&!incoming.owner_before(canonical),
      "GUI Bootstrap control differs from actual compositor owner");
 impl_->native_window->require_same_gui_screen(*canonical);impl_->healthy();
 if(impl_->native_runtime)impl_->native_runtime->bind_gui_screen(canonical);
 impl_->healthy();canonical->require_current();impl_->healthy();impl_->gui_screen=canonical;
}
void BnwSelectedStockLuaBootstrap::attach_renderer_window_owner(
 std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> renderer,
 BnwStockLuaClientExtentProvider provider){
 require_prepared_owned();
 need(renderer&&provider.current&&impl_->progress.first_prefix_executed&&impl_->host&&
      !impl_->progress.native_screen_queries&&!impl_->native_attachment_attempted,
      "Typed renderer attachment requires SAME source before first screen getter");
 impl_->native_attachment_attempted=true; // Before producer/native callbacks; no replay.
 try{
  auto window=ui::BnwCivilopediaNativeWindowBinding::attach_renderer(std::move(renderer),std::move(provider));
  impl_->healthy();
  const BnwStockLuaClientExtentProvider client{[window]{const auto retained=window;return retained->current_extent();}};
  impl_->native_window=std::move(window);impl_->client=client;
  impl_->healthy();
 }catch(...){impl_->fail();throw;}
}
std::optional<ui::BnwCivilopediaNativeRuntimeFrame>BnwSelectedStockLuaBootstrap::native_runtime_frame()const{
 if(impl_->native_window)(void)impl_->native_window->current_extent();
 if(!impl_->native_runtime)return {};
 return impl_->native_runtime->current();
}
std::weak_ptr<const void>BnwSelectedStockLuaBootstrap::native_window_storage()const{
 require_healthy_owned();
 if(!impl_->native_window)return {};
 (void)impl_->native_window->current_extent();
 return std::weak_ptr<const void>(impl_->native_window->impl_);
}
void BnwSelectedStockLuaBootstrap::retire_native_client()noexcept{
 // Mutable VM/native owners are published and released only on this thread.
 // SourceOwner's stable atomic lease handles foreign access revocation.
 if(std::this_thread::get_id()!=impl_->thread)return;
 if(impl_->native_runtime)impl_->native_runtime->retire();
 if(impl_->native_window)impl_->native_window->retire();
}
const BnwStockLuaBootstrapProgress &BnwSelectedStockLuaBootstrap::advance(){
 need(std::this_thread::get_id()==impl_->thread,"Stock diagnostic advance foreign thread");
 // Public diagnostics may observe their failed prefix, but cannot replay it or
 // call any owned production method. Production uses advance_owned exclusively.
 if(impl_->uncertain)return impl_->progress;
 return impl_->advance();
}
const BnwStockLuaBootstrapProgress &BnwSelectedStockLuaBootstrap::progress()const{return impl_->progress;}
const ui::BnwScrollBarLayoutFrame *BnwSelectedStockLuaBootstrap::scrollbar_frame()const{return impl_->scrollbar?&impl_->scrollbar->render_snapshot():nullptr;}
const ui::BnwRightTreeFrame *BnwSelectedStockLuaBootstrap::right_tree_frame()const{return impl_->right_tree?&impl_->right_tree->snapshot():nullptr;}
}

namespace civ5::runtime {
std::optional<bool> BnwSelectedStockLuaBootstrap::test_production_owned(
 BnwSelectedProductionLuaEvent event,std::int32_t player,std::int32_t city,std::int32_t item,const BnwStockLuaAttemptLease& lease){
 try{
  impl_->healthy();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&!impl_->operation->control_.owner_before(lease.control_),"Production TestAll foreign actual attempt");
  const auto serial=lease.operation_serial();need(impl_->host&&player>=0&&city>=0&&item>=0,"Production TestAll actual operands/VM absent");
  lease.require_operation(serial);impl_->healthy();
  const auto result=impl_->host->test_selected_production_owned(event,player,city,item,lease);
  impl_->healthy();lease.require_operation(serial);
  if(!result.errors.empty())throw ScriptError(result.errors.front());return result.value;
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::call_city_trained_owned(
 std::int32_t player,std::int32_t city,std::int32_t unit,const BnwStockLuaAttemptLease& lease){
 try{
  impl_->healthy();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&!impl_->operation->control_.owner_before(lease.control_),"CityTrained foreign actual attempt");
  const auto serial=lease.operation_serial();need(impl_->host&&player>=0&&city>=0&&unit>=0,"CityTrained actual operands/VM absent");
  lease.require_operation(serial);impl_->healthy();
  const auto result=impl_->host->call_selected_city_trained_owned(player,city,unit,lease);
  impl_->healthy();lease.require_operation(serial);
  if(!result.errors.empty())throw ScriptError(result.errors.front());
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::invoke_team_tech_owned(
 std::shared_ptr<const BnwOwnedTeamSetHasTechLuaDispatch> supplied,
 std::int64_t team,std::int64_t technology,std::int64_t player,bool announce,const BnwStockLuaAttemptLease& lease){
 try{
  require_prepared_owned();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
   !impl_->operation->control_.owner_before(lease.control_),"Team:SetHasTech foreign actual Bootstrap attempt");
  const auto serial=lease.operation_serial();
  need(impl_->host&&impl_->progress.first_prefix_executed&&!impl_->progress.context.empty(),
   "Team:SetHasTech actual SAME executed context/host unavailable");
  lease.require_operation(serial);impl_->healthy();
  impl_->host->bind_owned_team_set_has_tech(std::move(supplied));
  lease.require_operation(serial);impl_->healthy();
  impl_->host->invoke_owned_team_set_has_tech(impl_->progress.context,team,technology,true,player,announce);
  lease.require_operation(serial);impl_->healthy();
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
ScriptIntegerResult BnwSelectedStockLuaBootstrap::accumulate_team_happiness_founder_owned(
 std::int64_t player,const BnwStockLuaAttemptLease& lease){
 try{
  require_prepared_owned();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
   !impl_->operation->control_.owner_before(lease.control_),"Founder happiness foreign actual Bootstrap attempt");
  const auto serial=lease.operation_serial();lease.require_operation(serial);impl_->healthy();
  need(impl_->host&&impl_->progress.first_prefix_executed,"Founder happiness actual SAME executed VM unavailable");
  auto result=impl_->host->accumulate_owned_team_happiness_founder(player);
  lease.require_operation(serial);impl_->healthy();return result;
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
std::shared_ptr<const BnwMajorTechTransportConsumption> BnwSelectedStockLuaBootstrap::dispatch_major_tech_award_owned(
 std::shared_ptr<const BnwMajorTechTransportRequest> request,const BnwStockLuaAttemptLease& lease){
 try{
  require_prepared_owned();
  need(impl_->operation&&request&&!lease.control_.owner_before(impl_->operation->control_)&&
       !impl_->operation->control_.owner_before(lease.control_),"TECH_AWARD foreign actual Bootstrap request/attempt");
  const auto serial=lease.operation_serial();lease.require_operation(serial);impl_->healthy();request->require_current();
  need(request->operation_serial()==serial,"TECH_AWARD actual request operation changed");
  request->begin_actual_popup_dispatch(lease); // before argument/source allocations
  const auto event=request->event();
  need(event.kind==BnwMajorTechUiTransportKind::popup_add&&event.native_name=="BUTTONPOPUP_TECH_AWARD"&&
       event.data1==event.player&&event.data3==event.technology&&event.player>=0&&event.player<=INT32_MAX&&event.technology>=0&&event.technology<=INT32_MAX,
       "TECH_AWARD genuine transport operands differ");
  if(!impl_->tech_award_activated){
   need(!impl_->tech_award_activation_attempted,"TECH_AWARD original context activation cannot replay");
   impl_->tech_award_activation_attempted=true;impl_->prepare_tech_award_sources();
   const auto context=ui::BnwTechAwardContextOwner::create(request,impl_->mounts->resources);
   context->bind_selected_localization(impl_->root,impl_->selected,impl_->catalog);
   request->require_current();lease.require_operation(serial);
   impl_->host->bind_selected_tech_award_context_owned(context,lease);
   impl_->host->activate_selected_tech_award_owned(impl_->tech_award_sources.front().bytes,lease);
   impl_->require_tech_award_sources_current();lease.require_operation(serial);impl_->healthy();
   impl_->tech_award_activated=true;
  }
  impl_->require_tech_award_sources_current();request->require_current();lease.require_operation(serial);
  impl_->host->dispatch_selected_tech_award_owned(event.data1,event.data2,event.data3,event.flags,event.option1,event.option2,lease);
  impl_->require_tech_award_sources_current();request->require_current();lease.require_operation(serial);impl_->healthy();
  return request->finish_actual_popup_dispatch(lease); // queued, never paint/present or full-game success
 }catch(...){if(request)request->fail_actual_dispatch();impl_->fail();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::dispatch_initial_unit_setxy_owned(
 const BnwNativeUnitAllocatorCore& core,const ui::SelectedCityTrainCreateSource& source,
 const BnwStockLuaAttemptLease& lease){
 // No caller payload or completion boolean: the earned Core lease captures
 // the actual unpublished node after its entire native happiness return.
 std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch> issued;
 try{
  impl_->healthy();
  need(impl_->host&&impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
       !impl_->operation->control_.owner_before(lease.control_),"Initial UnitSetXY foreign actual SAME-VM attempt");
  const auto serial=lease.operation_serial();lease.require_operation(serial);impl_->healthy();
  issued=core.begin_initial_unit_setxy_dispatch(source,lease);
  need(bool(issued),"Initial UnitSetXY genuine earned Core lease absent");
  core.require_initial_unit_setxy_dispatch_current(*issued);
  const auto player=issued->owner(),unit=issued->full_id(),x=issued->x(),y=issued->y();
  core.require_initial_unit_setxy_dispatch_current(*issued);lease.require_operation(serial);impl_->healthy();
  const auto result=impl_->host->call_initial_unit_setxy_owned(player,unit,x,y,lease);
  if(!result.errors.empty())throw ScriptError(result.errors.front());
  impl_->healthy();lease.require_operation(serial);
  core.require_initial_unit_setxy_dispatch_current(*issued);
  core.finish_initial_unit_setxy_dispatch(source,*issued);
 }catch(...){
  if(issued)core.fail_initial_unit_setxy_dispatch(source,*issued);
  impl_->fail();throw; // real Lua effects are uncertain, never speculative rollback
 }
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::dispatch_initial_happiness_founder_owned(
 const BnwNativeUnitAllocatorCore& core,const ui::SelectedCityTrainCreateSource& source,
 const BnwStockLuaAttemptLease& lease){
 std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialHappinessFounderDispatch> issued;
 try{
  impl_->healthy();
  need(impl_->host&&impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
       !impl_->operation->control_.owner_before(lease.control_),"Initial happiness founder foreign actual SAME-VM attempt");
  const auto serial=lease.operation_serial();lease.require_operation(serial);impl_->healthy();
  issued=core.begin_initial_happiness_founder_dispatch(source,lease);
  need(bool(issued),"Initial happiness founder authentic earned Core lease absent");
  core.require_initial_happiness_founder_dispatch_current(*issued);
  const auto player=issued->owner();
  core.require_initial_happiness_founder_dispatch_current(*issued);lease.require_operation(serial);impl_->healthy();
  const auto result=impl_->host->accumulate_initial_unit_founder_owned(player,lease);
  if(!result.errors.empty())throw ScriptError(result.errors.front());
  impl_->healthy();lease.require_operation(serial);core.require_initial_happiness_founder_dispatch_current(*issued);
  // Only the real protected registry return is forwarded. Nil keeps native
  // GetReligionCreatedByPlayer fallback in Core; it is never invented as zero.
  core.finish_initial_happiness_founder_dispatch(source,*issued,result.value);
 }catch(...){
  if(issued)core.fail_initial_happiness_founder_dispatch(source,*issued);
  impl_->fail();throw;
 }
}
}

namespace civ5::runtime {
ScriptIntegerResult BnwSelectedStockLuaBootstrap::accumulate_engine_founder_owned(std::int32_t actor,const BnwStockLuaAttemptLease& lease){
 try{
  impl_->healthy();need(impl_->host&&impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
    !impl_->operation->control_.owner_before(lease.control_),"Engine UI foreign actual SAME-VM operation");
  const auto serial=lease.operation_serial();lease.require_operation(serial);
  const auto result=impl_->host->accumulate_engine_founder_owned(actor,lease);
  impl_->healthy();lease.require_operation(serial);return result;
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::dispatch_tech_award_input_owned(
 std::shared_ptr<const ui::BnwTechAwardInputDispatch> supplied,
 const BnwStockLuaAttemptLease& lease){
 need(bool(supplied),"TechAward queued input token absent");
 const auto token=supplied->retain_canonical(supplied);
 try{
  require_prepared_owned();
  need(impl_->operation&&!lease.control_.owner_before(impl_->operation->control_)&&
       !impl_->operation->control_.owner_before(lease.control_),"TechAward input foreign Bootstrap operation");
  const auto serial=lease.operation_serial();lease.require_operation(serial);impl_->healthy();
  need(impl_->tech_award_activated,"TechAward input original context never activated");
  const auto context=token->retain_context_for_dispatch();
  // Compare this canonical queued context to the SAME VM storage without
  // consulting the expired original operation's currentness.
  impl_->host->require_selected_tech_award_input_owner_owned(context,lease);
  impl_->require_tech_award_sources_current();token->require_current();lease.require_operation(serial);
  token->begin_actual_dispatch(lease); // before Lua argument/table allocation
  impl_->host->dispatch_selected_tech_award_input_owned(token,lease);
  impl_->require_tech_award_sources_current();token->require_active_dispatch();
  lease.require_operation(serial);impl_->healthy();
  token->finish_actual_dispatch(lease);
  lease.require_operation(serial);impl_->healthy();
 }catch(...){token->fail_actual_dispatch();impl_->fail();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::bind_engine_ui_localization_owned(ui::BnwEngineUiTurnOwner& receiver,const BnwStockLuaAttemptLease& lease){
 try{impl_->healthy();need(impl_->operation&&!impl_->diagnostic_world&&!lease.control_.owner_before(impl_->operation->control_)&&!impl_->operation->control_.owner_before(lease.control_),"Engine Locale foreign real SAME-VM operation");const auto serial=lease.operation_serial();lease.require_operation(serial);
  receiver.bind_selected_localization(impl_->root,impl_->selected,impl_->catalog,impl_->world,impl_->identity);
  impl_->healthy();lease.require_operation(serial);
  impl_->end_turn_engine=receiver.end_turn_engine_for_bootstrap_owned();
  impl_->healthy();lease.require_operation(serial);
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::drain_gameplay_events_owned(
 const BnwOwnedGameplayEventPumpLoan& pump,const BnwStockLuaAttemptLease& lease){
 try{
  need(impl_->thread==std::this_thread::get_id(),"Gameplay receiver foreign VM creator");
  impl_->healthy();
  need(impl_->operation&&!impl_->diagnostic_world&&
   !impl_->operation->control_.owner_before(lease.control_)&&
   !lease.control_.owner_before(impl_->operation->control_),
   "Gameplay receiver differs from actual SAME VM operation");
  const auto serial=lease.operation_serial();pump.require_for_source(lease);
  // Engine owns the weak-lock borrower inventory; Bootstrap does not introduce
  // an uncounted extra strong Engine pin around its callback.
  const std::weak_ptr<ui::BnwEngineUiTurnOwner> never_bound;
  if(impl_->end_turn_engine.owner_before(never_bound)||never_bound.owner_before(impl_->end_turn_engine))
   ui::BnwEngineUiTurnOwner::drain_end_turn_queue_from_owned_weak(
    impl_->end_turn_engine,pump,lease);
  pump.require_for_source(lease);lease.require_operation(serial);impl_->healthy();
  if(impl_->should_dim_queue){
   graphics::BnwUnitShouldDimSourceLoan source(lease);
   graphics::BnwUnitShouldDimReceiverLoan receiver(source,pump);
   while(true){
    source.full();pump.require_for_source(lease);
    auto proof=impl_->should_dim_queue->consume_one(receiver);
    source.full();pump.require_for_source(lease);
    if(!proof)break;
    proof.reset();source.full();pump.require_for_source(lease);
   }
   source.full();pump.require_for_source(lease);
  }
  pump.require_for_source(lease);lease.require_operation(serial);impl_->healthy();
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
std::pair<const void*,std::uint64_t> BnwSelectedStockLuaBootstrap::require_worldview_vm_origin_without_callbacks(
 const BnwSelectedWorldViewContextOwner& source,const LuaScriptHost& vm,
 const BnwStockLuaAttemptLease& lease,const BnwWorldViewSourceOperationLoan& loan)const {
 need(std::this_thread::get_id()==impl_->thread,"WorldView VM origin foreign creator");
 need(!impl_->uncertain&&impl_->operation&&!impl_->diagnostic_world&&impl_->host&&
  impl_->host.get()==&vm&&impl_->host.get()==impl_->host_owner&&impl_->worldview_sources&&
  impl_->worldview_sources.get()==&source&&
  !lease.control_.owner_before(impl_->operation->control_)&&!impl_->operation->control_.owner_before(lease.control_),
  "WorldView VM/source is not actual canonical Bootstrap member");
 const auto native=impl_->host->stock_worldview_provider_stamp(loan);
 const auto stamp=native?*native:impl_->host->stock_operation_stamp();
 need(stamp.vm==impl_->host_vm&&!stamp.failed&&stamp.failure_epoch==impl_->host_epoch,
      "WorldView actual registered VM/failure epoch changed");
 return {stamp.vm,stamp.failure_epoch};
}
void BnwSelectedStockLuaBootstrap::bind_worldview_sources_before_context_owned(
 const BnwWorldViewSourceOperationLoan& loan)const {
 require_prepared_owned();loan.require_current();
 need(impl_->host&&impl_->worldview_sources,"WorldView preContext binding actual Host/source absent");
 loan.require_same_vm(*impl_->host);
 // The genuine render Loan is now published, while native Context trees
 // have not been constructed. Reuse the existing strict SAME Host binder.
 impl_->host->bind_worldview_sources_owned(impl_->worldview_sources,loan);
 loan.require_current();loan.require_same_vm(*impl_->host);
}
void BnwSelectedStockLuaBootstrap::with_worldview_context_owned(const ui::BnwOwnedCameraGuiScreen& supplied,const ui::BnwWorldViewGridToWorldLoan& grid,
 const std::function<void(const BnwWorldViewSourceOperationLoan&,const ui::BnwOwnedCameraGuiScreen::Loan&,
  const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
  const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>& borrowed,WorldViewCallbackPurpose purpose){
 require_prepared_owned();const auto lease=*impl_->operation;
 const auto identity=lease.pin_active_scope_without_callbacks();lease.require_scope_identity_without_callbacks(identity);
 need(impl_->gui_screen&&impl_->gui_screen.get()==&supplied&&impl_->host&&impl_->worldview_sources,
      "WorldView native operation has no actual published GUI/VM/source");
 const auto gui=impl_->gui_screen;
 gui->with_actual_worldview_context_loans([&](const auto& gui_loan,const auto& window_loan){
  lease.require_scope_identity_without_callbacks(identity);
  BnwWorldViewNativeContextIssuer issuer(impl_->worldview_sources,lease,*this,*impl_->host,
                                        gui_loan,window_loan,grid,impl_->worldview_trees);
  const auto& loan=issuer.operation();const auto serial=loan.operation_serial();
  need(!impl_->worldview_operation,"WorldView Bootstrap native lender reentered");
  impl_->worldview_operation=&loan;
  struct End{Impl& p;const BnwWorldViewSourceOperationLoan* loan;~End(){if(p.worldview_operation!=loan)p.fail();p.worldview_operation=nullptr;}} end{*impl_,&loan};
  // Preserve the original four callback slots; append actual NotificationPanel fifth.
  const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5> trees={
    issuer.tree("WorldView"),issuer.tree("UnitFlagManager"),issuer.tree("ActionInfoPanel"),issuer.tree("InGame"),issuer.tree("NotificationPanel")};
  std::optional<std::function<void(const BnwWorldViewSourceOperationLoan&,
   const ui::BnwOwnedCameraGuiScreen::Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&,
   const std::array<std::shared_ptr<ui::BnwWorldViewControlTree>,5>&)>> target;
  if(purpose==WorldViewCallbackPurpose::initial_lua){
   // Only actual activate_worldview_context_owned selects this PRIVATE phase.
   // No graph/compiled-child completion is inferred from the phase selector.
   need(impl_->worldview_activation_attempted&&!impl_->worldview_graph&&
        !impl_->unitflag_native&&!impl_->action_info_graph,
        "Initial Lua resources require genuine first activation before graph publication");
   grid.with_notification_initial_lua_resources_owned(loan,gui_loan,window_loan,trees,[&]{
    try{
     grid.require_for_source(loan);loan.require_operation(serial);grid.require_for_source(loan);
     lease.require_scope_identity_without_callbacks(identity);
     target.emplace(borrowed);grid.require_for_source(loan);
     loan.require_operation(serial);grid.require_for_source(loan);lease.require_scope_identity_without_callbacks(identity);
     need(static_cast<bool>(*target),"WorldView native callback target absent");
     (*target)(loan,gui_loan,window_loan,trees);grid.require_for_source(loan);
     loan.require_operation(serial);grid.require_for_source(loan);lease.require_scope_identity_without_callbacks(identity);
     target.reset();grid.require_for_source(loan);
     loan.require_operation(serial);grid.require_for_source(loan);lease.require_scope_identity_without_callbacks(identity);
    }catch(...){
     // Destroy independent callback storage inside the actual Resource/Source
     // lender even if FIRST refuses. Never read borrowed providers after a
     // failed callback without another renderer/Source origin check.
     try{grid.require_for_source(loan);}catch(...){lease.invalidate();}
     target.reset();
     try{grid.require_for_source(loan);loan.require_operation(serial);grid.require_for_source(loan);
         lease.require_scope_identity_without_callbacks(identity);}catch(...){lease.invalidate();}
     lease.invalidate();throw;
    }
   });
   grid.require_for_source(loan);loan.require_operation(serial);grid.require_for_source(loan);
   lease.require_scope_identity_without_callbacks(identity);
  }else{
  try{
   loan.require_operation(serial);lease.require_scope_identity_without_callbacks(identity);
   target.emplace(borrowed);loan.require_operation(serial);lease.require_scope_identity_without_callbacks(identity);
   need(static_cast<bool>(*target),"WorldView native callback target absent");
   (*target)(loan,gui_loan,window_loan,trees);
   loan.require_operation(serial);lease.require_scope_identity_without_callbacks(identity);
   target.reset();loan.require_operation(serial);lease.require_scope_identity_without_callbacks(identity);
  }catch(...){target.reset();lease.invalidate();throw;}
  }
 });
 lease.require_scope_identity_without_callbacks(identity);impl_->healthy();lease.require_scope_identity_without_callbacks(identity);
}
void BnwSelectedStockLuaBootstrap::activate_worldview_context_owned(const ui::BnwOwnedCameraGuiScreen& gui,const ui::BnwWorldViewGridToWorldLoan& grid){
 try{
  require_prepared_owned();need(impl_->gui_screen&&impl_->host&&impl_->mounts&&impl_->progress.first_prefix_executed,
    "WorldView actual native activation requires retained first VM and composed GUI");
  need(impl_->gui_screen.get()==&gui,"WorldView activation requires actual current GUI member");
  if(impl_->worldview_activation_attempted){impl_->healthy();return;}
  impl_->worldview_activation_attempted=true;
  const auto lease=*impl_->operation;const auto serial=lease.operation_serial();
  impl_->worldview_sources=BnwSelectedWorldViewContextOwner::capture_owned(
      impl_->root,impl_->mounts->resources,lease,impl_->civis_sources);
  lease.require_operation(serial);impl_->healthy();
  const auto actual_gui=impl_->gui_screen;
  // Initial WorldView Locale needs the actual UI-owned Engine before any
  // stock child asks for text; turn/policy execution is not its issuer.
  {
   need(bool(impl_->civis_sources),"WorldView initial Engine binding requires actual own-bank source");
   impl_->civis_sources->require_current();lease.require_operation(serial);impl_->healthy();
   const auto selected_host=impl_->civis_sources->host_.lock();
   need(bool(selected_host),"WorldView initial Engine binding selected Host retired");
   actual_gui->require_same_selected_host(*selected_host);
   const auto actual_ui=actual_gui->retain_policy_action_ui();
   actual_gui->require_same_selected_host(*selected_host);
   lease.require_operation(serial);impl_->healthy();impl_->civis_sources->require_current();
   const std::weak_ptr<ui::BnwEngineUiTurnOwner> never_bound;
   if(impl_->end_turn_engine.owner_before(never_bound)||never_bound.owner_before(impl_->end_turn_engine)){
    const auto previous=impl_->end_turn_engine.lock();
    need(bool(previous),"WorldView previously issued Engine retired before initial binding");
    // Existing callbackful identity checker retains its checking/poison barriers.
    previous->require_end_turn_queue_identity(actual_ui,selected_host,impl_->world);
    actual_gui->require_same_selected_host(*selected_host);
    impl_->civis_sources->require_current();lease.require_operation(serial);impl_->healthy();
    need(!impl_->end_turn_engine.owner_before(previous)&&!previous.owner_before(impl_->end_turn_engine),
      "WorldView prior Engine control changed during initial binding preflight");
   }
   const auto engine=ui::BnwEngineUiTurnOwner::retain_for_actual_policy_ui(actual_ui,selected_host);
   if(impl_->end_turn_engine.owner_before(never_bound)||never_bound.owner_before(impl_->end_turn_engine)){
    const auto previous=impl_->end_turn_engine.lock();
    need(previous&&previous.get()==engine.get()&&!previous.owner_before(engine)&&!engine.owner_before(previous),
      "WorldView initial Engine binding differs from prior canonical owner");
   }
   // The lookup below selects all matching payloads, not only a winner.
   // Prepare exactly the base-file/DLC-declaration keys used by saved_rules.
   // Diagnostic only: local reached counts and monotonic time.
   const auto text_progress_creator=impl_->thread;
   const auto text_progress_started=std::chrono::steady_clock::now();
   std::size_t text_entries_reached=0,text_base_discovered=0,text_base_completed=0;
   std::size_t text_dlc_packages_reached=0,text_dlc_keys_reached=0,text_dlc_completed=0;
   std::size_t text_requests_started=0,text_requests_loaded=0,text_requests_completed=0;
   const auto text_progress=[&](const char* stage,const char* phase,
                              std::chrono::steady_clock::time_point began)noexcept{
    if(std::this_thread::get_id()!=text_progress_creator)return;
    const int saved_errno=errno;
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-began).count();
    std::fprintf(stderr,"[WORLDVIEW_TEXTDATA_PROGRESS] stage=%s phase=%s elapsed_ms=%lld entries_reached=%zu base_discovered=%zu base_completed=%zu dlc_packages_reached=%zu dlc_keys_reached=%zu dlc_completed=%zu requests_started=%zu requests_loaded=%zu requests_completed=%zu\n",
        stage,phase,static_cast<long long>(elapsed),text_entries_reached,
        text_base_discovered,text_base_completed,text_dlc_packages_reached,
        text_dlc_keys_reached,text_dlc_completed,text_requests_started,
        text_requests_loaded,text_requests_completed);
    errno=saved_errno;
   };
   text_progress("total","BEGIN",text_progress_started);
   const auto text_current=[&]{
    impl_->civis_sources->require_current();lease.require_operation(serial);impl_->healthy();
    actual_gui->require_same_selected_host(*selected_host);
    impl_->civis_sources->require_current();lease.require_operation(serial);impl_->healthy();
   };
   const auto load_text=[&](const assets::AssetKey& key){
    text_current();++text_requests_started;
    impl_->civis_sources->bank_->load_all(key,assets::CivisResourceParse::xml);++text_requests_loaded;
    text_current();++text_requests_completed;
    if(text_requests_completed%64==0)text_progress("load_all","MILESTONE64",text_progress_started);
   };
   const auto text_discovery_started=std::chrono::steady_clock::now();
   text_progress("base_discovery_sort","BEGIN",text_discovery_started);
   text_current();const auto base_text_root=impl_->root/"Assets/Gameplay/XML/NewText";text_current();
   std::vector<std::filesystem::path> base_text_files;
   text_current();const bool base_text_present=std::filesystem::exists(base_text_root);text_current();
   if(base_text_present){
    text_current();
    for(const auto& entry:std::filesystem::recursive_directory_iterator(
        base_text_root,std::filesystem::directory_options::skip_permission_denied)){
     text_current();
     ++text_entries_reached;
     const bool is_xml=entry.is_regular_file()&&ascii_lower(entry.path().extension().string())==".xml";
     text_current();
     if(is_xml){
      need(base_text_files.size()<4096,"WorldView base TextData closure exceeds its bound");
      base_text_files.push_back(entry.path());++text_base_discovered;text_current();
     }
    }
    text_current();
   }
   text_current();std::sort(base_text_files.begin(),base_text_files.end(),[](const auto& left,const auto& right){
    return ascii_lower(left.generic_string())<ascii_lower(right.generic_string());
   });text_current();
   text_progress("base_discovery_sort","END",text_discovery_started);
   const auto text_base_load_started=std::chrono::steady_clock::now();
   text_progress("base_load_all","BEGIN",text_base_load_started);
   for(const auto& file:base_text_files){
    text_current();const assets::AssetKey key(assets::path_utf8(file.filename()));text_current();load_text(key);++text_base_completed;
   }
   text_progress("base_load_all","END",text_base_load_started);
   std::set<std::size_t> text_packages;std::size_t text_declarations=0;
   const auto text_dlc_load_started=std::chrono::steady_clock::now();
   text_progress("dlc_load_all","BEGIN",text_dlc_load_started);
   for(const auto index:impl_->selected.enabled_dlc){
    text_current();
    need(index<impl_->catalog.packages.size()&&text_packages.insert(index).second,
      "WorldView TextData selected package index absent or duplicated");
    const auto& package=impl_->catalog.packages[index];
    need(package.kind==content::PackageKind::dlc,"WorldView TextData source requires actual DLC manifest");++text_dlc_packages_reached;
    content::PackageResourceQuery query;query.selector="TextData";query.enabled_categories={true,true,true};
    text_current();const auto declarations=content::package_resources(package,query);text_current();
    for(const auto& declaration:declarations){
     need(text_declarations<4096,"WorldView selected TextData closure exceeds its bound");++text_declarations;++text_dlc_keys_reached;
     text_current();const assets::AssetKey key(declaration.value);text_current();load_text(key);++text_dlc_completed;
    }
   }
   text_progress("dlc_load_all","END",text_dlc_load_started);
   const auto text_locale_bind_started=std::chrono::steady_clock::now();
   text_progress("engine_locale_bind","BEGIN",text_locale_bind_started);
   text_current();bind_engine_ui_localization_owned(*engine,lease);
   actual_gui->require_same_selected_host(*selected_host);
   impl_->civis_sources->require_current();lease.require_operation(serial);impl_->healthy();
   impl_->worldview_sources->bind_tooltip_localization_owned(impl_->end_turn_engine,lease);
   lease.require_operation(serial);impl_->healthy();
   text_progress("engine_locale_bind","END",text_locale_bind_started);
   text_progress("total","END",text_progress_started);
  } // Temporary pins retire before native loans; the actual UI owns Engine.
  with_worldview_context_owned(*actual_gui,grid,[&](const auto& loan,const auto&,const auto&,const auto& trees){
   loan.require_current();impl_->host->bind_worldview_sources_owned(impl_->worldview_sources,loan);loan.require_current();
   need(impl_->civis_sources&&impl_->worldview_input_policy,"WorldView input policy must genuinely initialize before Context physical load prefix");
   impl_->civis_sources->require_current();loan.require_current();
   impl_->worldview_input_policy->require_for_source(loan);
   loan.require_current();impl_->civis_sources->require_current();loan.require_current();
   impl_->host->bind_worldview_input_policy_owned(impl_->worldview_input_policy,loan);loan.require_current();
   const auto engine=impl_->end_turn_engine;
   const std::weak_ptr<BnwSelectedWorldViewContextOwner> native_sources=impl_->worldview_sources;
   const auto* const actual_vm=impl_->host.get();
   const auto require=[native_sources,actual_vm]{
    const auto source=native_sources.lock();if(!source)throw ScriptError("Native Locale canonical source retired");
    const auto& active=source->current_operation_loan();active.require_same_vm(*actual_vm);
    return active.operation_serial();
   };
   const auto post=[native_sources,actual_vm](std::uint64_t serial){
    const auto source=native_sources.lock();if(!source)throw ScriptError("Native Locale canonical source retired");
    const auto& active=source->current_operation_loan();active.require_operation(serial);active.require_same_vm(*actual_vm);
   };
   ScriptLocaleProvider locale;
   locale.current=[engine,require,post]{const auto serial=require();
    auto result=ui::BnwEngineUiTurnOwner::selected_localization_language_from_owned_weak(engine);post(serial);return result;};
   locale.select=[engine,require,post](std::string_view requested){const std::string pinned(requested);const auto serial=require();
    const auto actual=ui::BnwEngineUiTurnOwner::selected_localization_language_from_owned_weak(engine);post(serial);
    // Original selected session is immutable here: a foreign request is refused.
    return pinned==actual.type;};
   locale.convert_text_key=[engine,require,post](std::string_view tag,std::span<const ScriptValue> args){
    const std::string key(tag);const std::vector<ScriptValue> values(args.begin(),args.end());const auto serial=require();
    const auto text=ui::BnwEngineUiTurnOwner::selected_localization_from_owned_weak(engine,key);post(serial);
    if(values.empty())return text;
    // Reuse the existing verified positional catalog formatter. Its supported
    // scalar replacement does not claim Original plural/gender grammar.
    std::vector<std::string> arguments;
    for(const auto& value:values){
     if(const auto* v=std::get_if<std::string>(&value))arguments.push_back(*v);
     else if(const auto* v=std::get_if<std::int64_t>(&value))arguments.push_back(std::to_string(*v));
     else if(const auto* v=std::get_if<double>(&value)){
      if(!std::isfinite(*v)||std::trunc(*v)!=*v||*v<static_cast<double>(std::numeric_limits<std::int64_t>::min())||
       *v>=-static_cast<double>(std::numeric_limits<std::int64_t>::min()))throw ScriptError("Native Locale scalar numeric representation unsupported");
      arguments.push_back(std::to_string(static_cast<std::int64_t>(*v)));
     }else throw ScriptError("Native Locale scalar argument kind unsupported");
    }
    ui::TopPanelLocalizationCatalog formatter;formatter.rows.emplace(key,text);
    const auto result=formatter.format(key,arguments);post(serial);return result;
   };
   impl_->host->bind_worldview_locale_owned(std::move(locale),loan);loan.require_current();

   impl_->worldview_mouse_events=impl_->worldview_sources->native_mouse_callback_events();loan.require_current();
   const auto worldview_source=impl_->worldview_sources->prepared_source("WorldView");loan.require_same_source(*worldview_source);
   impl_->worldview_graph=BnwWorldViewScriptUiAdapter::build_owned(trees[0],worldview_source,*impl_->host,impl_->worldview_mouse_events);
   loan.require_current();impl_->host->enable_ui_control_graph(impl_->worldview_graph);loan.require_current();
   impl_->host->load_worldview_context_owned(loan,worldview_source,impl_->worldview_graph);loan.require_current();
   const auto unitflag_source=impl_->worldview_sources->prepared_source("UnitFlagManager");loan.require_same_source(*unitflag_source);
   impl_->unitflag_native=BnwSelectedUnitFlagContextOwner::build_owned(impl_->worldview_sources,unitflag_source,trees[1],*impl_->host,impl_->worldview_mouse_events);
   impl_->unitflag_native->require_host_operation_owned(loan,*impl_->host);
   const auto bindings=impl_->unitflag_native->bindings_for_host_owned(loan,*impl_->host);
   impl_->unitflag_native->require_host_operation_owned(loan,*impl_->host);
   BnwWorldViewScriptUiAdapter::bind_selected_container_owned(bindings.graph,trees[3]);
   impl_->unitflag_native->require_host_operation_owned(loan,*impl_->host);
   impl_->host->enable_ui_control_graph(bindings.graph);
   impl_->unitflag_native->require_host_operation_owned(loan,*impl_->host);
   impl_->host->load_worldview_context_owned(loan,bindings.source,bindings.graph);
   impl_->unitflag_native->require_host_operation_owned(loan,*impl_->host);
   const auto source=impl_->worldview_sources->prepared_source("ActionInfoPanel");loan.require_same_source(*source);
   impl_->worldview_mouse_events=impl_->worldview_sources->native_mouse_callback_events();loan.require_current();
   impl_->action_info_graph=BnwWorldViewScriptUiAdapter::build_owned(trees[2],source,*impl_->host,impl_->worldview_mouse_events);
   loan.require_current();impl_->host->enable_ui_control_graph(impl_->action_info_graph);loan.require_current();
   impl_->host->prepare_end_turn_consumer_owned(*impl_->worldview_sources,source);loan.require_current();
   impl_->host->load_worldview_context_owned(loan,source,impl_->action_info_graph);loan.require_current();
   // This real source performs its own immediate OnEndTurnDirty call. Genuine
   // missing gameplay services propagate; no listener count or ready flag
   // substitutes for successful execution of the selected original child.
   const auto consumer=impl_->host->seal_end_turn_consumer_owned();loan.require_current();
   ui::BnwEngineUiTurnOwner::attach_end_turn_consumer_from_owned_weak(impl_->end_turn_engine,consumer,lease);
   loan.require_current();lease.require_operation(serial);
  },WorldViewCallbackPurpose::initial_lua);
  lease.require_operation(serial);impl_->healthy();
 }catch(...){impl_->fail();throw;}
}
}

namespace civ5::runtime {
BnwSelectedStockLuaBootstrap::BnwSelectedStockLuaBootstrap(
 std::shared_ptr<const BnwCivisOwnedStockSourceBundle> sources,const SessionCheckpointIdentity& identity,
 std::shared_ptr<const LiveSessionState> world,const content::RulesDatabase& rules,
 ScriptMapProvider map,BnwStockLuaAttemptLease lease,std::shared_ptr<content::RulesDatabase> saved)
 :impl_(std::make_unique<Impl>(sources->recipe().canonical_payload_root,sources->recipe().catalog,
  sources->recipe().profile,identity,std::move(world),rules,std::move(map),
  std::shared_ptr<LiveSessionState>{},std::move(lease),std::move(saved),sources)){
 sources->require_current();
 if(impl_->progress.selected_sources_verified){try{impl_->prepare_unitflag_owned();}catch(...){impl_->fail();throw;}}
 sources->require_current();
}
}

#ifdef _WIN32
namespace civ5::runtime {
void BnwSelectedStockLuaBootstrap::note_worldview_native_input_entry_owned(){
 need(impl_&&impl_->thread==std::this_thread::get_id()&&!impl_->uncertain&&impl_->host&&
  impl_->host.get()==impl_->host_owner,"Native input capture requires SAME live Bootstrap/Host creator");
 const auto stamp=impl_->host->stock_operation_stamp();
 need(stamp.vm==impl_->host_vm&&!stamp.failed&&stamp.failure_epoch==impl_->host_epoch,
  "Native input capture retained VM/failure identity changed");
 impl_->host->note_worldview_native_input_entry_owned();
}
bool BnwSelectedStockLuaBootstrap::dispatch_worldview_input_owned(const BnwWorldViewSourceOperationLoan& loan,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,const ui::BnwWorldViewWindowInputLoan& native){
 require_prepared_owned();loan.require_current();native.require_claimed_for(window);
 need(impl_->worldview_operation==&loan&&impl_->worldview_sources&&impl_->host,"WorldView input lacks genuine current native members");
 const auto source=impl_->worldview_sources->prepared_source("WorldView");loan.require_same_source(*source);native.require_claimed_for(window);
 need(native.message_!=WM_CLOSE,"WorldView close occurrence requires dedicated shutdown entry");
 need(static_cast<std::uint64_t>(native.w_)<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),"WorldView native WPARAM outside exact int64");
 const bool result=impl_->host->dispatch_worldview_input_owned(loan,static_cast<std::int64_t>(native.message_),
   static_cast<std::int64_t>(native.w_),static_cast<std::int64_t>(native.l_));
 native.require_claimed_for(window);loan.require_current();native.require_claimed_for(window);return result;
}
void BnwSelectedStockLuaBootstrap::dispatch_worldview_shutdown_owned(const BnwWorldViewSourceOperationLoan& loan,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,const ui::BnwWorldViewWindowInputLoan& native){
 require_prepared_owned();loan.require_current();native.require_claimed_for(window);
 need(native.message_==WM_CLOSE&&impl_->worldview_operation==&loan&&impl_->worldview_sources&&impl_->host,
   "WorldView shutdown requires actual close occurrence/current native members");
 // Keep canonical physical InGame and UnitFlag graph members through the
 // actual Original OnShutdown reparent. These are not caller-supplied trees.
 need(impl_->worldview_trees[2]&&impl_->worldview_trees[0]&&impl_->unitflag_native,
   "UnitFlag shutdown must precede canonical InGame/selected-parent retirement");
 impl_->worldview_trees[0]->require_current();native.require_claimed_for(window);loan.require_current();
 impl_->host->dispatch_worldview_shutdown_owned(loan);
 native.require_claimed_for(window);loan.require_current();impl_->worldview_trees[0]->require_current();
 native.require_claimed_for(window);loan.require_current();
}
#include "bnw_context_input_policy_load_prefix.inc"
}
#endif

#include "bnw_worldview_created_queue_consumer.inc"

#include "bnw_unitflag_health_bootstrap_consumer.inc"
#include "bnw_unitflag_health_pump_consumer.inc"

#include "bnw_stock_mod_activated_entrypoints_owned.inc"
