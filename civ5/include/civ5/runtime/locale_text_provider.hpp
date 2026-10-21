#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace civ5::runtime {
// Independent explicit text capability. No existing Locale provider ABI is
// expanded, and this interface does not assert the original engine algorithm.
struct ScriptLocaleTextProvider {
    std::function<std::string(std::string_view)> lower;
    std::function<std::int64_t(std::string_view,std::string_view)> compare;
};
} // namespace civ5::runtime
