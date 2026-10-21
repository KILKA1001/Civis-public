#include "civ5/materials/water_current.hpp"
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
WaterCurrentConstants water_current_constants(std::int32_t grid_width, std::int32_t grid_height,
                                              WaterCurrentSettings settings, WaterCurrentConstants prior) {
    for (float value : {settings.flow_strength, settings.noise_strength, settings.noise_scale})
        if (!std::isfinite(value))
            throw std::invalid_argument("Nonfinite current-map setting");
    prior[12] = settings.flow_strength;
    prior[4] = settings.noise_strength;
    prior[8] = settings.noise_scale;
    prior[0] = static_cast<float>(grid_width);
    prior[1] = static_cast<float>(grid_height);
    return prior;
}
std::array<std::uint8_t, 100> water_current_packet() {
    std::array<std::uint8_t, 100> result{};
    result[0] = 0x5d;
    result[4] = 1;
    result[8] = 1;
    result[12] = 2;
    result[96] = 1;
    return result;
}
WaterCurrentQuad water_current_quad() {
    return {{{{1, 1, 0, 1, 1, 0, 1, 0},
              {-1, 1, 0, -1, 1, 0, 0, 0},
              {1, -1, 0, 1, -1, 0, 1, 1},
              {-1, -1, 0, -1, -1, 0, 0, 1}}},
            {2, 1, 0, 3, 1, 2}};
}
} // namespace civ5::materials
