#include "civ5/runtime/vp_research_ai_candidates.hpp"
#include <algorithm>
#include <cmath>

namespace civ5::runtime {
ResearchAiCandidates vp_build_research_ai_candidates(ResearchSelectionContext &selection,
    ResearchTurnContext &turn,const ResearchAiWeights &weights,std::int64_t id,bool free) {
    if(selection.setup.profile!=content::TechnologyProfile::vp542)
        throw ResearchAiUnavailable("VP research AI requires the VP GameCore profile");
    validate_research_ai_world(selection,turn,weights,id);
    const bool eligible=selection.players.major_players.contains(id)&&
        !selection.players.players.at(id).pregame_minor;
    auto result=collect_research_ai_candidates(selection,turn,weights,id,free,eligible);
    if(!eligible)return result;
    const auto inputs=research_ai_scoring_inputs(selection,turn,id,free);
    result.prefers_expensive=inputs.prefers_expensive;
    const auto *state=weights.players.at(id).profile_extensions.find<VpResearchAiVector>();
    if(!state)throw ResearchAiUnavailable("VP TechAI profile state unavailable");
    for(auto &choice:result.choices) {
        const auto estimate=research_turns_left(turn,id,choice.technology,true);
        if(!estimate.turns)throw ResearchAiUnavailable("Research AI requires finite research rate");
        choice.turns=*estimate.turns;
        const auto &tech=*selection.technologies.find(static_cast<std::uint64_t>(choice.technology));
        choice.adjusted_weight=vp_reweight_research_ai_candidate(choice.saved_weight,choice.turns,
            inputs.base,inputs.per_turn,result.prefers_expensive,free,
            state->permanent_yields_decrease_every_era,tech.era,
            selection.team_eras.at(selection.setup.players.at(id).team));
    }
    return result;
}
std::int64_t vp_reweight_research_ai_candidate(std::int64_t saved,std::int64_t turns,
    float base,float per,bool prefers_expensive,bool free,
    bool permanent_decrease,std::int64_t tech_era,std::int64_t player_era) {
    if(saved<0||turns<1)throw ResearchAiUnavailable("Invalid VP TechAI weight or research turns");
    // VP sums the two source float terms before assigning the double.
    const double factor=base+static_cast<float>(turns)*per;
    const double divisor=std::pow(static_cast<double>(turns),factor);
    const double weighted=(prefers_expensive||free)?static_cast<double>(saved)*divisor:
        static_cast<double>(saved)/divisor;
    if(std::isnan(weighted)||weighted<0)throw ResearchAiUnavailable("Invalid VP TechAI cost weight");
    // The source casts to int before clipping. Clip in wide space first so
    // large mod values cannot overflow an x86-style intermediate.
    auto result=static_cast<std::int64_t>(std::min(10000.0,weighted));
    if(permanent_decrease&&tech_era>player_era)result/=2;
    return std::clamp<std::int64_t>(result,1,10000);
}
} // namespace civ5::runtime
