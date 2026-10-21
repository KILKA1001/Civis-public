#pragma once

#include "civ5/runtime/bnw_team_tech_post_traits.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <memory>

namespace civ5::runtime {

// This is a publication guard, not a serialized technology effect. Once a
// callback starts, a throw may follow a write outside the detached world.
// The caller must reconcile an uncertain attempt instead of retrying it.
struct BnwTeamTechLuaAttemptMarker {
    std::int64_t team=63, technology=0;
    bool attempted=false, uncertain=false, quoted=false;
};

// A future gameplay-Lua owner must produce this from the actual selected
// stock/DLC/mod context epoch. A newly constructed empty Lua VM is not an
// epoch, even when complete_loading() succeeds. The host's gameplay getters
// must reference world.get(); all writes must be captured by its owner.
struct BnwTeamTechLuaEpoch {
    std::unique_ptr<LiveSessionState> world;
    std::unique_ptr<LuaScriptHost> host;
    const LiveSessionState *source_world=nullptr;
    const LiveSessionState *getter_world=nullptr;
    SessionCheckpointIdentity identity;
    std::vector<std::size_t> selected_dlc, selected_mods;
    bool all_selected_contexts_loaded=false;
    bool all_listeners_registered=false;
    bool all_external_writes_escrowed=false;
};

enum class BnwTeamTechLuaStatus { blocked, quoted, uncertain };
struct BnwTeamTechLuaAttempt {
    BnwTeamTechLuaStatus status=BnwTeamTechLuaStatus::blocked;
    std::optional<bool> hook_value;
    std::string reason;
    bool grant_complete=false;
};

// CvTeam::processTech calls GameEvents.TeamTechResearched.Call(team,tech,+1)
// after the free-unit and map loops. A quote retains the caller-owned epoch;
// it does not publish the VM, buffered effects or the complete tech grant.
// Null or unverified epochs block before dispatch. Errors after dispatch
// starts mark the live session uncertain, preventing an automatic retry.
BnwTeamTechLuaAttempt quote_bnw_team_tech_researched(
    LiveSessionState &after_post_traits,
    const SavedContentProfile &selected,
    const SessionCheckpointIdentity &identity,
    BnwTeamTechLuaEpoch *epoch,
    std::int64_t technology);

} // namespace civ5::runtime
