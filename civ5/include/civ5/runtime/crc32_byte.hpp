#pragma once
#include <array>
#include <cstdint>

namespace civ5::runtime {
namespace crc32_detail {
inline constexpr auto byte_table = [] {
    std::array<std::uint32_t, 256> values{};
    for (std::uint32_t n = 0; n < values.size(); ++n) {
        auto state = n;
        for (unsigned bit = 0; bit < 8; ++bit)
            state = (state >> 1) ^ ((state & 1) ? 0xedb88320u : 0u);
        values[n] = state;
    }
    return values;
}();
}
// The reflected polynomial's eight bit steps for one byte only. Callers own
// validation, case folding, initial state and any final complement.
constexpr std::uint32_t crc32_update_byte(std::uint32_t state, unsigned char byte) noexcept {
    return (state >> 8) ^ crc32_detail::byte_table[(state ^ byte) & 0xffu];
}
}
