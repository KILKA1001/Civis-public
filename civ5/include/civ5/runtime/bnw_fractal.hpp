#pragma once
#include "civ5/runtime/simulation_random.hpp"
#include <cstdint>
#include <vector>

namespace civ5::runtime {

// The bounded non-rift CvFractal::fracInit path used by BNW map scripts.
// Flags and random draw order follow CvGameCoreDLL_Expansion2/CvFractal.cpp.
class BnwFractal {
public:
    enum Flag : std::uint32_t {
        wrap_x = 1, wrap_y = 2, percent = 4, polar = 8,
        center_rift = 16, invert_heights = 32,
    };
    BnwFractal(int width, int height, int grain, std::uint32_t flags,
               int x_exponent, int y_exponent, SimulationRandom &map_random);
    int height_at(int x, int y) const;
    int height_from_percent(int percentage) const;
    // CvFractal::ridgeBuilder, bounded only where the SDK can retry forever.
    void build_ridges(int seeds, std::uint32_t flags, int ridge_blend,
                      int fractal_blend, SimulationRandom &map_random,
                      std::uint64_t maximum_seeds, std::uint64_t maximum_retries);

private:
    int &at(int x, int y) { return values_[x * (frac_y_ + 1) + y]; }
    int at(int x, int y) const { return values_[x * (frac_y_ + 1) + y]; }
    int width_, height_, frac_x_, frac_y_, x_inc_, y_inc_;
    std::uint32_t flags_;
    std::vector<int> values_;
};

} // namespace civ5::runtime
