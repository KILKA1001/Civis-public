#pragma once
#include "civ5/saves/player_knowledge.hpp"

namespace civ5::saves {
// VP GameCore serializes untagged technology and flavor arrays. Their widths
// come from the selected rules and are not required by BNW or other profiles.
struct VpPlayerKnowledgeWireDimensions {
    std::uint64_t techs = 0, flavors = 0;
};
inline const VpPlayerKnowledgeWireDimensions *vp_player_knowledge_dimensions(
    const PlayerKnowledgeDecodeContext &context) {
    return context.profile_extensions.find<VpPlayerKnowledgeWireDimensions>();
}
inline const VpPlayerKnowledgeWireDimensions *vp_player_knowledge_dimensions(
    const PlayerKnowledgeObservation &observation) {
    return observation.profile_extensions.find<VpPlayerKnowledgeWireDimensions>();
}
} // namespace civ5::saves
