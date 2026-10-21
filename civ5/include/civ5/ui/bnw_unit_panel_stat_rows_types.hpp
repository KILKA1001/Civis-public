#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace civ5::ui {
struct BnwUnitPanelNumericArgument {
    std::int64_t numerator=0, denominator=1;
    bool operator==(const BnwUnitPanelNumericArgument &) const = default;
};
struct BnwUnitPanelStatRowRead {
    // Unknown visibility is different from native SetHide(true): fallback
    // religion/tourism/heresy/cargo readers have not yet resolved that branch.
    std::optional<bool> shown;
    std::optional<std::string> value, label, tooltip_text;
    std::string label_key, tooltip_key;
    std::vector<BnwUnitPanelNumericArgument> tooltip_arguments;
    std::string value_blocker, visibility_blocker, tooltip_blocker;
    bool operator==(const BnwUnitPanelStatRowRead &) const = default;
};
struct BnwUnitPanelStatRowsRead {
    BnwUnitPanelStatRowRead movement, strength, ranged;
    bool operator==(const BnwUnitPanelStatRowsRead &) const = default;
};
struct BnwUnitPanelReadRows {
    std::int64_t player_id=-1, unit_id=-1;
    std::int32_t turn=-1;
    std::uint64_t frame_number=0;
    std::string source_world_revision, locale;
    BnwUnitPanelStatRowsRead rows;
};
} // namespace civ5::ui
