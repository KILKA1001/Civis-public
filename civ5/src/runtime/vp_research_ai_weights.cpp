#include "civ5/runtime/vp_research_ai_weights.hpp"

namespace civ5::runtime {
void restore_vp_research_ai_weights(ResearchAiPlayerWeights &result,
    const saves::PlayerKnowledgeBlock &techs,const saves::PlayerTraitsObservation &traits) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:techs.arrays)if(item.name&&std::string_view(item.name)=="tech_ai_weights") {
        if(found)throw std::runtime_error("Duplicate VP TechAI vector");
        found=&item;
    }
    if(!found||found->columns!=2||found->values.size()%2)
        throw std::runtime_error("Invalid VP TechAI vector");
    result.weight_slots=found->values.size()/2;
    VpResearchAiVector elements;
    const saves::PlayerTraitsField *trait=nullptr;
    for(const auto &item:traits.fields)if(item.name&&
        std::string_view(item.name)=="m_bPermanentYieldsDecreaseEveryEra") {
        if(trait)throw std::runtime_error("Duplicate VP research-era trait");
        trait=&item;
    }
    if(!trait||trait->values.size()!=1||trait->values[0]<0||trait->values[0]>1)
        throw std::runtime_error("Missing or invalid VP research-era trait");
    elements.permanent_yields_decrease_every_era=trait->values[0]!=0;
    for(std::uint64_t i=0;i<result.weight_slots;++i) {
        elements.saved_elements.emplace(i,found->values[2*i]);
        if(found->values[2*i+1])result.weights.emplace(i,found->values[2*i+1]);
    }
    result.profile_extensions.emplace<VpResearchAiVector>(std::move(elements));
}
} // namespace civ5::runtime
