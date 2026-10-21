#include "civ5/runtime/bnw_team_tech_post.hpp"
#include "civ5/content/info_types.hpp"
#include <array>
#include <limits>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;

bool flag(const content::TechnologyInfo &info,std::string_view name) {
    const auto it=info.booleans.find(name);
    return it!=info.booleans.end() && it->second;
}
std::int64_t integer(const content::TechnologyInfo &info,std::string_view name) {
    const auto it=info.integers.find(name);
    return it==info.integers.end() ? 0 : it->second;
}
bool saved_flag_matches(const saves::TeamObservation &team,
                        std::string_view technology_type,std::int64_t technology,
                        const content::InfoTypeCatalog &types) {
    const saves::PlotArrayObservation *hashes=nullptr,*flags=nullptr;
    for(const auto &array:team.arrays) {
        if(array.name && std::string_view(array.name)=="tech_hashes") hashes=&array;
        if(array.name && std::string_view(array.name)=="has_tech") flags=&array;
    }
    if(!hashes || !flags || hashes->values.size()!=flags->values.size()) return false;
    const auto hash=content::saved_info_type_hash(technology_type);
    if(!types.report().all_succeeded() || types.report().hash_collisions ||
       types.report().name_overwrites) return false;
    const auto *mapped=types.find_hash(hash);
    if(!mapped || mapped->table!="Technologies" || mapped->type!=technology_type ||
       mapped->id!=technology) return false;
    std::size_t matched=0;
    for(std::size_t row=0;row<hashes->values.size();++row) {
        if(hashes->values[row]!=static_cast<std::int64_t>(hash)) continue;
        ++matched;
        if(flags->values[row]!=1) return false;
    }
    return matched==1;
}
bool has_matching_resource_rule(const content::RulesDatabase &database,
                                const std::string &technology_type,bool &valid) {
    bool matched=false;
    const auto report=database.query_rows(
        {"SELECT TechReveal,TechCityTrade FROM Resources",{}},
        [&](const content::RulesRowView &row) {
            if(row.text("TechReveal")==technology_type ||
               row.text("TechCityTrade")==technology_type) matched=true;
        });
    valid=report.all_succeeded();
    return matched;
}
}

