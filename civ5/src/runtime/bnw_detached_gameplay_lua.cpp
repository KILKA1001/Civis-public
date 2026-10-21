#include "civ5/runtime/bnw_detached_gameplay_lua.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <set>

namespace civ5::runtime {

BnwDetachedGameplayLuaQuote quote_bnw_can_declare_war_detached_fixture(
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const ResearchContext &owned_players,
    const std::vector<TeamTechnologies> &owned_teams,
    std::span<const BnwDetachedGameplayLuaChunk> ordered_chunks,
    std::int64_t own_team,std::int64_t target_team) {
    BnwDetachedGameplayLuaQuote quote;
    if(owned_players.profile!=content::TechnologyProfile::bnw ||
       ordered_chunks.empty() || ordered_chunks.size()>1024) {
        quote.reason="BNW detached gameplay Lua fixture has no bounded ordered source plan";
        return quote;
    }
    std::set<std::string> names;
    for(const auto &chunk:ordered_chunks) {
        if(chunk.context!="gameplay" || chunk.name.empty() ||
           chunk.source.empty() || chunk.sha256.size()!=64 ||
           !names.insert(chunk.name).second) {
            quote.reason="BNW detached gameplay Lua fixture has unsupported context or duplicate source";
            return quote;
        }
        const auto bytes=std::span(
            reinterpret_cast<const std::uint8_t*>(chunk.source.data()),
            chunk.source.size());
        if(identify_bytes(bytes).sha256!=chunk.sha256) {
            quote.reason="BNW detached gameplay Lua fixture source SHA-256 mismatch";
            return quote;
        }
    }
    try {
        // Each quote gets a fresh VM and a private copy of the only gameplay
        // provider field that can lazily cache a source game option. The
        // external RulesDatabase and team research are queried read-only.
        auto players=owned_players;
        LuaScriptHost host({64ULL*1024*1024,2'000'000}, {}, {},
            lua_gameplay_services(database,players,owned_teams,types));
        for(const auto &chunk:ordered_chunks)
            host.execute(chunk.context,chunk.source,chunk.name);
        host.complete_loading();
        quote.decision=call_bnw_can_declare_war_lua(&host,own_team,target_team);
        quote.gate=BnwDetachedGameplayLuaGate::fixture_quoted_stock_context_unverified;
        quote.reason="BNW CanDeclareWar quoted in disposable gameplay Lua VM; selected stock context load order remains unverified";
    } catch(const std::exception &error) {
        quote.gate=BnwDetachedGameplayLuaGate::callback_unavailable;
        quote.reason=error.what();
    }
    return quote;
}

} // namespace civ5::runtime
