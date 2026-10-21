#pragma once

#include "civ5/ui/bnw_audio_saved_volume_numeric.hpp"

namespace civ5::ui::audio_settings_detail {

// Native85ee80..85eea9, finite-input numeric subset only. This operation is
// deliberately scalar: references do not establish an actual knob owner,
// registered subscriber, loaded setting or permission to deliver audio gain.
inline void apply_knob_direct_numeric(float& current, std::int32_t& transition_mode,
                                      float incoming) {
    require_saved_volume_numeric_rounding();
    const auto bits = std::bit_cast<std::uint32_t>(incoming);
    if ((bits & 0x7f800000u) == 0x7f800000u) {
        throw std::invalid_argument("BNW direct knob nonfinite branch unsupported");
    }
    // COMISS branch order retains -0 and positive subnormals in supported SSE
    // mode. Negative finite inputs clamp to +0; positive inputs above1 to1.
    float admitted = incoming;
    if (admitted < 0.0f) admitted = 0.0f;
    if (admitted > 1.0f) admitted = 1.0f;
    // Actual source writes current first, then clears interpolation mode.
    // All potentially throwing admission checks have completed before stores.
    current = admitted;
    transition_mode = 0;
}

} // namespace civ5::ui::audio_settings_detail
