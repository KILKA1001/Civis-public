#include "civ5/runtime/bnw_first_contact_approach_war_gate.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
const saves::TeamObservation &team(const saves::SavedSessionSnapshot &world,
    std::int64_t id) {
    if(!world.teams || world.teams->layout!=
            saves::GameStateLayout::expansion2_version1 ||
       world.teams->teams.size()!=64 || id<0 || id>=63 ||
       world.teams->teams[static_cast<std::size_t>(id)].index!=
           static_cast<std::size_t>(id))
        throw std::runtime_error("BNW canDeclareWar team roster unavailable");
    return world.teams->teams[static_cast<std::size_t>(id)];
}
std::int64_t scalar(const saves::TeamObservation &observation,
    std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:observation.fields)
        if(field.name && name==field.name) {
            if(found) throw std::runtime_error("BNW duplicate team scalar");
            found=&field;
        }
    if(!found) throw std::runtime_error("BNW canDeclareWar team scalar unavailable");
    return found->value;
}
std::int64_t relation(const saves::TeamObservation &observation,
    std::string_view name,std::int64_t other) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:observation.arrays)
        if(array.name && name==array.name) {
            if(found) throw std::runtime_error("BNW duplicate team relation");
            found=&array;
        }
    if(!found || found->columns!=1 || found->values.size()!=64 ||
       other<0 || other>=63)
        throw std::runtime_error("BNW canDeclareWar team relation unavailable");
    const auto value=found->values[static_cast<std::size_t>(other)];
    if(value<0 || value>1)
        throw std::runtime_error("BNW canDeclareWar team relation is not boolean");
    return value;
}
std::int64_t unchanged_relation(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t own,std::int64_t other,
    std::string_view name) {
    const auto a=relation(team(source.snapshot,own),name,other);
    const auto b=relation(team(state.world,own),name,other);
    if(a!=b)
        throw std::runtime_error("BNW canDeclareWar source/owned relation differs");
    return b;
}
bool owned_met(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t own,std::int64_t other) {
    const auto a=relation(team(source.snapshot,own),"has_met",other);
    const auto b=relation(team(state.world,own),"has_met",other);
    // This quote starts from the pre-contact detached world. CvTeam::meet
    // stages has_met in both directions before the later approach update;
    // the transition must not be written into the caller's live state here.
    if(a!=0 || b!=0)
        throw std::runtime_error("BNW first-contact has_met transition unavailable");
    return true;
}
bool team_alive(const saves::SavedSession &source,const LiveSessionState &state,
    std::int64_t id) {
    const auto a=scalar(team(source.snapshot,id),"alive_count");
    const auto b=scalar(team(state.world,id),"alive_count");
    if(a!=b || a<0 || a>64 || state.setup.players.size()!=64)
        throw std::runtime_error("BNW source/owned team alive count inconsistent");
    std::int64_t live=0;
    for(const auto &[_,player]:state.setup.players)
        if(player.alive && player.team==id) ++live;
    if(live!=b)
        throw std::runtime_error("BNW team alive count differs from player roster");
    return b>0;
}
std::string_view c_string(std::string_view text) {
    return text.substr(0,text.find('\0'));
}
std::optional<std::int64_t> first_option(
    const std::vector<saves::PregameOption> &options,std::string_view name) {
    for(const auto &option:options)
        if(c_string(option.name)==name) return option.value;
    return {};
}
bool game_option(const saves::SavedSession &source,
    const LiveSessionState &state,const content::RulesDatabase &database,
    std::string_view name) {
    if(!source.snapshot.pregame || !state.world.pregame ||
       source.snapshot.pregame->layout!=
           saves::GameStateLayout::expansion2_version1 ||
       state.world.pregame->layout!=
           saves::GameStateLayout::expansion2_version1)
        throw std::runtime_error("BNW canDeclareWar game options unavailable");
    const auto &saved=source.snapshot.pregame->game_options;
    const auto &world=state.world.pregame->game_options;
    if(saved.size()!=world.size())
        throw std::runtime_error("BNW source/owned game option roster differs");
    for(std::size_t i=0;i<saved.size();++i)
        if(saved[i].name!=world[i].name || saved[i].value!=world[i].value)
            throw std::runtime_error("BNW source/owned game option differs");
    const auto raw=first_option(saved,name);
    const auto owned=first_option(state.setup.game_options,name);
    if(raw && !owned)
        throw std::runtime_error("BNW saved game option missing from owned setup");
    std::optional<std::int64_t> database_default;
    if(!raw) {
        const auto report=database.query_rows({
            "SELECT \"Default\" FROM GameOptions WHERE Type=? LIMIT 1",
            {std::string(name)}},[&](const auto &row) {
                database_default=row.integer_at(0);
            },1);
        if(!report.all_succeeded())
            throw std::runtime_error("BNW game option default lookup failed");
    }
    const auto value=raw.value_or(database_default.value_or(0));
    if(owned && *owned!=value)
        throw std::runtime_error("BNW setup game option differs from source/rules");
    return value==1; // CvGame::isOption compares with one.
}
} // namespace

