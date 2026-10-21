#pragma once

#include <bit>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#if defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace civ5::ui::audio_settings_detail {

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

// Native9b6f2d passes this integer fallback to the settings virtual reader.
// This constant alone does not establish missing-key or load-routing behavior.
inline constexpr std::int32_t native_reader_fallback_percent = 100;

// Integer-only admission predicate. Status flags (bits0..5) are irrelevant;
// RNE, no DAZ/FTZ, and all six exception masks are mandatory. This helper does
// not set MXCSR and cannot cause an unmasked floating-point exception.
inline constexpr bool saved_volume_mxcsr_supported(std::uint32_t csr) noexcept {
    return (csr & 0xffc0u) == 0x1f80u;
}

inline void require_saved_volume_numeric_rounding() {
#if defined(_M_X64) || defined(__x86_64__)
    // Read/control admission comes before any float classification or operation.
    if (!saved_volume_mxcsr_supported(_mm_getcsr())) {
        throw std::invalid_argument("BNW audio numeric codec requires masked x64 SSE RNE without DAZ/FTZ");
    }
    if (std::fegetround() != FE_TONEAREST) {
        throw std::invalid_argument("BNW audio numeric codec requires round to nearest");
    }
#else
    throw std::invalid_argument("BNW audio numeric codec requires x64 SSE");
#endif
}

// Native9b7069..9b70a3: signed percentage clamp, CVTSI2SS, MULSS with
// exact source bits3c23d70a. Pure numeric operation, not a settings owner.
inline float knob_from_saved_volume_percent(std::int32_t percent) {
    require_saved_volume_numeric_rounding();
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    const float scale = std::bit_cast<float>(std::uint32_t{0x3c23d70a});
    volatile float rounded = static_cast<float>(percent) * scale;
    return rounded;
}

// Native9b71cc..9b71ee: MULSS by100f then CVTTSS2SI and integer clamp.
// Admit finite unit-interval knobs only; other native conversion/fenv branches
// are outside this candidate. Truncation deliberately preserves native drift.
inline std::int32_t saved_volume_percent_from_knob(float knob) {
    require_saved_volume_numeric_rounding();
    const auto bits = std::bit_cast<std::uint32_t>(knob);
    // Bit admission keeps negative subnormal values distinct from negative zero
    // and rejects NaNs/infinities before any comparison can raise an exception.
    if (((bits & 0x80000000u) != 0 && bits != 0x80000000u) ||
        (bits & 0x7fffffffu) > 0x3f800000u) {
        throw std::invalid_argument("BNW audio saved knob branch unsupported");
    }
    const float scale = std::bit_cast<float>(std::uint32_t{0x42c80000});
    volatile float rounded = knob * scale;
    auto percent = static_cast<std::int32_t>(rounded);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

} // namespace civ5::ui::audio_settings_detail
