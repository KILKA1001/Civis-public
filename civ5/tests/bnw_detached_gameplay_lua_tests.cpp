#include "civ5/runtime/bnw_detached_gameplay_lua.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool yes,const char *why) {
    if(!yes) throw std::runtime_error(why);
}
civ5::runtime::BnwDetachedGameplayLuaChunk chunk(
    std::string name,std::string script) {
    using civ5::runtime::identify_bytes;
    const auto bytes=std::span(
        reinterpret_cast<const std::uint8_t*>(script.data()),script.size());
    const auto digest=identify_bytes(bytes).sha256;
    return {"gameplay",std::move(name),std::move(script),digest};
}
}

int main() {
    using namespace civ5;
    try {
        content::RulesDatabase database;
        const auto types=database.cache_info_types();
        runtime::ResearchContext players;
        players.profile=content::TechnologyProfile::bnw;
        players.game_turn=42;
        const std::vector<runtime::TeamTechnologies> teams;
        const std::vector<runtime::BnwDetachedGameplayLuaChunk> ordered{
            chunk("=first.lua",R"lua(
                count=0
                GameEvents.CanDeclareWar.Add(function(ours,theirs)
                    count=count+1
                    return Game.GetGameTurn()==42 and ours==3 and theirs==7
                        and count==1
                end)
            )lua"),
            chunk("=second.lua",R"lua(
                GameEvents.CanDeclareWar.Add(function(ours,theirs)
                    return ours==3 and theirs==7
                end)
            )lua")};
        const auto first=runtime::quote_bnw_can_declare_war_detached_fixture(
            database,types,players,teams,ordered,3,7);
        const auto replay=runtime::quote_bnw_can_declare_war_detached_fixture(
            database,types,players,teams,ordered,3,7);
        check(first.gate==runtime::BnwDetachedGameplayLuaGate::
                  fixture_quoted_stock_context_unverified &&
              first.decision && first.decision->handled &&
              first.decision->decision==runtime::BnwCanDeclareWarLuaDecision::allow &&
              replay.decision &&
              replay.decision->decision==runtime::BnwCanDeclareWarLuaDecision::allow &&
              !first.can_commit() && players.game_turn==42,
              "BNW disposable Lua VM did not replay ordered read-only hook");
        auto wrong=ordered;
        wrong[0].sha256=std::string(64,'0');
        check(runtime::quote_bnw_can_declare_war_detached_fixture(
                  database,types,players,teams,wrong,3,7).gate==
                  runtime::BnwDetachedGameplayLuaGate::source_plan_unavailable,
              "BNW detached Lua accepted changed script bytes");
        wrong=ordered;
        wrong[1].context="ui";
        check(runtime::quote_bnw_can_declare_war_detached_fixture(
                  database,types,players,teams,wrong,3,7).gate==
                  runtime::BnwDetachedGameplayLuaGate::source_plan_unavailable,
              "BNW detached Lua accepted unverified cross-context order");
        const std::vector<runtime::BnwDetachedGameplayLuaChunk> missing_api{
            chunk("=missing-ui.lua",R"lua(
                GameEvents.CanDeclareWar.Add(function()
                    UI.AddNotification('unsupported side effect')
                    return true
                end)
            )lua")};
        check(runtime::quote_bnw_can_declare_war_detached_fixture(
                  database,types,players,teams,missing_api,3,7).gate==
                  runtime::BnwDetachedGameplayLuaGate::callback_unavailable,
              "BNW detached Lua silently ignored a missing UI effect");
        std::cout << "BNW detached gameplay Lua fixture PASS\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
