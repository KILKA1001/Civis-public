#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace civ5::runtime {

struct LiveSessionState;

enum class BnwPlayerFlavorRecipient {
    technology_ai,
    policy_ai,
    wonder_production_ai,
};

// CvPlayer::Read appends these targets to CvFlavorManager in source order.
// They are identities, never pointers or executable AI/flavor services.
struct BnwPlayerFlavorRecipients {
    bool current=false;
    std::map<std::int64_t,std::vector<BnwPlayerFlavorRecipient>> by_player;
};

// Expansion2 CvPlayer.cpp:22174-22184. Registration is conditional on
// SS_TAKEN/SS_COMPUTER, excludes BARBARIAN_PLAYER, and does not propagate
// flavors at load time. Missing or divergent source inputs block the rebuild.
void rebuild_bnw_player_flavor_recipients(LiveSessionState &state);

const std::vector<BnwPlayerFlavorRecipient> &bnw_player_flavor_recipients(
    const LiveSessionState &state,std::int64_t player);

} // namespace civ5::runtime
