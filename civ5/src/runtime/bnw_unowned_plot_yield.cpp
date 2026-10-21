#include "civ5/runtime/bnw_unowned_plot_yield.hpp"
#include "civ5/runtime/live_session.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
bool no_improvement(const saves::SavedMapCellProjection &cell) {
    // CvPlot::Read uses ReadHashed for v5+; hash 0 becomes NO_IMPROVEMENT
    // (CvInfosSerializationHelper.cpp:164-187). Older plots store enum -1.
    return (cell.improvement_type && *cell.improvement_type==-1 &&
            !cell.improvement_type_hash) ||
           (!cell.improvement_type && cell.improvement_type_hash &&
            *cell.improvement_type_hash==0);
}
}

BnwUnownedPlotYieldQuote finish_bnw_unowned_plot_yield(
    std::int64_t nature,std::int64_t extra) {
    require(nature>=0 && nature<=INT32_MAX && extra>=INT32_MIN &&
            extra<=INT32_MAX && nature+extra>=INT32_MIN &&
            nature+extra<=INT32_MAX,
            "BNW plot-yield subtotal exceeds source i32 arithmetic range");
    return {nature,extra,std::max<std::int64_t>(0,nature+extra),false};
}

std::optional<BnwUnownedPlotYieldQuote> bnw_plot_early_zero_yield(
    const LiveSessionState &world,std::int64_t plot,std::int64_t yield) {
    require(world.setup.profile==content::TechnologyProfile::bnw &&
            world.world.map_projection && plot>=0 && yield>=0 && yield<6 &&
            static_cast<std::uint64_t>(plot)<
                world.world.map_projection->cells.size(),
            "BNW early plot yield needs a selected map and valid yield");
    const auto &cell=world.world.map_projection->cells[
        static_cast<std::size_t>(plot)];
    require(cell.index==static_cast<std::uint64_t>(plot) &&
            cell.terrain_type && cell.potential_city_work,
            "BNW early plot yield needs decoded terrain and workability");
    if(*cell.terrain_type==-1 || !*cell.potential_city_work)
        return BnwUnownedPlotYieldQuote{.early_zero=true};
    return std::nullopt;
}

BnwUnownedPlotYieldQuote bnw_unowned_plot_yield(
    BnwNatureYieldContext &nature,
    const LiveSessionState &world,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::int64_t plot,
    std::int64_t yield) {
    require(world.setup.profile==content::TechnologyProfile::bnw &&
            world.world.map_projection &&
            &nature.water.map==&*world.world.map_projection &&
            &nature.water.players==&world.setup &&
            &nature.water.cities==&world.cities &&
            &nature.resources.roster==&world.player_research &&
            plot>=0 &&
            static_cast<std::uint64_t>(plot)<nature.water.map.cells.size() &&
            yield>=0 && yield<6 && nature.rules.yields.contains(yield),
            "BNW plot yield requires one owned source-bound nature world");
    const auto &map=nature.water.map;
    const auto &cell=map.cells[static_cast<std::size_t>(plot)];
    const auto xy=map.grid.coordinates(static_cast<std::uint64_t>(plot));
    // Preserve this public query's source/history binding even when the SDK
    // returns before it asks for getPlotExtraYield.
    const auto extras=restore_bnw_game_plot_extras(world,source,identity);
    if(const auto early=bnw_plot_early_zero_yield(world,plot,yield))
        return *early;
    require(cell.index==static_cast<std::uint64_t>(plot) &&
            cell.x==xy[0] && cell.y==xy[1] &&
            cell.x<=INT32_MAX && cell.y<=INT32_MAX &&
            cell.terrain_type && cell.potential_city_work &&
            cell.owner && no_improvement(cell) &&
            cell.route &&
            *cell.owner==-1 &&
            cell.route->type==-1,
            "BNW plot-yield branch needs an unowned plot without city/improvement/route");
    const auto q=bnw_nature_yield(nature,plot,yield,-1,false);
    const auto extra=extras.extra_yield(static_cast<std::int64_t>(cell.x),
                                        static_cast<std::int64_t>(cell.y),yield);
    return finish_bnw_unowned_plot_yield(q.total,extra);
}

} // namespace civ5::runtime
