#include "civ5/runtime/bnw_team_tech_post_traits.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
const saves::PlayerTraitsField &trait_free_units(
    const saves::SavedPlayerObservation &player) {
    const saves::PlayerTraitsField *found=nullptr;
    for(const auto &field:player.traits.fields)
        if(field.name && std::string_view(field.name)=="m_aFreeTraitUnits") {
            require(!found,"Duplicate BNW barbarian free-trait-unit field");
            found=&field;
        }
    require(found,"Missing BNW barbarian free-trait-unit field");
    return *found;
}
}

BnwBarbarianPostTraitsResult run_bnw_barbarian_post_traits(
    LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    std::int64_t technology) {
    BnwBarbarianPostTraitsResult result;
    auto draft=state;
    try {
        const auto *prior=draft.setup.extensions.find<BnwBarbarianTraitResetTransient>();
        require(prior && prior->player==63 && prior->reset_and_init_applied &&
                !draft.setup.extensions.find<BnwBarbarianPostTraitsTransient>(),
                "BNW barbarian trait reset must precede free-unit query exactly once");
        const auto *tech=technology>=0
            ?technologies.find(static_cast<std::uint64_t>(technology)):nullptr;
        require(tech && tech->base.type=="TECH_AGRICULTURE" &&
                technology==0 && draft.world.all_players &&
                draft.world.all_players->players.size()==64,
                "Selected BNW Agriculture/player snapshot unavailable");
        const auto plan=preflight_bnw_barbarian_player_map_tech(
            draft,technologies,database,types,technology);
        require(plan.first_pending==BnwTechPlayerMapGate::reset_and_init_traits &&
                plan.ever_alive_members==std::vector<std::int64_t>{63} &&
                !plan.nonzero_player_modifiers && !plan.free_promotions,
                "BNW barbarian processTech prefix changed before trait free units");

        // CvTeam::setHasTech enters processTech only after city-strength and
        // resource reveal/connection scans. A zero-city team and no matching
        // resource rules make those preceding scans inert in this scenario.
        require(draft.world.all_players->players[63].cities.cities.empty(),
                "BNW barbarian city strength update remains unimplemented");
        const auto connected=database.query_rows(
            {"SELECT ID FROM Resources WHERE TechCityTrade=?",{tech->base.type}},
            [&](const content::RulesRowView &) {
                throw std::runtime_error(
                    "BNW Agriculture resource connection requires live plot effects");
            });
        require(connected.all_succeeded(),
                "BNW Agriculture resource connection rules unavailable");

        // CvPlayerTraits::GetFirstFreeUnit sets index=0 and prereq=eTech.
        // Reset cleared m_aFreeTraitUnits, and no active trait repopulated it.
        const auto &free=trait_free_units(
            draft.world.all_players->players[63]);
        require(free.encoding==saves::PlayerTraitsEncoding::counted_pairs &&
                free.wire_count && *free.wire_count==0 && free.values.empty(),
                "BNW barbarian trait free-unit query has an effect");
        result.resource_reveal_rules=plan.resource_reveal_rules;
        result.route_build_rules=plan.route_build_rules;
        require(!result.resource_reveal_rules,
                "BNW Agriculture resource plot yield/layout update remains");
        require(!result.route_build_rules,
                "BNW Agriculture route city-plot update remains");

        draft.setup.extensions.emplace<BnwBarbarianPostTraitsTransient>(
            BnwBarbarianPostTraitsTransient{63,63,technology,0,true,true});
        result.blocker="BNW Lua TeamTechResearched dispatch and later setHasTech effects remain";
        state=std::move(draft);
        result.applied=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}

} // namespace civ5::runtime
