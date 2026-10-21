#include "civ5/runtime/bnw_unit_kill_frontier.hpp"
#include "civ5/runtime/bnw_unit_can_save_buffer.hpp"
#include "civ5/runtime/bnw_unit_can_save_lua_adapter.hpp"
#include "civ5/content/info_types.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW kill frontier was accepted");
}
void scalar(saves::PlayerCoreObservation &core,const char *name,
            std::int64_t value) {
    saves::PlayerCoreField field{};
    field.name=name;field.encoding=saves::PlayerCoreEncoding::i32;
    field.values={value};core.fields.push_back(std::move(field));
}
std::int64_t read(const saves::PlayerCoreObservation &core,
                  std::string_view wanted) {
    for(const auto &field:core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values[0];
    throw std::runtime_error("Synthetic BNW player counter missing");
}
std::int64_t &counter(saves::PlayerCoreObservation &core,
                      std::string_view wanted) {
    for(auto &field:core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values[0];
    throw std::runtime_error("Synthetic BNW player counter missing");
}
void unit_scalar(saves::SavedUnitObservation &unit,const char *name,
                 saves::UnitCoreEncoding encoding,std::int64_t value) {
    saves::UnitCoreField field{};
    field.name=name;field.encoding=encoding;field.values={value};
    unit.core.fields.push_back(std::move(field));
}
saves::UnitTailNode tail_scalar(const char *name,const char *encoding,
                                std::int64_t value) {
    saves::UnitTailNode node{};
    node.name=name;node.encoding=encoding;node.values={value};return node;
}
saves::SavedSessionSnapshot world(std::uint32_t unit_hash,
                                  std::uint32_t class_hash) {
    saves::SavedSessionSnapshot source{};
    source.game_state.layout=bnw;source.map_header.layout=bnw;
    source.map_header.grid={1,1};
    source.map_plots.emplace();source.map_plots->layout=bnw;
    source.map_plots->plots.emplace_back();
    source.map_plots->plots[0].fields.push_back({"area",0});
    source.map_plots->plots[0].arrays.push_back({"units",2,{0,0}});
    source.map_projection.emplace();source.map_projection->grid={1,1};
    source.map_projection->cells.emplace_back();
    source.map_projection->cells[0].units=
        std::vector<saves::SavedPlotUnitReference>{{0,0}};
    source.map_regions.emplace();source.map_regions->layout=bnw;
    source.map_regions->containers.emplace_back();
    auto &areas=source.map_regions->containers.back();
    areas.name="areas";areas.wire_count=1;areas.records.emplace_back();
    auto &area=areas.records.back();
    area.fields={{"id",0},{"units",1}};
    auto counts=std::vector<std::int64_t>(64);counts[0]=1;
    area.arrays.push_back({"units_per_player",1,std::move(counts)});
    source.pregame.emplace();source.pregame->layout=bnw;
    source.pregame->archive.integer_arrays["team_types"]=
        std::vector<std::int64_t>(64,0);
    source.teams.emplace();source.teams->layout=bnw;
    source.teams->teams.resize(64);
    for(std::size_t t=0;t<64;++t) source.teams->teams[t].index=t;
    saves::TeamHashedArray hashed{};
    hashed.name="unit_class_count";hashed.wire_count=1;
    hashed.rows.push_back({class_hash,{1}});
    source.teams->teams[0].hashes.push_back(std::move(hashed));
    source.all_players.emplace();source.all_players->layout=bnw;
    source.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        auto &player=source.all_players->players[p];
        player.index=p;player.saved_player_id=p;
        player.units.layout=bnw;player.units.header.layout=bnw;
    }
    auto &player=source.all_players->players[0];player.core.layout=bnw;
    saves::PlayerCoreField classes{};
    classes.name="m_paiUnitClassCount";
    classes.encoding=saves::PlayerCoreEncoding::vector_i32;
    classes.wire_count=1;classes.values={1};
    player.core.fields.push_back(std::move(classes));
    scalar(player.core,"m_iNumBuilders",1);
    scalar(player.core,"m_iUnhappinessFromUnits",0);
    scalar(player.core,"m_iExtraUnitCost",0);
    scalar(player.core,"m_iNumNukeUnits",0);
    scalar(player.core,"m_iNumMilitaryUnits",0);
    scalar(player.core,"m_iCombatExperience",0);
    scalar(player.core,"m_iNavalCombatExperience",0);
    scalar(player.core,"m_iLifetimeCombatExperience",20);
    auto &units=player.units;
    units.header.source_slots=1;
    units.header.fields={{"num_slots",1},{"last_index",0},
        {"free_list_head",-1},{"free_list_count",0},{"current_id",8192}};
    units.header.next_free_indices={-1};units.header.wire_unit_count=1;
    units.units.emplace_back();
    auto &unit=units.units.back();unit.core.layout=bnw;unit.tail.layout=bnw;
    unit_scalar(unit,"m_iID",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eOwner",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eOriginalOwner",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_iX",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_iY",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eUnitType",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"idUnitType",saves::UnitCoreEncoding::hash32,unit_hash);
    unit_scalar(unit,"m_iExperience",saves::UnitCoreEncoding::i32,20);
    unit_scalar(unit,"m_iArmyId",saves::UnitCoreEncoding::i32,-1);
    unit_scalar(unit,"m_eCapturingPlayer",saves::UnitCoreEncoding::i32,-1);
    unit_scalar(unit,"m_bEmbarked",saves::UnitCoreEncoding::boolean,0);
    unit_scalar(unit,"m_bCapturedAsIs",saves::UnitCoreEncoding::boolean,0);
    saves::UnitTailNode religion{};
    religion.name="religion";religion.encoding="record";
    religion.children.push_back(tail_scalar("version","u32",2));
    religion.children.push_back(tail_scalar("religion","i32",-1));
    religion.children.push_back(tail_scalar("strength","i32",0));
    religion.children.push_back(tail_scalar("spreads_left","i32",0));
    unit.tail.blocks.push_back(std::move(religion));
    return source;
}
void test() {
    const auto unit_hash=content::saved_info_type_hash("UNIT_WORKER");
    const auto class_hash=content::saved_info_type_hash("UNITCLASS_WORKER");
    runtime::BnwUnitClassDeathRules classes;
    classes.class_slots=1;classes.class_hashes={class_hash};
    classes.units.emplace(0,runtime::BnwUnitClassBinding{
        0,0,unit_hash,class_hash});
    runtime::BnwUnitSupportRules support;
    support.units.emplace(0,runtime::BnwUnitSupportRule{
        0,unit_hash,true,false,100,0,0,-1});
    runtime::BnwDeathXpRules xp_rules;
    xp_rules.loss_percent=50;
    xp_rules.units.emplace(0,runtime::BnwDeathXpUnitRule{
        0,unit_hash,runtime::BnwDeathXpDomain::land});
    auto source=world(unit_hash,class_hash);
    const auto frontier=runtime::plan_bnw_unit_kill_frontier(
        source,0,0,1,classes,support,xp_rules);
    check(!frontier.can_commit_kill && !frontier.entry.can_advance &&
          frontier.entry.requires_script_system_query &&
          frontier.entry.requires_can_save_unit &&
          frontier.entry.requires_selected_ui_query &&
          !frontier.entry.requires_army_service &&
          frontier.entry.blocker==runtime::BnwUnitKillEntryBlocker::
              script_system_and_can_save_unit_unresolved &&
          frontier.entry.lua_arguments.owner==0 &&
          frontier.entry.lua_arguments.unit_id==0 &&
          frontier.entry.lua_arguments.unit_type==0 &&
          frontier.entry.lua_arguments.x==0 &&
          frontier.entry.lua_arguments.y==0 &&
          !frontier.entry.lua_arguments.delayed &&
          frontier.entry.lua_arguments.killer==1 &&
          !frontier.capture.capture_valid &&
          frontier.xp.loss==10 && frontier.support_death.builder_delta==-1,
          "BNW kill frontier did not compose SDK-ordered stages");
    {
        using runtime::BnwCanSaveOutcome;
        using runtime::BnwCanSaveScriptSystem;
        using runtime::BnwCanSaveStageEvidence;
        const auto unresolved=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,{});
        check(unresolved.outcome==BnwCanSaveOutcome::blocked &&
              unresolved.pending_effects.empty() &&
              !unresolved.can_commit_kill,
              "BNW CanSaveUnit accepted missing detached Lua stage");
        bool called=false;
        const auto absent=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,
            [&](const runtime::BnwUnitKillLuaArguments &args,auto &) {
                called=true;
                check(args.owner==0 && args.unit_id==0 && args.killer==1,
                      "BNW CanSaveUnit argument order changed");
                BnwCanSaveStageEvidence evidence;
                evidence.script_system=BnwCanSaveScriptSystem::absent;
                return evidence;
            });
        check(called && absent.outcome==
              BnwCanSaveOutcome::proceed_to_selected_ui &&
              absent.pending_effects.empty() && !absent.can_commit_kill,
              "BNW absent script system did not skip CallTestAll");
        const auto uncertain=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,
            [](const auto &,auto &effects) {
                effects.push_back({"world","partial",{}});
                return BnwCanSaveStageEvidence{};
            });
        check(uncertain.outcome==BnwCanSaveOutcome::blocked &&
              uncertain.pending_effects.empty(),
              "BNW CanSaveUnit accepted uncertain script-system state");
        const auto inconsistent_absent=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,
            [](const auto &,auto &) {
                BnwCanSaveStageEvidence evidence;
                evidence.script_system=BnwCanSaveScriptSystem::absent;
                evidence.test_all_completed=true;
                return evidence;
            });
        check(inconsistent_absent.outcome==BnwCanSaveOutcome::blocked,
              "BNW absent script system accepted a reported Lua call");
        const auto stage=[&](std::optional<bool> answer) {
            return [answer](const runtime::BnwUnitKillLuaArguments &args,
                            std::vector<runtime::BnwCanSaveEffectIntent> &effects) {
                check(args.owner==0 && args.unit_id==0,
                      "BNW CanSaveUnit called with wrong unit");
                effects.push_back({"world","script_counter",{std::int64_t{1}}});
                BnwCanSaveStageEvidence evidence;
                evidence.script_system=BnwCanSaveScriptSystem::present;
                evidence.selected_entrypoints_loaded=true;
                evidence.test_all_completed=true;
                evidence.all_effects_buffered=true;
                evidence.test_all.value=answer;
                return evidence;
            };
        };
        for(const auto answer:{std::optional<bool>{true},
                               std::optional<bool>{false},
                               std::optional<bool>{}}) {
            const auto result=runtime::stage_bnw_can_save_unit(
                source,frontier.entry,stage(answer));
            check(result.outcome==(answer.value_or(false)?
                    BnwCanSaveOutcome::cancel_kill:
                    BnwCanSaveOutcome::proceed_to_selected_ui) &&
                  result.pending_effects.size()==1 &&
                  !result.can_commit_kill,
                  "BNW CanSaveUnit lost cancel/no-listener/effect semantics");
        }
        const auto incomplete=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,
            [](const auto &,auto &effects) {
                effects.push_back({"world","partial",{}});
                BnwCanSaveStageEvidence evidence;
                evidence.script_system=BnwCanSaveScriptSystem::present;
                evidence.selected_entrypoints_loaded=true;
                evidence.test_all_completed=true;
                evidence.test_all.value=false;
                return evidence;
            });
        check(incomplete.outcome==BnwCanSaveOutcome::blocked &&
              incomplete.pending_effects.empty(),
              "BNW CanSaveUnit published unconfirmed partial effects");
        const auto failed=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,
            [](const auto &,auto &effects) {
                effects.push_back({"world","partial",{}});
                BnwCanSaveStageEvidence evidence;
                evidence.script_system=BnwCanSaveScriptSystem::present;
                evidence.selected_entrypoints_loaded=true;
                evidence.test_all_completed=true;
                evidence.all_effects_buffered=true;
                evidence.test_all.errors={"handler failed"};
                return evidence;
            });
        check(failed.outcome==BnwCanSaveOutcome::blocked &&
              failed.pending_effects.empty() &&
              failed.reason.find("handler failed")!=std::string::npos,
              "BNW CanSaveUnit ignored Lua callback failure");
        const auto thrown=runtime::stage_bnw_can_save_unit(
            source,frontier.entry,
            [](const auto &,auto &effects)->BnwCanSaveStageEvidence {
                effects.push_back({"world","partial",{}});
                throw std::runtime_error("stage threw");
            });
        check(thrown.outcome==BnwCanSaveOutcome::blocked &&
              thrown.pending_effects.empty() &&
              thrown.reason.find("stage threw")!=std::string::npos,
              "BNW CanSaveUnit did not roll back thrown stage");
        auto tampered=frontier.entry;
        tampered.lua_arguments.unit_id=123;
        called=false;
        const auto stale=runtime::stage_bnw_can_save_unit(
            source,tampered,
            [&](const auto &,auto &)->BnwCanSaveStageEvidence {
                called=true;return {};
            });
        check(stale.outcome==BnwCanSaveOutcome::blocked && !called,
              "BNW CanSaveUnit invoked Lua on stale unit identity");
        struct IsolatedLua final:runtime::BnwCanSaveUnitLuaEscrow {
            std::string script;
            BnwCanSaveScriptSystem presence=BnwCanSaveScriptSystem::present;
            bool capture_complete=true;
            int forks=0;
            BnwCanSaveScriptSystem script_system(
                const saves::SavedSessionSnapshot &,
                const runtime::BnwUnitKillEntry &) override {
                return presence;
            }
            std::unique_ptr<runtime::LuaScriptHost> fork_exact_detached(
                const saves::SavedSessionSnapshot &,
                const runtime::BnwUnitKillEntry &) override {
                ++forks;
                auto host=std::make_unique<runtime::LuaScriptHost>();
                host->execute("gameplay",script,"=isolated-can-save.lua");
                host->complete_loading();
                return host;
            }
            std::optional<std::vector<runtime::BnwCanSaveEffectIntent>>
            collect_all_effects(runtime::LuaScriptHost &) override {
                if(!capture_complete) return std::nullopt;
                return std::vector<runtime::BnwCanSaveEffectIntent>{};
            }
        };
        const auto no_escrow=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,nullptr);
        check(no_escrow.resolution.outcome==BnwCanSaveOutcome::blocked &&
              !no_escrow.detached_host,
              "BNW live Lua host bypassed exact detached escrow");
        IsolatedLua isolated;
        isolated.script=R"lua(
            seen=0
            GameEvents.CanSaveUnit.Add(function(owner,id)
                assert(owner==0 and id==0)
                seen=seen+1
                return true
            end)
            GameEvents.Check.Add(function() return seen==1 end)
        )lua";
        auto saved=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,&isolated);
        check(saved.resolution.outcome==BnwCanSaveOutcome::cancel_kill &&
              saved.detached_host &&
              saved.detached_host->test_all("Check").value==true &&
              !saved.resolution.can_commit_kill,
              "BNW detached Lua true did not cancel/retain VM state");
        isolated.script=R"lua(
            GameEvents.CanSaveUnit.Add(function() return false end)
        )lua";
        auto not_saved=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,&isolated);
        check(not_saved.resolution.outcome==
                BnwCanSaveOutcome::proceed_to_selected_ui &&
              not_saved.detached_host && !not_saved.resolution.can_commit_kill,
              "BNW detached Lua false did not continue to UI boundary");
        isolated.script="GameEvents.CanSaveUnit.Add(function() error('veto failed') end)";
        auto lua_error=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,&isolated);
        check(lua_error.resolution.outcome==BnwCanSaveOutcome::blocked &&
              !lua_error.detached_host &&
              lua_error.resolution.reason.find("veto failed")!=std::string::npos,
              "BNW detached Lua error retained uncertain VM state");
        isolated.script="GameEvents.CanSaveUnit.Add(function() return false end)";
        isolated.capture_complete=false;
        auto uncaptured=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,&isolated);
        check(uncaptured.resolution.outcome==BnwCanSaveOutcome::blocked &&
              !uncaptured.detached_host,
              "BNW Lua host accepted incomplete external-effect capture");
        isolated.capture_complete=true;
        isolated.presence=BnwCanSaveScriptSystem::absent;
        const auto before_absent=isolated.forks;
        auto no_script=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,&isolated);
        check(no_script.resolution.outcome==
                BnwCanSaveOutcome::proceed_to_selected_ui &&
              !no_script.detached_host && isolated.forks==before_absent,
              "BNW absent script system unexpectedly forked a Lua VM");
        isolated.presence=BnwCanSaveScriptSystem::unknown;
        auto unknown_script=runtime::stage_bnw_can_save_unit_lua_host(
            source,frontier.entry,&isolated);
        check(unknown_script.resolution.outcome==BnwCanSaveOutcome::blocked &&
              !unknown_script.detached_host &&
              isolated.forks==before_absent,
              "BNW unknown script system proceeded without Lua evidence");
    }
    auto result=runtime::project_bnw_unit_kill_frontier_diagnostic(
        source,frontier,classes,support,xp_rules);
    const auto &owner=result.all_players->players[0];
    check(read(owner.core,"m_iNumBuilders")==0 &&
          read(owner.core,"m_iLifetimeCombatExperience")==10 &&
          owner.core.fields[0].values[0]==0 &&
          result.teams->teams[0].hashes[0].rows[0].values[0]==0 &&
          owner.units.units.empty() && owner.units.header.wire_unit_count==0 &&
          result.map_plots->plots[0].arrays[0].values.empty() &&
          result.map_projection->cells[0].units->empty() &&
          result.map_regions->containers[0].records[0].fields[1].value==0,
          "BNW kill frontier detached projection omitted a proved domain");
    check(read(source.all_players->players[0].core,"m_iNumBuilders")==1 &&
          read(source.all_players->players[0].core,
               "m_iLifetimeCombatExperience")==20 &&
          source.all_players->players[0].units.units.size()==1,
          "BNW kill frontier mutated source");
    auto stale=frontier;stale.support_death.before[0]++;
    rejects([&]{(void)runtime::project_bnw_unit_kill_frontier_diagnostic(
        source,stale,classes,support,xp_rules);});
    stale=frontier;stale.can_commit_kill=true;
    rejects([&]{(void)runtime::project_bnw_unit_kill_frontier_diagnostic(
        source,stale,classes,support,xp_rules);});
    stale=frontier;stale.entry.lua_arguments.unit_id=7;
    rejects([&]{(void)runtime::project_bnw_unit_kill_frontier_diagnostic(
        source,stale,classes,support,xp_rules);});
    stale=frontier;stale.entry.can_advance=true;
    rejects([&]{(void)runtime::project_bnw_unit_kill_frontier_diagnostic(
        source,stale,classes,support,xp_rules);});
    for(auto &field:source.all_players->players[0].units.units[0].core.fields)
        if(field.name && std::string_view(field.name)=="m_iArmyId")
            field.values[0]=3;
    const auto army=runtime::plan_bnw_unit_kill_frontier(
        source,0,0,1,classes,support,xp_rules);
    check(army.entry.requires_army_service && !army.entry.can_advance,
          "BNW kill entry ignored live army-removal dependency");
    for(auto &field:source.all_players->players[0].units.units[0].core.fields)
        if(field.name && std::string_view(field.name)=="m_iArmyId")
            field.values[0]=-1;
    counter(source.all_players->players[0].core,"m_iCombatExperience")=15;
    const auto threshold=runtime::plan_bnw_unit_kill_frontier(
        source,0,0,1,classes,support,xp_rules);
    check(threshold.xp.setter_branch_unported,
          "BNW kill frontier missed XP setter branch");
    rejects([&]{(void)runtime::project_bnw_unit_kill_frontier_diagnostic(
        source,threshold,classes,support,xp_rules);});
    std::cout << "PASS BNW detached kill frontier composition\n";
}
} // namespace
int main() {
    try {test();return 0;}
    catch(const std::exception &error) {
        std::cerr << "FAIL BNW kill frontier: " << error.what() << '\n';
        return 1;
    }
}
