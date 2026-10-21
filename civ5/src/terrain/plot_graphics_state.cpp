#include "civ5/terrain/plot_graphics_state.hpp"

namespace civ5::terrain {
PlotGraphicsState derive_plot_graphics_state(const PlotGraphicsInputs &input) {
    PlotGraphicsState result;
    result.resource = input.city || input.fog == PlotFogMode::unexplored ? -1 : input.resource;
    result.improvement = input.revealed_improvement;
    if (result.improvement == -1 && input.any_build_progress && input.fog == PlotFogMode::visible) {
        for (const auto &build : input.builds) {
            if (build.improvement != -1 && build.progress > 0) {
                result.improvement = build.improvement;
                break;
            }
        }
    }
    // Pillage is checked before the half-built choice in the SDK.
    if (input.improvement_pillaged && input.fog == PlotFogMode::visible)
        result.improvement_state = 3;
    else if (result.improvement != input.revealed_improvement)
        result.improvement_state = 1;

    result.route = input.revealed_route;
    if (result.route != -1) {
        if (result.route == 0)
            result.route_state = input.route_pillaged ? 2 : input.trade_route ? 6 : 0;
        else if (result.route == 1)
            result.route_state = input.route_pillaged ? 3 : input.trade_route ? 7 : 1;
    } else if (input.any_build_progress && input.fog == PlotFogMode::visible) {
        for (const auto &build : input.builds) {
            if (build.route != -1 && build.progress > 0) {
                result.route = build.route;
                if (result.route == 0) result.route_state = 4;
                else if (result.route == 1) result.route_state = 5;
                break;
            }
        }
    }
    return result;
}
} // namespace civ5::terrain
