#pragma once
#include <cstdint>
#include <functional>

namespace civ5::runtime {
// A separately requested diagnostic capability; no existing provider ABI is
// extended. The selected SDK token is not a GameOptions database row ID.
struct ScriptSelectedGameOptionProvider {
    std::int64_t no_religion_token=21;
    std::function<bool(std::int64_t)> is_option;
};
}
