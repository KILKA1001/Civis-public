#include "civ5/runtime/bnw_team_tech_player_map.hpp"
#include <array>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr std::array<std::string_view,6> player_fields={
    "FeatureProductionModifier","UnitFortificationModifier",
    "UnitBaseHealModifier","WorkerSpeedModifier",
    "InfluenceSpreadModifier","ExtraVotesPerDiplomat"};
}

BnwTechPlayerMapPlan preflight_bnw_barbarian_player_map_tech(
    const LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    std::int64_t technology) {
    BnwTechPlayerMapPlan result;
    auto draft=state;
    const auto prior=run_bnw_barbarian_domain_yield_prefix(
        draft,technologies,database,types,technology);
    if(!prior.applied) {
        result.blocker=prior.blocker;
        return result;
    }
    const auto *info=technologies.find(static_cast<std::uint64_t>(technology));
    if(!info || info->base.type.empty()) {
        result.blocker="BNW acquired technology rule unavailable";
        return result;
    }

    // CvTeam::processTech 6150-6199 visits every player slot, including
    // defeated members. Zero modifier calls are simple no-op additions.
    for(std::int64_t player=0;player<64;++player) {
        const auto it=draft.setup.players.find(player);
        if(it==draft.setup.players.end()) {
            result.blocker="BNW complete player roster unavailable";
            return result;
        }
        if(it->second.team!=63 || !it->second.ever_alive) continue;
        result.ever_alive_members.push_back(player);
    }
    for(const auto field:player_fields) {
        const auto it=info->integers.find(field);
        if(it==info->integers.end()) {
            result.blocker="BNW player technology modifier rule unavailable";
            return result;
        }
        if(it->second!=0)
            result.nonzero_player_modifiers+=result.ever_alive_members.size();
    }
    for(const auto &[slot,value]:info->free_promotions.values) {
        if(slot>=info->free_promotions.extent) {
            result.blocker="BNW free-promotion technology slot invalid";
            return result;
        }
        if(value!=0)
            result.free_promotions+=result.ever_alive_members.size();
    }

    // These are later source branches, assessed independently so the blocked
    // trait reset does not conceal a separate map/world effect.
    const auto resources=database.query_rows(
        {"SELECT TechReveal FROM Resources",{}},
        [&](const content::RulesRowView &row) {
            if(row.text("TechReveal")==info->base.type)
                ++result.resource_reveal_rules;
        });
    const auto routes=database.query_rows(
        {"SELECT Builds.ID FROM Builds INNER JOIN Routes "
         "ON Builds.RouteType=Routes.Type WHERE Builds.PrereqTech=?",
         {info->base.type}},
        [&](const content::RulesRowView &) {++result.route_build_rules;});
    if(!resources.all_succeeded() || !routes.all_succeeded()) {
        result.blocker="BNW map resource/route rules unavailable";
        return result;
    }

    if(result.nonzero_player_modifiers) {
        result.first_pending=BnwTechPlayerMapGate::player_modifiers;
        result.blocker="Nonzero player technology modifier requires live player write";
    } else if(result.free_promotions) {
        result.first_pending=BnwTechPlayerMapGate::free_promotions;
        result.blocker="Technology grants free player promotion";
    } else if(!result.ever_alive_members.empty()) {
        // Reset/InitPlayerTraits recreates many cached scalar, vector and
        // trait-gated values. RecomputePolicyCostModifier then writes a player
        // scalar. Neither call is a no-op just because this tech has no trait.
        result.first_pending=BnwTechPlayerMapGate::reset_and_init_traits;
        result.blocker="Ever-alive barbarian member requires Reset/InitPlayerTraits and policy-cost recompute";
    } else if(result.resource_reveal_rules) {
        result.first_pending=BnwTechPlayerMapGate::resource_plot_yields;
        result.blocker="Resource reveal requires plot updateYield/layout";
    } else if(result.route_build_rules) {
        result.first_pending=BnwTechPlayerMapGate::city_routes;
        result.blocker="Unlocked route build requires city-plot route scan";
    } else {
        result.first_pending=BnwTechPlayerMapGate::lua_team_tech_researched;
        result.blocker="Lua TeamTechResearched and later acquisition effects remain";
    }
    return result;
}

} // namespace civ5::runtime
