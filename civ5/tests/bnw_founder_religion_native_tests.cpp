#include "civ5/runtime/bnw_founder_religion_native.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *reason){if(!value)throw std::runtime_error(reason);}
template<class F>void rejects(F f,const char *why){try{f();}catch(const std::exception &){return;}throw std::runtime_error(why);}
LiveSessionState fixture_world(){
 LiveSessionState state{.random={SimulationRandom(RandomAlgorithm::bnw,{}),SimulationRandom(RandomAlgorithm::bnw,{})}};
 state.setup.profile=content::TechnologyProfile::bnw;state.setup.players[0]={};state.setup.players[1]={};state.setup.game_turn=19;
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 state.world.game_state.layout=bnw;state.world.payload_end_offset=200;
 auto &religions=state.world.religion_culture;religions.layout=bnw;religions.religions_version=4;religions.start_offset=10;religions.culture_offset=30;religions.end_offset=50;
 auto &header=state.world.map_header;header.layout=bnw;header.grid={2,1};header.wrap_x=true;
 state.world.map_projection=saves::SavedMapProjection{};auto &map=*state.world.map_projection;map.grid=header.grid;map.wrap_x=true;
 state.world.map_plots=saves::MapPlotsObservation{};state.world.map_plots->layout=bnw;
 for(std::size_t i=0;i<2;++i){saves::SavedMapCellProjection cell;cell.index=i;cell.x=i;cell.y=0;cell.owner=i;map.cells.push_back(cell);saves::PlotObservation raw;raw.index=i;raw.x=i;raw.y=0;raw.fields.push_back({"owner",static_cast<std::int64_t>(i)});state.world.map_plots->plots.push_back(raw);}
 return state;
}
saves::ReligionObservation entry(std::int32_t id,std::int32_t founder,bool pantheon=false,std::int32_t x=0,std::int32_t y=0){saves::ReligionObservation e;e.version=4;e.religion=id;e.founder=founder;e.pantheon=pantheon;e.holy_city_x=x;e.holy_city_y=y;return e;}
// Explicit contract fixture; it does not claim selected stock Lua loading.
class FixtureOwner:public BnwGameplayLuaEpochOwner {
 public:
 SavedContentProfile profile;SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
 std::string script;bool attest=true,missing=false,change_ownership=false;int publishes=0;
 FixtureOwner(){profile.can_activate=true;profile.all_requirements_located=true;profile.enabled_dlc={2,5};}
 BnwGameplayLuaForkResult fork_at(const LiveSessionState &state) override {
  if(missing)return {{},"active selected lifecycle is unavailable"};
  auto e=std::make_unique<BnwTeamTechLuaEpoch>();e->world=std::make_unique<LiveSessionState>(state);e->source_world=&state;e->getter_world=e->world.get();e->identity=identity;e->selected_dlc=profile.enabled_dlc;
  e->all_selected_contexts_loaded=attest;e->all_listeners_registered=attest;e->all_external_writes_escrowed=attest;
  if(change_ownership){e->world->world.map_projection->cells[0].owner=1;e->world->world.map_plots->plots[0].fields[0].value=1;}
  e->host=std::make_unique<LuaScriptHost>(ScriptBudget{8*1024*1024,1000000});e->host->execute("fixture-only",script,"=not-stock-context");e->host->complete_loading();return {std::move(e),{}};
 }
 BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) override {++publishes;return {false,false,"fixture never publishes"};}
};
void fixture_checks(){
 auto state=fixture_world();auto model=BnwFounderReligionReadModel::bind(state);
 check(model.religion_count()==0&&model.religion_created_by_player(0)==-1&&!model.eligible_for_founder_benefits(-1,0),"Known empty list native query failed");
 auto &list=state.world.religion_culture.religions;
 list={entry(0,0,true),entry(1,0),entry(2,0,false,2),entry(1,1)};model=BnwFounderReligionReadModel::bind(state);
 check(model.religion_created_by_player(0)==2&&model.religion(1)->founder==0,"Native last-created/first-match ordering changed");
 check(model.religion(0)==nullptr&&model.religion(0,0)&&!model.eligible_for_founder_benefits(0,0),"Pantheon player-specific GetReligion changed");
 check(model.eligible_for_founder_benefits(2,0)&&!model.eligible_for_founder_benefits(2,1),"Wrapped holy plot ownership/founder eligibility changed");
 list={entry(1,0,false,0,-1)};model=BnwFounderReligionReadModel::bind(state);check(!model.eligible_for_founder_benefits(1,0),"Out-of-map holy plot became eligible");
 list={entry(1,0),entry(1,1)};model=BnwFounderReligionReadModel::bind(state);check(!model.eligible_for_founder_benefits(1,1),"Duplicate GetReligion stopped using first record");
 auto unknown=state;unknown.world.religion_culture.religions_version.reset();rejects([&]{BnwFounderReligionReadModel::bind(unknown);},"Missing religion version became empty religion");
 unknown=state;unknown.world.map_projection->cells[0].owner.reset();rejects([&]{BnwFounderReligionReadModel::bind(unknown);},"Unknown holy plot owner became false eligibility");
 unknown=state;unknown.world.map_projection->cells[0].owner=1;rejects([&]{BnwFounderReligionReadModel::bind(unknown);},"Raw/projected ownership mismatch accepted");
 unknown=state;unknown.world.religion_culture.culture_offset=unknown.world.religion_culture.start_offset;rejects([&]{BnwFounderReligionReadModel::bind(unknown);},"Unknown religion list boundary accepted");
 FixtureOwner owner;const auto query=[&]{return quote_bnw_happiness_from_religion(state,owner.profile,owner.identity,owner,0);};
 list.clear();owner.missing=true;check(!query().founder_resolved,"Empty native religion list bypassed mandatory Lua owner");owner.missing=false;
 owner.attest=false;check(!query().founder_resolved,"Empty VM became a verified stock epoch");owner.attest=true;
 auto zero=query();check(zero.founder_resolved&&zero.happiness_quoted&&zero.native_fallback_used&&zero.founder_religion==-1&&zero.happiness==0&&zero.epoch,"Known empty native fallback failed");
 list={entry(1,0)};auto founded=query();check(founded.founder_resolved&&founded.founder_religion==1&&!founded.happiness_quoted&&!founded.happiness&&founded.epoch,"Founded belief happiness was silently zeroed");
 owner.change_ownership=true;auto lost=query();check(lost.happiness_quoted&&lost.happiness==0&&lost.founder_religion==-1&&*state.world.map_projection->cells[0].owner==0,"Founder fallback used original map instead of same Lua fork");owner.change_ownership=false;
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function(p)assert(p==0);return -1 end)";auto overridden=query();check(overridden.happiness_quoted&&!overridden.native_fallback_used&&overridden.founder_religion==-1,"Explicit Lua NO_RELIGION did not override native eligible religion");
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function(p)assert(p==0);return 0 end)";list={entry(0,0,true)};auto pantheon_quote=query();check(pantheon_quote.founder_religion==0&&pantheon_quote.happiness_quoted&&pantheon_quote.happiness==0,"Lua zero override fell back or found another player's pantheon");
 owner.script="GameEvents.GetFounderBenefitsReligion.Add(function()return 1234 end)";auto absent=query();check(absent.founder_religion==1234&&absent.happiness_quoted&&absent.happiness==0,"Native GetReligion-null branch changed");
 check(owner.publishes==0&&state.setup.game_turn==19,"Native query published world/VM effects");
}
void selected_check(const char *save,const char *game_root){
 auto selected=ui::bootstrap_selected_bnw_preview(save,game_root,0);if(!selected.preview)throw std::runtime_error(selected.preview.blocker->reason);
 const auto &state=selected.preview.host->session().state();auto model=BnwFounderReligionReadModel::bind(state);
 check(selected.profile.enabled_dlc.size()==14&&selected.profile.enabled_mods.empty()&&model.religion_count()==0,"Selected Gandhi religion/profile changed");
 for(const auto &[id,player]:state.setup.players)check(model.religion_created_by_player(id)==-1&&!model.eligible_for_founder_benefits(-1,id),"Selected Gandhi native empty list differs");
 FixtureOwner unavailable;unavailable.profile=selected.profile;unavailable.identity=selected.preview.host->identity();unavailable.missing=true;
 auto quote=quote_bnw_happiness_from_religion(state,selected.profile,unavailable.identity,unavailable,0);
 check(!quote.founder_resolved&&!quote.happiness_quoted&&!quote.epoch&&!selected.preview.host->session().readiness().can_continue(),"Selected Gandhi unknown Lua lifecycle became happy/continuable");
 std::cout<<"Selected Gandhi native read model PASS: zero religions; mandatory Lua remains blocked\n";
}
}
int main(int argc,char **argv){check(argc==1||argc==3,"Expected no args or selected save/game root");fixture_checks();if(argc==3)selected_check(argv[1],argv[2]);std::cout<<"PASS: native founder and religion-happiness contract fixtures; no publication\n";}
