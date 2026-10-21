#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include <stdexcept>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
static_assert(std::is_nothrow_move_assignable_v<BnwLivePlotImpassability>);
void require(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
std::vector<BnwPlotImpassabilitySource> sources(
    const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection,
            "BNW live impassability requires a complete owned map");
    const auto &map=*world.map_projection;
    require(map.grid.width==world.map_header.grid.width &&
            map.grid.height==world.map_header.grid.height &&
            map.wrap_x==world.map_header.wrap_x &&
            map.wrap_y==world.map_header.wrap_y &&
            map.cells.size()==map.grid.size() &&
            world.map_plots->plots.size()==map.cells.size(),
            "BNW live impassability map identity mismatch");
    std::vector<BnwPlotImpassabilitySource> result;
    result.reserve(map.cells.size());
    for(std::size_t i=0;i<map.cells.size();++i) {
        const auto &cell=map.cells[i];
        const auto &saved=world.map_plots->plots[i];
        const auto xy=map.grid.coordinates(i);
        require(cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
                saved.index==i && saved.x==xy[0] && saved.y==xy[1],
                "BNW live impassability plot identity mismatch");
        result.push_back({cell.index,cell.x,cell.y,cell.terrain_type,
                          cell.feature_type,cell.feature_type_hash});
    }
    return result;
}
} // namespace

void rebuild_bnw_live_plot_impassability(
    LiveSessionState &state,const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity) {
    state.plot_impassability.current=false;
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            !identity.rules_sha256.empty() &&
            identify_bytes(rules.gameplay_fingerprint_data()).sha256==
                identity.rules_sha256,
            "BNW live impassability activated rules identity mismatch");
    BnwLivePlotImpassability replacement;
    replacement.source=sources(state.world);
    replacement.values=restore_plot_passability(state.world,rules,types);
    require(replacement.values.layout==bnw &&
            replacement.values.plots.size()==replacement.source.size(),
            "BNW live impassability restore returned an incomplete map");
    replacement.rules_sha256=identity.rules_sha256;
    replacement.current=true;
    state.plot_impassability=std::move(replacement);
}

const PlotPassability &bnw_live_plot_impassability(const LiveSessionState &state) {
    const auto &cache=state.plot_impassability;
    require(cache.current && cache.values.layout==bnw &&
            !cache.rules_sha256.empty(),
            "BNW live impassability cache is not current");
    require(cache.source==sources(state.world) &&
            cache.values.plots.size()==cache.source.size(),
            "BNW live impassability source changed");
    return cache.values;
}

} // namespace civ5::runtime