BnwTeamTechPostPrefix run_bnw_barbarian_tech_post_prefix(
    const LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    std::int64_t technology) {
    BnwTeamTechPostPrefix result;
    result.team=63;
    result.technology=technology;
    const auto stop=[&](BnwTeamTechPostStep step,std::string reason) {
        result.first_pending=step;
        result.pending_effects.push_back(step);
        result.blocker=std::move(reason);
        return result;
    };
    if(state.world.game_state.layout!=bnw ||
       technologies.profile!=content::TechnologyProfile::bnw ||
       !technologies.all_succeeded() || technology<0 ||
       static_cast<std::uint64_t>(technology)>=technologies.collection.slots ||
       !state.world.teams || state.world.teams->layout!=bnw ||
       state.world.teams->teams.size()!=64 || state.technologies.size()!=64)
        return stop(BnwTeamTechPostStep::city_strength,
                    "BNW post-acquisition state or active technology rules incomplete");
    const auto *info=technologies.find(static_cast<std::uint64_t>(technology));
    if(!info || info->base.type.empty() || flag(*info,"Repeat"))
        return stop(BnwTeamTechPostStep::city_strength,
                    "BNW barbarian technology is missing or repeatable");
    result.technology_type=info->base.type;
    if(!state.technologies[63].has_tech(technology) ||
       !saved_flag_matches(state.world.teams->teams[63],info->base.type,
                           technology,types))
        return stop(BnwTeamTechPostStep::city_strength,
                    "CvTeamTechs::SetHasTech must first reconcile owned and raw technology flags");

    // CvTeam::setHasTech 5112-5127: city strength of every living member.
    // A barbarian city is possible in modded content; inspect the live roster.
    if(!state.world.all_players || state.world.all_players->players.size()!=64)
        return stop(BnwTeamTechPostStep::city_strength,
                    "BNW player/city roster unavailable for city-strength update");
    for(const auto &[player,member]:state.setup.players) {
        if(!member.alive || member.team!=63) continue;
        if(player<0 || player>=64 ||
           !state.world.all_players->players[static_cast<std::size_t>(player)].cities.cities.empty())
            return stop(BnwTeamTechPostStep::city_strength,
                        "Living barbarian city requires updateStrengthValue");
    }
    result.proven_no_effect.push_back(BnwTeamTechPostStep::city_strength);

    // 5132+: a matching rule may link a resource or update an owner's totals.
    // Reject conservatively even if a map scan might later prove it absent.
    bool valid=false;
    const bool resource=has_matching_resource_rule(database,info->base.type,valid);
    if(!valid || resource)
        return stop(BnwTeamTechPostStep::resource_reveal_and_trade,
                    valid ? "Resource reveal/trade requires live plot effects" :
                            "Active BNW resource rules unavailable");
    result.proven_no_effect.push_back(BnwTeamTechPostStep::resource_reveal_and_trade);

    // CvTeam::processTech 6005-6101. These branches do nothing for a zero
    // Agriculture rule. A nonzero value must use the corresponding GameCore
    // setter, which can have effects beyond the serialized scalar.
    if(flag(*info,"AllowsWorldCongress"))
        return stop(BnwTeamTechPostStep::process_world_congress,
                    "Technology enables the World Congress");
    result.proven_no_effect.push_back(BnwTeamTechPostStep::process_world_congress);
    if(flag(*info,"TriggersArchaeologicalSites"))
        return stop(BnwTeamTechPostStep::process_archaeological_sites,
                    "Technology creates archaeological sites");
    result.proven_no_effect.push_back(BnwTeamTechPostStep::process_archaeological_sites);
    constexpr std::array<std::string_view,16> scalar_flags={
        "ExtraWaterSeeFrom","MapCentering","MapTrading","TechTrading",
        "GoldTrading","AllowEmbassyTradingAllowed","OpenBordersTradingAllowed",
        "DefensivePactTradingAllowed","ResearchAgreementTradingAllowed",
        "TradeAgreementTradingAllowed","PermanentAllianceTradingAllowed",
        "BridgeBuilding","WaterWork","AllowsEmbarking",
        "AllowsDefensiveEmbarking","EmbarkedAllWaterPassage"};
    for(const auto name:scalar_flags)
        if(flag(*info,name))
            return stop(BnwTeamTechPostStep::process_team_scalar_prefix,
                        "Nonzero technology flag requires GameCore effect: "+std::string(name));
    if(integer(*info,"EmbarkedMoveChange")!=0)
        return stop(BnwTeamTechPostStep::process_team_scalar_prefix,
                    "Embarked movement modifier requires GameCore effect");
    result.proven_no_effect.push_back(BnwTeamTechPostStep::process_team_scalar_prefix);

    // Next source operation is route/build movement and build-time changes.
    // Later player trait reset, Lua and engine callbacks remain mandatory even
    // when every database relation is empty. Never publish this partial grant.
    result=stop(BnwTeamTechPostStep::process_route_and_build_changes,
                "Route/build, building, improvement, player trait, Lua, tech-share and engine effects remain");
    result.pending_effects.push_back(BnwTeamTechPostStep::process_player_traits);
    result.pending_effects.push_back(BnwTeamTechPostStep::process_map_yields_and_city_routes);
    result.pending_effects.push_back(BnwTeamTechPostStep::process_lua_team_tech_researched);
    result.pending_effects.push_back(BnwTeamTechPostStep::remaining_set_has_tech);
    result.pending_effects.push_back(BnwTeamTechPostStep::gameplay_tech_acquired);
    return result;
}

} // namespace civ5::runtime
