#pragma once
#include "civ5/saves/game_progression.hpp"

namespace civ5::saves {
// VP GameCore source collection spans after rule loading, including ID holes.
// Units, UnitClasses, BuildingClasses, Projects, SpecialUnits, Victories, Contracts.
struct VpGameProgressionWireDimensions {
    std::array<std::uint64_t, 7> rows{};
};

inline const VpGameProgressionWireDimensions *vp_game_progression_dimensions(
    const ProgressionWireDimensions &dimensions) {
    return dimensions.profile_extensions.find<VpGameProgressionWireDimensions>();
}
} // namespace civ5::saves
