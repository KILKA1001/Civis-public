#include "civ5/runtime/bnw_sight_research_clear.hpp"
#include "civ5/runtime/bnw_sight_first_contact.hpp"
#include "civ5/runtime/bnw_sight_meet_effect_plan.hpp"
#include "civ5/runtime/bnw_team_meet_lua_quote.hpp"
#include "civ5/runtime/bnw_meet_proximity_stage.hpp"
#include "civ5/runtime/bnw_player_proximity_checkpoint.hpp"
#include "civ5/runtime/bnw_team_meet_checkpoint.hpp"
#include "civ5/runtime/bnw_first_contact_military_preflight.hpp"
#include "civ5/runtime/bnw_first_contact_economic_preflight.hpp"
#include "civ5/runtime/bnw_first_contact_posture_preflight.hpp"
#include "civ5/runtime/bnw_first_contact_expansion_preflight.hpp"
#include "civ5/runtime/bnw_first_contact_target_preflight.hpp"
#include "civ5/runtime/bnw_first_contact_relationship_preflight.hpp"
#include "civ5/runtime/bnw_first_contact_approach_preflight.hpp"
#include "civ5/runtime/bnw_diplomacy_scratchpad_load.hpp"
#include "civ5/runtime/bnw_first_contact_approach_after_load.hpp"
#include "civ5/runtime/bnw_first_contact_approach_random_quote.hpp"
#include "civ5/runtime/bnw_first_contact_rng_stage.hpp"
#include "civ5/runtime/bnw_first_contact_war_decision.hpp"
#include "civ5/runtime/bnw_first_contact_approach_war_gate.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t unit(const saves::SavedUnitObservation &value,std::string_view key) {
    for(const auto &f:value.core.fields)
        if(f.name && key==f.name && f.values.size()==1) return f.values[0];
    throw std::runtime_error("BNW meet selected unit field absent");
}
void set_unit(saves::SavedUnitObservation &value,std::string_view key,
              std::int64_t changed) {
    for(auto &f:value.core.fields)
        if(f.name && key==f.name && f.values.size()==1) {
            f.values[0]=changed;return;
        }
    throw std::runtime_error("BNW meet selected unit field absent for write");
}
std::vector<std::int64_t> &plot_units(saves::PlotObservation &plot) {
    for(auto &a:plot.arrays)
        if(a.name && std::string_view(a.name)=="units" && a.columns==2)
            return a.values;
    throw std::runtime_error("BNW meet plot roster absent");
}
std::vector<std::int64_t> &team_array(saves::TeamObservation &team,
                                      std::string_view key) {
    for(auto &a:team.arrays)
        if(a.name && key==a.name) return a.values;
    throw std::runtime_error("BNW meet team array absent");
}
const std::vector<std::int64_t> &team_array(
    const saves::TeamObservation &team,std::string_view key) {
    for(const auto &a:team.arrays)
        if(a.name && key==a.name) return a.values;
    throw std::runtime_error("BNW meet team array absent");
}
BnwSetXYRequest candidate(const LiveSessionState &state) {
    const auto &grid=state.world.map_header.grid;
    for(const auto &player:state.world.all_players->players)
        for(const auto &value:player.units.units) {
            const auto x=unit(value,"m_iX"),y=unit(value,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                BnwSetXYRequest request{unit(value,"m_eOwner"),
                                         unit(value,"m_iID"),x+dx,y+dy};
                if(preflight_bnw_unit_sight(state,request).candidate)
                    return request;
            }
        }
    throw std::runtime_error("Selected Gandhi has no BNW sight candidate");
}
bool alive(const saves::SavedPlayerObservation &player) {
    for(const auto &f:player.core.fields)
        if(f.name && std::string_view(f.name)=="m_bAlive" &&
           f.values.size()==1) return f.values[0]==1;
    throw std::runtime_error("BNW meet player alive field absent");
}
std::int64_t proximity(const saves::SavedPlayerObservation &player,
                       std::int64_t other) {
    for(const auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_aiProximityToPlayer" &&
           other>=0 && static_cast<std::size_t>(other)<field.values.size())
            return field.values[static_cast<std::size_t>(other)];
    throw std::runtime_error("BNW meet selected proximity field absent");
}
void set_proximity(saves::SavedPlayerObservation &player,
                   std::int64_t other,std::int64_t value) {
    for(auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_aiProximityToPlayer" &&
           other>=0 && static_cast<std::size_t>(other)<field.values.size()) {
            field.values[static_cast<std::size_t>(other)]=value;return;
        }
    throw std::runtime_error("BNW meet selected proximity field absent for write");
}
std::int64_t player_scalar(const saves::SavedPlayerObservation &player,
                           std::string_view key) {
    for(const auto &field:player.core.fields)
        if(field.name && key==field.name && field.values.size()==1)
            return field.values[0];
    throw std::runtime_error("BNW selected player core scalar absent");
}
void set_player_scalar(saves::SavedPlayerObservation &player,
                       std::string_view key,std::int64_t value) {
    for(auto &field:player.core.fields)
        if(field.name && key==field.name && field.values.size()==1) {
            field.values[0]=value;return;
        }
    throw std::runtime_error("BNW selected player core scalar absent for write");
}
void move_roster(LiveSessionState &copy,std::int64_t owner,std::int64_t id,
                 std::uint64_t old_plot,std::uint64_t new_plot) {
    auto &old_raw=plot_units(copy.world.map_plots->plots.at(old_plot));
    auto &old_projection=*copy.world.map_projection->cells.at(old_plot).units;
    auto &new_raw=plot_units(copy.world.map_plots->plots.at(new_plot));
    auto &new_projection=*copy.world.map_projection->cells.at(new_plot).units;
    bool erased=false;
    for(std::size_t i=0;i+1<old_raw.size();i+=2)
        if(old_raw[i]==owner && old_raw[i+1]==id) {
            old_raw.erase(old_raw.begin()+i,old_raw.begin()+i+2);
            erased=true;break;
        }
    check(erased,"BNW meet source raw plot roster lacks unit");
    const auto old=std::find(old_projection.begin(),old_projection.end(),
                             saves::SavedPlotUnitReference{owner,id});
    check(old!=old_projection.end(),"BNW meet source projection lacks unit");
    old_projection.erase(old);
    new_raw.push_back(owner);new_raw.push_back(id);
    new_projection.push_back({owner,id});
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    const auto source_hash=identify_file(save).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(static_cast<bool>(bootstrap.preview.host),
          "Selected BNW meet preview did not bootstrap");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source();
    const auto &state=host.session().state();
    const auto input=host.preview_input();
    check(static_cast<bool>(input.frame_sources.database),
          "Selected BNW meet rules database absent");
    const auto &database=*input.frame_sources.database;
    const auto types=database.cache_info_types();
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto request=candidate(state);
    {
    const auto baseline=apply_bnw_sight_meets_detached(source,state,request,
        database,types,defines,host.identity());
    check(baseline.calculated && baseline.detached &&
          baseline.contacts.empty() && !baseline.can_commit(),
          baseline.reason.c_str());
    }
    {
    const auto baseline_share=apply_bnw_sight_tech_share_detached(
        source,state,request,database,types,defines,host.identity());
    check(baseline_share.calculated && baseline_share.detached &&
          baseline_share.checks.empty() && !baseline_share.can_commit(),
          baseline_share.reason.c_str());
    }
    {
    const auto baseline_diplomacy=preflight_bnw_sight_diplomacy(
        source,state,request,database,types,defines,host.identity());
    check(baseline_diplomacy.calculated && baseline_diplomacy.detached &&
          baseline_diplomacy.intents.empty() &&
          !baseline_diplomacy.can_commit(),
          baseline_diplomacy.reason.c_str());
    }
    {
    const auto empty_plan=plan_bnw_sight_meet_effects(
        source,state,request,database,types,defines,host.identity());
    check(empty_plan.calculated && empty_plan.events.empty() &&
          empty_plan.contacts.empty() && !empty_plan.can_commit(),
          empty_plan.reason.c_str());
    check(quote_bnw_team_meet_lua_fixture(empty_plan,{}).gate==
              BnwTeamMeetLuaGate::no_new_contact,
          "BNW selected Gandhi no-contact path invoked TeamMeet Lua");
    }
    const auto trace=trace_bnw_unit_sight_event_order(state,request,
        database,types,defines,host.identity());
    check(trace.calculated && !trace.exploration.sight.plan.newly_visible.empty(),
          "Selected BNW meet sight trace absent");
    const auto &mapping=state.world.pregame->archive.integer_arrays.at("team_types");
    const auto revealing=mapping.at(static_cast<std::size_t>(request.owner));
    check(revealing>=0 && revealing<63,"Selected BNW revealing team invalid");
    auto synthetic=state;
    const auto turn=state.setup.game_turn;
    const auto &grid=state.world.map_header.grid;
    std::int64_t target_owner=-1,target_id=-1,target_team=-1;
    std::uint64_t old_plot=grid.size();
    for(const auto &p:state.world.all_players->players) {
        if(!alive(p) || p.saved_player_id==request.owner) continue;
        const auto team=mapping.at(static_cast<std::size_t>(p.saved_player_id));
        if(team<0 || team>=63 || team==revealing ||
           team_array(synthetic.world.teams->teams.at(revealing),"has_met")
             .at(static_cast<std::size_t>(team))!=0) continue;
        for(const auto &value:p.units.units) {
            const auto x=unit(value,"m_iX"),y=unit(value,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            target_owner=p.saved_player_id;target_id=unit(value,"m_iID");
            target_team=team;old_plot=grid.index(x,y);break;
        }
        if(target_owner>=0) break;
    }
    check(target_owner>=0,"Selected Gandhi lacks an uncontacted other unit");
    const auto destination=grid.index(request.to_x,request.to_y);
    std::uint64_t moving_from=grid.size();
    for(const auto &value:state.world.all_players->players.at(
            static_cast<std::size_t>(request.owner)).units.units)
        if(unit(value,"m_iID")==request.unit_id)
            moving_from=grid.index(unit(value,"m_iX"),unit(value,"m_iY"));
    const auto edge=std::find_if(trace.exploration.sight.plan.newly_visible.begin(),
        trace.exploration.sight.plan.newly_visible.end(),
        [&](const auto &candidate) {
            const auto &cell=state.world.map_projection->cells.at(candidate.plot);
            return candidate.plot!=destination &&
                   candidate.plot!=moving_from && candidate.plot!=old_plot &&
                   cell.units && cell.units->empty() &&
                   (!cell.plot_city || cell.plot_city->id==-1);
        });
    check(edge!=trace.exploration.sight.plan.newly_visible.end(),
          "Selected Gandhi lacks an empty reveal plot for synthetic meet");
    const auto meeting_plot=edge->plot;
    for(auto &value:synthetic.world.all_players->players.at(
            static_cast<std::size_t>(target_owner)).units.units)
        if(unit(value,"m_iID")==target_id) {
            const auto &cell=synthetic.world.map_projection->cells.at(meeting_plot);
            set_unit(value,"m_iX",cell.x);set_unit(value,"m_iY",cell.y);
        }
    move_roster(synthetic,target_owner,target_id,old_plot,meeting_plot);
    BnwSightMeetEffectPlan effect_plan;
    {
    auto duplicate=synthetic;
    auto &duplicate_raw=plot_units(duplicate.world.map_plots->plots.at(meeting_plot));
    duplicate_raw.push_back(target_owner);duplicate_raw.push_back(target_id);
    duplicate.world.map_projection->cells.at(meeting_plot).units->push_back(
        {target_owner,target_id});
    const auto repeated=apply_bnw_sight_meets_detached(source,duplicate,request,
        database,types,defines,host.identity());
    check(!repeated.calculated && !repeated.detached &&
          repeated.reason.find("duplicated on plot")!=std::string::npos,
          "BNW duplicated plot unit handle escaped meet preflight");
    }
    {
    auto contact=apply_bnw_sight_meets_detached(source,synthetic,request,
        database,types,defines,host.identity());
    check(contact.calculated && contact.detached &&
          contact.contacts.size()==1 && contact.contacts[0].new_contact &&
          contact.contacts[0].other_team==target_team &&
          !contact.can_commit(),contact.reason.c_str());
    effect_plan=plan_bnw_sight_meet_effects(
        source,synthetic,request,database,types,defines,host.identity());
    check(effect_plan.calculated && effect_plan.contacts.size()==1 &&
          !effect_plan.can_commit() && effect_plan.events.size()>=12 &&
          effect_plan.events.front().kind==BnwMeetEffectKind::has_met &&
          effect_plan.events.back().kind==BnwMeetEffectKind::lua_team_meet,
          effect_plan.reason.c_str());
    std::size_t first_gameplay=effect_plan.events.size();
    std::size_t second_has_met=effect_plan.events.size();
    std::size_t first_ai=effect_plan.events.size();
    for(std::size_t i=0;i<effect_plan.events.size();++i) {
        const auto &event=effect_plan.events[i];
        if(event.kind==BnwMeetEffectKind::gameplay_met_team &&
           first_gameplay==effect_plan.events.size()) first_gameplay=i;
        if(event.kind==BnwMeetEffectKind::has_met && i>0 &&
           second_has_met==effect_plan.events.size()) second_has_met=i;
        if(event.kind==BnwMeetEffectKind::first_contact_ai &&
           first_ai==effect_plan.events.size()) first_ai=i;
    }
    check(first_ai<first_gameplay && first_gameplay<second_has_met &&
          second_has_met<effect_plan.events.size()-1 &&
          effect_plan.events[second_has_met].team==target_team,
          "BNW meet first-contact AI, engine event and reciprocal side order changed");
    const auto missing=check_bnw_meet_effect_capabilities(effect_plan,{});
    check(!missing.complete && !missing.missing_event_indices.empty() &&
          !missing.can_commit(),
          "BNW meet missing AI/Lua/UI callback did not fail closed");
    const std::array available{
        BnwMeetEffectKind::proximity,
        BnwMeetEffectKind::first_contact_ai,
        BnwMeetEffectKind::third_party_ai,
        BnwMeetEffectKind::clear_research_queue,
        BnwMeetEffectKind::score_ui_dirty,
        BnwMeetEffectKind::gameplay_met_team,
        BnwMeetEffectKind::lua_team_meet};
    const auto declared=check_bnw_meet_effect_capabilities(effect_plan,available);
    check(declared.complete && !declared.can_commit(),
          "BNW meet complete callback catalogue incorrectly enabled commit");
    auto mislabeled=effect_plan;
    mislabeled.events.back().external=false;
    const auto rejected_mislabel=check_bnw_meet_effect_capabilities(
        mislabeled,available);
    check(!rejected_mislabel.complete &&
          rejected_mislabel.missing_event_indices.back()==
              mislabeled.events.size()-1,
          "BNW TeamMeet Lua callback was accepted as an internal effect");
    const std::string team_meet_lua=R"lua(
        seen={}
        GameEvents.TeamMeet.Add(function(other,ours)
            assert(other==1 and ours==0)
            seen[#seen+1]='first'
        end)
        GameEvents.TeamMeet.Add(function(other,ours)
            assert(other==1 and ours==0)
            seen[#seen+1]='second'
        end)
        GameEvents.CheckTeamMeet.Add(function()
            return table.concat(seen,',')=='first,second'
        end)
    )lua";
    const auto team_meet_bytes=std::span(
        reinterpret_cast<const std::uint8_t*>(team_meet_lua.data()),
        team_meet_lua.size());
    const BnwDetachedGameplayLuaChunk team_meet_chunk{
        "gameplay","=team-meet-fixture.lua",team_meet_lua,
        identify_bytes(team_meet_bytes).sha256};
    const std::array team_meet_chunks{team_meet_chunk};
    const auto team_meet_quote=quote_bnw_team_meet_lua_fixture(
        effect_plan,team_meet_chunks);
    check(team_meet_quote.gate==
              BnwTeamMeetLuaGate::fixture_quoted_stock_context_unverified &&
          team_meet_quote.first_argument==target_team &&
          team_meet_quote.second_argument==revealing &&
          team_meet_quote.handled && !team_meet_quote.can_commit(),
          team_meet_quote.reason.c_str());
    auto bad_team_meet=team_meet_chunks;
    bad_team_meet[0].sha256=std::string(64,'0');
    check(quote_bnw_team_meet_lua_fixture(effect_plan,bad_team_meet).gate==
              BnwTeamMeetLuaGate::source_plan_unavailable,
          "BNW TeamMeet Lua accepted changed fixture hash");
    const std::string throwing_team_meet=R"lua(
        GameEvents.TeamMeet.Add(function() error('contact hook failed') end)
    )lua";
    const auto throwing_bytes=std::span(
        reinterpret_cast<const std::uint8_t*>(throwing_team_meet.data()),
        throwing_team_meet.size());
    const std::array throwing_chunks{BnwDetachedGameplayLuaChunk{
        "gameplay","=team-meet-error.lua",throwing_team_meet,
        identify_bytes(throwing_bytes).sha256}};
    check(quote_bnw_team_meet_lua_fixture(effect_plan,throwing_chunks).gate==
              BnwTeamMeetLuaGate::callback_failed,
          "BNW TeamMeet Lua listener failure was treated as a completed hook");
    auto wrong_order=effect_plan;
    std::swap(wrong_order.events.back(),wrong_order.events.front());
    check(quote_bnw_team_meet_lua_fixture(wrong_order,team_meet_chunks).gate==
              BnwTeamMeetLuaGate::effect_plan_unavailable,
          "BNW TeamMeet Lua accepted hook before reciprocal team effects");
    const auto &a=contact.detached->world.teams->teams.at(revealing);
    const auto &b=contact.detached->world.teams->teams.at(target_team);
    check(team_array(a,"has_met").at(target_team)==1 &&
          team_array(b,"has_met").at(revealing)==1 &&
          team_array(a,"turn_team_met").at(target_team)==turn &&
          team_array(b,"turn_team_met").at(revealing)==turn &&
          team_array(synthetic.world.teams->teams.at(revealing),"has_met").at(target_team)==0,
          "BNW detached meet flags, turns, or original rollback disagree");
    const auto meet_section=make_bnw_team_meet_checkpoint(
        contact.detached->world,state.world,host.identity(),contact.contacts[0]);
    check(meet_section.id==bnw_team_meet_checkpoint_section_id &&
          meet_section.flags==0,
          "BNW team meet checkpoint section identity changed");
    const auto meet_output=std::filesystem::current_path() /
        ("bnw_team_meet_"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count())+
         ".CivisSave");
    saves::write_civis_save(meet_output,{1,0,{meet_section}});
    const auto meet_document=saves::read_civis_save(meet_output);
    std::filesystem::remove(meet_output);
    check(meet_document.sections.size()==1,
          "BNW team meet atomic CivisSave round-trip lost section");
    auto imported_document=saves::make_civis_import_document(source.source,source_hash);
    imported_document.sections.push_back(meet_section);
    const auto imported_output=std::filesystem::current_path() /
        ("bnw_team_meet_import_"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count())+
         ".CivisSave");
    saves::write_civis_save(imported_output,imported_document);
    const auto loaded_meet=saves::read_loaded_save(imported_output);
    std::filesystem::remove(imported_output);
    check(loaded_meet.live_bnw_team_meet==meet_section.bytes,
          "BNW team meet section was lost by full Civis import");
    bool unclaimed_meet_rejected=false;
    try {
        (void)saves::observe_loaded_session(loaded_meet,{});
    } catch(const std::exception &) {unclaimed_meet_rejected=true;}
    check(unclaimed_meet_rejected,
          "Unclaimed BNW team meet section reached source-only observation");
    {
    auto restored=state.world;
    restore_bnw_team_meet_checkpoint(meet_document.sections[0],restored,
        state.world,host.identity());
    check(team_array(restored.teams->teams.at(revealing),"has_met")
              .at(target_team)==1 &&
          team_array(restored.teams->teams.at(target_team),"has_met")
              .at(revealing)==1 &&
          team_array(restored.teams->teams.at(revealing),"turn_team_met")
              .at(target_team)==turn &&
          team_array(restored.teams->teams.at(target_team),"turn_team_met")
              .at(revealing)==turn,
          "BNW team meet checkpoint lost reciprocal flags or turns");
    }
    {
    auto attempted=state.world;
    auto damaged=meet_document.sections[0];
    damaged.bytes[damaged.bytes.size()-8]^=1; // second old turn
    bool rejected=false;
    try {
        restore_bnw_team_meet_checkpoint(damaged,attempted,
            state.world,host.identity());
    } catch(const std::exception &) {rejected=true;}
    check(rejected &&
          team_array(attempted.teams->teams.at(revealing),"has_met")
              .at(target_team)==0 &&
          team_array(attempted.teams->teams.at(target_team),"has_met")
              .at(revealing)==0,
          "BNW team meet restore partially applied after second-side rejection");
    auto wrong_rules=host.identity();
    wrong_rules.rules_sha256=std::string(64,'0');
    rejected=false;
    try {
        restore_bnw_team_meet_checkpoint(meet_document.sections[0],attempted,
            state.world,wrong_rules);
    } catch(const std::exception &) {rejected=true;}
    check(rejected &&
          team_array(attempted.teams->teams.at(revealing),"has_met")
              .at(target_team)==0,
          "BNW team meet restore accepted foreign rules identity");
    }
    {
    auto wrong_roster=state.world;
    wrong_roster.pregame->archive.integer_arrays.at("team_types")
        .at(static_cast<std::size_t>(request.owner))=target_team;
    bool rejected=false;
    try {
        restore_bnw_team_meet_checkpoint(meet_document.sections[0],wrong_roster,
            state.world,host.identity());
    } catch(const std::exception &) {rejected=true;}
    check(rejected &&
          team_array(wrong_roster.teams->teams.at(revealing),"has_met")
              .at(target_team)==0,
          "BNW team meet restore accepted changed player/team roster");
    }
    {
    auto &unrelated=team_array(contact.detached->world.teams->teams.at(
        revealing),"has_met").at(static_cast<std::size_t>(revealing));
    unrelated^=1;
    bool rejected=false;
    try {
        (void)make_bnw_team_meet_checkpoint(contact.detached->world,
            state.world,host.identity(),contact.contacts[0]);
    } catch(const std::exception &) {rejected=true;}
    unrelated^=1;
    check(rejected,"BNW team meet writer accepted unrelated team mutation");
    }
    }
    {
    const auto no_grant=apply_bnw_sight_tech_share_detached(
        source,synthetic,request,database,types,defines,host.identity());
    check(no_grant.calculated && no_grant.detached &&
          no_grant.checks.size()==2 &&
          !no_grant.checks[0].first_grant_tech &&
          !no_grant.checks[1].first_grant_tech &&
          !no_grant.can_commit(),no_grant.reason.c_str());
    }
    {
    const auto diplomacy=preflight_bnw_sight_diplomacy(
        source,synthetic,request,database,types,defines,host.identity());
    check(diplomacy.calculated && diplomacy.detached &&
          diplomacy.intents.size()>=6 && !diplomacy.can_commit(),
          diplomacy.reason.c_str());
    check(diplomacy.intents[0].call==
              BnwMeetDiplomacyCall::proximity_our_to_their &&
          diplomacy.intents[1].call==
              BnwMeetDiplomacyCall::proximity_their_to_our &&
          diplomacy.intents[2].call==BnwMeetDiplomacyCall::first_contact &&
          diplomacy.intents[0].recipient_team==revealing &&
          diplomacy.intents[0].other_team==target_team &&
          std::any_of(diplomacy.intents.begin(),diplomacy.intents.end(),
              [&](const auto &intent) {
                  return intent.call==
                         BnwMeetDiplomacyCall::third_party_met_someone &&
                         intent.recipient_team==revealing;
              }),
          "BNW meet diplomacy SDK player callback order changed");
    }
    {
    auto first_contact=apply_bnw_sight_first_contact_detached(
        source,synthetic,request,database,types,defines,host.identity());
    check(first_contact.calculated && first_contact.detached &&
          first_contact.blocker==
              BnwSightFirstContactBlocker::first_contact_ai_unimplemented &&
          first_contact.proximity_changes.size()==2 &&
          first_contact.proximity_changes[0].player==request.owner &&
          first_contact.proximity_changes[0].other==target_owner &&
          first_contact.proximity_changes[1].player==target_owner &&
          first_contact.proximity_changes[1].other==request.owner &&
          !first_contact.can_commit(),first_contact.reason.c_str());
    for(const auto &change:first_contact.proximity_changes) {
        check(proximity(first_contact.detached->world.all_players->players.at(
                  static_cast<std::size_t>(change.player)),change.other)==
                  change.new_value &&
              proximity(synthetic.world.all_players->players.at(
                  static_cast<std::size_t>(change.player)),change.other)==
                  change.old_value,
              "BNW detached proximity changed source or lost SDK order");
    }
    const auto proximity_stage=stage_bnw_meet_proximity(
        effect_plan,first_contact,state.world,host.identity());
    check(proximity_stage.staged && proximity_stage.checkpoint &&
          proximity_stage.checkpoint->id==
              bnw_player_proximity_checkpoint_section_id &&
          proximity_stage.receipts.size()==2 &&
          proximity_stage.receipts[0].event_index<
              proximity_stage.receipts[1].event_index &&
          verify_bnw_meet_proximity_receipts(proximity_stage,effect_plan,
              first_contact,state.world,host.identity()) &&
          !proximity_stage.can_commit(),proximity_stage.reason.c_str());
    auto forged_receipt=proximity_stage;
    forged_receipt.receipts[1].checkpoint_sha256[0]^=1;
    check(!verify_bnw_meet_proximity_receipts(forged_receipt,effect_plan,
              first_contact,state.world,host.identity()),
          "BNW meet accepted a forged second proximity receipt");
    auto corrupt_stage=proximity_stage;
    corrupt_stage.checkpoint->bytes.back()^=1;
    check(!verify_bnw_meet_proximity_receipts(corrupt_stage,effect_plan,
              first_contact,state.world,host.identity()),
          "BNW meet accepted a corrupt ID20 proximity checkpoint");
    auto wrong_proximity_order=effect_plan;
    std::swap(wrong_proximity_order.events[
        proximity_stage.receipts[0].event_index],
        wrong_proximity_order.events[
        proximity_stage.receipts[1].event_index]);
    check(!stage_bnw_meet_proximity(wrong_proximity_order,first_contact,
              state.world,host.identity()).staged,
          "BNW meet staged reciprocal proximity calls in the wrong order");
    const auto military=preflight_bnw_first_contact_military(
        source,synthetic,request,database,types,defines,host.identity());
    check(military.source_verified && !military.can_commit() &&
          military.recipient==request.owner && military.target==target_owner &&
          military.elapsed_turns==state.maintenance.elapsed_turns,
          military.reason.c_str());
    const auto saved_might_turn=player_scalar(
        synthetic.world.all_players->players.at(
            static_cast<std::size_t>(request.owner)),
        "m_iTurnMightRecomputed");
    const auto target_cities=synthetic.world.all_players->players.at(
        static_cast<std::size_t>(target_owner)).cities.header.wire_city_count;
    if(saved_might_turn<state.maintenance.elapsed_turns)
        check(!military.first_call_completed &&
              military.gate==
                  BnwFirstContactMilitaryGate::own_might_lazy_recompute,
              "BNW first-contact read stale own might as current");
    else if(target_cities==0)
        check(military.first_call_completed &&
              military.gate==
                  BnwFirstContactMilitaryGate::next_economic_ai_unimplemented,
              "BNW first-contact missed SDK no-city military skip");
    else
        check(!military.first_call_completed &&
              (military.gate==
                   BnwFirstContactMilitaryGate::target_might_lazy_recompute ||
               military.gate==
                   BnwFirstContactMilitaryGate::military_assessment_unimplemented),
              "BNW first-contact marked military assessment complete");
    std::cout << "BNW first-contact military gate " <<
        static_cast<int>(military.gate) << " own-cache " <<
        military.own_might_cache_turn << " elapsed " <<
        military.elapsed_turns << " target-cities " << target_cities << '\n';
    const auto economic=preflight_bnw_first_contact_economic(
        source,synthetic,request,database,types,defines,host.identity());
    check(economic.source_verified && !economic.can_commit() &&
          economic.recipient==request.owner && economic.target==target_owner &&
          economic.elapsed_turns==state.maintenance.elapsed_turns &&
          economic.target_city_count==target_cities &&
          economic.military_gate==military.gate,
          economic.reason.c_str());
    check(military.first_call_completed && target_cities==0 &&
          economic.economic_call_completed &&
          economic.gate==
              BnwFirstContactEconomicGate::next_military_posture_unimplemented,
          "Selected Gandhi missed SDK no-city economic skip");
    const auto posture=preflight_bnw_first_contact_posture(
        source,synthetic,request,database,types,defines,host.identity());
    check(posture.source_verified && !posture.can_commit() &&
          posture.recipient==request.owner && posture.target==target_owner &&
          posture.elapsed_turns==state.maintenance.elapsed_turns &&
          posture.target_city_count==0 &&
          posture.economic_gate==
              BnwFirstContactEconomicGate::next_military_posture_unimplemented &&
          posture.military_posture_call_completed &&
          posture.gate==
              BnwFirstContactPostureGate::next_expansion_posture_unimplemented,
          "Selected Gandhi missed SDK no-city military posture skip");
    const auto expansion=preflight_bnw_first_contact_expansion(
        source,synthetic,request,database,types,defines,host.identity());
    check(expansion.source_verified && !expansion.can_commit() &&
          expansion.recipient==request.owner && expansion.target==target_owner &&
          expansion.elapsed_turns==state.maintenance.elapsed_turns &&
          expansion.target_city_count==0 &&
          expansion.recipient_city_count==
              synthetic.world.all_players->players.at(
                  static_cast<std::size_t>(request.owner))
                  .cities.header.wire_city_count &&
          expansion.expansion_call_completed &&
          expansion.exit!=BnwFirstContactExpansionExit::none &&
          expansion.gate==
              BnwFirstContactExpansionGate::next_target_value_unimplemented,
          expansion.reason.c_str());
    const auto target_value=preflight_bnw_first_contact_target(
        source,synthetic,request,database,types,defines,host.identity());
    check(target_value.source_verified && !target_value.can_commit() &&
          target_value.recipient==request.owner &&
          target_value.target==target_owner &&
          target_value.elapsed_turns==state.maintenance.elapsed_turns &&
          target_value.target_city_count==0 &&
          target_value.expansion_gate==
              BnwFirstContactExpansionGate::next_target_value_unimplemented &&
          target_value.target_value_call_completed &&
          target_value.gate==
              BnwFirstContactTargetGate::next_relationship_branch_unimplemented,
          "Selected Gandhi missed SDK no-city target value skip");
    const auto relationship=preflight_bnw_first_contact_relationship(
        source,synthetic,request,database,types,defines,host.identity());
    check(relationship.source_verified && !relationship.can_commit() &&
          relationship.recipient==request.owner &&
          relationship.target==target_owner &&
          relationship.elapsed_turns==state.maintenance.elapsed_turns &&
          relationship.target_city_count==0 && !relationship.target_minor &&
          relationship.major_opinion_call_completed &&
          relationship.gate==
              BnwFirstContactRelationshipGate::major_approach_unimplemented,
          relationship.reason.c_str());
    const auto approach=preflight_bnw_first_contact_approach(
        source,synthetic,request,database,types,defines,host.identity());
    check(approach.source_verified && !approach.can_commit() &&
          approach.major_branch_verified &&
          approach.recipient==request.owner && approach.target==target_owner &&
          approach.recipient_team==revealing &&
          approach.target_team==target_team &&
          approach.relationship_gate==
              BnwFirstContactRelationshipGate::major_approach_unimplemented &&
          approach.gate==BnwFirstContactApproachGate::scratchpad_unrestored &&
          approach.reason.find("m_paeApproachScratchPad")!=std::string::npos,
          approach.reason.c_str());
    auto scratchpad=restore_bnw_diplomacy_scratchpad_after_player_read(
        source,state,database,host.identity());
    check(state.diplomacy_scratchpad.current() &&
          state.diplomacy_scratchpad.value_at(request.owner)==-1 &&
          state.diplomacy_scratchpad.value_at(target_owner)==-1,
          "BNW live session did not own the restored diplomacy scratchpad");
    check(scratchpad.current() && !scratchpad.can_publish_contact() &&
          scratchpad.matches_load_clock(synthetic) &&
          scratchpad.gate()==
              BnwDiplomacyScratchpadLoadGate::ready_after_player_read &&
          scratchpad.value_at(request.owner)==-1 &&
          scratchpad.value_at(target_owner)==-1 &&
          !scratchpad.value_at(63).has_value(),
          scratchpad.reason().c_str());
    const auto loaded_approach=preflight_bnw_first_contact_approach_after_load(
        source,synthetic,request,database,types,defines,host.identity(),scratchpad);
    check(!loaded_approach.can_commit() &&
          loaded_approach.recipient==request.owner &&
          loaded_approach.target==target_owner &&
          loaded_approach.gate==
              BnwFirstContactApproachAfterLoadGate::random_factor_unimplemented &&
          loaded_approach.nuke_inputs_verified &&
          loaded_approach.own_nuke_units>=0 &&
          loaded_approach.target_nuke_units>=0 &&
          loaded_approach.target_nuke_likelihood>=0,
          loaded_approach.reason.c_str());
    const auto rng_before=synthetic.random.gameplay.state();
    const auto random_quote=quote_bnw_first_contact_approach_random(
        source,synthetic,request,database,types,defines,host.identity(),scratchpad);
    check(random_quote.gate==
              BnwApproachRandomGate::next_can_declare_war_unimplemented &&
          !random_quote.can_commit() &&
          !random_quote.draws.empty() &&
          random_quote.rng_before==rng_before &&
          synthetic.random.gameplay.state()==rng_before &&
          random_quote.rng_after.calls==rng_before.calls+
              random_quote.draws.size(),
          random_quote.reason.c_str());
    const auto rng_stage=stage_bnw_first_contact_approach_rng(
        source,synthetic,request,database,types,defines,host.identity(),scratchpad);
    check(rng_stage.staged && rng_stage.checkpoint && rng_stage.receipt &&
          rng_stage.receipt->gameplay_before==rng_before &&
          rng_stage.receipt->gameplay_after==random_quote.rng_after &&
          rng_stage.receipt->draws.size()==random_quote.draws.size() &&
          verify_bnw_first_contact_rng_receipt(rng_stage,synthetic.random,
              host.identity()) && !rng_stage.can_commit() &&
          synthetic.random.gameplay.state()==rng_before,
          rng_stage.reason.c_str());
    auto forged_rng=rng_stage;
    forged_rng.receipt->gameplay_after.calls++;
    check(!verify_bnw_first_contact_rng_receipt(forged_rng,synthetic.random,
              host.identity()),
          "BNW first-contact accepted forged RNG draw count");
    auto corrupt_rng=rng_stage;
    corrupt_rng.checkpoint->bytes.back()^=1;
    check(!verify_bnw_first_contact_rng_receipt(corrupt_rng,synthetic.random,
              host.identity()),
          "BNW first-contact accepted corrupt ID13 RNG checkpoint");
    auto foreign_rng_identity=host.identity();
    foreign_rng_identity.rules_sha256=std::string(64,'0');
    check(!verify_bnw_first_contact_rng_receipt(rng_stage,synthetic.random,
              foreign_rng_identity),
          "BNW first-contact RNG receipt accepted foreign rules");
    const auto war_gate=quote_bnw_first_contact_approach_war_gate(
        source,synthetic,request,database,types,defines,host.identity(),scratchpad);
    check(!war_gate.can_commit() &&
          (war_gate.gate==BnwApproachWarGate::lua_can_declare_war_unavailable ||
           war_gate.gate==BnwApproachWarGate::next_human_difficulty_unimplemented) &&
          war_gate.recipient_team==revealing &&
          war_gate.target_team==target_team &&
          war_gate.rng_before==random_quote.rng_before &&
          war_gate.rng_after==random_quote.rng_after &&
          synthetic.random.gameplay.state()==rng_before,
          war_gate.reason.c_str());
    check(war_gate.gate==BnwApproachWarGate::lua_can_declare_war_unavailable,
          "Selected Gandhi first contact no longer reaches CanDeclareWar Lua");
    const auto lua_chunk=[](std::string source_text,const char *name) {
        const auto bytes=std::span(
            reinterpret_cast<const std::uint8_t*>(source_text.data()),
            source_text.size());
        return BnwDetachedGameplayLuaChunk{
            "gameplay",name,source_text,identify_bytes(bytes).sha256};
    };
    const std::array no_war_listeners{
        lua_chunk("local fixture_loaded=true","=no-war-listener.lua")};
    const auto no_listener=quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),
        no_war_listeners,request.owner,target_owner);
    check(no_listener.gate==
              BnwFirstContactWarDecisionGate::selected_gameplay_lua_order_unverified &&
          no_listener.lua_decision &&
          no_listener.lua_decision->decision==BnwCanDeclareWarLuaDecision::allow &&
          !no_listener.lua_decision->handled &&
          no_listener.weights==war_gate.weights && !no_listener.can_commit() &&
          no_listener.unique_best_approach.has_value() &&
          synthetic.random.gameplay.state()==rng_before,
          no_listener.reason.c_str());
    std::cout<<"BNW CanDeclareWar fixture approach "<<
        *no_listener.unique_best_approach<<" next gate "<<
        static_cast<int>(no_listener.next_after_fixture)<<'\n';
    check(no_listener.tail && no_listener.tail->gate==BnwApproachTailGate::quoted &&
          no_listener.receipt && no_listener.receipt->approach==6 &&
          no_listener.receipt->war_face==-1 &&
          !no_listener.receipt->war_face_write_required &&
          no_listener.next_after_fixture==
              BnwFirstContactAfterFixtureGate::approach_computed_private &&
          verify_bnw_first_contact_approach_receipt(no_listener,source,synthetic,
              request,rng_stage,war_gate,database,types,defines,scratchpad,
              host.identity(),no_war_listeners,request.owner,target_owner),
          "Selected Gandhi NEUTRAL tail/receipt failed source replay");
    auto forged_approach_receipt=no_listener;
    forged_approach_receipt.receipt->after_tail[6]++;
    check(!verify_bnw_first_contact_approach_receipt(forged_approach_receipt,
              source,synthetic,request,rng_stage,war_gate,database,types,defines,
              scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
          "BNW approach tail receipt accepted changed weights");
    auto forged_approach_tail=no_listener;
    forged_approach_tail.tail->war_face=2;
    check(!verify_bnw_first_contact_approach_receipt(forged_approach_tail,
              source,synthetic,request,rng_stage,war_gate,database,types,defines,
              scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
          "BNW approach tail receipt accepted a changed proposed WarFace");
    {
        auto human_source=source;
        auto human_world=synthetic;
        const auto index=static_cast<std::size_t>(target_owner);
        human_source.snapshot.pregame->archive.integer_arrays.at("slot_status")[index]=3;
        human_world.world.pregame->archive.integer_arrays.at("slot_status")[index]=3;
        human_world.setup.players.at(target_owner).slot_status=3;
        const auto human_rng=stage_bnw_first_contact_approach_rng(human_source,
            human_world,request,database,types,defines,host.identity(),scratchpad);
        check(human_rng.staged,human_rng.reason.c_str());
        const auto human_gate=quote_bnw_first_contact_approach_war_gate(human_source,
            human_world,request,database,types,defines,host.identity(),scratchpad);
        const auto human_quote=quote_bnw_first_contact_war_decision_fixture(
            human_source,human_world,request,human_rng,human_gate,database,types,
            defines,scratchpad,host.identity(),no_war_listeners,request.owner,target_owner);
        check(human_quote.receipt && human_quote.receipt->target_human &&
              human_quote.tail && human_quote.tail->gate==BnwApproachTailGate::quoted &&
              verify_bnw_first_contact_approach_receipt(human_quote,human_source,
                  human_world,request,human_rng,human_gate,database,types,defines,
                  scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
              human_quote.reason.c_str());
        std::int64_t active_modifier=-1;
        const auto modifier_rule=database.query_rows({
            "SELECT AIDeclareWarProb FROM HandicapInfos WHERE ID=?",
            {human_world.setup.players.at(target_owner).handicap}},
            [&](const auto &row) {active_modifier=row.integer_at(0);},1);
        check(modifier_rule.all_succeeded() && active_modifier>=0 &&
              human_quote.receipt->human_war_modifier==active_modifier,
              "BNW approach did not bind human modifier from active HandicapInfos");
        human_world.world.pregame->archive.integer_arrays.at("handicaps")[index]++;
        check(!verify_bnw_first_contact_approach_receipt(human_quote,human_source,
                  human_world,request,human_rng,human_gate,database,types,defines,
                  scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
              "BNW approach receipt accepted changed raw human handicap");
        human_world.world.pregame->archive.integer_arrays.at("handicaps")[index]--;
        human_world.setup.players.at(target_owner).handicap++;
        check(!verify_bnw_first_contact_approach_receipt(human_quote,human_source,
                  human_world,request,human_rng,human_gate,database,types,defines,
                  scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
              "BNW approach receipt accepted changed owned human handicap");
        check(human_world.random.gameplay.state()==rng_before,
              "Human approach tail advanced caller gameplay RNG");
    }
    {
        auto war_source=source;
        auto war_world=synthetic;
        const auto personality=[](auto &snapshot,std::int64_t player) {
            auto &fields=snapshot.all_players->players.at(
                static_cast<std::size_t>(player)).diplomacy.fields;
            for(auto &field:fields)
                if(field.name && std::string_view(field.name)==
                        "m_paiPersonalityMajorCivApproachBiases") {
                    check(field.values.size()==7,"BNW synthetic WAR personality shape differs");
                    field.values={120,0,0,0,0,0,0};return;
                }
            throw std::runtime_error("BNW synthetic WAR personality unavailable");
        };
        personality(war_source.snapshot,request.owner);
        personality(war_world.world,request.owner);
        const auto war_rng=stage_bnw_first_contact_approach_rng(war_source,
            war_world,request,database,types,defines,host.identity(),scratchpad);
        check(war_rng.staged,war_rng.reason.c_str());
        const auto war_native=quote_bnw_first_contact_approach_war_gate(war_source,
            war_world,request,database,types,defines,host.identity(),scratchpad);
        const auto war_quote=quote_bnw_first_contact_war_decision_fixture(
            war_source,war_world,request,war_rng,war_native,database,types,
            defines,scratchpad,host.identity(),no_war_listeners,request.owner,target_owner);
        check(war_quote.receipt && war_quote.receipt->approach==0 &&
              war_quote.receipt->war_face==1 &&
              war_quote.receipt->war_face_write_required &&
              verify_bnw_first_contact_approach_receipt(war_quote,war_source,
                  war_world,request,war_rng,war_native,database,types,defines,
                  scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
              war_quote.reason.c_str());
        for(auto &field:war_world.world.all_players->players.at(
                static_cast<std::size_t>(request.owner)).diplomacy.fields)
            if(field.name && std::string_view(field.name)=="m_paeWarFace")
                field.values.at(static_cast<std::size_t>(target_owner))=2;
        check(!verify_bnw_first_contact_approach_receipt(war_quote,war_source,
                  war_world,request,war_rng,war_native,database,types,defines,
                  scratchpad,host.identity(),no_war_listeners,request.owner,target_owner),
              "BNW approach receipt accepted a changed owned saved WarFace");
        check(war_world.random.gameplay.state()==rng_before,
              "WAR approach tail advanced caller gameplay RNG");
    }
    const std::array veto_listeners{
        lua_chunk(R"lua(
            GameEvents.CanDeclareWar.Add(function(ours,theirs)
                assert(ours==0 and theirs==1)
                return false
            end)
        )lua","=veto-war.lua")};
    const auto veto=quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),
        veto_listeners,request.owner,target_owner);
    check(veto.gate==
              BnwFirstContactWarDecisionGate::selected_gameplay_lua_order_unverified &&
          veto.lua_decision && veto.lua_decision->handled &&
          veto.lua_decision->decision==BnwCanDeclareWarLuaDecision::veto &&
          veto.weights[0]==0 && !veto.can_commit(),
          veto.reason.c_str());
    const std::array broken_listeners{
        lua_chunk("GameEvents.CanDeclareWar.Add(function() error('veto failed') end)",
                  "=broken-war.lua")};
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),
        broken_listeners,request.owner,target_owner).gate==
              BnwFirstContactWarDecisionGate::lua_callback_failed,
          "BNW CanDeclareWar listener error escaped fail-closed boundary");
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),
        {},request.owner,target_owner).gate==
              BnwFirstContactWarDecisionGate::fixture_source_unavailable,
          "BNW CanDeclareWar accepted missing ordered Lua sources");
    const std::array ordered_listeners{
        lua_chunk(R"lua(
            fixture_count=0
            GameEvents.CanDeclareWar.Add(function(ours,theirs)
                assert(ours==0 and theirs==1 and fixture_count==0)
                fixture_count=1
                return false
            end)
        )lua","=first-veto.lua"),
        lua_chunk(R"lua(
            GameEvents.CanDeclareWar.Add(function(ours,theirs)
                assert(ours==0 and theirs==1 and fixture_count==1)
                fixture_count=2
                return true
            end)
        )lua","=second-allow.lua")};
    const auto ordered_veto=quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),ordered_listeners,request.owner,target_owner);
    check(ordered_veto.gate==
              BnwFirstContactWarDecisionGate::selected_gameplay_lua_order_unverified &&
          ordered_veto.lua_decision && ordered_veto.lua_decision->handled &&
          ordered_veto.lua_decision->decision==BnwCanDeclareWarLuaDecision::veto &&
          ordered_veto.weights[0]==0 && !ordered_veto.can_commit(),
          "BNW CallTestAll skipped a later listener or lost false aggregate");
    auto wrong_listener_order=ordered_listeners;
    std::reverse(wrong_listener_order.begin(),wrong_listener_order.end());
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),wrong_listener_order,request.owner,target_owner).gate==
              BnwFirstContactWarDecisionGate::lua_callback_failed,
          "BNW CallTestAll failed to preserve the explicit source order");
    auto corrupt_lua=no_war_listeners;
    corrupt_lua[0].source+=" -- unpinned change";
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),corrupt_lua,request.owner,target_owner).gate==
              BnwFirstContactWarDecisionGate::fixture_source_unavailable,
          "BNW CanDeclareWar accepted a changed pinned Lua source");
    const std::array missing_api{
        lua_chunk("GameEvents.CanDeclareWar.Add(function() return Map.GetPlot(0,0) end)",
            "=missing-map-provider.lua")};
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),missing_api,request.owner,target_owner).gate==
              BnwFirstContactWarDecisionGate::lua_callback_failed,
          "BNW CanDeclareWar silently accepted an unavailable map API");
    auto forged_weights=rng_stage;
    auto forged_native_gate=war_gate;
    ++forged_weights.receipt->local_weights[0];
    ++forged_native_gate.weights[0];
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,forged_weights,forged_native_gate,database,types,
        defines,scratchpad,host.identity(),no_war_listeners,request.owner,
        target_owner).gate==
              BnwFirstContactWarDecisionGate::earlier_rng_or_team_gate_unavailable,
          "BNW CanDeclareWar trusted a matching forged weight and gate pair");
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,rng_stage,war_gate,database,types,defines,
        scratchpad,host.identity(),no_war_listeners,target_owner,request.owner).gate==
              BnwFirstContactWarDecisionGate::earlier_rng_or_team_gate_unavailable,
          "BNW CanDeclareWar accepted the wrong first-contact player pair");
    check(synthetic.random.gameplay.state()==rng_before &&
          synthetic.random.map.state()==rng_stage.receipt->map_before,
          "BNW CanDeclareWar fixture published diagnostic random draws");
    auto forged_war_rng=rng_stage;
    forged_war_rng.receipt->gameplay_after.calls++;
    check(quote_bnw_first_contact_war_decision_fixture(
        source,synthetic,request,forged_war_rng,war_gate,database,types,defines,
        scratchpad,host.identity(),
        no_war_listeners,request.owner,target_owner).gate==
              BnwFirstContactWarDecisionGate::earlier_rng_or_team_gate_unavailable,
          "BNW CanDeclareWar accepted a forged preceding RNG receipt");
    {
    auto changed=synthetic;
    auto &fields=changed.world.teams->teams.at(
        static_cast<std::size_t>(revealing)).fields;
    const auto alive=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="alive_count";
    });
    check(alive!=fields.end(),"BNW selected team alive counter unavailable");
    ++alive->value;
    const auto changed_gate=quote_bnw_first_contact_approach_war_gate(
        source,changed,request,database,types,defines,
        host.identity(),scratchpad);
    check(changed_gate.gate==BnwApproachWarGate::team_state_unavailable ||
          changed_gate.gate==BnwApproachWarGate::random_prefix_unavailable,
          "BNW war gate accepted an altered owned team alive count");
    }
    {
    auto changed=synthetic;
    (void)changed.random.gameplay.draw(2);
    check(quote_bnw_first_contact_approach_random(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwApproachRandomGate::source_rng_unavailable &&
          synthetic.random.gameplay.state()==rng_before,
          "BNW quote accepted an unexplained gameplay RNG draw");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto projection=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_paeWarProjection";
    });
    check(projection!=fields.end() && projection->rows==1 &&
          projection->columns==63 &&
          static_cast<std::size_t>(target_owner)<projection->values.size(),
          "BNW selected war-projection row unavailable");
    projection->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::war_projection_unavailable,
          "BNW approach accepted changed owned war projection");
    }
    {
    auto changed=synthetic;
    auto &arrays=changed.world.teams->teams.at(
        static_cast<std::size_t>(revealing)).arrays;
    const auto peace=std::find_if(arrays.begin(),arrays.end(),[](const auto &a) {
        return a.name && std::string_view(a.name)=="turn_made_peace";
    });
    check(peace!=arrays.end() && peace->values.size()==64 &&
          static_cast<std::size_t>(target_team)<peace->values.size(),
          "BNW selected peace-turn team row unavailable");
    ++peace->values[static_cast<std::size_t>(target_team)];
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::peace_treaty_unavailable,
          "BNW approach accepted changed owned peace treaty turn");
    }
    const auto &personality_fields=source.snapshot.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto personality=std::find_if(
        personality_fields.begin(),personality_fields.end(),[](const auto &f) {
            return f.name &&
                std::string_view(f.name)=="m_paiPersonalityMajorCivApproachBiases";
        });
    check(personality!=personality_fields.end() &&
          personality->rows==1 && personality->columns==7 &&
          personality->values.size()==7 &&
          loaded_approach.pre_opinion_weights[6]==loaded_approach.neutral_default+
              loaded_approach.current_bias+personality->values[6] &&
          loaded_approach.pre_opinion_weights[1]==personality->values[1],
          "BNW approach post-Read neutral/current/personality weight order differs");
    const auto &opinion_fields=source.snapshot.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto opinion=std::find_if(
        opinion_fields.begin(),opinion_fields.end(),[](const auto &f) {
            return f.name && std::string_view(f.name)=="m_paeMajorCivOpinion";
        });
    check(opinion!=opinion_fields.end() && opinion->rows==1 &&
          opinion->columns==22 && opinion->values.size()==22 &&
          loaded_approach.opinion==
              opinion->values[static_cast<std::size_t>(target_owner)] &&
          (loaded_approach.opinion!=-1 ||
           loaded_approach.pre_friendship_weights==
               loaded_approach.pre_opinion_weights),
          "BNW approach selected opinion row or no-op differs from SDK");
    const auto friendship=std::find_if(
        opinion_fields.begin(),opinion_fields.end(),[](const auto &f) {
            return f.name && std::string_view(f.name)=="m_pabDoFAccepted";
        });
    check(friendship!=opinion_fields.end() && friendship->values.size()==22 &&
          loaded_approach.friendship_accepted==
              (friendship->values[static_cast<std::size_t>(target_owner)]==1),
          "BNW selected DoF row does not match approach branch");
    if(!loaded_approach.friendship_accepted)
        check(loaded_approach.pre_denouncement_weights==
                  loaded_approach.pre_friendship_weights,
              "BNW non-DoF approach changed friendship weights");
    else {
        auto expected=loaded_approach.pre_friendship_weights;
        for(const auto &[name,index]:{
                std::pair{"APPROACH_DECEPTIVE_WORKING_WITH_PLAYER",2u},
                std::pair{"APPROACH_FRIENDLY_WORKING_WITH_PLAYER",5u},
                std::pair{"APPROACH_HOSTILE_WORKING_WITH_PLAYER",1u},
                std::pair{"APPROACH_GUARDED_WORKING_WITH_PLAYER",3u}}) {
            const auto *entry=defines.find(name);
            check(entry && std::holds_alternative<std::int64_t>(entry->lookup.value),
                  "BNW selected DoF rule unavailable");
            expected[index]+=std::get<std::int64_t>(entry->lookup.value);
        }
        check(loaded_approach.pre_denouncement_weights==expected,
              "BNW DoF approach weights differ from active rules");
    }
    const auto denounced=std::find_if(
        opinion_fields.begin(),opinion_fields.end(),[](const auto &f) {
            return f.name && std::string_view(f.name)=="m_pabDenouncedPlayer";
        });
    check(denounced!=opinion_fields.end() && denounced->values.size()==22 &&
          loaded_approach.denounced_target==
              (denounced->values[static_cast<std::size_t>(target_owner)]==1),
          "BNW selected denouncement row does not match approach branch");
    if(!loaded_approach.denounced_target)
        check(loaded_approach.pre_betrayal_weights==
                  loaded_approach.pre_denouncement_weights,
              "BNW non-denouncement approach changed weights");
    else {
        auto expected=loaded_approach.pre_denouncement_weights;
        for(const auto &[name,index]:{
                std::pair{"APPROACH_WAR_DENOUNCED",0u},
                std::pair{"APPROACH_HOSTILE_DENOUNCED",1u},
                std::pair{"APPROACH_GUARDED_DENOUNCED",3u},
                std::pair{"APPROACH_FRIENDLY_DENOUNCED",5u},
                std::pair{"APPROACH_DECEPTIVE_DENOUNCED",2u}}) {
            const auto *entry=defines.find(name);
            check(entry && std::holds_alternative<std::int64_t>(entry->lookup.value),
                  "BNW selected denouncement rule unavailable");
            expected[index]+=std::get<std::int64_t>(entry->lookup.value);
        }
        check(loaded_approach.pre_betrayal_weights==expected,
              "BNW denouncement approach weights differ from active rules");
    }
    const auto betrayal=std::find_if(
        opinion_fields.begin(),opinion_fields.end(),[](const auto &f) {
            return f.name && std::string_view(f.name)=="m_pabFriendDenouncedUs";
        });
    check(betrayal!=opinion_fields.end() && betrayal->values.size()==22 &&
          loaded_approach.friend_betrayal==
              (betrayal->values[static_cast<std::size_t>(target_owner)]==1),
          "BNW friend betrayal row or branch differs from SDK");
    const auto &target_diplomacy=source.snapshot.all_players->players.at(
        static_cast<std::size_t>(target_owner)).diplomacy.fields;
    const auto reciprocal=std::find_if(
        target_diplomacy.begin(),target_diplomacy.end(),[](const auto &f) {
            return f.name && std::string_view(f.name)=="m_pabDenouncedPlayer";
        });
    check(reciprocal!=target_diplomacy.end() && reciprocal->values.size()==22 &&
          loaded_approach.reciprocal_denouncement==
              (reciprocal->values[static_cast<std::size_t>(request.owner)]==1),
          "BNW reciprocal denouncement must read target DiplomacyAI");
    {
    auto changed=synthetic;
    ++changed.setup.game_turn;
    check(!scratchpad.matches_load_clock(changed),
          "BNW approach accepted a post-Read scratchpad after game clock advanced");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto war=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_paeWarState";
    });
    check(war!=fields.end() && war->rows==1 && war->columns==63 &&
          war->values.size()==63,
          "BNW selected recipient has no war-state source row");
    war->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::war_state_unavailable,
          "BNW approach accepted a changed war-state row");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto row=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_paeMajorCivOpinion";
    });
    check(row!=fields.end() && row->values.size()==22,
          "BNW selected recipient has no opinion source row");
    row->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::opinion_unavailable,
          "BNW approach accepted a changed opinion row");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto row=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_pabDoFAccepted";
    });
    check(row!=fields.end() && row->values.size()==22,
          "BNW selected recipient has no DoF source row");
    row->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::friendship_unavailable,
          "BNW approach accepted a changed DoF row");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto row=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_pabDenouncedPlayer";
    });
    check(row!=fields.end() && row->values.size()==22,
          "BNW selected recipient has no denouncement source row");
    row->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::denounced_unavailable,
          "BNW approach accepted a changed denouncement row");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(target_owner)).diplomacy.fields;
    const auto row=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_pabDenouncedPlayer";
    });
    check(row!=fields.end() && row->values.size()==22,
          "BNW target has no reciprocal denouncement row");
    row->values[static_cast<std::size_t>(request.owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::reciprocal_denouncement_unavailable,
          "BNW approach accepted a changed target DiplomacyAI row");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto row=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name &&
            std::string_view(f.name)=="m_pabPlayerBrokenMilitaryPromise";
    });
    check(row!=fields.end() && row->values.size()==22,
          "BNW selected recipient has no military promise row");
    row->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::promise_unavailable,
          "BNW approach accepted a changed military promise row");
    }
    {
    auto changed=synthetic;
    auto &fields=changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).diplomacy.fields;
    const auto row=std::find_if(fields.begin(),fields.end(),[](const auto &f) {
        return f.name && std::string_view(f.name)=="m_paeMilitaryThreat";
    });
    check(row!=fields.end() && row->values.size()==63,
          "BNW selected recipient has no military threat row");
    row->values[static_cast<std::size_t>(target_owner)]=99;
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::military_threat_unavailable,
          "BNW approach accepted a changed military threat row");
    }
    {
    auto changed=synthetic;
    set_player_scalar(changed.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)),"m_iNumNukeUnits",99);
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::nuke_inputs_unavailable,
          "BNW approach accepted a changed owned nuke counter");
    }
    {
    auto changed=synthetic;
    const auto *nuke=types.find("FLAVOR_USE_NUKE");
    check(nuke && nuke->table=="Flavors" && nuke->id>=0,
          "BNW active nuke flavor type unavailable");
    auto &blocks=changed.world.all_players->players.at(
        static_cast<std::size_t>(target_owner)).knowledge.blocks;
    const auto manager=std::find_if(blocks.begin(),blocks.end(),[](const auto &b) {
        return b.name && std::string_view(b.name)=="flavor_manager";
    });
    check(manager!=blocks.end(),"BNW target FlavorManager block unavailable");
    const auto personality=std::find_if(
        manager->arrays.begin(),manager->arrays.end(),[](const auto &a) {
            return a.name && std::string_view(a.name)=="personality_flavors";
        });
    check(personality!=manager->arrays.end() &&
          static_cast<std::size_t>(nuke->id)<personality->values.size(),
          "BNW target nuke personality flavor unavailable");
    ++personality->values[static_cast<std::size_t>(nuke->id)];
    check(preflight_bnw_first_contact_approach_after_load(
              source,changed,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::nuke_inputs_unavailable,
          "BNW approach accepted a changed owned personality flavor");
    }
    scratchpad.invalidate();
    check(!scratchpad.current() &&
          !scratchpad.value_at(target_owner).has_value() &&
          preflight_bnw_first_contact_approach_after_load(
              source,synthetic,request,database,types,defines,
              host.identity(),scratchpad).gate==
              BnwFirstContactApproachAfterLoadGate::scratchpad_foreign_or_stale &&
          approach.gate==BnwFirstContactApproachGate::scratchpad_unrestored,
          "BNW scratchpad diagnostic silently enabled first-contact approach");
    {
    auto wrong_identity=host.identity();
    wrong_identity.payload_sha256=std::string(64,'0');
    const auto foreign=restore_bnw_diplomacy_scratchpad_after_player_read(
        source,state,database,wrong_identity);
    check(!foreign.current() &&
          foreign.gate()==
              BnwDiplomacyScratchpadLoadGate::source_identity_inconsistent,
          "BNW scratchpad restoration accepted foreign source identity");
    }
    {
    auto advanced=state;
    ++advanced.setup.game_turn;
    const auto stale=restore_bnw_diplomacy_scratchpad_after_player_read(
        source,advanced,database,host.identity());
    check(!stale.current() &&
          stale.gate()==BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary,
          "BNW scratchpad restoration accepted a later live game turn");
    }
    {
    auto stale_might=synthetic;
    set_player_scalar(stale_might.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)),"m_iTurnMightRecomputed",
        state.maintenance.elapsed_turns-1);
    const auto stale_gate=preflight_bnw_first_contact_military(
        source,stale_might,request,database,types,defines,host.identity());
    check(stale_gate.source_verified && !stale_gate.first_call_completed &&
          stale_gate.gate==
              BnwFirstContactMilitaryGate::own_might_lazy_recompute,
          "BNW first-contact accepted stale lazy might cache");
    const auto stale_economic=preflight_bnw_first_contact_economic(
        source,stale_might,request,database,types,defines,host.identity());
    check(!stale_economic.economic_call_completed &&
          stale_economic.gate==
              BnwFirstContactEconomicGate::earlier_military_unavailable &&
          stale_economic.military_gate==
              BnwFirstContactMilitaryGate::own_might_lazy_recompute,
          "BNW first-contact economic bypassed stale military cache");
    const auto stale_posture=preflight_bnw_first_contact_posture(
        source,stale_might,request,database,types,defines,host.identity());
    check(!stale_posture.military_posture_call_completed &&
          stale_posture.gate==
              BnwFirstContactPostureGate::earlier_economic_unavailable,
          "BNW first-contact posture bypassed stale military cache");
    const auto stale_expansion=preflight_bnw_first_contact_expansion(
        source,stale_might,request,database,types,defines,host.identity());
    check(!stale_expansion.expansion_call_completed &&
          stale_expansion.gate==
              BnwFirstContactExpansionGate::earlier_posture_unavailable,
          "BNW first-contact expansion bypassed stale military cache");
    const auto stale_target=preflight_bnw_first_contact_target(
        source,stale_might,request,database,types,defines,host.identity());
    check(!stale_target.target_value_call_completed &&
          stale_target.gate==
              BnwFirstContactTargetGate::earlier_expansion_unavailable,
          "BNW first-contact target value bypassed stale military cache");
    const auto stale_relationship=preflight_bnw_first_contact_relationship(
        source,stale_might,request,database,types,defines,host.identity());
    check(!stale_relationship.major_opinion_call_completed &&
          stale_relationship.gate==
              BnwFirstContactRelationshipGate::earlier_target_value_unavailable,
          "BNW first-contact relationship bypassed stale military cache");
    const auto stale_approach=preflight_bnw_first_contact_approach(
        source,stale_might,request,database,types,defines,host.identity());
    check(!stale_approach.major_branch_verified &&
          stale_approach.gate==
              BnwFirstContactApproachGate::earlier_relationship_unavailable,
          "BNW first-contact approach bypassed stale military cache");
    }
    {
    auto wrong_clock=synthetic;
    ++wrong_clock.maintenance.elapsed_turns;
    const auto rejected_clock=preflight_bnw_first_contact_military(
        source,wrong_clock,request,database,types,defines,host.identity());
    check(rejected_clock.source_verified &&
          rejected_clock.gate==
              BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
          "BNW first-contact accepted divergent elapsed clock mirrors");
    const auto rejected_economic=preflight_bnw_first_contact_economic(
        source,wrong_clock,request,database,types,defines,host.identity());
    check(!rejected_economic.economic_call_completed &&
          rejected_economic.gate==
              BnwFirstContactEconomicGate::earlier_military_unavailable &&
          rejected_economic.military_gate==
              BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
          "BNW first-contact economic accepted divergent elapsed clock");
    const auto rejected_posture=preflight_bnw_first_contact_posture(
        source,wrong_clock,request,database,types,defines,host.identity());
    check(!rejected_posture.military_posture_call_completed &&
          rejected_posture.gate==
              BnwFirstContactPostureGate::earlier_economic_unavailable,
          "BNW first-contact posture accepted divergent elapsed clock");
    const auto rejected_expansion=preflight_bnw_first_contact_expansion(
        source,wrong_clock,request,database,types,defines,host.identity());
    check(!rejected_expansion.expansion_call_completed &&
          rejected_expansion.gate==
              BnwFirstContactExpansionGate::earlier_posture_unavailable,
          "BNW first-contact expansion accepted divergent elapsed clock");
    const auto rejected_target=preflight_bnw_first_contact_target(
        source,wrong_clock,request,database,types,defines,host.identity());
    check(!rejected_target.target_value_call_completed &&
          rejected_target.gate==
              BnwFirstContactTargetGate::earlier_expansion_unavailable,
          "BNW first-contact target value accepted divergent elapsed clock");
    }
    {
    auto wrong_roster=synthetic;
    ++wrong_roster.world.all_players->players.at(
        static_cast<std::size_t>(target_owner)).cities.header.wire_city_count;
    const auto rejected=preflight_bnw_first_contact_economic(
        source,wrong_roster,request,database,types,defines,host.identity());
    check(!rejected.economic_call_completed &&
          rejected.gate==
              BnwFirstContactEconomicGate::earlier_military_unavailable &&
          rejected.military_gate==
              BnwFirstContactMilitaryGate::earlier_contact_unavailable &&
          rejected.reason.find("city rosters disagree")!=std::string::npos,
          "BNW first-contact economic accepted divergent city roster");
    const auto rejected_posture=preflight_bnw_first_contact_posture(
        source,wrong_roster,request,database,types,defines,host.identity());
    check(!rejected_posture.military_posture_call_completed &&
          rejected_posture.gate==
              BnwFirstContactPostureGate::earlier_economic_unavailable &&
          rejected_posture.reason.find("city rosters disagree")!=
              std::string::npos,
          "BNW first-contact posture accepted divergent city roster");
    const auto rejected_expansion=preflight_bnw_first_contact_expansion(
        source,wrong_roster,request,database,types,defines,host.identity());
    check(!rejected_expansion.expansion_call_completed &&
          rejected_expansion.gate==
              BnwFirstContactExpansionGate::earlier_posture_unavailable,
          "BNW first-contact expansion accepted divergent city roster");
    const auto rejected_target=preflight_bnw_first_contact_target(
        source,wrong_roster,request,database,types,defines,host.identity());
    check(!rejected_target.target_value_call_completed &&
          rejected_target.gate==
              BnwFirstContactTargetGate::earlier_expansion_unavailable,
          "BNW first-contact target value accepted divergent city roster");
    }
    {
    auto wrong_capital=synthetic;
    ++wrong_capital.cities.players.at(request.owner).capital;
    const auto rejected=preflight_bnw_first_contact_expansion(
        source,wrong_capital,request,database,types,defines,host.identity());
    check(!rejected.expansion_call_completed &&
          rejected.gate==
              BnwFirstContactExpansionGate::capital_roster_inconsistent,
          "BNW first-contact expansion accepted divergent capital ID");
    const auto rejected_target=preflight_bnw_first_contact_target(
        source,wrong_capital,request,database,types,defines,host.identity());
    check(!rejected_target.target_value_call_completed &&
          rejected_target.gate==
              BnwFirstContactTargetGate::earlier_expansion_unavailable,
          "BNW first-contact target value accepted divergent capital ID");
    }
    {
    auto wrong_class=synthetic;
    wrong_class.cities.players.at(target_owner).minor=true;
    const auto rejected=preflight_bnw_first_contact_relationship(
        source,wrong_class,request,database,types,defines,host.identity());
    check(!rejected.major_opinion_call_completed &&
          rejected.gate==
              BnwFirstContactRelationshipGate::player_class_inconsistent &&
          rejected.reason.find("class mirrors disagree")!=std::string::npos,
          "BNW first-contact accepted divergent minor classification");
    const auto rejected_approach=preflight_bnw_first_contact_approach(
        source,wrong_class,request,database,types,defines,host.identity());
    check(!rejected_approach.major_branch_verified &&
          rejected_approach.gate==
              BnwFirstContactApproachGate::earlier_relationship_unavailable,
          "BNW major approach accepted divergent minor classification");
    }
    auto persisted_changes=first_contact.proximity_changes;
    if(std::none_of(persisted_changes.begin(),persisted_changes.end(),
                    [](const auto &change) {
                        return change.old_value!=change.new_value;
                    })) {
        // The Gandhi turn-zero pair may already have the SDK result. Give the
        // codec a different valid pair value to test an actual write/reload.
        const auto changed=persisted_changes[0].old_value==0 &&
                           persisted_changes[1].old_value==0 ? 1 : 0;
        for(auto &change:persisted_changes) {
            change.new_value=changed;
            set_proximity(first_contact.detached->world.all_players->players.at(
                static_cast<std::size_t>(change.player)),change.other,changed);
        }
    }
    const auto proximity_section=make_bnw_player_proximity_checkpoint(
        first_contact.detached->world,state.world,host.identity(),
        persisted_changes);
    check(proximity_section.id==bnw_player_proximity_checkpoint_section_id &&
          proximity_section.flags==0,
          "BNW player proximity section identity changed");
    const auto output=std::filesystem::current_path() /
        ("bnw_proximity_"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count())+
         ".CivisSave");
    saves::write_civis_save(output,{1,0,{proximity_section}});
    const auto document=saves::read_civis_save(output);
    std::filesystem::remove(output);
    check(document.sections.size()==1,
          "BNW player proximity atomic CivisSave round-trip lost section");
    auto imported_proximity=saves::make_civis_import_document(source.source,source_hash);
    imported_proximity.sections.push_back(proximity_section);
    const auto proximity_output=std::filesystem::current_path() /
        ("bnw_proximity_import_"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count())+
         ".CivisSave");
    saves::write_civis_save(proximity_output,imported_proximity);
    const auto loaded_proximity=saves::read_loaded_save(proximity_output);
    std::filesystem::remove(proximity_output);
    check(loaded_proximity.live_bnw_player_proximity==proximity_section.bytes,
          "BNW player proximity section was lost by full Civis import");
    bool unclaimed_proximity_rejected=false;
    try {
        (void)saves::observe_loaded_session(loaded_proximity,{});
    } catch(const std::exception &) {unclaimed_proximity_rejected=true;}
    check(unclaimed_proximity_rejected,
          "Unclaimed BNW player proximity reached source-only observation");
    {
    bool found=false,rejected=false;
    for(auto &field:first_contact.detached->world.all_players->players.at(
            static_cast<std::size_t>(request.owner)).core.fields)
        if(field.name && std::string_view(field.name)=="m_iTotalLandScored" &&
           field.values.size()==1) {
            field.values[0]^=1;found=true;
            try {
                (void)make_bnw_player_proximity_checkpoint(
                    first_contact.detached->world,state.world,host.identity(),
                    persisted_changes);
            } catch(const std::exception &) {rejected=true;}
            field.values[0]^=1;
            break;
        }
    check(found && rejected,
          "BNW proximity writer accepted another player-core mutation");
    }
    {
    auto restored=state.world;
    restore_bnw_player_proximity_checkpoint(document.sections[0],restored,
        state.world,host.identity());
    for(const auto &change:persisted_changes)
        check(proximity(restored.all_players->players.at(
                  static_cast<std::size_t>(change.player)),change.other)==
                  change.new_value,
              "BNW player proximity checkpoint round-trip lost value");
    }
    {
    auto attempted=state.world;
    auto tampered=document.sections[0];
    tampered.bytes[tampered.bytes.size()-8]^=1; // second old value
    bool rejected=false;
    try {
        restore_bnw_player_proximity_checkpoint(tampered,attempted,
            state.world,host.identity());
    } catch(const std::exception &) {rejected=true;}
    check(rejected &&
          proximity(attempted.all_players->players.at(
              static_cast<std::size_t>(request.owner)),target_owner)==
              persisted_changes[0].old_value &&
          proximity(attempted.all_players->players.at(
              static_cast<std::size_t>(target_owner)),request.owner)==
              persisted_changes[1].old_value,
          "BNW proximity restore partially applied after second-slot rejection");
    auto wrong_rules=host.identity();
    wrong_rules.rules_sha256=std::string(64,'0');
    rejected=false;
    try {
        restore_bnw_player_proximity_checkpoint(document.sections[0],attempted,
            state.world,wrong_rules);
    } catch(const std::exception &) {rejected=true;}
    check(rejected &&
          proximity(attempted.all_players->players.at(
              static_cast<std::size_t>(request.owner)),target_owner)==
              persisted_changes[0].old_value,
          "BNW proximity restore accepted foreign active rules");
    }
    {
    auto malformed=state.world;
    ++malformed.all_players->players.at(
        static_cast<std::size_t>(request.owner)).cities.header.wire_city_count;
    bool rejected=false;
    try {
        restore_bnw_player_proximity_checkpoint(document.sections[0],malformed,
            state.world,host.identity());
    } catch(const std::exception &) {rejected=true;}
    check(rejected && proximity(malformed.all_players->players.at(
              static_cast<std::size_t>(request.owner)),target_owner)==
              proximity(state.world.all_players->players.at(
                  static_cast<std::size_t>(request.owner)),target_owner),
          "BNW proximity restore accepted corrupt roster or partially applied");
    }
    {
    auto bad_cities=synthetic;
    ++bad_cities.world.all_players->players.at(
        static_cast<std::size_t>(request.owner)).cities.header.wire_city_count;
    const auto bad_roster=apply_bnw_sight_first_contact_detached(
        source,bad_cities,request,database,types,defines,host.identity());
    check(!bad_roster.calculated && !bad_roster.detached &&
          bad_roster.proximity_changes.empty() &&
          bad_roster.blocker==
              BnwSightFirstContactBlocker::city_roster_unavailable,
          "BNW proximity accepted a divergent raw city roster");
    }
    auto wrong_proximity_identity=host.identity();
    wrong_proximity_identity.rules_sha256=std::string(64,'0');
    const auto wrong_proximity=apply_bnw_sight_first_contact_detached(
        source,synthetic,request,database,types,defines,
        wrong_proximity_identity);
    check(!wrong_proximity.calculated && !wrong_proximity.detached &&
          wrong_proximity.proximity_changes.empty() &&
          wrong_proximity.blocker==
              BnwSightFirstContactBlocker::earlier_preflight_rejected,
          "BNW proximity accepted foreign rules identity");
    }
    const auto tech_rules=content::load_technology_rules(database,types,
        content::TechnologyProfile::bnw,5);
    std::int64_t queued_tech=-1;
    for(std::uint64_t i=0;i<tech_rules.collection.slots;++i)
        if(tech_rules.find(i)) {
            queued_tech=static_cast<std::int64_t>(i);break;
        }
    check(queued_tech>=0,"Selected BNW has no queued technology rule");
    {
    auto queued=synthetic;
    queued.player_research.players.at(target_owner).queue={queued_tech};
    queued.research_turn.players.at(target_owner).researching={queued_tech};
    const auto cleared=apply_bnw_sight_research_clear_detached(
        source,queued,request,database,types,defines,host.identity());
    check(cleared.calculated && cleared.detached &&
          cleared.cleared_players==std::vector<std::int64_t>{target_owner} &&
          cleared.detached->player_research.players.at(target_owner).queue.empty() &&
          cleared.detached->research_turn.players.at(target_owner)
              .researching.empty() &&
          queued.player_research.players.at(target_owner).queue.size()==1 &&
          queued.research_turn.players.at(target_owner).researching.size()==1 &&
          cleared.research_checkpoint && cleared.turn_checkpoint &&
          cleared.selection_checkpoint && !cleared.can_commit(),
          cleared.reason.c_str());
    {
    auto reloaded=queued;
    restore_research_checkpoint(cleared.research_checkpoint->common,
        cleared.research_checkpoint->profile,reloaded.technologies,
        reloaded.player_research,reloaded.setup,tech_rules,host.identity());
    restore_research_turn_checkpoint(*cleared.turn_checkpoint,
        reloaded.research_turn,reloaded.setup,tech_rules,host.identity());
    restore_research_selection_checkpoint(*cleared.selection_checkpoint,
        reloaded.research_selection,reloaded.player_research,
        reloaded.research_turn,reloaded.setup,tech_rules,host.identity(),
        true,true);
    check(reloaded.player_research.players.at(target_owner).queue.empty() &&
          reloaded.research_turn.players.at(target_owner).researching.empty(),
          "BNW meet queue clear companion checkpoint round-trip disagrees");
    }
    {
    auto foreign=queued;
    auto wrong_checkpoint=host.identity();
    wrong_checkpoint.rules_sha256=std::string(64,'0');
    bool wrong_rejected=false;
    try {
        restore_research_selection_checkpoint(*cleared.selection_checkpoint,
            foreign.research_selection,foreign.player_research,
            foreign.research_turn,foreign.setup,tech_rules,wrong_checkpoint);
    } catch(const std::exception &) {wrong_rejected=true;}
    check(wrong_rejected &&
          foreign.player_research.players.at(target_owner).queue.size()==1 &&
          foreign.research_turn.players.at(target_owner).researching.size()==1,
          "BNW meet research checkpoint accepted foreign rules identity");
    }
    {
    auto active_queue=queued;
    active_queue.world.pregame->archive.integers["active_player"]=target_owner;
    const auto active_clear=apply_bnw_sight_research_clear_detached(
        source,active_queue,request,database,types,defines,host.identity());
    check(active_clear.calculated && active_clear.detached &&
          active_clear.popup_dirty_intents==
              std::vector<std::pair<std::int64_t,std::int64_t>>{
                  {target_owner,queued_tech}} &&
          active_clear.active_team_dirty_intents==
              std::vector<std::int64_t>{target_team},
          "BNW meet active research UI intents differ from SDK order");
    }
    {
    auto mismatched=queued;
    mismatched.research_turn.players.at(target_owner).researching.clear();
    const auto mismatch=apply_bnw_sight_research_clear_detached(
        source,mismatched,request,database,types,defines,host.identity());
    check(!mismatch.calculated && !mismatch.detached &&
          mismatch.reason.find("queue and researching flags disagree")!=
              std::string::npos,
          "BNW meet queue clear accepted divergent owned views");
    }
    }
    {
    auto always_war=synthetic;
    always_war.setup.game_options.insert(always_war.setup.game_options.begin(),
        {"GAMEOPTION_ALWAYS_WAR",1});
    always_war.world.pregame->game_options.insert(
        always_war.world.pregame->game_options.begin(),
        {"GAMEOPTION_ALWAYS_WAR",1});
    const auto war_block=preflight_bnw_sight_diplomacy(
        source,always_war,request,database,types,defines,host.identity());
    check(!war_block.calculated && !war_block.detached &&
          war_block.intents.size()==1 &&
          war_block.intents[0].call==BnwMeetDiplomacyCall::declare_war &&
          war_block.reason.find("declareWar")!=std::string::npos,
          "BNW always-war contact was not blocked before diplomacy");
    }
    {
    auto stale_roster=synthetic;
    stale_roster.setup.players.at(request.owner).alive=false;
    const auto stale=preflight_bnw_sight_diplomacy(
        source,stale_roster,request,database,types,defines,host.identity());
    check(!stale.calculated && !stale.detached &&
          stale.reason.find("setup view disagrees")!=std::string::npos,
          "BNW diplomacy accepted divergent player alive views");
    }
    {
    auto grant=synthetic;
    team_array(grant.world.teams->teams.at(revealing),"tech_share")[0]=1;
    std::int64_t shared_tech=-1;
    for(std::uint64_t i=0;i<tech_rules.collection.slots;++i)
        if(tech_rules.find(i) &&
           !grant.technologies.at(revealing).has_tech(
                static_cast<std::int64_t>(i))) {
            shared_tech=static_cast<std::int64_t>(i);break;
        }
    check(shared_tech>=0,"Selected BNW lacks a shareable tech slot");
    grant.technologies.at(target_team).set_has_tech(shared_tech,true);
    const auto blocked_grant=apply_bnw_sight_tech_share_detached(
        source,grant,request,database,types,defines,host.identity());
    check(!blocked_grant.calculated && !blocked_grant.detached &&
          !blocked_grant.checks.empty() &&
          blocked_grant.checks[0].first_grant_tech &&
          blocked_grant.reason.find("setHasTech")!=std::string::npos &&
          team_array(grant.world.teams->teams.at(revealing),"has_met")
            .at(target_team)==0,
          "BNW tech-share grant did not block before publication");
    }
    {
    auto asymmetric=synthetic;
    team_array(asymmetric.world.teams->teams.at(target_team),"has_met")
        .at(revealing)=1;
    const auto rejected=apply_bnw_sight_meets_detached(source,asymmetric,request,
        database,types,defines,host.identity());
    check(!rejected.calculated && !rejected.detached &&
          rejected.reason.find("asymmetric")!=std::string::npos,
          "BNW asymmetric prior meet did not rollback");
    }
    auto wrong=host.identity();wrong.payload_sha256=std::string(64,'0');
    check(!apply_bnw_sight_meets_detached(source,state,request,database,types,
        defines,wrong).calculated,"BNW meet accepted wrong payload identity");
    check(identify_file(save).sha256==source_hash,
          "BNW detached meet changed original selected save");
    std::cout << "BNW detached meet " << revealing << "->" << target_team <<
        " at turn " << turn << '\n';
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW detached sight meet\n";
        return 0;
    } catch(const std::exception &e) {
        std::cerr << "FAIL BNW detached sight meet: " << e.what() << '\n';
        return 1;
    }
}
