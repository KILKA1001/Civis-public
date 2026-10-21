#include "civ5/runtime/bnw_live_map_player_city_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t land_score(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_iTotalLandScored") {
            require(!found,"Duplicate BNW player land score");found=&field;
        }
    require(found && found->values.size()==1,
            "BNW player land score is unavailable");
    return found->values.front();
}
saves::CivisSaveSection committed_random_checkpoint(
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    require(source.snapshot.progression.layout==
                saves::GameStateLayout::expansion2_version1 &&
            state.random.map.algorithm()==RandomAlgorithm::bnw &&
            state.random.gameplay.algorithm()==RandomAlgorithm::bnw,
            "BNW composed RNG export requires both actual BNW streams");
    // The immutable payload supplies the original constructor history. If
    // this is already a CivisSave, its authenticated ID13 is the latest
    // committed history; copying that section would silently replay draws.
    auto previous=restore_session_random(source.snapshot.progression);
    if(source.source.live_simulation_random)
        restore_simulation_random_checkpoint(
            {static_cast<std::uint32_t>(
                 saves::CivisSaveSectionId::live_simulation_random),0,
             *source.source.live_simulation_random},previous,identity);
    const auto check=[](const SimulationRandom &live,const SimulationRandom &prior) {
        const auto &now=live.state();const auto &old=prior.state();
        require(now.calls>=old.calls && now.resets>=old.resets,
                "BNW composed RNG export regresses saved draw/reset history");
        require(now.calls!=old.calls || now.resets!=old.resets || now.seed==old.seed,
                "BNW composed RNG seed changed without any draw/reset");
    };
    check(state.random.map,previous.map);
    check(state.random.gameplay,previous.gameplay);
    // Reuse the existing version1 codec and exact source/rules identity.
    // No world mirror, source payload, RNG counter or destination is mutated.
    return make_simulation_random_checkpoint(state.random,identity);
}
constexpr std::string_view uncovered[]={
    "pregame setup and source prefix mutation",
    "game core, progression, clock and history",
    "game religion, culture, leagues, trade and final systems",
    "saved database, Lua VM and mod user data",
    "map regions, live visibility side effects and pathfinding caches",
    "unit relocation/roster, area accounting and full setXY effects",
    "team technologies, diplomacy, first-contact AI and Lua effects",
    "player non-strategy economy, religion, policy effects and AI state",
    "variable strategy collections, live AI caches and pending strategy effects",
    "dynamic city creation, deletion, roster and collection headers",
    "live city yields, population, unhappiness and maintenance contexts",
    "city SDK load effects, AI and derived query caches",
    "unit core, tail, orders and movement",
    "supporting classes and object references",
    "owned research selection, research turn and technology effects",
    "owned treasury, science and gameplay RNG streams",
    "pending UI, engine and Lua publication intents"
};
} // namespace

BnwMapPlayerCityPartialExport make_bnw_live_map_player_city_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(!source.source.live_bnw_units &&
            !source.source.live_bnw_area_units &&
            !source.source.live_bnw_team_meet,
            "BNW unit/area/meet sections need separate action composition");
    require(identify_bytes(rules.gameplay_fingerprint_data()).sha256==
                identity.rules_sha256 && types.report().all_succeeded(),
            "BNW map export active rules or type catalog changed");
    // Every component uses one immutable payload/rules identity. Neither
    // builder mutates the state or publishes a partially prepared document.
    auto players_cities=make_bnw_live_player_city_partial_export(
        state,source,identity,original_source_sha256);
    auto map=make_bnw_map_plot_checkpoint(
        state.world,source.snapshot,identity,features,&rules,&types,&defines);

    // ID17 v3+ and ID24 both carry m_iTotalLandScored. Match their decoded
    // results exactly, as LiveSession::restore_bnw requires after map restore.
    auto map_world=source.snapshot,core_world=source.snapshot;
    restore_bnw_map_plot_checkpoint(map,map_world,identity,features,
                                    &rules,&types,&defines);
    const auto &parts=players_cities.document.sections;
    const auto core=std::find_if(parts.begin(),parts.end(),[](const auto &part) {
        return part.id==bnw_player_core_checkpoint_section_id;
    });
    require(core!=parts.end(),"BNW composed player core section missing");
    restore_bnw_player_core_checkpoint(
        *core,core_world,source.snapshot,identity);
    require(map_world.all_players && core_world.all_players &&
            map_world.all_players->players.size()==
                core_world.all_players->players.size(),
            "BNW map/player score roster changed");
    for(std::size_t i=0;i<map_world.all_players->players.size();++i) {
        const auto &mapped=map_world.all_players->players[i];
        const auto &cored=core_world.all_players->players[i];
        require(mapped.saved_player_id==cored.saved_player_id &&
                land_score(mapped)==land_score(cored),
                "BNW map and player core land scores disagree");
    }
    auto &document=players_cities.document;
    document.sections.erase(std::remove_if(document.sections.begin(),
        document.sections.end(),[](const auto &part) {
            return part.id==static_cast<std::uint32_t>(
                saves::CivisSaveSectionId::live_bnw_map_plots);
        }),document.sections.end());
    document.sections.push_back(std::move(map));
    BnwMapPlayerCityPartialExport result;
    result.document=std::move(document);
    result.uncovered_domains.assign(std::begin(uncovered),std::end(uncovered));
    return result;
}

BnwMapPlayerCityClockPartialExport make_bnw_live_map_player_city_clock_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    if(source.source.live_bnw_game_clock) {
        // An imported CivisSave retains the original Civ5Save snapshot. Read
        // its previous clock overlay against that baseline before replacing
        // ID16, so a later export cannot silently roll the turn backwards.
        auto prior=source.snapshot;
        BnwGameClockState prior_clock;
        restore_bnw_game_clock_checkpoint(
            {static_cast<std::uint32_t>(
                 saves::CivisSaveSectionId::live_bnw_game_clock),0,
             *source.source.live_bnw_game_clock},
            prior,prior_clock,identity);
        require(state.world.pregame && prior.pregame &&
                state.world.pregame->archive.integers.at("game_turn")>=
                    prior.pregame->archive.integers.at("game_turn"),
                "BNW clock export regresses the previously saved turn");
    }
    // The clock codec checks all three turn owners and the immutable source
    // baseline before the composed document can be returned.
    auto clock=make_bnw_game_clock_checkpoint(state,source.snapshot,identity);
    auto base=make_bnw_live_map_player_city_partial_export(
        state,source,identity,original_source_sha256,
        features,rules,types,defines);
    // All source/rule/roster validation above completes before preparing ID13.
    // Always replace inherited ID13 with the SAME current simulation streams.
    auto random=committed_random_checkpoint(state,source,identity);
    auto &parts=base.document.sections;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[](const auto &part) {
        return part.id==static_cast<std::uint32_t>(
                   saves::CivisSaveSectionId::live_bnw_game_clock) ||
               part.id==static_cast<std::uint32_t>(
                   saves::CivisSaveSectionId::live_simulation_random);
    }),parts.end());
    parts.push_back(std::move(clock));
    parts.push_back(std::move(random));
    BnwMapPlayerCityClockPartialExport result;
    result.document=std::move(base.document);
    result.uncovered_domains=std::move(base.uncovered_domains);
    for(auto &domain:result.uncovered_domains) {
        if(domain=="game core, progression, clock and history")
            domain="game core, progression and history";
        if(domain=="owned treasury, science and gameplay RNG streams")
            domain="owned treasury and science contexts";
    }
    return result;
}

} // namespace civ5::runtime
