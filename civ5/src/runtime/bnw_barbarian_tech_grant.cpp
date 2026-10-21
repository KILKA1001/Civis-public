#include "civ5/runtime/bnw_barbarian_tech_grant.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::int64_t field(const saves::TeamObservation &team,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:team.fields)
        if(entry.name && name==entry.name) {
            require(!found,"Duplicate BNW barbarian team field");
            found=&entry;
        }
    require(found,"Missing BNW barbarian team field");
    return found->value;
}
bool is_minor(const LiveSessionState &state,const std::vector<bool> &minor,
              std::int64_t team) {
    // CvTeam::isMinorCiv scans player slots 22..63 and stops at the first
    // living member of this team.
    for(std::int64_t id=22;id<64;++id) {
        const auto player=state.setup.players.find(id);
        require(player!=state.setup.players.end(),"BNW barbarian player roster incomplete");
        if(player->second.alive && player->second.team==team)
            return minor.at(static_cast<std::size_t>(id));
    }
    return false;
}
std::int64_t percent(const content::GlobalDefineCache &defines) {
    require(defines.profile()==content::GlobalDefineProfile::bnw,
            "BNW barbarian technology requires BNW defines");
    const auto *value=defines.find("BARBARIAN_TECH_PERCENT");
    require(value && value->lookup.found && value->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(value->lookup.value),
            "BNW BARBARIAN_TECH_PERCENT unavailable");
    const auto result=std::get<std::int64_t>(value->lookup.value);
    require(result>=0 && result<=INT32_MAX,
            "BNW BARBARIAN_TECH_PERCENT outside source i32");
    return result;
}
bool boolean(const content::TechnologyInfo &tech,std::string_view name) {
    const auto it=tech.booleans.find(name);
    require(it!=tech.booleans.end(),"BNW barbarian technology effect rule unavailable");
    return it->second;
}
std::vector<BnwBarbarianTechEffect> effects(const content::TechnologyInfo &tech) {
    using E=BnwBarbarianTechEffect;
    std::vector<E> result;
    if(boolean(tech,"Repeat")) {
        result.push_back(E::repeat_count_and_research_reset);
        result.push_back(E::repeat_future_technology_score);
    } else {
        result.push_back(E::team_technology_write);
        result.push_back(E::team_set_has_tech_lua);
        result.push_back(E::team_city_strength);
        result.push_back(E::resource_reveal_and_connection);
    }
    // For a repeat tech the team HasTech flag remains false, so the following
    // CvTeam::setHasTech block is skipped. All normal grants enter it.
    if(!boolean(tech,"Repeat")) {
        result.push_back(E::process_technology);
        result.push_back(E::team_tech_researched_lua); // inside processTech
        result.push_back(E::artifact_notifications);
        result.push_back(E::era_evaluation);
        if(boolean(tech,"MapVisible")) result.push_back(E::map_reveal);
        result.push_back(E::best_route);
        result.push_back(E::team_player_research_and_yields);
        result.push_back(E::technology_sharing);
        result.push_back(E::owed_culture_buildings);
    }
    // bAnnounce=false suppresses research announcements but not the final
    // happiness, interface notification and gameplay engine callbacks.
    result.push_back(E::team_player_happiness);
    result.push_back(E::interface_notification);
    result.push_back(E::gameplay_technology_event);
    return result;
}
} // namespace

BnwBarbarianTechGate plan_bnw_barbarian_technology_gate(
    const LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw && state.world.teams &&
            state.world.teams->layout==bnw && state.world.pregame &&
            state.world.pregame->layout==bnw,
            "BNW barbarian technology requires a decoded BNW world");
    const auto &teams=*state.world.teams;
    require(teams.dimensions.teams==64 && teams.dimensions.civ_teams==63 &&
            teams.dimensions.major==22 && teams.teams.size()==64 &&
            state.technologies.size()==64 && state.setup.players.size()==64,
            "BNW barbarian technology roster differs from Expansion2");
    const auto minor=state.world.pregame->archive.flag_arrays.find("minor_nation_civs");
    require(minor!=state.world.pregame->archive.flag_arrays.end() &&
            minor->second.size()==64,
            "BNW barbarian minor-nation flags unavailable");
    require(technologies.profile==content::TechnologyProfile::bnw &&
            technologies.all_succeeded() && technologies.collection.holes==0 &&
            technologies.collection.slots<=INT32_MAX,
            "BNW barbarian technology rules incomplete");
    BnwBarbarianTechGate gate;
    gate.percent=percent(defines);
    std::vector<std::int64_t> major_teams;
    for(std::int64_t id=0;id<64;++id) {
        const auto &team=teams.teams.at(static_cast<std::size_t>(id));
        const auto &owned=state.technologies.at(static_cast<std::size_t>(id));
        require(team.index==static_cast<std::uint64_t>(id) && field(team,"id")==id &&
                owned.team()==id && owned.profile()==content::TechnologyProfile::bnw &&
                owned.slots()==technologies.collection.slots &&
                owned.remap().all_mapped() && owned.remap().duplicates==0 &&
                owned.remap().holes==0,
                "BNW barbarian technology team identity or mapping invalid");
        const auto alive=field(team,"alive_count");
        require(alive>=0 && alive<=64,"BNW barbarian alive count invalid");
        if(id<63 && alive>0 && !is_minor(state,minor->second,id))
            major_teams.push_back(id);
    }
    // CvTeam::DoBarbarianTech is called only for a living barbarian team.
    require(field(teams.teams[63],"alive_count")>0,
            "BNW barbarian technology requires living barbarian team");
    require(!major_teams.empty(),"BNW barbarian technology has no living major team");
    gate.alive_major_teams=static_cast<std::int64_t>(major_teams.size());
    gate.threshold=std::max<std::int64_t>(1,gate.alive_major_teams*gate.percent/100);
    const auto &barbarians=state.technologies[63];
    std::int64_t leader=-1;
    for(std::int64_t player=0;player<64;++player) {
        const auto &slot=state.setup.players.at(player);
        if(slot.team==63 && slot.alive) {leader=player;break;}
    }
    if(leader<0)
        for(std::int64_t player=0;player<64;++player)
            if(state.setup.players.at(player).team==63) {leader=player;break;}
    require(leader>=0,"BNW barbarian team leader unavailable");
    for(std::uint64_t tech=0;tech<technologies.collection.slots;++tech) {
        const auto *rule=technologies.find(tech);
        require(rule,"BNW barbarian technology rule slot missing");
        if(barbarians.has_tech(static_cast<std::int64_t>(tech))) continue;
        std::int64_t owners=0;
        for(const auto team:major_teams)
            if(state.technologies[static_cast<std::size_t>(team)].has_tech(
                   static_cast<std::int64_t>(tech))) ++owners;
        if(owners>=gate.threshold)
            gate.grants.push_back({static_cast<std::int64_t>(tech),rule->base.type,
                                   owners,rule->era,effects(*rule)});
    }
    gate.safe=gate.grants.empty();
    if(!gate.safe)
        gate.blocker="BNW barbarian tech " + gate.grants.front().type +
                     " requires source-ordered CvTeam::setHasTech effects and Lua/engine callbacks";
    return gate;
}

LiveActionResult run_bnw_barbarian_technology_gate(
    LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines) {
    try {
        const auto gate=plan_bnw_barbarian_technology_gate(state,technologies,defines);
        if(!gate.safe) return {LiveActionOutcome::rejected,gate.blocker};
        return {LiveActionOutcome::committed,{}};
    } catch(const std::exception &error) {
        return {LiveActionOutcome::rejected,error.what()};
    }
}

} // namespace civ5::runtime
