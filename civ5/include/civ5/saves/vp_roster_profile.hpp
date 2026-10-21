#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
// VP GameCore's rules-derived serialized resource-array width. This describes
// source bytes only; Civis player, team and resource collections remain wide.
struct VpRosterWireDimensions {
    std::uint64_t resource_slots = 0;
};

inline const VpRosterWireDimensions *vp_roster_dimensions(const RosterWireDimensions &dimensions) {
    return dimensions.profile_extensions.find<VpRosterWireDimensions>();
}
} // namespace civ5::saves
