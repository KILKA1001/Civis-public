#include "civ5/materials/water_animation.hpp"
#include <cmath>
#include <stdexcept>

namespace civ5::materials {
namespace {
void finite(double value) {
    if (!std::isfinite(value))
        throw std::invalid_argument("Nonfinite water animation input or result");
}
} // namespace
WaterSurfaceSettings advance_water_surface(WaterSurfaceSettings settings, double frame_delta,
                                           WaterAnimationRates rates, bool renderer_active,
                                           bool water_update_enabled) {
    if (!renderer_active || !water_update_enabled)
        return settings;
    finite(frame_delta);
    const float delta = static_cast<float>(frame_delta);
    finite(delta);
    finite(rates.river);
    finite(rates.ocean);
    finite(settings.river_phase);
    finite(settings.water_phase);
    const float river_step = (rates.river * delta) * 4.f;
    const float ocean_step = rates.ocean * delta;
    const float river = river_step + settings.river_phase;
    const float ocean = ocean_step + settings.water_phase;
    finite(river);
    finite(ocean);
    settings.river_phase = river;
    settings.water_phase = ocean;
    return settings;
}
void WaterFrameClock::tick(double clock_seconds) {
    for (const double value : {clock_seconds, elapsed, delta, previous, scale, fixed_step, maximum_delta})
        finite(value);
    const double next = fixed_step > 0 ? previous + fixed_step : clock_seconds;
    const double difference = ticks == 0 ? 0. : next - previous;
    double new_delta = scale * difference;
    if (new_delta > maximum_delta && maximum_delta > 0)
        new_delta = maximum_delta;
    const double new_elapsed = elapsed + new_delta;
    finite(next);
    finite(new_delta);
    finite(new_elapsed);
    previous = next;
    elapsed = new_elapsed;
    delta = new_delta;
    ++ticks;
}
} // namespace civ5::materials
