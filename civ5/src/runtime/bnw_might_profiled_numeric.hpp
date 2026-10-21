#pragma once
#include <bit>
#include <cstdint>
#include <limits>

// Isolated numerical diagnostic only: no LiveSession, mutator, readiness or receipt API.
namespace civ5::runtime::diagnostic {
enum class MightPrecision : unsigned { pc24 = 24, pc53 = 53, pc64 = 64 };
enum class MightNumericError { none, invalid_precision, negative_total,
                               negative_multiplier, nonfinite_multiplier, int32_overflow };
struct MightNumericResult {
    MightNumericError error;
    std::int32_t value;
    constexpr bool has_value() const noexcept { return error == MightNumericError::none; }
};

// Original1014E721 FILD int32 ->1014E724 FMUL binary32 -> native truncation.
// Profiles model round-nearest/ties-even, masked exceptions. FILD is exact even
// at PC24. No premature int->float rounding or float product store is permitted.
constexpr MightNumericResult might_multiply_truncate(std::int32_t unit_total,
                                                     std::uint32_t multiplier_bits,
                                                     MightPrecision precision) noexcept {
    const auto pc = static_cast<unsigned>(precision);
    if (pc != 24 && pc != 53 && pc != 64)
        return {MightNumericError::invalid_precision, 0};
    if (unit_total < 0) return {MightNumericError::negative_total, 0};
    const auto exponent = (multiplier_bits >> 23) & 0xffu;
    if (exponent == 0xffu) return {MightNumericError::nonfinite_multiplier, 0};
    if ((multiplier_bits & 0x80000000u) != 0)
        return {MightNumericError::negative_multiplier, 0};
    const auto significand = (multiplier_bits & 0x7fffffu) | (exponent ? 0x800000u : 0u);
    // At most31+24=55 bits, so the exact product cannot overflow uint64.
    std::uint64_t product = std::uint64_t(static_cast<std::uint32_t>(unit_total)) * significand;
    int binary_exponent = exponent ? static_cast<int>(exponent) - 150 : -149;
    if (product == 0) return {MightNumericError::none, 0};
    const auto width = static_cast<unsigned>(std::bit_width(product));
    if (width > pc) {
        const auto discard = width - pc; // bounded1..31 for accepted PC values
        const auto mask = (std::uint64_t{1} << discard) - 1;
        const auto lost = product & mask;
        const auto half = std::uint64_t{1} << (discard - 1);
        auto rounded = product >> discard;
        if (lost > half || (lost == half && (rounded & 1u))) ++rounded;
        product = rounded;
        binary_exponent += static_cast<int>(discard);
    }
    constexpr auto maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
    std::uint64_t truncated = 0;
    if (binary_exponent >= 0) {
        if (binary_exponent > 30 || product > (maximum >> binary_exponent))
            return {MightNumericError::int32_overflow, 0};
        truncated = product << binary_exponent;
    } else {
        const auto shift = static_cast<unsigned>(-binary_exponent);
        truncated = shift >= 64 ? 0 : product >> shift;
        if (truncated > maximum) return {MightNumericError::int32_overflow, 0};
    }
    return {MightNumericError::none, static_cast<std::int32_t>(truncated)};
}
} // namespace civ5::runtime::diagnostic
