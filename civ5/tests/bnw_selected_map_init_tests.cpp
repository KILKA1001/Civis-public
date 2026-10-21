#include "civ5/runtime/bnw_selected_map_init.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/bnw_creation_config.hpp"
#include <array>
#include <cfenv>
#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <exception>
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static_assert(!std::is_default_constructible_v<BnwSelectedMapSourceOwner>);
static_assert(!std::is_copy_constructible_v<BnwSelectedMapSourceOwner>);
// Actual loose handles for small authored test sources. These are numeric
// component scenarios, never stock gameplay or native fresh Game issuers.
struct Fixture {
 std::filesystem::path root;
 Fixture():root(std::filesystem::temp_directory_path()/
  ("civis-selected-map-init-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))){
  std::filesystem::create_directories(root/"Maps");
 }
 ~Fixture(){std::error_code ec;const auto base=std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(),ec);
  if(ec)return;
  const auto parent=std::filesystem::weakly_canonical(root.parent_path(),ec);
  if(!ec&&parent==base&&root.filename().string().starts_with("civis-selected-map-init-"))std::filesystem::remove_all(root,ec);}
 void write(std::string_view text){std::ofstream f(root/"Maps/Constructor.lua",std::ios::binary);f.write(text.data(),static_cast<std::streamsize>(text.size()));check(static_cast<bool>(f),"Cannot write actual numeric fixture source");}
 assets::ResourceMountTable resources()const{
  const auto source=std::make_shared<assets::LooseSource>(root);
  return assets::ResourceMountTable(assets::resource_lookup(source),assets::resource_lookup_all(source),assets::resource_keys(source));
 }
};
void failures_have_no_result(const BnwSelectedMapInitResult& result){
 check(!result.init&&result.constructor.has_blocker(BnwMapScriptBlockerCode::script_failed),"Failed actual constructor must not publish numeric result");
}
void run(const std::filesystem::path& installation){
 content::Catalog catalog;
 catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
 catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
 constexpr std::array<std::size_t,2> dlcs{0,1};
 auto parent=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
 check(parent->content_ready(),"Actual BNW selected rules unavailable");
 auto resources=build_bnw_map_script_resources(installation,parent->profile,catalog);
 BnwMapSetupContext setup;
 setup.world_size=3;setup.climate=0;setup.graphics_initialized=false;
 const SimulationRandom random(RandomAlgorithm::bnw,{123,0,0});
 const auto before=random.state();
 const BnwMapSourceKey stock_key{"Assets/Maps/InlandSea.lua",{},{}},fixture_key{"Maps/Constructor.lua",{}, {}};
 // The join preserves actual map epoch and actual activation independently.
 // These cases execute only real installed rules; no LanguageInput is minted.
 {
  auto diagnostic=bootstrap_bnw_fresh_game_content(installation,catalog,dlcs);
  check(diagnostic.content_ready(),"Joined by-value activation diagnostics unavailable");
  std::weak_ptr<content::RulesDatabase> rules_life=diagnostic.rules;
  diagnostic.rules.reset();
  check(!rules_life.expired()&&!diagnostic.content_ready(),
   "Private actual facade receipt must survive public Rules reset without claiming readiness");
  diagnostic=BnwFreshGameContentResult{};
  check(rules_life.expired(),"Private facade receipt leaked after empty diagnostic move assignment");
 }
 {
  auto actual=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
  auto job=BnwSelectedMapJob::from_selected_installation(actual,installation,catalog,stock_key,setup,random);
  auto original=actual->rules;
  std::weak_ptr<content::RulesDatabase> rules_life=original;
  const auto table=job->resource_source_view();unsigned callbacks=0;
  actual->rules=std::shared_ptr<content::RulesDatabase>(original.get(),[](auto*){});
  original.reset();
  {
   assets::ScopedResourceResolveObserver observer(*table,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){++callbacks;});
   bool denied=false;try{job->advance_init_data_query();}catch(const ScriptError&){denied=true;}
   check(denied&&callbacks==0&&!rules_life.expired(),
    "SAME-get alien Rules control must refuse before VFS and keep original facade alive");
  }
  actual->rules=rules_life.lock();
  bool sticky=false;try{job->require_current();}catch(const ScriptError&){sticky=true;}
  check(sticky,"Restoring public Rules control cannot revive poisoned content epoch");
 }
 {
  auto actual=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
  const BnwCreationChoices choices{"WORLDSIZE_STANDARD","CLIMATE_TEMPERATE","SEALEVEL_MEDIUM",
   "GAMESPEED_STANDARD","ERA_ANCIENT","HANDICAP_PRINCE"};
  auto diagnostic=BnwCreationConfig::from_selected_rules(actual,choices);
  check(!diagnostic->metadata().rules_sha256.empty(),
   "Joined real Rules metadata diagnostics unavailable");
  const auto original=actual->rules;
  actual->rules=std::shared_ptr<content::RulesDatabase>(original.get(),[](auto*){});
  bool denied=false;try{(void)diagnostic->metadata();}catch(const BnwCreationConfigRefusal& e){
   denied=e.code()==BnwCreationConfigError::rules_retired;
  }
  check(denied,"Diagnostic Config accepted SAME-get alien Rules control");
  actual->rules=original;
  bool sticky=false;try{diagnostic->require_current();}catch(const BnwCreationConfigRefusal& e){
   sticky=e.code()==BnwCreationConfigError::rules_retired;
  }
  check(sticky,"Restored public Rules revived failed Config");
 }
 {
  auto actual=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
  auto job=BnwSelectedMapJob::from_selected_installation(actual,installation,catalog,stock_key,setup,random);
  auto destination=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
  auto destination_job=BnwSelectedMapJob::from_selected_installation(destination,installation,catalog,stock_key,setup,random);
  *destination=std::move(*actual);
  bool source_denied=false,destination_denied=false;
  try{job->require_current();}catch(const ScriptError&){source_denied=true;}
  try{destination_job->require_current();}catch(const ScriptError&){destination_denied=true;}
  check(source_denied&&destination_denied&&!actual->rules,
   "Joined move assignment must retire both published epochs and empty source Rules");
  bool rebound=false;try{(void)BnwSelectedMapJob::from_selected_installation(destination,installation,catalog,stock_key,setup,random);}
  catch(const ScriptError&){rebound=true;}
  check(rebound,"Joined move assignment rebound canonical Content authority");
 }
 auto stock=query_bnw_selected_map_init_data(parent,resources,stock_key,setup,random);
 check(stock.init&&stock.init->table_result,"Actual InlandSea constructor/query did not execute");
 check(stock.init->fields==ScriptMapInitFields{52,32,false,false,90,-90},"Actual InlandSea numeric GetMapInitData differs");
 check(stock.constructor.source&&!stock.constructor.included_sources.empty()&&!stock.constructor.map,
  "Actual source/includes must remain observed without generating a map");
 check(random.state()==before&&stock.constructor.map_random_before==before&&stock.constructor.map_random_after,
  "Constructor must preserve caller RNG and report its own real stream");
 bool worker_refused=false;
 std::exception_ptr worker_error;
 std::thread worker([&]{try{auto value=query_bnw_selected_map_init_data(parent,resources,stock_key,setup,random);
  for(const auto& b:value.constructor.blockers)if(b.message.find("Immutable preview rules scope already sealed")!=std::string::npos)worker_refused=!value.init;
 }catch(...){worker_error=std::current_exception();}});
 worker.join();if(worker_error)std::rethrow_exception(worker_error);
 check(worker_refused,"Foreign thread must not re-seal or revive actual immutable rules");
 std::int32_t width=0,height=0;bool found=false;
 const auto q=parent->rules->query_rows({"SELECT * FROM Worlds WHERE ID=? LIMIT 1",{std::int64_t{3}}},[&](const auto& row){
  check(row.integer("GridWidth")>=0&&row.integer("GridWidth")<=INT32_MAX&&row.integer("GridHeight")>=0&&row.integer("GridHeight")<=INT32_MAX,"Fixture native Worlds defaults overflow");
  width=static_cast<std::int32_t>(row.integer("GridWidth"));height=static_cast<std::int32_t>(row.integer("GridHeight"));found=true;
 });
 check(q.all_succeeded()&&found,"Actual Worlds defaults missing");
 const ScriptMapInitFields initial{width,height,true,false,90,-90};
 Fixture fixture;
 const auto call=[&](std::string_view script){fixture.write(script);auto table=fixture.resources();return query_bnw_selected_map_init_data(parent,table,fixture_key,setup,random);};
 for(const auto script:{"local actual_constructor=true","function GetMapInitData(w) return 42 end"}){
  const auto result=call(script);check(result.init&&!result.init->table_result&&result.init->fields==initial,"Native absent/non-table fallback differs");
 }
 auto empty=call("function GetMapInitData(w) return {} end");
 check(empty.init&&empty.init->table_result&&empty.init->fields==initial,"Native table nil fields must preserve all initial fields");
 const auto old_round=std::fegetround();check(old_round!=-1,"Cannot read native rounding mode");
 check(std::fesetround(FE_TONEAREST)==0,"Cannot select native rounding fixture");
 auto fractions=call("function GetMapInitData(w) return {Width=1.5,Height=2.5,WrapX=0,WrapY='',TopLatitude=-1.5,BottomLatitude=-2.5} end");
 const auto restored=std::fesetround(old_round);check(restored==0,"Cannot restore rounding mode");
 check(fractions.init&&fractions.init->fields==ScriptMapInitFields{2,2,true,true,-2,-2},"Native FISTP/truth semantics differ");
 // The script checks whether constructor and query share the SAME environment;
 // one top-level draw plus one query draw must be reported, without committing
 // either draw to the caller's supplied stream.
 auto draws=call("local marker=Map.Rand(7,'constructor');function GetMapInitData(w) return {Width=marker+Map.Rand(7,'query'),Height=w} end");
 SimulationRandom expected=random;const auto first=expected.draw(7),second=expected.draw(7);
 check(draws.init&&draws.init->fields.width==static_cast<std::int32_t>(first+second)&&draws.init->fields.height==3&&
  draws.constructor.map_random_after==expected.state()&&random.state()==before,"Actual same-context constructor/query RNG order differs");
 failures_have_no_result(call("function GetMapInitData(w) error('actual query failure') end"));
 failures_have_no_result(call("function GetMapInitData(w) return {Width=0/0} end"));
 failures_have_no_result(call("function GetMapInitData(w) return {Width=2147483648} end"));
 failures_have_no_result(call("function GetMapInitData(w) coroutine.yield() end"));
 auto child=call("function GetMapInitData(w) ContextPtr:LoadNewContext('Unissued') return {} end");
 failures_have_no_result(child);
 bool refused=false;
 auto other=std::make_shared<BnwFreshGameContentResult>();setup.selected_rules=other.get();
 try{static_cast<void>(query_bnw_selected_map_init_data(parent,resources,stock_key,setup,random));}catch(const ScriptError&){refused=true;}
 check(refused,"Foreign selected content parent must refuse before source/VM work");
 setup.selected_rules=nullptr;
 auto bad_hash=query_bnw_selected_map_init_data(parent,resources,
  {"Assets/Maps/InlandSea.lua",{},std::string(64,'0')},setup,random);
 check(!bad_hash.init&&bad_hash.constructor.has_blocker(BnwMapScriptBlockerCode::source_hash_mismatch),
  "Source SHA mismatch must refuse before issuing a constructor owner");
 // Real existing observer can revoke this actual content parent's facade
 // during source currentness. No supplied owner/flags issue the operation.

 // Retained job owns the actual source table and VM after factory return.
 // Its native-init barrier publishes no plots and consumes no extra RNG.
 auto retained=BnwSelectedMapJob::from_selected_installation(parent,installation,catalog,stock_key,setup,random);
 auto loaded=retained->observe();check(!loaded.source.init&&loaded.initialized_plots==0,
  "Retained constructor fabricated init/grid completion");
 retained->advance_init_data_query();auto queried=retained->observe();
 check(queried.source.init&&queried.source.init->fields==ScriptMapInitFields{52,32,false,false,90,-90}&&
  queried.initialized_plots==0&&queried.source.constructor.source&&!queried.source.constructor.included_sources.empty(),
  "Retained actual InlandSea query/source differs");
 bool mandatory=false;try{retained->advance_native_map_init();}catch(const BnwSelectedMapNativeServicesUnavailable&){mandatory=true;}
 const auto blocked=retained->observe();check(mandatory&&blocked.initialized_plots==0&&
  blocked.boundary==BnwSelectedMapJobBoundary::native_pregame_operands_required&&blocked.map_random==queried.map_random,
  "Native missing predecessors were skipped or consumed RNG/published grid");
 // A detached test-root cannot pretend to be the actual activation installation.
 bool wrong_installation=false;try{(void)BnwSelectedMapJob::from_selected_installation(parent,fixture.root,catalog,
  {"Assets/Maps/Retained.lua",{},{}},setup,random);}catch(const ScriptError&){wrong_installation=true;}
 check(wrong_installation,"Retained job accepted unrelated rule/map installation");
 auto wrong_catalog=catalog;wrong_catalog.packages.at(1).document.attributes.push_back({"forged","recipe"});
 bool wrong_recipe=false;try{(void)BnwSelectedMapJob::from_selected_installation(parent,installation,wrong_catalog,stock_key,setup,random);}
 catch(const ScriptError&){wrong_recipe=true;}check(wrong_recipe,"Retained job accepted fabricated Catalog recipe");
 // No-op owners cannot authenticate admission, even for the SAME actual facade.
 auto forged=std::shared_ptr<const BnwFreshGameContentResult>(parent.get(),[](const auto*){});
 bool forged_refused=false;try{(void)BnwSelectedMapJob::from_selected_installation(forged,installation,catalog,stock_key,setup,random);}
 catch(const ScriptError&){forged_refused=true;}check(forged_refused,"Forged no-op content controlblock admitted");
 // Regression of the actual observer deletion schedule: retained providers own
 // stable actual rules, and postcallback checks borrow no deleted public facade.
 {
  auto actual=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
  std::weak_ptr<BnwFreshGameContentResult> facade_life=actual;
  std::weak_ptr<content::RulesDatabase> rules_life=actual->rules;
  auto noop=std::shared_ptr<const BnwFreshGameContentResult>(actual.get(),[](const auto*){});
  auto job=BnwSelectedMapJob::from_selected_installation(actual,installation,catalog,stock_key,setup,random);
  const auto table=job->resource_source_view();bool released=false;
  {
   assets::ScopedResourceResolveObserver observer(*table,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
    if(!released){released=true;actual.reset();}
   });
   bool denied=false;try{job->advance_init_data_query();}catch(const ScriptError&){denied=true;}
   check(released&&facade_life.expired()&&!rules_life.expired()&&denied,
    "Actual parent deletion/no-op residue freed rules or returned query instead of strict refusal");
  }
  bool sticky=false;try{job->require_current();}catch(const ScriptError&){sticky=true;}
  check(sticky,"Destroyed actual content parent epoch revived");noop.reset();
 }
 {
  auto actual=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
  auto job=BnwSelectedMapJob::from_selected_installation(actual,installation,catalog,stock_key,setup,random);
  auto moved=std::make_shared<BnwFreshGameContentResult>(std::move(*actual));
  bool denied=false;try{job->advance_init_data_query();}catch(const ScriptError&){denied=true;}
  check(denied,"Published content facade move revived retained provider epoch");
  bool new_denied=false;try{(void)BnwSelectedMapJob::from_selected_installation(moved,installation,catalog,stock_key,setup,random);}
  catch(const ScriptError&){new_denied=true;}check(new_denied,"Moved published content owner minted fresh epoch");
 }
 {
  auto job=BnwSelectedMapJob::from_selected_installation(parent,installation,catalog,stock_key,setup,random);
  const auto table=job->resource_source_view();bool entered=false,inner_denied=false;
  {
   assets::ScopedResourceResolveObserver observer(*table,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
    if(!entered){entered=true;try{job->require_current();}catch(const ScriptError&){inner_denied=true;}}
   });
   bool denied=false;try{job->advance_init_data_query();}catch(const ScriptError&){denied=true;}
   check(entered&&inner_denied&&denied,"Actual callback reentry failed to quarantine retained job");
  }
 }
 // Genuine currentness lookup can destroy the facade. In-flight operations
 // retain the Impl/VM, retire after callback, and refuse before result return.
 auto retiring=BnwSelectedMapJob::from_selected_installation(parent,installation,catalog,stock_key,setup,random);
 const auto retiring_table=retiring->resource_source_view();auto* borrowed=retiring.get();bool dropped=false;
 {
  assets::ScopedResourceResolveObserver observer(*retiring_table,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
   if(!dropped){dropped=true;retiring.reset();}
  });
  bool denied=false;try{borrowed->advance_init_data_query();}catch(const ScriptError&){denied=true;}
  check(dropped&&!retiring&&denied,"Actual retained facade destruction callback did not refuse safely");
 }
 // Revoke the SAME content parent during an actual retained table lookup.
 auto revoked_job=BnwSelectedMapJob::from_selected_installation(parent,installation,catalog,stock_key,setup,random);
 const auto actual_table=revoked_job->resource_source_view();bool job_revoked=false;
 {
  assets::ScopedResourceResolveObserver observer(*actual_table,[&](const assets::AssetKey&,std::span<const assets::MountedResource>,bool){
   if(!job_revoked){job_revoked=true;parent->rules.reset();}
  });
  bool denied=false;try{revoked_job->advance_init_data_query();}catch(const ScriptError&){denied=true;}
  check(job_revoked&&denied,"Actual retained parent revocation published query result");
 }
 bool sticky=false;try{revoked_job->require_current();}catch(const ScriptError&){sticky=true;}
 check(sticky,"Retained revoked job was revived");
 // Restore a fresh real selected content instance for old diagnostic tests.
 parent=bootstrap_bnw_fresh_game_content_owner(installation,catalog,dlcs);
 check(parent->content_ready(),"Actual selected content recreation unavailable");
 unsigned root_resolves=0;bool revoked=false;
 assets::ScopedResourceResolveObserver observer(resources,[&](const assets::AssetKey& key,
  std::span<const assets::MountedResource>,bool all){
  if(all&&key.normalized_name()==assets::normalize_asset_name(stock_key.relative_path)){
   ++root_resolves;
   if(root_resolves==3){parent->rules.reset();revoked=true;}
  }
 });
 auto revoked_query=query_bnw_selected_map_init_data(parent,resources,stock_key,setup,random);
 bool reached_refusal=false;
 for(const auto& b:revoked_query.constructor.blockers)
  if(b.message.find("Selected map rules parent changed during VFS lookup")!=std::string::npos)reached_refusal=true;
 check(revoked&&reached_refusal&&!revoked_query.init,
  "Actual VFS observer revocation must refuse before stale DB facade borrow");
 std::cout<<"Actual stock numeric constructor and detached source scenarios PASS; InitMap/session/UI authority remains unavailable\n";
}
}
int main(int argc,char** argv){try{check(argc==2,"Usage: bnw_selected_map_init_tests <installation>");run(argv[1]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
