#include "civ5/terrain/plot_graphics_state.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
void expect(const PlotGraphicsInputs &input, PlotGraphicsState wanted, const char *case_name) {
    if (derive_plot_graphics_state(input) != wanted)
        throw std::runtime_error(case_name);
}
}
int main() {
    try {
        PlotGraphicsInputs p;
        p.resource = 7;
        p.revealed_improvement = 11;
        expect(p, {7, 11, 2, -1, 8}, "visible finished improvement");
        p.city = true;
        expect(p, {-1, 11, 2, -1, 8}, "city hides resource");
        p.city = false;
        p.fog = PlotFogMode::unexplored;
        expect(p, {-1, 11, 2, -1, 8}, "unexplored hides resource");

        const std::array builds{PlotBuildProgress{-1, 1, 2}, PlotBuildProgress{25, -1, 0},
                                PlotBuildProgress{26, 0, 3}, PlotBuildProgress{27, 1, 4}};
        p = {};
        p.any_build_progress = true;
        p.builds = builds;
        expect(p, {-1, 26, 1, 1, 5}, "first positive improvement and route build");
        p.improvement_pillaged = true;
        expect(p, {-1, 26, 3, 1, 5}, "visible pillage precedes construction");
        p.fog = PlotFogMode::explored;
        expect(p, {-1, -1, 2, -1, 8}, "hidden progress and pillage");
        p.fog = PlotFogMode::visible;
        p.any_build_progress = false;
        expect(p, {-1, -1, 3, -1, 8}, "no build scan without source flag");

        p = {};
        p.revealed_route = 0;
        p.trade_route = true;
        expect(p, {-1, -1, 2, 0, 6}, "trade road");
        p.route_pillaged = true;
        expect(p, {-1, -1, 2, 0, 2}, "pillaged road overrides trade");
        p.revealed_route = 1;
        expect(p, {-1, -1, 2, 1, 3}, "pillaged railroad");
        p.route_pillaged = false;
        expect(p, {-1, -1, 2, 1, 7}, "trade railroad");
        p.trade_route = false;
        expect(p, {-1, -1, 2, 1, 1}, "regular railroad");
        p.revealed_route = 9;
        expect(p, {-1, -1, 2, 9, 8}, "unknown route retains absent state");
        std::cout << "PASS plot graphics state producer\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL plot graphics state producer: " << error.what() << '\n';
        return 1;
    }
}
