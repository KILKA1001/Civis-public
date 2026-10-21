#include "civ5/runtime/bnw_sight_meet_effect_plan.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
void push(BnwSightMeetEffectPlan &plan,BnwMeetEffectKind kind,
          std::int64_t team,std::int64_t other,
          std::int64_t player=-1,std::int64_t other_player=-1,
          std::int64_t third=-1,bool external=false) {
    require(plan.events.size()<100000,
            "BNW meet effect event budget exceeded");
    plan.events.push_back({kind,team,other,player,other_player,third,external});
}
bool minor_team(const LiveSessionState &state,std::int64_t team) {
    require(state.world.pregame && state.setup.players.size()==64 &&
            team>=0 && team<63,
            "BNW meet effect team/roster is unavailable");
    const auto &flags=state.world.pregame->archive.flag_arrays;
    const auto it=flags.find("minor_nation_civs");
    require(it!=flags.end() && it->second.size()>=64,
            "BNW meet effect minor flags are unavailable");
    // CvTeam::isMinorCiv scans players 22..63 and returns for the first
    // living member of this team.
    for(std::int64_t p=22;p<64;++p) {
        const auto found=state.setup.players.find(p);
        require(found!=state.setup.players.end(),
                "BNW meet effect player roster is incomplete");
        if(found->second.alive && found->second.team==team)
            return it->second.at(static_cast<std::size_t>(p));
    }
    return false;
}
std::int64_t active_team(const LiveSessionState &state) {
    require(static_cast<bool>(state.world.pregame),
            "BNW meet effect active player setup is unavailable");
    const auto it=state.world.pregame->archive.integers.find("active_player");
    require(it!=state.world.pregame->archive.integers.end() &&
            it->second>=0 && it->second<64 &&
            state.setup.players.contains(it->second),
            "BNW meet effect active player is unavailable");
    return state.setup.players.at(it->second).team;
}
BnwMeetEffectKind translate(BnwMeetDiplomacyCall call) {
    switch(call) {
    case BnwMeetDiplomacyCall::proximity_our_to_their:
    case BnwMeetDiplomacyCall::proximity_their_to_our:
        return BnwMeetEffectKind::proximity;
    case BnwMeetDiplomacyCall::first_contact:
        return BnwMeetEffectKind::first_contact_ai;
    case BnwMeetDiplomacyCall::third_party_met_someone:
        return BnwMeetEffectKind::third_party_ai;
    case BnwMeetDiplomacyCall::clear_research_queue:
        return BnwMeetEffectKind::clear_research_queue;
    case BnwMeetDiplomacyCall::declare_war:
        throw std::runtime_error(
            "BNW meet always-war declareWar is unavailable before contact");
    }
    throw std::runtime_error("Unknown BNW meet diplomacy callback");
}
bool external(BnwMeetEffectKind kind) {
    switch(kind) {
    case BnwMeetEffectKind::first_contact_ai:
    case BnwMeetEffectKind::third_party_ai:
    case BnwMeetEffectKind::proximity:
    case BnwMeetEffectKind::clear_research_queue:
    case BnwMeetEffectKind::score_ui_dirty:
    case BnwMeetEffectKind::gameplay_met_team:
    case BnwMeetEffectKind::lua_team_meet:
        return true;
    case BnwMeetEffectKind::has_met:
    case BnwMeetEffectKind::turn_team_met:
    case BnwMeetEffectKind::update_tech_share:
        return false;
    }
    throw std::runtime_error("Unknown BNW meet effect kind");
}
void side(BnwSightMeetEffectPlan &plan,
          const BnwSightDiplomacyPreflightResult &diplomacy,
          std::size_t &cursor,std::int64_t team,std::int64_t other,
          std::int64_t active) {
    // CvTeam::makeHasMet: flags, turn, updateTechShare, optional war,
    // player loops, research queue, UI score, minor branch, engine event.
    push(plan,BnwMeetEffectKind::has_met,team,other);
    push(plan,BnwMeetEffectKind::turn_team_met,team,other);
    push(plan,BnwMeetEffectKind::update_tech_share,team,other);
    for(std::size_t i=0;i<diplomacy.intents.size();++i) {
        const auto &intent=diplomacy.intents[i];
        if(intent.recipient_team!=team) continue;
        require(i==cursor && intent.other_team==other,
                "BNW meet diplomacy intents lost source side order");
        ++cursor;
        const auto kind=translate(intent.call);
        push(plan,kind,team,other,intent.recipient_player,
             intent.other_player,intent.third_player,external(kind));
    }
    if(team==active || other==active)
        push(plan,BnwMeetEffectKind::score_ui_dirty,team,other,
             -1,-1,-1,true);
    push(plan,BnwMeetEffectKind::gameplay_met_team,team,other,
         -1,-1,-1,true);
}
} // namespace

