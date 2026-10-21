#include "civ5/runtime/vp_research_ai_choice.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
// Pinned VP CvRandom.h: hash32, CvSeeder::mixRawAssign and CvGame::randCore.
std::uint32_t hash32(std::uint32_t input) {
    const std::uint32_t state=input*747796405u+2891336453u;
    const std::uint32_t word=((state>>((state>>28u)+4u))^state)*277803737u;
    return (word>>22u)^word;
}
std::uint32_t mix_raw(std::uint32_t seed,std::uint32_t value) {
    return seed^(value+0x9e3779b9u+(seed<<6u)+(seed>>2u));
}
std::uint32_t hashed(std::uint64_t value) {
    auto result=hash32(static_cast<std::uint32_t>(value));
    // Source saves fit uint32. Wider Civis game values remain distinguishable.
    if(value>UINT32_MAX)result=mix_raw(result,hash32(static_cast<std::uint32_t>(value>>32u)));
    return result;
}
std::uint64_t roll(std::uint64_t limit,std::uint64_t map_seed,std::int64_t game_turn,
    std::int64_t player,std::int64_t known) {
    if(!limit||game_turn<0||player<0||known<0)throw ResearchAiUnavailable("Invalid VP TechAI seeded roll");
    auto extra=mix_raw(0xd0b45b02u,hashed(static_cast<std::uint64_t>(player)));
    extra=mix_raw(extra,hashed(static_cast<std::uint64_t>(known)));
    auto game=mix_raw(static_cast<std::uint32_t>(map_seed),hashed(static_cast<std::uint64_t>(game_turn)));
    if(map_seed>UINT32_MAX)game=mix_raw(game,hashed(map_seed>>32u));
    game=mix_raw(game,extra);
    if(limit<=UINT32_MAX)return game%limit;
    // Preserve VP's 32-bit result for normal source ranges; extend the seed
    // deterministically when Civis hosts a wider weighted catalog.
    const auto extended=(std::uint64_t{game}<<32u)|hash32(game);
    return extended%limit;
}
}
std::int64_t vp_choose_research_ai_candidate(const ResearchAiCandidates &candidates,
    std::int64_t cutoff,std::uint64_t map_seed,std::int64_t game_turn,
    std::int64_t player,std::int64_t known) {
    if(candidates.choices.empty())return -1;
    auto choices=candidates.choices;
    for(const auto &choice:choices)if(choice.adjusted_weight<0)
        throw ResearchAiUnavailable("Negative VP TechAI candidate weight");
    std::stable_sort(choices.begin(),choices.end(),[](const auto &a,const auto &b){return a.adjusted_weight>b.adjusted_weight;});
    const auto highest=choices.front().adjusted_weight;
    if(highest==0)return choices.at(static_cast<std::size_t>(roll(choices.size(),map_seed,game_turn,player,known))).technology;
    cutoff=std::clamp<std::int64_t>(cutoff,0,100);
    const auto threshold=wide::multiply_divide(highest,cutoff,100);
    std::int64_t total=0;
    for(const auto &choice:choices)if(choice.adjusted_weight>=threshold)
        total=wide::add(total,choice.adjusted_weight);
    if(total<=0)throw ResearchAiUnavailable("VP TechAI has no selectable weight");
    auto value=roll(static_cast<std::uint64_t>(total),map_seed,game_turn,player,known);
    for(const auto &choice:choices)if(choice.adjusted_weight>=threshold) {
        const auto weight=static_cast<std::uint64_t>(choice.adjusted_weight);
        if(value<weight)return choice.technology;
        value-=weight;
    }
    throw ResearchAiUnavailable("VP TechAI weighted roll did not select a technology");
}
std::optional<std::int64_t> vp_research_ai_script_override(LuaScriptHost &scripts,
    const ResearchSelectionContext &selection,std::int64_t player) {
    if(!selection.options||!selection.options->enabled("EVENTS_AI_OVERRIDE_TECH"))return {};
    if(!scripts.loading_complete())throw ResearchAiUnavailable("VP research Lua content loading is incomplete");
    const ScriptValue args[]={player,false};
    const auto result=scripts.accumulate_int("AiOverrideChooseNextTech",args);
    if(!result.errors.empty())throw ResearchAiUnavailable("VP research Lua override failed: "+result.errors.front());
    if(!scripts.loading_complete())throw ResearchAiUnavailable("VP research Lua content became incomplete");
    if(!result.value||*result.value<0||static_cast<std::uint64_t>(*result.value)>=selection.technologies.collection.slots)
        return {};
    const auto team_id=selection.setup.players.at(player).team;
    for(const auto &team:selection.teams)if(team.team()==team_id) {
        if(team.has_tech(*result.value))return {};
        return *result.value;
    }
    throw ResearchAiUnavailable("VP research override team is unavailable");
}
VpResearchAiChoiceResult vp_choose_research_ai(ResearchAiChoiceContext &c,std::int64_t player) {
    if(c.selection.setup.profile!=content::TechnologyProfile::vp542)
        throw ResearchAiUnavailable("VP research AI requires the VP GameCore profile");
    if(const auto override=vp_research_ai_script_override(c.scripts,c.selection,player))
        return {*override,true,false};
    const auto candidates=build_research_ai_candidates(c.selection,c.turn,c.weights,player,false);
    const auto team=c.selection.setup.players.at(player).team;
    std::optional<std::int64_t> known;
    for(const auto &entry:c.selection.teams)if(entry.team()==team) {
        known=entry.known_count();break;
    }
    if(!known)throw ResearchAiUnavailable("Research AI team unavailable");
    const auto technology=vp_choose_research_ai_candidate(candidates,
        research_ai_handicap_setting(c,"TechChoiceCutoffThreshold"),c.selection.setup.map_random_seed,
        c.selection.setup.game_turn,player,*known);
    return {technology,false,technology>=0};
}
} // namespace civ5::runtime
