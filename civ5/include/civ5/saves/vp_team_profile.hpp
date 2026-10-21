#pragma once
#include "civ5/saves/team_state.hpp"

namespace civ5::saves {
// VP GameCore's untagged team arrays follow the selected rules' collection
// spans. BNW and other GameCore profiles do not require these fields.
struct VpTeamContentDimensions {
    std::uint64_t vote_sources = 0, small_awards = 0, techs = 0, routes = 0, builds = 0, projects = 0;
    std::uint64_t unit_classes = 0, building_classes = 0, buildings = 0, terrains = 0, features = 0,
                  improvements = 0, victories = 0;
};
inline const VpTeamContentDimensions *vp_team_dimensions(const TeamWireDimensions &dimensions) {
    return dimensions.profile_extensions.find<VpTeamContentDimensions>();
}
} // namespace civ5::saves
