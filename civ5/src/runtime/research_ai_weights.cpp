#include "civ5/runtime/research_ai_weights.hpp"
#include "civ5/runtime/vp_research_ai_weights.hpp"

namespace civ5::runtime {
namespace {
template<class T> const T &named(const std::vector<T> &items, std::string_view name) {
    const T *found=nullptr;
    for(const auto &item:items)if(item.name&&std::string_view(item.name)==name) {
        if(found)throw std::runtime_error("Duplicate saved TechAI field");
        found=&item;
    }
    if(!found)throw std::runtime_error("Missing saved TechAI field: "+std::string(name));
    return *found;
}
}
std::optional<std::int64_t> ResearchAiPlayerWeights::weight_at(std::uint64_t slot) const {
    if(slot>=weight_slots)return {};
    const auto it=weights.find(slot);
    return it==weights.end()?0:it->second;
}
ResearchAiWeights restore_research_ai_weights(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &setup, const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types) {
    using content::TechnologyProfile;
    const auto vp=setup.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snapshot.all_players||snapshot.all_players->layout!=layout||
       technologies.profile!=setup.profile||!technologies.all_succeeded()||!types.report().all_succeeded())
        throw std::runtime_error("TechAI import requires complete matching rules and save profile");
    ResearchAiWeights result;result.profile=setup.profile;
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(id<0||!setup.players.contains(id)||saved.knowledge.layout!=layout)
            throw std::runtime_error("TechAI import player/profile mismatch");
        const auto &techs=named(saved.knowledge.blocks,"player_techs");
        ResearchAiPlayerWeights player;
        if(vp)restore_vp_research_ai_weights(player,techs,saved.traits);
        else {
            const auto &array=named(techs.hashes,"tech_ai_weights");
            const auto count=array.wire_count>0?static_cast<std::uint64_t>(array.wire_count):0;
            if(count!=array.rows.size())
                throw std::runtime_error("Invalid BNW TechAI row count");
            // BNW Read clears/resizes the vector to the current technology
            // count and accepts a hash only when its ID is below the saved
            // count. The additional slots guard prevents the source OOB write.
            player.weight_slots=technologies.collection.slots;
            for(const auto &row:array.rows) {
                if(row.values.size()!=1)throw std::runtime_error("Invalid BNW TechAI weight row");
                if(row.type_hash==0)continue;
                const auto *type=types.find_hash(row.type_hash);
                if(!type||type->id<0||static_cast<std::uint64_t>(type->id)>=count||
                   static_cast<std::uint64_t>(type->id)>=player.weight_slots) {
                    ++player.unresolved_rows;continue;
                }
                const auto slot=static_cast<std::uint64_t>(type->id);
                if(row.values.front())player.weights[slot]=row.values.front();
                else player.weights.erase(slot); // last saved duplicate wins
            }
        }
        if(!result.players.emplace(id,std::move(player)).second)
            throw std::runtime_error("Duplicate TechAI player");
    }
    if(result.players.size()!=setup.players.size())throw std::runtime_error("TechAI roster mismatch");
    return result;
}
} // namespace civ5::runtime
