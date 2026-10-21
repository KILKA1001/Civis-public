#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("LAND grid test failed");
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected);
}
} // namespace
int main() {
    try {
        check(land_unit_half(1) == 0x3c00);
        check(land_unit_half(.5f) == 0x3800);
        check(land_unit_half(0.00006103515625f) == 0x400);
        check(land_unit_half(.500244140625f) == 0x3800); // even tie
        check(land_unit_half(.500732421875f) == 0x3802); // odd tie
        for (float value : {0.f, -1.f, 1.01f, std::numeric_limits<float>::quiet_NaN()})
            rejects([&] { land_unit_half(value); });
        for (auto [w, h] : {std::pair{65, 65}, {65, 64}, {64, 65}, {2, 2}, {7, 11}, {23, 23}, {33, 17}})
            for (int step : {1, 2, 3}) {
                LandGrid grid(w, h, step);
                check(grid.vertices().size() == static_cast<std::size_t>(w * h));
                check(grid.indices().size() ==
                      static_cast<std::size_t>(((w - 2 + step) / step) * ((h - 2 + step) / step) * 6));
                check(*std::max_element(grid.indices().begin(), grid.indices().end()) < w * h);
                check(grid.vertices().back().position ==
                      std::array<float, 3>{float(w - 1), float(h - 1), 14.f});
                check(grid.vertices().back().uv == std::array<std::uint16_t, 2>{0x3c00, 0x3c00});
                check(std::all_of(grid.vertices().begin(), grid.vertices().end(),
                                  [](const auto &v) { return v.normal == std::array<std::uint16_t, 4>{}; }));
                std::int64_t doubled_area = 0;
                for (std::size_t i = 0; i < grid.indices().size(); i += 3) {
                    const int a = grid.indices()[i], b = grid.indices()[i + 1], c = grid.indices()[i + 2];
                    const auto area = (b % w - a % w) * (c / w - a / w) - (b / w - a / w) * (c % w - a % w);
                    check(area < 0);
                    doubled_area -= area;
                }
                check(doubled_area == 2 * (w - 1) * (h - 1));
            }
        const auto index = land_grid_indices(3, 2, 1);
        check(index == std::vector<std::uint16_t>{4, 1, 3, 3, 1, 0, 5, 2, 1, 4, 5, 1});
        check(land_grid_indices(66, 66, 1).size() == 65 * 65 * 6);
        for (auto [w, h, s] : {std::array{1, 2, 1}, {2, 1, 1}, {66, 2, 1}, {2, 66, 1}, {2, 2, 0}, {2, 2, 4}})
            rejects([&] { LandGrid grid(w, h, s); });
        rejects([] { land_grid_indices(66, 66, 2); });
        auto stream = land_control_stream(65, 65);
        check(stream.addresses.size() == 87296);
        std::size_t start = 0;
        for (std::size_t i = 0; i < 5; ++i) {
            const int size = (64 >> i) + 1;
            check(stream.levels[i].columns == size && stream.levels[i].rows == size);
            check(stream.levels[i].first_address == start);
            check(stream.addresses[start] == 0x0202);
            start += stream.levels[i].address_count;
        }
        check(start == stream.addresses.size());
        check(land_control_stream(2, 2).addresses.size() == 80);
        rejects([] { land_control_stream(66, 2); });
        std::cout << "PASS " << checks << " native LAND grid checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
