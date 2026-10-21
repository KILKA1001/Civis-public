#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include "civ5/runtime/bnw_barbarian_tech_grant.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
using Status=BnwBarbarianTeamHasTechStatus;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
void synthetic() {
    TeamTechnologies unloaded_tech(content::TechnologyProfile::bnw,63,2);
    LuaScriptHost unloaded;
    check(write_bnw_barbarian_team_has_tech(unloaded_tech,0,&unloaded).status==
              Status::rejected && !unloaded_tech.has_tech(0),
          "Unloaded BNW Lua changed the technology bit");
    check(write_bnw_barbarian_team_has_tech(unloaded_tech,0,nullptr).status==
              Status::rejected && !unloaded_tech.has_tech(0),
          "Missing BNW Lua changed the technology bit");
    check(write_bnw_barbarian_team_has_tech(unloaded_tech,2,&unloaded).status==
              Status::rejected && !unloaded_tech.has_tech(0),
          "Out-of-range BNW technology changed the team");

    LuaScriptHost no_listener;
    no_listener.complete_loading();
    const auto absent=write_bnw_barbarian_team_has_tech(
        unloaded_tech,0,&no_listener);
    check(absent.status==Status::committed && !absent.hook_handled &&
          unloaded_tech.has_tech(0) && unloaded_tech.last_tech()==0,
          "No-listener TeamSetHasTech did not commit source bit and last tech");
    check(write_bnw_barbarian_team_has_tech(unloaded_tech,0,&no_listener).status==
              Status::rejected,
          "Already-owned technology invoked TeamSetHasTech twice");

    TeamTechnologies failed_tech(content::TechnologyProfile::bnw,63,2);
    LuaScriptHost failed;
    failed.execute("gameplay",R"lua(
        GameEvents.TeamSetHasTech.Add(function(team,tech,owned)
            assert(team==63 and tech==0 and owned==true)
            error('expected hook failure')
        end)
    )lua","=failed-team-tech.lua");
    failed.complete_loading();
    const auto uncertain=write_bnw_barbarian_team_has_tech(failed_tech,0,&failed);
    check(uncertain.status==Status::uncertain && !uncertain.reason.empty() &&
          failed_tech.has_tech(0) && failed_tech.last_tech()==0,
          "Failed Lua callback was rolled back or incorrectly reported committed");
}
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &source=*bootstrap.preview.host;
    auto state=source.session().state(); // Detached: this is not a full tech grant.
    const auto gate=plan_bnw_barbarian_technology_gate(
        state,source.technologies(),source.defines());
    check(gate.grants.size()==1 && gate.grants[0].technology==0 &&
          gate.grants[0].type=="TECH_AGRICULTURE" &&
          !state.technologies[63].has_tech(0),
          "Selected Gandhi barbarian technology changed");

    ScriptGameplayProvider gameplay;
    gameplay.query=[](std::string_view,std::span<const ScriptValue>)
        ->std::unique_ptr<ScriptQuery>{return nullptr;};
    gameplay.ids=[](bool teams){
        std::vector<std::int64_t> ids;
        for(std::int64_t id=0;id<64;++id) ids.push_back(id);
        (void)teams;
        return ids;
    };
    gameplay.has_tech=[&](std::int64_t team,std::int64_t tech){
        return state.technologies.at(static_cast<std::size_t>(team)).has_tech(tech);
    };
    gameplay.player_team=[&](std::int64_t player){
        return state.setup.players.at(player).team;
    };
    gameplay.player_alive=[&](std::int64_t player){
        return state.setup.players.at(player).alive;
    };
    gameplay.info_types=[](){return std::vector<std::pair<std::string,std::int64_t>>{};};
    LuaScriptHost lua({}, {}, {},std::move(gameplay));
    lua.execute("gameplay",R"lua(
        called=0
        GameEvents.TeamSetHasTech.Add(function(team,tech,owned)
            assert(team==63 and tech==0 and owned==true)
            assert(Teams[63]:IsHasTech(0)) -- callback follows the bit write
            called=called+1
            return false -- CallHook result cannot veto the source write
        end)
        GameEvents.Verify.Add(function() return called==1 end)
    )lua","=gandhi-team-tech.lua");
    lua.complete_loading();
    const auto result=write_bnw_barbarian_team_has_tech(
        state.technologies[63],0,&lua);
    check(result.status==Status::committed && result.hook_handled &&
          state.technologies[63].has_tech(0) &&
          state.technologies[63].last_tech()==0 &&
          lua.test_all("Verify").value==true,
          "Selected Gandhi TeamSetHasTech order or Lua arguments changed");
    check(!source.session().state().technologies[63].has_tech(0),
          "Selected source session was mutated by detached low-level grant");
    std::cout<<"selected BNW low-level TeamSetHasTech TECH_AGRICULTURE: hook observed owned bit\n";
}
} // namespace

