#include "civ5/runtime/bnw_setup_plot_grid.hpp"

namespace civ5::runtime {

std::array<std::optional<std::size_t>, 37>
BnwSetupPlotGrid::city_plots(std::size_t index) const {
    // Expansion2 CvGlobals::aiCityPlotX/Y, indices 0..36, interpreted by
    // CvGameCoreUtils::plotCity in hex-space storage order.
    static constexpr std::array<std::int32_t, 37> dx{
        0,
        0, 1, 1, 0, -1, -1,
        0, 1, 2, 2, 2, 1, 0, -1, -2, -2, -2, -1,
        0, 1, 2, 3, 3, 3, 3, 2, 1, 0, -1, -2, -3, -3, -3, -3, -2, -1};
    static constexpr std::array<std::int32_t, 37> dy{
        0,
        1, 0, -1, -1, 0, 1,
        2, 1, 0, -1, -2, -2, -2, -1, 0, 1, 2, 2,
        3, 2, 1, 0, -1, -2, -3, -3, -3, -3, -2, -1, 0, 1, 2, 3, 3, 3};
    std::array<std::optional<std::size_t>, 37> result{};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = hex_offset(index, dx[i], dy[i]);
    return result;
}

void BnwSetupPlotGrid::update_potential_city_work_after_land_added(std::size_t index) {
    // CvPlot::setPlotType calls updatePotentialCityWork for each plotCity
    // candidate. That function scans the candidate's own city ring and calls
    // updateYield only when m_bPotentialCityWork changes. The fresh setup
    // path is monotone ocean -> land; any already true bit remains true.
    for (const auto candidate : city_plots(index)) {
        if (!candidate || plots_[*candidate].potential_city_work) continue;
        bool valid = false;
        for (const auto member : city_plots(*candidate)) {
            if (member && plots_[*member].plot_type != 3) {
                valid = true;
                break;
            }
        }
        if (valid) {
            plots_[*candidate].potential_city_work = true;
            city_work_yield_calls_.push_back(*candidate);
        }
    }
}

} // namespace civ5::runtime