BnwApproachWarGateQuote quote_bnw_first_contact_approach_war_gate(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad) {
    BnwApproachWarGateQuote quote;
    const auto random=quote_bnw_first_contact_approach_random(
        source,state,request,database,types,defines,identity,scratchpad);
    if(random.gate!=BnwApproachRandomGate::next_can_declare_war_unimplemented) {
        quote.reason="BNW JonRand quote unavailable: "+random.reason;
        return quote;
    }
    quote.weights=random.weights;
    quote.rng_before=random.rng_before;
    quote.rng_after=random.rng_after;
    const auto prior=preflight_bnw_first_contact_approach(
        source,state,request,database,types,defines,identity);
    quote.recipient_team=prior.recipient_team;
    quote.target_team=prior.target_team;
    quote.gate=BnwApproachWarGate::team_state_unavailable;
    try {
        const auto own=quote.recipient_team,other=quote.target_team;
        if(own<0 || other<0 || own>=63 || other>=63 || own==other)
            throw std::runtime_error("BNW first-contact team pair unavailable");
        // Expansion2 CvTeam.cpp:970-1025, then CvDiplomacyAI.cpp:3421-3447.
        // These early returns precede the mandatory Lua CanDeclareWar test.
        if(!team_alive(source,state,own) || !team_alive(source,state,other))
            goto cannot_declare;
        quote.at_war=unchanged_relation(source,state,own,other,"at_war")!=0;
        if(quote.at_war) goto cannot_declare;
        (void)owned_met(source,state,own,other);
        (void)owned_met(source,state,other,own);
        if(unchanged_relation(source,state,own,other,"force_peace")!=0)
            goto cannot_declare;
        // CvTeam::canChangeWarPeace: options, same-team check and both
        // permanent-war-peace directions, all before the second option read.
        quote.gate=BnwApproachWarGate::game_option_unavailable;
        if(game_option(source,state,database,"GAMEOPTION_NO_CHANGING_WAR_PEACE") ||
           game_option(source,state,database,"GAMEOPTION_ALWAYS_PEACE") ||
           game_option(source,state,database,"GAMEOPTION_ALWAYS_WAR"))
            goto cannot_declare;
        quote.gate=BnwApproachWarGate::team_state_unavailable;
        if(unchanged_relation(source,state,own,other,"permanent_war_peace") ||
           unchanged_relation(source,state,other,own,"permanent_war_peace"))
            goto cannot_declare;
        quote.gate=BnwApproachWarGate::game_option_unavailable;
        if(game_option(source,state,database,"GAMEOPTION_ALWAYS_PEACE"))
            goto cannot_declare;
        quote.can_declare_without_lua=true;
        quote.gate=BnwApproachWarGate::lua_can_declare_war_unavailable;
        quote.reason="BNW canDeclareWar reached mandatory Lua CanDeclareWar test";
        return quote;
    cannot_declare:
        // canDeclareWar returned false. The caller reads isAtWar again; if
        // already at war it preserves WAR weight, otherwise clears it.
        quote.gate=BnwApproachWarGate::team_state_unavailable;
        quote.at_war=unchanged_relation(source,state,own,other,"at_war")!=0;
        if(!quote.at_war) {
            quote.weights[0]=0;
            quote.war_weight_zeroed=true;
        }
        quote.gate=BnwApproachWarGate::next_human_difficulty_unimplemented;
        quote.reason="BNW canDeclareWar early return quoted; same-team and human difficulty branches remain";
    } catch(const std::exception &error) {
        quote.reason=error.what();
    }
    return quote;
}

} // namespace civ5::runtime
