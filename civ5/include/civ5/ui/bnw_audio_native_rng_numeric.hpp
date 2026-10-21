#pragma once
#include "civ5/ui/bnw_audio_saved_volume_numeric.hpp"
#include <bit>
#include <cstdint>
#include <limits>

namespace civ5::ui::audio_rng_detail {
// Numeric fields ONLY. Public fields and a copied instance are not an issued
// Audio context, an original startup-history import, or voice/event permission.
// Actual ca7c40 .data initializer: state7, calls0, retained seed7, modulus and
// reciprocal below. No seed source from Soren/game RNG, clocks or std::random.
struct NativeAudioRngNumericFields {
 std::uint32_t state=7;
 std::uint32_t draws=0;
};
inline constexpr std::uint32_t native_audio_initial_seed=7;
inline constexpr std::uint32_t native_audio_modulus=0xffffcfefu;
inline constexpr std::uint64_t native_audio_reciprocal_bits=0x3df0000301109066ULL;
static_assert(sizeof(double)==8&&std::numeric_limits<double>::is_iec559);

// Original481800, PC53/RNE subset. CRT8fd32a establishes PC53; an external
// precision/rounding change is outside this supported transfer. Require masked
// x64 SSE RNE, no DAZ/FTZ before mutation. Unsupported modes do not claim to
// duplicate original native floating-point exception side effects.
inline std::uint32_t step_native_audio_rng_numeric(
 NativeAudioRngNumericFields& fields,std::uint32_t bound){
 audio_settings_detail::require_saved_volume_numeric_rounding();
 // Unsigned32 wrap on both stores is intentional. Bound0/1 STILL advance.
 ++fields.draws;
 fields.state=fields.state*std::uint32_t{0x343fd}+std::uint32_t{0x269ec3};
 fields.state%=native_audio_modulus;
 const double reciprocal=std::bit_cast<double>(native_audio_reciprocal_bits);
 // Two PC53 operations; preserve first rounded product before second.
 // Native uses unsigned corrections after FILD then truncating FISTP64.
 volatile double first=static_cast<double>(fields.state)*reciprocal;
 volatile double second=first*static_cast<double>(bound);
 return static_cast<std::uint32_t>(second);
}
}
