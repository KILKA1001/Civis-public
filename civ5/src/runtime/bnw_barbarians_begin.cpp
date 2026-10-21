#include "civ5/runtime/bnw_barbarians_begin.hpp"
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
LiveActionResult reject(std::string reason) {
    return {LiveActionOutcome::rejected,std::move(reason)};
}
} // namespace

std::int64_t resolve_bnw_barbarian_camp_improvement(
    const content::GlobalDefineCache &defines) {
    if(defines.profile()!=content::GlobalDefineProfile::bnw)
        throw std::runtime_error("BNW barbarian turn requires activated BNW defines");
    const auto *entry=defines.find("BARBARIAN_CAMP_IMPROVEMENT");
    if(!entry || !entry->lookup.found || !entry->lookup.error.empty() ||
       !std::holds_alternative<std::int64_t>(entry->lookup.value))
        throw std::runtime_error("BNW barbarian camp improvement define unavailable");
    const auto value=std::get<std::int64_t>(entry->lookup.value);
    if(value<0 || value>std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("BNW barbarian camp improvement exceeds source range");
    return value;
}

LiveActionResult run_bnw_barbarians_begin_phase(
    LiveSessionState &state,std::int64_t camp_improvement) {
    if(camp_improvement<0 ||
       camp_improvement>std::numeric_limits<std::int32_t>::max())
        return reject("BNW barbarian camp improvement ID invalid");
    auto &world=state.world;
    if(world.game_state.layout!=bnw || world.map_header.layout!=bnw ||
       !world.map_projection || !world.supporting_classes ||
       world.supporting_classes->layout!=bnw ||
       world.supporting_classes->barbarians_version!=1)
        return reject("BNW barbarian turn requires decoded map and supporting classes");
    const auto count=world.map_header.grid.size();
    if(count==0 || world.map_projection->grid.width!=world.map_header.grid.width ||
       world.map_projection->grid.height!=world.map_header.grid.height ||
       world.map_projection->cells.size()!=count ||
       world.supporting_classes->plot_count!=count ||
       world.supporting_classes->barbarians.size()!=count)
        return reject("BNW barbarian plot counts disagree");

    // Preflight every plot before writing any counter, even when called
    // outside the detached whole-turn transaction.
    std::vector<std::int64_t> next;
    next.reserve(static_cast<std::size_t>(count));
    for(std::uint64_t index=0;index<count;++index) {
        const auto &cell=world.map_projection->cells.at(static_cast<std::size_t>(index));
        const auto &barb=world.supporting_classes->barbarians.at(
            static_cast<std::size_t>(index));
        const auto xy=world.map_header.grid.coordinates(index);
        if(cell.index!=index || cell.x!=xy[0] || cell.y!=xy[1])
            return reject("BNW barbarian plot identity mismatch at "+
                          std::to_string(index));
        if(barb.attacked || barb.counter<std::numeric_limits<std::int16_t>::min() ||
           barb.counter>std::numeric_limits<std::int16_t>::max() ||
           barb.units_spawned<std::numeric_limits<std::int16_t>::min() ||
           barb.units_spawned>std::numeric_limits<std::int16_t>::max())
            return reject("BNW barbarian source data invalid at "+
                          std::to_string(index));
        auto value=barb.counter;
        if(value>0) {
            if(!cell.improvement_type || *cell.improvement_type< -1)
                return reject("BNW barbarian improvement unresolved at "+
                              std::to_string(index));
            if(*cell.improvement_type!=camp_improvement)
                value=-1;
            else --value;
        } else if(value< -1) {
            ++value;
        }
        next.push_back(value);
    }
    for(std::size_t index=0;index<next.size();++index) {
        auto &barb=world.supporting_classes->barbarians[index];
        if(barb.counter>0 && next[index]==-1)
            barb.units_spawned=-1;
        barb.counter=next[index];
    }
    return {LiveActionOutcome::committed,{}};
}

BnwTurnHandler make_bnw_barbarians_begin_handler(
    std::int64_t camp_improvement) {
    return {BnwTurnPhase::game_barbarians_begin,
            [camp_improvement](LiveSessionState &state,std::int64_t player) {
                if(player!=-1)
                    return reject("BNW barbarian begin is a game phase");
                return run_bnw_barbarians_begin_phase(state,camp_improvement);
            }};
}

} // namespace civ5::runtime
