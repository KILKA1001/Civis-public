#pragma once
#include <cstdint>
#include <stdexcept>

namespace civ5::runtime::wide {
inline std::int64_t add(std::int64_t a, std::int64_t b) {
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b))
        throw std::overflow_error("Integer addition exceeds int64");
    return a + b;
}
inline std::int64_t subtract(std::int64_t a, std::int64_t b) {
    if ((b > 0 && a < INT64_MIN + b) || (b < 0 && a > INT64_MAX + b))
        throw std::overflow_error("Integer subtraction exceeds int64");
    return a - b;
}
// Exact signed (a*b)/divisor, truncated toward zero. A portable 128-bit
// intermediate allows the product to exceed int64 when the quotient fits.
// Only the positive divisor and final int64 result constrain this operation.
inline std::int64_t multiply_divide(std::int64_t a, std::int64_t b, std::int64_t divisor) {
    if (divisor <= 0) throw std::invalid_argument("Integer divisor must be positive");
    const auto magnitude = [](std::int64_t v) {
        const auto u = static_cast<std::uint64_t>(v);
        return v < 0 ? std::uint64_t{0} - u : u;
    };
    const auto x = magnitude(a), y = magnitude(b), d = static_cast<std::uint64_t>(divisor);
    std::uint64_t q;
    if (y == 0 || x <= UINT64_MAX / y) {
        q = (x * y) / d;
    } else {
        constexpr std::uint64_t mask = 0xffffffffULL;
        const auto low = (x & mask) * (y & mask);
        const auto middle1 = (x >> 32) * (y & mask) + (low >> 32);
        const auto middle2 = (x & mask) * (y >> 32) + (middle1 & mask);
        const auto high = (x >> 32) * (y >> 32) + (middle1 >> 32) + (middle2 >> 32);
        const auto bottom = (middle2 << 32) | (low & mask);
        if (high >= d) throw std::overflow_error("Integer quotient exceeds uint64");
        auto remainder = high;
        q = 0;
        for (unsigned bit = 64; bit-- > 0;) {
            const auto carry = remainder >> 63;
            remainder = (remainder << 1) | ((bottom >> bit) & 1);
            if (carry || remainder >= d) {
                remainder -= d;
                q |= std::uint64_t{1} << bit;
            }
        }
    }
    const bool negative = (a < 0) != (b < 0);
    const auto limit = negative ? (std::uint64_t{1} << 63) : static_cast<std::uint64_t>(INT64_MAX);
    if (q > limit) throw std::overflow_error("Integer quotient exceeds int64");
    if (negative && q == (std::uint64_t{1} << 63)) return INT64_MIN;
    return negative ? -static_cast<std::int64_t>(q) : static_cast<std::int64_t>(q);
}
// Exact signed (a*b + c)/divisor. The additional term is applied to the full
// product before truncation, including cancellation across zero. Used by the
// source's half-divisor rounding without constraining its numerator to int64.
inline std::int64_t multiply_add_divide(std::int64_t a, std::int64_t b, std::int64_t c, std::int64_t divisor) {
    if (divisor <= 0) throw std::invalid_argument("Integer divisor must be positive");
    const auto mag = [](std::int64_t v) { const auto u = static_cast<std::uint64_t>(v); return v < 0 ? 0-u : u; };
    const auto x = mag(a), y = mag(b), z = mag(c), d = static_cast<std::uint64_t>(divisor);
    constexpr std::uint64_t mask = 0xffffffffULL;
    const auto low = (x & mask)*(y & mask);
    const auto m1 = (x >> 32)*(y & mask) + (low >> 32);
    const auto m2 = (x & mask)*(y >> 32) + (m1 & mask);
    auto high = (x >> 32)*(y >> 32) + (m1 >> 32) + (m2 >> 32);
    auto bottom = (m2 << 32) | (low & mask);
    bool negative = (a < 0) != (b < 0);
    if (negative == (c < 0)) {
        const auto old = bottom;
        bottom += z;
        high += bottom < old ? 1 : 0;
    } else if (high == 0 && bottom < z) {
        bottom = z-bottom;
        negative = c < 0;
    } else {
        const auto borrow = bottom < z;
        bottom -= z;
        high -= borrow ? 1 : 0;
    }
    if (high >= d) throw std::overflow_error("Integer quotient exceeds uint64");
    std::uint64_t q = 0, remainder = high;
    for (unsigned bit = 64; bit-- > 0;) {
        const auto carry = remainder >> 63;
        remainder = (remainder << 1) | ((bottom >> bit) & 1);
        if (carry || remainder >= d) { remainder -= d; q |= std::uint64_t{1} << bit; }
    }
    const auto limit = negative ? (std::uint64_t{1} << 63) : static_cast<std::uint64_t>(INT64_MAX);
    if (q > limit) throw std::overflow_error("Integer quotient exceeds int64");
    if (negative && q == (std::uint64_t{1} << 63)) return INT64_MIN;
    return negative ? -static_cast<std::int64_t>(q) : static_cast<std::int64_t>(q);
}
} // namespace civ5::runtime::wide