BnwSightMeetEffectPlan plan_bnw_sight_meet_effects(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightMeetEffectPlan plan;
    try {
        auto diplomacy=preflight_bnw_sight_diplomacy(
            source,state,request,database,types,defines,identity);
        require(diplomacy.calculated && diplomacy.detached,
                diplomacy.reason.empty()
                    ? "BNW meet effect plan needs no-grant diplomacy preflight"
                    : diplomacy.reason.c_str());
        const auto tech=apply_bnw_sight_tech_share_detached(
            source,state,request,database,types,defines,identity);
        require(tech.calculated && tech.detached,
                "BNW meet effect plan needs detached contact roster");
        const auto active=active_team(state);
        std::size_t cursor=0;
        for(const auto &contact:tech.contacts) {
            if(!contact.new_contact) continue;
            require(!minor_team(state,contact.revealing_team) &&
                    !minor_team(state,contact.other_team),
                    "BNW minor contact needs capital reveal, MinorCivAI and notifications");
            side(plan,diplomacy,cursor,contact.revealing_team,
                 contact.other_team,active);
            side(plan,diplomacy,cursor,contact.other_team,
                 contact.revealing_team,active);
            // CvTeam::meet calls TeamMeet once, after both makeHasMet calls.
            push(plan,BnwMeetEffectKind::lua_team_meet,
                 contact.revealing_team,contact.other_team,-1,-1,-1,true);
            plan.contacts.push_back(contact);
        }
        require(cursor==diplomacy.intents.size() && plan.contacts.size()<=1,
                "BNW meet contact/intents require interleaved contact effects");
        plan.calculated=true;
        plan.reason=plan.contacts.empty()
            ? "BNW sight has no new team contact"
            : "BNW meet effects are ordered but AI, Lua, UI and engine event owners are not executed";
    } catch(const std::exception &error) {
        plan.calculated=false;
        plan.events.clear();plan.contacts.clear();
        plan.reason=error.what();
    }
    return plan;
}

BnwMeetEffectCapabilityCheck check_bnw_meet_effect_capabilities(
    const BnwSightMeetEffectPlan &plan,
    std::span<const BnwMeetEffectKind> available) {
    BnwMeetEffectCapabilityCheck check;
    if(!plan.calculated) {
        check.reason=plan.reason.empty()
            ? "BNW meet effect plan is unavailable":plan.reason;
        return check;
    }
    for(std::size_t i=0;i<plan.events.size();++i) {
        const auto &event=plan.events[i];
        bool required=false;
        try {required=external(event.kind);}
        catch(const std::exception &error) {
            check.reason=error.what();
            check.missing_event_indices.push_back(i);
            return check;
        }
        if(required!=event.external) {
            check.reason="BNW meet external callback classification changed";
            check.missing_event_indices.push_back(i);
            return check;
        }
        if(!required) continue;
        if(std::find(available.begin(),available.end(),event.kind)==
           available.end()) check.missing_event_indices.push_back(i);
    }
    check.complete=check.missing_event_indices.empty();
    check.reason=check.complete
        ? "All external effect kinds have declared owners; effects are not executed"
        : "BNW meet external AI, Lua, UI or engine event callback is unavailable";
    return check;
}
} // namespace civ5::runtime
