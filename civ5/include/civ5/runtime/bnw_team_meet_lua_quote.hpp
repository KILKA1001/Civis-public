#pragma once
#include "civ5/runtime/bnw_detached_gameplay_lua.hpp"
#include "civ5/runtime/bnw_sight_meet_effect_plan.hpp"

namespace civ5::runtime {

enum class BnwTeamMeetLuaGate : std::uint8_t {
    effect_plan_unavailable,
    no_new_contact,
    source_plan_unavailable,
    callback_failed,
    fixture_quoted_stock_context_unverified,
};
struct BnwTeamMeetLuaQuote {
    BnwTeamMeetLuaGate gate=BnwTeamMeetLuaGate::effect_plan_unavailable;
    std::int64_t first_argument=-1,second_argument=-1;
    bool handled=false;
    std::string reason;
    // The quote never proves earlier AI effects or publishes Lua/world state.
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvTeam::meet: TeamMeet is called once after both makeHasMet
// sides, with (other team, revealing team). Executes only SHA-256-pinned
// fixture chunks in a fresh disposable gameplay VM. The real selected Lua
// load order and callback effects remain outside this diagnostic boundary.
BnwTeamMeetLuaQuote quote_bnw_team_meet_lua_fixture(
    const BnwSightMeetEffectPlan &plan,
    std::span<const BnwDetachedGameplayLuaChunk> ordered_chunks);
} // namespace civ5::runtime
