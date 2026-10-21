#pragma once
#include "civ5/runtime/bnw_can_declare_war_lua.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include <vector>

namespace civ5::runtime {

struct BnwDetachedGameplayLuaChunk {
    std::string context,name,source,sha256;
};
enum class BnwDetachedGameplayLuaGate : std::uint8_t {
    source_plan_unavailable,
    callback_unavailable,
    fixture_quoted_stock_context_unverified,
};
struct BnwDetachedGameplayLuaQuote {
    BnwDetachedGameplayLuaGate gate=BnwDetachedGameplayLuaGate::source_plan_unavailable;
    std::optional<BnwCanDeclareWarLuaResult> decision;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Executes explicitly ordered, SHA-256-pinned gameplay fixture chunks in a
// fresh disposable Lua VM. Gameplay getters reference a copied ResearchContext
// and immutable teams/rules. No UI, map, pregame, save-data or modding mutator
// provider is installed. A failed required API blocks; all VM writes vanish
// when the quote returns. This proves the CanDeclareWar bridge's isolated
// execution, NOT the retail stock InGame context load order or whole-turn Lua.
// Stock publication requires a selected resource/entrypoint loader and an
// external-effect replay contract, both still absent.
BnwDetachedGameplayLuaQuote quote_bnw_can_declare_war_detached_fixture(
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const ResearchContext &owned_players,
    const std::vector<TeamTechnologies> &owned_teams,
    std::span<const BnwDetachedGameplayLuaChunk> ordered_chunks,
    std::int64_t own_team,std::int64_t target_team);

} // namespace civ5::runtime
