#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refused(F&& f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"Invalid reveal/Game/VM request accepted");}
template<class F>void owner_refused(F&& f){bool no=false;try{f();}catch(const BnwLuaCalculatedPlotYieldUnavailable&){no=true;}need(no,"Retired or foreign Game/reveal owner accepted");}
std::unique_ptr<LuaScriptHost> vm(const std::shared_ptr<const BnwLuaCalculatedPlotYieldOwner>& owner,ScriptPregameProvider pregame={}){
 return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},
  ScriptGameplayProvider{},ScriptSaveDataProvider{},std::move(pregame),bnw_lua_calculated_plot_services(owner));
}
bool raw_bit(const saves::PlotObservation& plot,std::int64_t team){
 const saves::PlotArrayObservation* words=nullptr;
 for(const auto& a:plot.arrays)if(a.name&&std::string_view(a.name)=="revealed_words"){
  need(words==nullptr,"Duplicate fixture reveal words");words=&a;
 }
 need(team>=0&&team<64&&words&&words->columns==1&&words->values.size()==4,"Actual fixture native reveal shape differs");
 const auto slot=static_cast<std::size_t>(team);const auto w=words->values[slot/32];
 need(w>=0&&w<=UINT32_MAX,"Actual fixture reveal word exceeds native u32");
 return (static_cast<std::uint32_t>(w)&(std::uint32_t{1}<<(slot%32)))!=0;
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=identify_file(save).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Actual immutable host absent");
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());
 auto rules=std::make_shared<const BnwNormalPlotYieldRules>(BnwNormalPlotYieldRules::load(host->database(),host->identity().rules_sha256));
 need(!session->bnw_game_debug_mode(),"Actual selected Game is no longer false-debug");
 const auto& world=session->state().world;need(world.pregame&&world.all_players&&world.map_plots&&world.map_projection,"Actual reached source absent");
 const auto active=world.pregame->archive.integers.at("active_player");need(active>=0&&active<64,"Actual selected Game has no active player");
 // Independently verify native single-player Read hook, rather than using preview_id.
 need(session->state().setup.game_type==0,"Expected actual selected single-player fixture");
 std::int64_t hook=-1;
 for(std::int64_t id=0;id<63;++id)if(session->state().setup.is_human(session->state().setup.players.at(id))){hook=id;break;}
 need(hook==active,"Actual Game::Read active-player hook/source differs");
 const auto expected=world.pregame->archive.integer_arrays.at("team_types").at(static_cast<std::size_t>(active));
 need(expected==session->state().setup.players.at(active).team,"Actual raw/owned team assignments differ");
 auto cap=make_bnw_lua_calculated_plot_yield_owner(session,rules);owner_refused([&]{(void)cap->pregame_is_multiplayer_game();});owner_refused([&]{(void)cap->player_is_turn_active(active);});
 owner_refused([&]{(void)cap->active_player();});
 owner_refused([&]{(void)cap->active_team();});owner_refused([&]{(void)cap->is_revealed(632,expected,false);});
 // Deliberately wrong provider actor is a negative transport input, not Game
 // issuing authority. The genuine capsule must supersede it without calling it.
 std::size_t wrong_provider_calls=0;const auto wrong_actor=(active+1)%64;
 auto wrong_provider=host->pregame_services();
 wrong_provider.active_player=[&]{++wrong_provider_calls;return wrong_actor;};
 owner_refused([&]{(void)cap->local_client_has_sent_turn_complete();});owner_refused([&]{(void)cap->local_client_has_sent_turn_all_complete();});owner_refused([&]{(void)cap->local_client_is_processing_messages();});
 auto lua=vm(cap,wrong_provider);need(cap->active_player()==active&&cap->active_team()==expected,"Active reads did not use native Pregame");
 lua->execute("active","assert(type(Game.GetActiveTeam)=='function');assert(Game.GetActivePlayer()=="+std::to_string(active)+");assert(Game.GetActiveTeam()=="+std::to_string(expected)+");assert(Game.GetActiveTeam('ignored',{})=="+std::to_string(expected)+")","=same-game-active-team");
 need(wrong_provider_calls==0,"Genuine Game getter invoked wrong caller provider");
 // A real ordinary VM/Game capsule still cannot mint the private Source client.
 owner_refused([&]{(void)cap->local_client_has_sent_turn_complete();});owner_refused([&]{(void)cap->local_client_has_sent_turn_all_complete();});owner_refused([&]{(void)cap->local_client_is_processing_messages();});
 lua->execute("no-client","assert(Network==nil and Game.IsProcessingMessages==nil)","=ordinary-vm-does-not-mint-client-reset-owner");
 const saves::PlayerCoreField* native_turn=nullptr;
 for(const auto& f:world.all_players->players.at(static_cast<std::size_t>(active)).core.fields)
  if(f.name&&std::string_view(f.name)=="m_bTurnActive"){need(!native_turn,"Duplicate genuine turn flag");native_turn=&f;}
 need(native_turn&&native_turn->encoding==saves::PlayerCoreEncoding::boolean&&native_turn->values.size()==1&&
  (native_turn->values[0]==0||native_turn->values[0]==1),"Actual fixture turn flag absent");
 need(cap->player_is_turn_active(active)==(native_turn->values[0]!=0),"Genuine Player turn read differed from native bool");
 need(cap->pregame_is_multiplayer_game()==(world.pregame->archive.integers.at("game_type")==1),"Genuine PreGame multiplayer read differed from native game type");
 owner_refused([&]{(void)cap->player_is_turn_active(-1);});owner_refused([&]{(void)cap->player_is_turn_active(64);});
 lua->execute("legacy-turn-api","assert(Player==nil or Player.IsTurnActive==nil)","=ordinary-vm-does-not-mint-worldview-player-api");
 bool turn_foreign=false;std::thread turn_worker([&]{try{(void)cap->player_is_turn_active(active);}catch(const BnwLuaCalculatedPlotYieldUnavailable&){turn_foreign=true;}});turn_worker.join();need(turn_foreign,"Foreign Player turn query reached raw world");
 std::size_t bits=0;bool found_true=false,found_false=false;
 for(const std::int32_t plot:{632,673,592})for(std::int64_t team=0;team<64;++team){
  const auto value=raw_bit(world.map_plots->plots.at(static_cast<std::size_t>(plot)),team);
  const auto& mirror=world.map_projection->cells.at(static_cast<std::size_t>(plot)).revealed_teams;
  need(mirror&&mirror->contains(team)==value,"Actual raw/projected fixture differs");
  need(cap->is_revealed(plot,team,false)==value&&cap->is_revealed(plot,team,true)==value,"Actual false-debug reveal differs");
  const auto p="local p=Map.GetPlotByIndex("+std::to_string(plot)+");local t="+std::to_string(team)+";local b="+(value?"true":"false")+";";
  lua->execute("reveals",p+"assert(type(p.IsRevealed)=='function');assert(p:IsRevealed(t)==b and p:IsRevealed(t,nil)==b and p:IsRevealed(t,false)==b and p:IsRevealed(t,true)==b);assert(p:IsRevealed(t,0)==b and p:IsRevealed(t,'')==b and p:IsRevealed(t,{})==b)","=current-native-bits-and-optbool");
  ++bits;found_true=found_true||value;found_false=found_false||!value;
 }
 need(bits==192&&found_true&&found_false,"Actual reveal coverage lacks both source branches");
 const bool zero=cap->is_revealed(632,0,false),one=cap->is_revealed(632,1,false);
 lua->execute("native","local p=Map.GetPlotByIndex(632);assert(p:IsRevealed()=="+std::string(zero?"true":"false")+" and p:IsRevealed(nil)=="+(zero?"true":"false")+");assert(p:IsRevealed('0.4',false)=="+(zero?"true":"false")+" and p:IsRevealed('0.6',false)=="+(one?"true":"false")+")","=native-rounded-team");
 lua->complete_loading();need(lua->loading_complete(),"Successful bounded reveal host failed loading");
 // Genuine independent preview override: accepted preview selection is not Game authority.
 std::int64_t alternate=-1;
 for(const auto& [id,player]:session->state().setup.players)if(id!=active&&player.alive){alternate=id;break;}
 need(alternate>=0,"Actual selected fixture lacks a distinct living preview actor");
 {
  auto other=ui::bootstrap_selected_bnw_preview(save,game,alternate,true);
  if(other.preview.blocker)throw std::runtime_error(other.preview.blocker->reason);
  std::shared_ptr<ui::SelectedBnwPreviewHost> parent(std::move(other.preview.host));
  need(parent&&parent->active_player_id()==alternate,"Actual alternate preview not selected");
  auto actual=std::shared_ptr<const LiveSession>(parent,&parent->session());
  auto same_rules=std::make_shared<const BnwNormalPlotYieldRules>(BnwNormalPlotYieldRules::load(parent->database(),parent->identity().rules_sha256));
  auto owner=make_bnw_lua_calculated_plot_yield_owner(actual,same_rules);auto machine=vm(owner);
  need(actual->state().world.pregame->archive.integers.at("active_player")==active&&owner->active_player()==active&&owner->active_team()==expected,
   "Caller preview selection became active Game authority");
 }
 bool worker=false;std::thread foreign([&]{try{(void)cap->active_team();}catch(const BnwLuaCalculatedPlotYieldUnavailable&){worker=true;}});foreign.join();need(worker,"Foreign active-team query reached world");
 worker=false;std::thread foreign_bit([&]{try{(void)cap->is_revealed(632,0,false);}catch(const BnwLuaCalculatedPlotYieldUnavailable&){worker=true;}});foreign_bit.join();need(worker,"Foreign reveal reached world");
 {
  auto raw=lua_map_services(*world.map_projection,bnw_live_plot_impassability(session->state()));
  LuaScriptHost absent({}, {},{},{},{},wrong_provider,std::move(raw));
  absent.execute("absent","assert(Map.GetPlotByIndex(632).IsRevealed==nil and Game.GetActiveTeam==nil);assert(Game.GetActivePlayer()=="+std::to_string(wrong_actor)+")","=legacy-provider-retained-without-capsule");absent.complete_loading();
  need(wrong_provider_calls==1,"Legacy provider behavior changed without genuine capsule");
 }
 for(const auto script:{
  "assert(not pcall(Plot.IsRevealed))",
  "assert(not pcall(Plot.IsRevealed,{}))",
  "assert(not pcall(Plot.IsRevealed,{__instance=632},0))",
  "assert(not pcall(Plot.IsRevealed,{__instance=newproxy(true)},0))",
  "assert(not pcall(Plot.IsRevealed,Map.GetPlotByIndex(632),-1,true))",
  "assert(not pcall(Plot.IsRevealed,Map.GetPlotByIndex(632),64,false))",
  "assert(not pcall(Plot.IsRevealed,Map.GetPlotByIndex(632),0/0))",
  "assert(not pcall(Plot.IsRevealed,Map.GetPlotByIndex(632),9007199254740994))",
  "local n=0;local p=setmetatable({},{__index=function() n=n+1;return Map.GetPlotByIndex(632).__instance end});assert(not pcall(Plot.IsRevealed,p,0));assert(n==0)"
 }){
  auto owner=make_bnw_lua_calculated_plot_yield_owner(session,rules);auto bad=vm(owner);
  bad->execute("bad",script,"=caught-reveal-error-poisons");refused([&]{bad->complete_loading();});need(!bad->loading_complete(),"Caught malformed reveal gained readiness");
 }
 owner_refused([&]{(void)cap->is_revealed(INT64_MAX,0,false);});owner_refused([&]{(void)cap->is_revealed(632,INT64_MAX,false);});
 refused([&]{auto duplicate=vm(cap);});need(cap->active_team()==expected,"Rejected second VM retired the original");
 lua.reset();owner_refused([&]{(void)cap->local_client_has_sent_turn_complete();});owner_refused([&]{(void)cap->local_client_has_sent_turn_all_complete();});owner_refused([&]{(void)cap->local_client_is_processing_messages();});owner_refused([&]{(void)cap->pregame_is_multiplayer_game();});owner_refused([&]{(void)cap->player_is_turn_active(active);});owner_refused([&]{(void)cap->active_player();});owner_refused([&]{(void)cap->active_team();});owner_refused([&]{(void)cap->is_revealed(632,0,false);});
 auto generic=std::make_shared<const LiveSession>(LiveSession::from_restored_bnw(session->source(),session->state()));
 owner_refused([&]{(void)make_bnw_lua_calculated_plot_yield_owner(generic,rules);});generic.reset();
 cap=make_bnw_lua_calculated_plot_yield_owner(session,rules);lua=vm(cap,wrong_provider);
 const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
 owner_refused([&]{(void)cap->active_player();});owner_refused([&]{(void)cap->active_team();});owner_refused([&]{(void)cap->is_revealed(632,0,false);});
 lua->execute("stale","assert(not pcall(Game.GetActivePlayer));assert(not pcall(Game.GetActiveTeam))","=stale-genuine-does-not-fallback-to-wrong-provider");
 need(wrong_provider_calls==1,"Stale genuine Game getter fell back to wrong provider");refused([&]{lua->complete_loading();});lua.reset();
 cap=make_bnw_lua_calculated_plot_yield_owner(session,rules);lua=vm(cap);need(cap->active_team()==expected,"Healthy replacement altered native team");
 auto moved=std::make_shared<LiveSession>(std::move(host->session()));
 owner_refused([&]{(void)cap->active_team();});lua.reset();
 cap=make_bnw_lua_calculated_plot_yield_owner(moved,rules);lua=vm(cap);
 host.reset();session.reset();rules.reset();need(cap->active_team()==expected&&cap->is_revealed(632,0,false)==zero,"Strong actual parent lifetime failed");
 (void)moved->mutable_state();owner_refused([&]{(void)cap->active_team();});owner_refused([&]{(void)cap->is_revealed(632,0,false);});
 lua->execute("revoked","assert(not pcall(Game.GetActiveTeam));assert(not pcall(Plot.IsRevealed,{},0))","=retirement-before-Lua-receiver");refused([&]{lua->complete_loading();});
 need(identify_file(save).sha256==original,"Original selected save changed");
 std::cout<<"PASS prepared SAME Game Read active team independent preview,192 current reveal bits/native args/poison, VM/thread/generation/strong parents; no icon/event/frame grant\n";
}
}
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
