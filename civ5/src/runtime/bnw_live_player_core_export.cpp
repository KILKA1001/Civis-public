#include "civ5/runtime/bnw_live_player_core_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
bool sha256(std::string_view value) {
    if(value.size()!=64) return false;
    return std::all_of(value.begin(),value.end(),[](char c) {
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
    "player strategies, economy, religion, policies and AI state",
    "city roster, core, production, management and tail",
    "unit roster, core, tail, orders and movement",
    "supporting classes and object references",
    "owned research selection, research turn and technology effects",
    "owned city yields, unhappiness, maintenance, treasury and science",
    "owned gameplay and map RNG streams",
    "pending UI, engine and Lua publication intents",
    "derived load hooks and invalidated world query caches"
};
} // namespace

BnwPlayerCorePartialExport make_bnw_live_player_core_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256) {
    require(!state.player_early_publication,
            "BNW player core export has unpublished early player UI effects");
    require_bnw_unit_serial_export_unchanged(state.world,source.snapshot);
    require(!source.source.live_bnw_unit_serial,
            "BNW partial export cannot carry quarantined FLTA serial without allocator restore");
    require(sha256(identity.payload_sha256) && sha256(identity.rules_sha256) &&
            sha256(original_source_sha256),
            "BNW partial export identity is not lowercase SHA-256");
    require(!source.source.bytes.empty() &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256,
            "BNW partial export payload identity changed");
    if(source.source.source_sha256)
        require(*source.source.source_sha256==original_source_sha256,
                "BNW partial export original source identity changed");
    require(!source.source.live_bnw_player_proximity,
            "BNW ID20 proximity overlaps full ID24 player core");
    require(source.snapshot.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1,
            "BNW partial export mixed GameCore profile");
    // Build all sections locally before returning anything to the caller.
    // The codec checks every player, field, roster, hash and fixed wire shape.
    require(!source.source.live_bnw_population_totals,"BNW generic partial export cannot replace pending population companions");
    require_bnw_population_totals_export_unchanged(state.world,source.snapshot);
    const auto core=make_bnw_player_core_checkpoint(
        state.world,source.snapshot,identity,true);
    auto document=saves::make_civis_import_document(
        source.source,original_source_sha256);
    const auto id=bnw_player_core_checkpoint_section_id;
    document.sections.erase(std::remove_if(document.sections.begin(),
        document.sections.end(),[&](const saves::CivisSaveSection &part) {
            return part.id==id;
        }),document.sections.end());
    document.sections.push_back(core);
    BnwPlayerCorePartialExport result;
    result.document=std::move(document);
    result.uncovered_domains.assign(std::begin(uncovered),std::end(uncovered));
    return result;
}
} // namespace civ5::runtime
