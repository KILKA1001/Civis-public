#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/bnw_displayed_plot_yield.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refused(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Invalid native numeric/VM owner request accepted");}
template<class F>void owner_refused(F&& f){bool denied=false;try{f();}catch(const BnwLuaCalculatedPlotYieldUnavailable&){denied=true;}need(denied,"Retired/generic/foreign calculated plot owner accepted");}
std::unique_ptr<LuaScriptHost> vm(const std::shared_ptr<const BnwLuaCalculatedPlotYieldOwner>& owner){
 return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},
  ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},bnw_lua_calculated_plot_services(owner));
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Actual immutable host absent");
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());
 auto rules=std::make_shared<const BnwNormalPlotYieldRules>(BnwNormalPlotYieldRules::load(host->database(),host->identity().rules_sha256));
 need(!session->bnw_game_debug_mode(),"Selected genuine Game import is no longer debug false");
 const auto& state=session->state();
 for(const auto fixture:{std::pair<std::size_t,const char*>{673,"FEATURE_FOREST"},std::pair<std::size_t,const char*>{592,"FEATURE_JUNGLE"}}){
  const auto* type=host->types().find(fixture.second);
  need(type&&type->table=="Features"&&state.world.map_projection->cells.at(fixture.first).feature_type_hash==type->hash,"Actual selected feature fixture changed");
 }
 auto calculated=make_bnw_lua_calculated_plot_yield_owner(session,rules);
 owner_refused([&]{(void)calculated->value(632,0,false);}); // No actual VM yet.
 auto lua=vm(calculated);
 std::size_t numeric=0;
 for(const std::int32_t plot:{632,673,592})for(std::int32_t yield=0;yield<6;++yield){
  const auto normal=calculate_bnw_normal_plot_yield(state,session->source(),host->identity(),*rules,plot,yield);
  const auto displayed=inspect_bnw_displayed_plot_yield(session,rules,plot,yield).value();
  const auto prefix="local p=Map.GetPlotByIndex("+std::to_string(plot)+"); ";
  const auto script=prefix+"assert(type(p.CalculateYield)=='function'); assert(p:CalculateYield("+std::to_string(yield)+",false)=="+std::to_string(normal)+"); assert(p:CalculateYield("+std::to_string(yield)+",true)=="+std::to_string(displayed)+"); assert(p:CalculateYield('"+std::to_string(yield)+"',nil,'ignored')=="+std::to_string(normal)+")";
  lua->execute("numeric",script,"=genuine-numeric-transport");++numeric;
 }
 const auto food=calculated->value(632,0,false),shown=calculated->value(632,0,true),production=calculated->value(632,1,false);
 need(food>0&&numeric==18,"Actual road-home FOOD/source coverage absent");
 lua->execute("native","local p=Map.GetPlotByIndex(632); local f="+std::to_string(food)+"; local s="+std::to_string(shown)+"; assert(p:CalculateYield()==f and p:CalculateYield(nil)==f and p:CalculateYield(nil,false)==f); assert(p:CalculateYield(0,0)==s and p:CalculateYield(0,'')==s and p:CalculateYield(0,{})==s); assert(p:CalculateYield('0.4',false)==f and p:CalculateYield('0.6',false)=="+std::to_string(production)+")","=native-conversion-and-truthiness");
 lua->complete_loading();need(lua->loading_complete(),"Successful bounded numeric host failed loading");
 // A second real Lua allocation cannot transplant this SAME typed owner.
 refused([&]{auto duplicate=vm(calculated);});
 need(calculated->value(632,0,false)==food,"Rejected second VM retired original VM lease");
 bool worker=false;std::thread foreign([&]{try{(void)calculated->value(632,0,false);}catch(const BnwLuaCalculatedPlotYieldUnavailable&){worker=true;}});foreign.join();need(worker,"Foreign numeric query reached Game/world");
 // Old geometry remains available without manufacturing CalculateYield.
 {
  auto rawmap=lua_map_services(*state.world.map_projection,bnw_live_plot_impassability(state));
  LuaScriptHost absent({}, {},{},{},{},{},std::move(rawmap));
  absent.execute("absent","assert(Map.GetPlotByIndex(632).CalculateYield==nil)","=optional-service-absence");absent.complete_loading();
 }
 // Every caught malformed/native request keeps failed/ready poisoned. Use a
 // NEW real VM and NEW numeric owner per case, with unchanged selected inputs.
 for(const auto source:{
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield))",
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield,{}))",
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield,{__instance=632}))",
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield,{__instance=newproxy(true)},0,false))",
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield,p,6,false))",
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield,p,9007199254740994,false))",
  "local p=Map.GetPlotByIndex(632); assert(not pcall(p.CalculateYield,p,0/0,false))",
  "local p=Map.GetPlotByIndex(632); local calls=0; local receiver=setmetatable({}, {__index=function() calls=calls+1; return p.__instance end}); assert(not pcall(p.CalculateYield,receiver,0,false)); assert(calls==0)"
 }){
  auto cap=make_bnw_lua_calculated_plot_yield_owner(session,rules);auto bad=vm(cap);
  bad->execute("malformed",source,"=caught-error-must-poison");refused([&]{bad->complete_loading();});need(!bad->loading_complete(),"Caught Lua error gained readiness");
 }
 owner_refused([&]{(void)calculated->value(-1,0,false);});owner_refused([&]{(void)calculated->value(632,-1,false);});
 owner_refused([&]{(void)calculated->value(INT64_MAX,0,false);});
 lua.reset();owner_refused([&]{(void)calculated->value(632,0,false);}); // Retained session/cap, dead exact VM.
 auto generic=std::make_shared<const LiveSession>(LiveSession::from_restored_bnw(session->source(),session->state()));
 owner_refused([&]{(void)make_bnw_lua_calculated_plot_yield_owner(generic,rules);});generic.reset();
 calculated=make_bnw_lua_calculated_plot_yield_owner(session,rules);lua=vm(calculated);
 const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
 owner_refused([&]{(void)calculated->value(632,0,false);});lua.reset();
 // A new factory binds replacement genuine source/state/Game fields. An old
 // healthy checkpoint never resurrects the old VM/owner control.
 calculated=make_bnw_lua_calculated_plot_yield_owner(session,rules);lua=vm(calculated);
 need(calculated->value(632,0,false)==food,"Actual checkpoint replacement changed fixture value");
 auto moved=std::make_shared<LiveSession>(std::move(host->session()));
 owner_refused([&]{(void)calculated->value(632,0,false);});lua.reset();
 calculated=make_bnw_lua_calculated_plot_yield_owner(moved,rules);lua=vm(calculated);
 host.reset();session.reset();rules.reset();need(calculated->value(632,0,false)==food,"Strong actual parent/rules lifetime failed");
 (void)moved->mutable_state();owner_refused([&]{(void)calculated->value(632,0,false);});
 lua->execute("revoked","local p={__instance=nil}; assert(not pcall(Plot.CalculateYield,p,0,true))","=source-revoke-before-receiver");
 refused([&]{lua->complete_loading();});
 need(identify_file(save).sha256==original,"Original selected save changed");
 std::cout<<"PASS prepared genuine numeric transport18 pairs, native args/raw receiver poisoning, one VM lifetime, generation/move/revoke/strong parents; no stock YieldIconManager/UI/refresh/fullturn claim\n";
}
}
static_assert(!std::is_default_constructible_v<BnwLuaCalculatedPlotYieldOwner>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and canonical installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
