#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace source_fixture {
inline constexpr std::array<std::string_view,4> names{"BUILD_FARM","BUILD_MINE","BUILD_ROAD","BUILD_RAILROAD"};
inline constexpr std::array<std::uint32_t,4> hashes{0x6acb706bu,0x5b7573bdu,0xa71d649fu,0x132e8cb1u};
}
