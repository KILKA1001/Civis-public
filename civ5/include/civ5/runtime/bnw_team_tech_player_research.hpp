#pragma once

#include "civ5/runtime/bnw_team_tech_best_route.hpp"

namespace civ5::runtime {

enum class BnwTeamTechResearchDirtyBit {
    popup, research_buttons, game_data, score,
};
struct BnwTeamTechPlayerResearchEffect {
    std::int64_t player=-1;
    bool popped=false;
    std::size_t cities_invalidated=0;
    bool rank_flags_changed=false;
    bool popup_dirty=false, research_buttons_dirty=false;
    std::vector<BnwTeamTechResearchDirtyBit> dirty_order;
};
struct BnwTeamTechPlayerResearchMarker {
    std::int64_t team=63, technology=0;
    std::vector<BnwTeamTechPlayerResearchEffect> effects;
};
struct BnwTeamTechPlayerResearchResult {
    bool applied=false, grant_complete=false;
    std::vector<BnwTeamTechPlayerResearchEffect> effects;
    // Companion overlay groups required when the changed detached world is
    // eventually serialized. This is a plan, not a committed checkpoint.
    std::vector<saves::CivisSaveSectionId> checkpoint_sections;
    std::string blocker;
};

// CvTeam::setHasTech player loop, in ascending player slot order. Mutates only
// the detached world. Engine dirty intents remain in the marker for the owner
// to publish with the enclosing transaction; this does not finish the grant.
BnwTeamTechPlayerResearchResult run_bnw_team_tech_player_research(
    LiveSessionState &detached,
    const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,
    std::int64_t technology);

} // namespace civ5::runtime
#include <memory>

namespace civ5::runtime {
class BnwMajorTeamHasTechPending;
struct BnwTeamTechResearchDirtyEvent {
 std::int64_t player=-1,team=-1;
 BnwTeamTechResearchDirtyBit kind=BnwTeamTechResearchDirtyBit::popup;
 std::uint64_t generation=0;
 bool operator==(const BnwTeamTechResearchDirtyEvent&)const=default;
};
// Retained source-order dirty notifications, not renderer completion or
// authority to acquire a technology. The genuine future UI owner consumes it.
struct BnwTeamTechResearchPublication {
 std::vector<BnwTeamTechResearchDirtyEvent> events;
 std::size_t written=0;
};
class BnwTeamTechPlayerResearchPrepared final {
 public:
 ~BnwTeamTechPlayerResearchPrepared();
 private:
 friend class BnwMajorTeamHasTechPending;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwTeamTechPlayerResearchPrepared(std::unique_ptr<Impl>);
 static std::unique_ptr<BnwTeamTechPlayerResearchPrepared> prepare(
   LiveSessionState&,const content::InfoTypeCatalog&,const content::TechnologyRules&,
   std::int64_t team,std::int64_t technology,std::uint64_t generation);
 void apply(LiveSessionState&);
 void require_stored(const LiveSessionState&)const;
 const std::vector<BnwTeamTechPlayerResearchEffect>& effects()const;
};
}
