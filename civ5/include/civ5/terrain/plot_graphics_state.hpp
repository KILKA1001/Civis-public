#pragma once
#include <cstdint>
#include <span>

namespace civ5::terrain {
// The SDK's CvPlot::updateLayout input to GameplayPlotStateChange. Callers
// resolve active-team visibility, revealed types and build IDs beforehand;
// raw saved cells alone cannot supply those runtime decisions.
enum class PlotFogMode : std::uint8_t { visible = 0, unexplored = 1, explored = 2 };
struct PlotBuildProgress {
    std::int32_t improvement = -1;
    std::int32_t route = -1;
    std::int32_t progress = 0;
};
struct PlotGraphicsInputs {
    PlotFogMode fog = PlotFogMode::visible;
    bool city = false;
    std::int32_t resource = -1;
    std::int32_t revealed_improvement = -1;
    bool improvement_pillaged = false;
    std::int32_t revealed_route = -1; // SDK: NO_ROUTE=-1, ROAD=0, RAILROAD=1
    bool route_pillaged = false;
    bool trade_route = false;
    bool any_build_progress = false;
    std::span<const PlotBuildProgress> builds;
};
struct PlotGraphicsState {
    std::int32_t resource = -1;
    std::int32_t improvement = -1;
    std::int32_t improvement_state = 2; // 1 half-built, 2 complete, 3 pillaged
    std::int32_t route = -1;
    std::uint8_t route_state = 8; // 0..7 SDK road/rail states; 8 absent
    bool operator==(const PlotGraphicsState &) const = default;
};
// Exact producer branch order from the installed BNW SDK CvPlot::updateLayout.
// This is distinct from the retail 5415F0 40-byte polygon-plot callback.
PlotGraphicsState derive_plot_graphics_state(const PlotGraphicsInputs &input);
} // namespace civ5::terrain
