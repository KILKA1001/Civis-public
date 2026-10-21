#pragma once
#include "civ5/saves/player_strategy.hpp"

namespace civ5::saves {
// VP GameCore source collection spans, including absent and sparse rule IDs.
// These widths describe its serialized strategy block, not host capacities.
struct VpPlayerStrategyWireDimensions {
    std::uint64_t policies = 0, branches = 0, flavors = 0, economic = 0, military = 0, grand = 0;
};
inline const VpPlayerStrategyWireDimensions *vp_player_strategy_dimensions(
    const PlayerStrategyWireDimensions &dimensions) {
    return dimensions.profile_extensions.find<VpPlayerStrategyWireDimensions>();
}
} // namespace civ5::saves