namespace { void selected_major_prefix(const std::filesystem::path&,const std::filesystem::path&); }
int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    synthetic();
    if(argc==3){selected(argv[1],argv[2]);selected_major_prefix(argv[1],argv[2]);}
}


namespace {
// This is a real CvTeam::setHasTech prefix command over the selected mutable
// Session, not a detached TeamTechs flag edit or synthetic Lua-positive fixture.
void selected_major_prefix(const std::filesystem::path& save,const std::filesystem::path& root){
 auto boot=ui::bootstrap_selected_bnw_preview(save,root,0);
 check(boot.preview.host&&!boot.preview.blocker,"Actual major prefix selected Host unavailable");
 ui::SelectedBnwReadOnlyPreview preview(std::move(boot.preview));
 const auto lease=preview.session().read_state_lease();const auto generation=preview.session().state_generation();
 const auto revision=lease->ui_source_revision;
 const auto rng=lease->random.gameplay.state(),map_rng=lease->random.map.state();
 BnwMajorTeamHasTechCommand cmd;cmd.player=0;cmd.team=lease->setup.players.at(0).team;
 cmd.expected_turn=lease->setup.game_turn;cmd.expected_generation=generation;cmd.expected_ui_source_revision=revision;
 std::unique_ptr<TeamTechnologies> baseline;
 preview.with_owned_host_camera_input([&](auto h,const auto*){
  for(const auto& team:lease->technologies)if(team.team()==cmd.team){baseline=std::make_unique<TeamTechnologies>(team);
   for(std::uint64_t id=0;id<h->technologies().collection.slots;++id){const auto* info=h->technologies().find(id);
    if(info&&info->base.type!="TECH_SATELLITES"&&!info->booleans.at("Repeat")&&!team.has_tech(static_cast<std::int64_t>(id))){cmd.technology=static_cast<std::int64_t>(id);break;}}}
 });
 if(cmd.technology<0){std::cout<<"UNAVAILABLE actual missing nonrepeat major technology\n";return;}
 const auto refuse=[&](const BnwMajorTeamHasTechCommand& request){bool denied=false;
  try{(void)BnwMajorTeamHasTechPending::begin_nonrepeat_acquisition(preview,request);}catch(const BnwMajorTeamHasTechUnavailable&){denied=true;}
  check(denied,"Major stale/overlap request admitted");};
 auto stale=cmd;stale.expected_ui_source_revision=revision+1;refuse(stale);
 check(lease->ui_source_revision==revision,"Rejected major command wrote revision");
 const auto pending=BnwMajorTeamHasTechPending::begin_nonrepeat_acquisition(preview,cmd);
 check(preview.session().read_state_lease()==lease&&preview.session().state_generation()==generation&&
  lease->ui_source_revision==revision+1,"Major prefix replaced actual world/generation or omitted revision");
 check(pending->phase()==BnwMajorTeamHasTechPhase::team_set_has_tech_pending,"Major prefix skipped mandatory Lua");
 auto expected=*baseline;check(expected.set_has_tech(cmd.technology,true),"Actual baseline already owned chosen technology");
 preview.with_owned_host_camera_input([&](auto h,const auto*){
  pending->require_stored_current(*h);
  bool seen=false;for(const auto& raw:lease->world.teams->teams){
   bool matching=false;for(const auto& field:raw.fields)if(field.name&&std::string_view(field.name)=="id")matching=field.value==cmd.team;
   if(!matching)continue;seen=true;
   const auto decoded=restore_team_technologies(raw,h->technologies(),h->types());
   check(decoded.nondefault_values()==expected.nondefault_values()&&decoded.last_tech()==cmd.technology,
    "Actual major raw flags/lastTech differ from expected native prefix");
  }check(seen,"Actual major raw team missing after command");
  for(const auto& team:lease->technologies)if(team.team()==cmd.team)
   check(team.nondefault_values()==expected.nondefault_values()&&team.last_tech()==cmd.technology,
    "Actual major owned flags differ from raw native prefix");
 });
 check(lease->random.gameplay.state()==rng&&lease->random.map.state()==map_rng&&lease->setup.game_turn==cmd.expected_turn,
  "Paired major native prefix changed RNG or game turn");
 auto overlap=cmd;overlap.expected_ui_source_revision=revision+1;refuse(overlap);
 check(!preview.session().readiness().can_continue(),"Major prefix falsely granted full continuation");
 std::cout<<"PASS actual major paired HasTech/lastTech prefix; UNAVAILABLE actual SAMEVM TeamSetHasTech and city/resource/process/Traits continuation\n";
}
}
