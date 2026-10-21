#pragma once
#include "civ5/runtime/research_context.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {
class LuaScriptHost;
// VP's spy fields live in the VP GameCore extension. The original record is
// retained for fields whose gameplay interpretation has not yet been ported.
struct VpSpy {
    std::string name;
    std::int64_t rank=0, experience=0, city_x=0, city_y=0, state=0, focus=0;
    std::int64_t revive_counter=0, vassal_diplomat_player=-1;
    bool diplomat=false, evaluate_reassignment=false, passive=false;
    std::int64_t counterspy_mission_turn=0, active_mission_turn=0;
};
struct VpSpyNotification {
    std::int64_t city_x=0,city_y=0,attacking_player=-1,spy_result=0;
    std::int64_t amount_stolen=0,stolen_tech=-1,mission=-1,great_work=-1;
};
struct VpIntrigue {
    std::int64_t discovering_player=-1,source_player=-1,target_player=-1,diplomacy_player=-1;
    std::int64_t building=-1,project=-1,unit=-1,type=0,turn=0,city_x=0,city_y=0,spy_id=-1;
    std::string spy_name;
    bool shared=false;
};
struct VpEspionagePlayer {
    std::vector<VpSpy> spies;
    // Mutable VP GameCore inputs for spy network points and city security.
    // The saved source vectors are decoded once; later policy, trait and
    // tourism effects update these values in the live profile.
    std::int64_t extra_rank_bonus=0,offensive_strength_modifier=0;
    std::int64_t network_points_bonus=0,policy_faster_modifier=0;
    std::int64_t security_modifier=0;
    std::map<std::int64_t,std::int64_t> cultural_influence_times100;
    std::vector<VpSpyNotification> spy_messages;
    std::vector<VpIntrigue> intrigue;
    saves::PlayerEspionageRecord saved_record;
};
struct VpEspionageProfileState {
    std::map<std::int64_t,VpEspionagePlayer> players;
};
VpEspionageProfileState &ensure_vp_espionage_profile(ResearchContext &world);
const VpEspionageProfileState *find_vp_espionage_profile(const ResearchContext &world);
void restore_vp_espionage_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world);

struct VpSpyMessageDispatch {
    std::vector<std::string> hook_errors;
};
// VP GameCore queues the defender's message after the optional gameplay Lua
// hook. The hook sees seven scalar values, not the VP-only queue record.
VpSpyMessageDispatch vp_add_spy_message(ResearchContext &world,std::int64_t defender,
    VpSpyNotification message,const content::CustomModOptions &options,LuaScriptHost *scripts);
// VP updates the first matching queued message only. A later message at the
// same coordinates remains unchanged, matching CvPlayerEspionage.
bool vp_update_spy_message(ResearchContext &world,std::int64_t defender,
    std::int64_t city_x,std::int64_t city_y,std::int64_t attacking_player,
    std::int64_t stolen_tech,std::int64_t amount_stolen,std::int64_t great_work);
// AddIntrigueMessage's early return for building-army intrigue. Duplicate
// comparison uses source, target and intrigue type, then a 30-turn timer;
// city, spy and building fields do not participate for these two types.
bool vp_recent_building_army_intrigue(const ResearchContext &world,std::int64_t discoverer,
    std::int64_t source,bool amphibious);
// AddIntrigueMessage's source/target/type and 30-turn duplicate rule.
// Deception ignores city and other details; coop war additionally compares
// the city coordinates and absent building/project/unit fields.
bool vp_recent_diplomatic_intrigue(const ResearchContext &world,std::int64_t discoverer,
    std::int64_t source,std::int64_t target,bool coop_war,
    std::int64_t city_x,std::int64_t city_y);

struct VpEspionageTurnFrontier {
    std::size_t spy_count=0, queued_messages=0, queued_intrigue=0;
    std::optional<std::size_t> first_spy_needing_host;
    std::int64_t first_spy_state=-1;
    bool safe_no_gameplay_effect=false;
    std::string unresolved;
};
// CvPlayerEspionage::DoTurn drains messages before processing spies. Only an
// empty queue and idle/terminated spies prove a gameplay no-op without the
// notification, map, city and event hosts. This probe never mutates state.
VpEspionageTurnFrontier vp_espionage_turn_frontier(const ResearchContext &world,std::int64_t player);
} // namespace civ5::runtime
