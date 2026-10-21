#include "civ5/runtime/bnw_founder_benefits_lua.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5::runtime;
namespace {
void check(bool v,const char *why) {if(!v)throw std::runtime_error(why);}
// Contract fixture only. This does not load or attest retail stock listeners.
class FixtureOwner:public BnwGameplayLuaEpochOwner {
 public:
 SavedContentProfile profile;
 SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
 std::string script; bool attest=true, bad_identity=false, bad_getters=false;
 int publishes=0;
 FixtureOwner() {profile.can_activate=true;profile.all_requirements_located=true;profile.enabled_dlc={2,5};}
 BnwGameplayLuaForkResult fork_at(const LiveSessionState &state) override {
  auto e=std::make_unique<BnwTeamTechLuaEpoch>();
  e->world=std::make_unique<LiveSessionState>(state);e->source_world=&state;
  e->getter_world=bad_getters?&state:e->world.get();e->identity=identity;
  if(bad_identity)e->identity.rules_sha256=std::string(64,'c');
  e->selected_dlc=profile.enabled_dlc;e->all_selected_contexts_loaded=attest;
  e->all_listeners_registered=attest;e->all_external_writes_escrowed=attest;
  ScriptMapProvider map;map.grid_size=[] {return std::pair<std::int64_t,std::int64_t>{1,1};};
  map.random=[w=e->world.get()](std::int64_t,std::string_view){++w->setup.game_turn;return std::int64_t{0};};
  e->host=std::make_unique<LuaScriptHost>(ScriptBudget{8*1024*1024,1000000},
   ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
   ScriptSaveDataProvider{},ScriptPregameProvider{},std::move(map));
  e->host->execute("fixture-only",script,"=not-selected-stock");e->host->complete_loading();
  return {std::move(e),{}};
 }
 BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) override {
  ++publishes;return {false,false,"fixture cannot publish"};
 }
};
}
int main() {
 LiveSessionState state{.random={SimulationRandom(RandomAlgorithm::bnw,{}),SimulationRandom(RandomAlgorithm::bnw,{})}};state.setup.profile=civ5::content::TechnologyProfile::bnw;
 state.setup.players[0]={};state.setup.game_turn=31;
 FixtureOwner owner;
 const auto quote=[&]{return quote_bnw_founder_benefits_lua(state,owner.profile,owner.identity,owner,0);};
 owner.attest=false;auto empty=quote();
 check(empty.status==BnwFounderBenefitsLuaStatus::blocked&&!empty.epoch,"Loading-complete empty VM is not lifecycle proof");
 owner.attest=true;auto no_listener=quote();
 check(no_listener.status==BnwFounderBenefitsLuaStatus::native_fallback_required&&no_listener.epoch&&!no_listener.religion,"No-listener quote lost native fallback");
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function(p) assert(p==0);return -1 end)";
 auto none=quote();check(none.status==BnwFounderBenefitsLuaStatus::override_quoted&&none.religion==-1&&none.epoch,"Explicit NO_RELIGION must bypass fallback");
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function(p) assert(p==0);return 2 end);GameEvents.GetFounderBenefitsReligion.Add(function(p) assert(p==0);return 3 end)";
 auto sum=quote();check(sum.religion==5&&sum.epoch,"Native accumulator did not sum from zero");
 owner.bad_identity=true;check(quote().status==BnwFounderBenefitsLuaStatus::blocked,"Stale identity accepted");owner.bad_identity=false;
 owner.bad_getters=true;check(quote().status==BnwFounderBenefitsLuaStatus::blocked,"Source-world getter accepted");owner.bad_getters=false;
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function()Map.Rand(1,'fixture');error('fail after escrowed write')end)";
 auto failed=quote();check(failed.status==BnwFounderBenefitsLuaStatus::blocked&&!failed.epoch&&state.setup.game_turn==31&&owner.publishes==0,"Failed query published fork or world effects");
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function()Map.Rand(1,'fixture');return 0 end)";
 auto success=quote();check(success.religion==0&&success.epoch->world->setup.game_turn==32&&state.setup.game_turn==31&&owner.publishes==0,"Quoted side effect escaped isolated epoch");
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function()return 2147483648 end)";
 check(quote().status==BnwFounderBenefitsLuaStatus::blocked,"Wide accumulator narrowed silently");
 std::cout<<"PASS: founder accumulator contract fixtures; stock lifecycle remains blocked\n";
}
