#pragma once
#include "water_surface.hpp"

namespace civ5::materials {
struct WaterAnimationRates {
    // Original constructor loads 0x3e3851eb from 0x9d7ec8. Decimal .18f
    // rounds to the adjacent 0x3e3851ec and drifts during accumulation.
    float river = 0x1.70a3d6p-3f;
    float ocean = .03f;
};

// Caller supplies the processed frame delta in seconds (original timer+0x10).
// The dispatcher rounds that double to float before calling WaterSystem. Time
// is accumulated per frame, with the original operation order and no wrapping.
// An inactive renderer or disabled water update retains both phase bit
// patterns.
WaterSurfaceSettings advance_water_surface(WaterSurfaceSettings settings, double frame_delta,
                                           WaterAnimationRates rates = {}, bool renderer_active = true,
                                           bool water_update_enabled = true);

// Original frame timer0x970d50, after its clock provider returns seconds. Clock
// sampling is supplied by the host. Fixed steps, scaling, positive delta cap,
// first-tick behavior and U32 tick wrap follow the original timer state.
struct WaterFrameClock {
    double elapsed = 0, delta = 0, previous = 0;
    double scale = 1, fixed_step = -1, maximum_delta = .2;
    std::uint32_t ticks = 0;
    void tick(double clock_seconds);
};
} // namespace civ5::materials
