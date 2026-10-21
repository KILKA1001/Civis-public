#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <cstdint>
#include <map>
#include <vector>

namespace civ5::saves { struct SavedSession; }

namespace civ5::runtime {

struct LiveSessionState;

struct BnwPlayerPlotList {
    std::uint64_t source_wire_count=0;
    std::vector<std::int32_t> plots;
};
struct BnwPlayerPlotListLoad {
    std::uint64_t map_plot_count=0;
    std::map<std::int64_t,BnwPlayerPlotList> players;
    bool current=false;
};

// SDK CvPlayer::Read (Expansion2 CvPlayer.cpp:22235-22248) resets m_aiPlots
// to numPlots entries of -1, then copies the saved prefix. This reproduces
// that owned value without calling UpdatePlots, which can reorder the list.
// The source payload stays immutable. Rebuild after a player-tail mutation;
// no gameplay setter, map ownership update, network callback or Steam effect
// is published by this read-only load context.
BnwPlayerPlotListLoad rebuild_bnw_player_plot_lists(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
