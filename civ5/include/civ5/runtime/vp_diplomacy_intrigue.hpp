#pragma once
#include "civ5/runtime/player_research.hpp"
#include "civ5/content/custom_mod_options.hpp"
#include <map>
#include <optional>
#include <vector>

namespace civ5::runtime {
struct VpDiplomacyIntrigueRelation {
    std::int64_t approach=-1,cached_surface=-1,dof_accepted_turn=-1;
};
struct VpDiplomacyIntriguePlayer {
    std::map<std::int64_t,VpDiplomacyIntrigueRelation> relations;
    std::map<std::pair<std::int64_t,std::int64_t>,std::int64_t> coop_wars;
};
// CvDiplomacyAI state and the serialized major-player iteration domain stay
// behind the selected VP GameCore boundary. IDs are live host IDs, not array
// positions or a fixed-width source-player limit.
struct VpDiplomacyIntrigueProfileState {
    std::vector<std::int64_t> major_order;
    std::map<std::int64_t,VpDiplomacyIntriguePlayer> players;
};
VpDiplomacyIntrigueProfileState &ensure_vp_diplomacy_intrigue_profile(ResearchContext &world);
const VpDiplomacyIntrigueProfileState *find_vp_diplomacy_intrigue_profile(const ResearchContext &world);
void restore_vp_diplomacy_intrigue_profile(const saves::SavedSessionSnapshot &snapshot,
                                           ResearchContext &world);

enum class VpDiplomaticIntrigueKind { coop_war,deception };
struct VpDiplomaticIntrigueCandidate {
    VpDiplomaticIntrigueKind kind=VpDiplomaticIntrigueKind::deception;
    std::int64_t other_player=-1,diplomacy_player=-1,reported_player=-1;
};
struct VpDiplomaticIntrigueScan {
    bool target_human_diplomacy=false;
    std::size_t active_major_count=0,coop_pairs_examined=0,deception_players_examined=0;
    std::optional<VpDiplomaticIntrigueCandidate> candidate;
};
// Exact VP intrigue order and source seeded shuffle. A candidate still needs
// AddIntrigueMessage; a scan without one has no diplomatic intrigue effect.
VpDiplomaticIntrigueScan vp_scan_diplomatic_intrigue(
    const ResearchContext &setup,const PlayerResearchContext &diplomacy,
    const content::CustomModOptions &options,std::int64_t spy_owner,
    std::int64_t target_owner,std::int64_t city_x,std::int64_t city_y);
} // namespace civ5::runtime
