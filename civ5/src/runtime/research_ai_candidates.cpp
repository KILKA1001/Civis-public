#include "civ5/runtime/research_ai_candidates.hpp"
#include "civ5/runtime/vp_research_ai_candidates.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <cmath>
#include <limits>

namespace civ5::runtime {
namespace {
float source_float(const content::GlobalDefineCache &defines,std::string_view name) {
    const auto *entry=defines.find(name);
    if(!entry||entry->kind!=content::GlobalDefineKind::source_float||
       !entry->lookup.error.empty()||!std::holds_alternative<double>(entry->lookup.value))
        throw ResearchAiUnavailable("Research AI source float unavailable: "+std::string(name));
    return static_cast<float>(std::get<double>(entry->lookup.value));
}
bool expensive(std::int64_t overflow,std::int64_t science) {
    if(science<0||overflow<0)throw ResearchAiUnavailable("Negative research AI science/overflow");
    return science<=INT64_MAX/2&&overflow>science*2;
}
std::int64_t bnw_weight(std::int64_t saved,std::int64_t turns,float base,float per,bool wants_expensive) {
    // SDK stores the product in a double after float arithmetic, then adds
    // the source float base. Preserve the two rounding stages.
    const double extra=per*static_cast<float>(turns);
    const double factor=static_cast<double>(base)+extra;
    const double divisor=std::pow(static_cast<double>(turns),factor);
    const double value=wants_expensive?static_cast<double>(saved)*divisor:static_cast<double>(saved)/divisor;
    if(!std::isfinite(value)||value<0||value>=0x1p63)
        throw ResearchAiUnavailable("BNW research AI weight exceeds host range");
    return static_cast<std::int64_t>(value);
}
}
void validate_research_ai_world(const ResearchSelectionContext &selection,
    const ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t id) {
    const auto profile=selection.setup.profile;
    if(selection.state.profile!=profile||selection.turn.profile!=profile||
       turn.state.profile!=profile||turn.progress.setup.profile!=profile||
       weights.profile!=profile||&selection.turn!=&turn.state||
       &selection.players!=&turn.progress.players||&selection.setup!=&turn.progress.setup||
       &selection.technologies!=&turn.progress.technologies||
       selection.teams.data()!=turn.progress.teams.data()||
       selection.teams.size()!=turn.progress.teams.size()||!weights.players.contains(id))
        throw ResearchAiUnavailable("Research AI requires one shared GameCore world");
}
ResearchAiCandidates collect_research_ai_candidates(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t id,bool free,bool eligible) {
    validate_research_ai_world(selection,turn,weights,id);
    ResearchAiCandidates result;
    if(!eligible)return result;
    const auto &saved=weights.players.at(id);
    for(const auto &[slot,info]:selection.technologies.entries) {
        if(slot>INT64_MAX)throw ResearchAiUnavailable("Research AI technology ID exceeds int64");
        const auto tech=static_cast<std::int64_t>(slot);
        if(!can_research(selection,id,tech))continue;
        if(free&&!can_research_for_free(selection,id,tech))continue;
        const auto weight=saved.weight_at(slot);
        if(!weight||*weight<0)throw ResearchAiUnavailable("Saved research AI weight unavailable");
        result.choices.push_back({tech,*weight,*weight,0});
    }
    return result;
}
ResearchAiScoringInputs research_ai_scoring_inputs(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,std::int64_t id,bool force_expensive) {
    const auto science=runtime::science_times100(turn.science,turn.economy,turn.treasury,turn.maintenance,
        turn.treasury_rules,turn.city_rules,turn.maintenance_rules,turn.progress.defines,
        turn.progress.options,id,false,turn.income_services).total_times100;
    const auto overflow=selection.players.players.at(id).overflow_times100;
    const auto prefers_expensive=force_expensive?true:expensive(overflow,science);
    const auto base=source_float(turn.progress.defines,"AI_RESEARCH_WEIGHT_BASE_MOD");
    const auto per=source_float(turn.progress.defines,"AI_RESEARCH_WEIGHT_MOD_PER_TURN_LEFT");
    return {base,per,prefers_expensive};
}
ResearchAiCandidates build_research_ai_candidates(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t id,bool free) {
    if(selection.setup.profile==content::TechnologyProfile::vp542)
        return vp_build_research_ai_candidates(selection,turn,weights,id,free);
    if(selection.setup.profile!=content::TechnologyProfile::bnw)
        throw ResearchAiUnavailable("Research AI GameCore profile unsupported");
    auto result=collect_research_ai_candidates(selection,turn,weights,id,free,true);
    if(free)return result; // BNW does not ReweightByCost for free techs.
    const auto inputs=research_ai_scoring_inputs(selection,turn,id,false);
    result.prefers_expensive=inputs.prefers_expensive;
    for(auto &choice:result.choices) {
        const auto estimate=research_turns_left(turn,id,choice.technology,true);
        if(!estimate.turns)throw ResearchAiUnavailable("Research AI requires finite research rate");
        choice.turns=*estimate.turns;
        choice.adjusted_weight=bnw_weight(choice.saved_weight,choice.turns,
            inputs.base,inputs.per_turn,result.prefers_expensive);
    }
    return result;
}
} // namespace civ5::runtime
