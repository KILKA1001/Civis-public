#include "civ5/runtime/bnw_live_city_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
bool sha256(std::string_view value) {
    return value.size()==64 &&
        std::all_of(value.begin(),value.end(),[](char c) {
            return (c>='0' && c<='9') || (c>='a' && c<='f');
        });
}
constexpr std::string_view uncovered[]={
    "pregame setup and source prefix mutation",
    "game core, progression, clock and history",
    "game religion, culture, leagues, trade and final systems",
    "saved database, Lua VM and mod user data",
    "map header, plots, projections and regions",
    "team technologies, diplomacy and other team state",
    "player core, strategies, economy, policies, religion and AI state",
    "dynamic city creation, deletion, roster and header changes",
    "live city yields, population, unhappiness and maintenance contexts",
    "city SDK load effects, AI and derived query caches",
    "unit roster, core, tail, orders and movement",
    "supporting classes and object references",
    "owned research selection, research turn and technology effects",
    "owned treasury, science and gameplay RNG streams",
    "pending UI, engine and Lua publication intents"
};
} // namespace

BnwCityPartialExport make_bnw_live_city_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256) {
    require_bnw_unit_serial_export_unchanged(state.world,source.snapshot);
    require(!source.source.live_bnw_unit_serial,
            "BNW partial export cannot carry quarantined FLTA serial without allocator restore");
    require(sha256(identity.payload_sha256) && sha256(identity.rules_sha256) &&
            sha256(original_source_sha256),
            "BNW city partial export identity is not lowercase SHA-256");
    require(!source.source.bytes.empty() &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256,
            "BNW city partial export payload identity changed");
    if(source.source.source_sha256)
        require(*source.source.source_sha256==original_source_sha256,
                "BNW city partial export original source identity changed");
    require(!source.source.live_city_production,
            "BNW ID7 live production needs rules-based reconciliation with ID25/26");
    require(source.snapshot.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1,
            "BNW city partial export mixed GameCore profile");

    require(!source.source.live_bnw_population_totals,"BNW generic partial export cannot replace pending population companions");
    require_bnw_population_totals_export_unchanged(state.world,source.snapshot);
    // Each codec checks the immutable roster and schema against the same
    // source. The quadruple validates all companions before any publication.
    auto cities=make_bnw_city_checkpoint_quadruple(
        state.world,source.snapshot,identity,true);
    // Quadruple admission above proves all rosters and companion domains.
    // Emit the strict core baseline schema in the actual general export path,
    // while retaining legacy low-level diagnostic pair/quad writers.
    auto core_only=source.snapshot;
    for(std::size_t p=0;p<core_only.all_players->players.size();++p) {
        auto &to=core_only.all_players->players[p].cities.cities;
        const auto &from=state.world.all_players->players[p].cities.cities;
        for(std::size_t c=0;c<to.size();++c) to[c].core=from[c].core;
    }
    cities.core=make_bnw_city_core_baseline_checkpoint(core_only,source.snapshot,identity,true);
    auto document=saves::make_civis_import_document(
        source.source,original_source_sha256);
    document.sections.erase(std::remove_if(document.sections.begin(),
        document.sections.end(),[](const saves::CivisSaveSection &part) {
            return part.id>=bnw_city_core_checkpoint_section_id &&
                   part.id<=bnw_city_tail_checkpoint_section_id;
        }),document.sections.end());
    document.sections.push_back(cities.core);
    document.sections.push_back(cities.production);
    document.sections.push_back(cities.management);
    document.sections.push_back(cities.tail);
    BnwCityPartialExport result;
    result.document=std::move(document);
    result.uncovered_domains.assign(std::begin(uncovered),std::end(uncovered));
    return result;
}

} // namespace civ5::runtime
