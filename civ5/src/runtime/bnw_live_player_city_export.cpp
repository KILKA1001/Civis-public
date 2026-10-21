#include "civ5/runtime/bnw_live_player_city_export.hpp"
#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint32_t,4> city_ids{
    bnw_city_core_checkpoint_section_id,
    bnw_city_production_checkpoint_section_id,
    bnw_city_management_checkpoint_section_id,
    bnw_city_tail_checkpoint_section_id};
constexpr std::string_view uncovered[]{
    "pregame setup and source prefix mutation",
    "game core, progression, clock and history",
    "game religion, culture, leagues, trade and final systems",
    "saved database, Lua VM and mod user data",
    "map header, plots, projections and regions",
    "team technologies, diplomacy and other team state",
    "player non-strategy economy, religion, policy effects and AI state",
    "variable strategy collections, live AI caches and pending strategy effects",
    "dynamic city creation, deletion, roster and collection headers",
    "live city yields, population, unhappiness and maintenance contexts",
    "city SDK load effects, AI and derived query caches",
    "unit roster, core, tail, orders and movement",
    "supporting classes and object references",
    "owned research selection, research turn and technology effects",
    "owned treasury, science and gameplay RNG streams",
    "pending UI, engine and Lua publication intents"
};
} // namespace

BnwPlayerCityPartialExport make_bnw_live_player_city_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256) {
    // Both builders validate the same source and identity and only create
    // local documents. Failure of either leaves the caller's state untouched.
    auto players=make_bnw_live_player_state_partial_export(
        state,source,identity,original_source_sha256);
    auto cities=make_bnw_live_city_partial_export(
        state,source,identity,original_source_sha256);
    auto &parts=players.document.sections;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[](const auto &part) {
        return std::find(city_ids.begin(),city_ids.end(),part.id)!=city_ids.end();
    }),parts.end());
    for(const auto id:city_ids) {
        const auto found=std::find_if(cities.document.sections.begin(),
            cities.document.sections.end(),[&](const auto &part) {
                return part.id==id;
            });
        if(found==cities.document.sections.end() ||
           std::find_if(std::next(found),cities.document.sections.end(),
               [&](const auto &part) {return part.id==id;})!=
                   cities.document.sections.end())
            throw std::runtime_error("BNW city export section count changed");
        parts.push_back(std::move(*found));
    }
    BnwPlayerCityPartialExport result;
    result.document=std::move(players.document);
    result.uncovered_domains.assign(std::begin(uncovered),std::end(uncovered));
    return result;
}

} // namespace civ5::runtime
