#include "civ5/content/research_rules.hpp"
#include <stdexcept>

namespace civ5::content {
ResearchRules load_research_rules(const RulesDatabase &rules, TechnologyProfile profile,
                                  const InfoCollectionBudget &budget) {
    if (profile != TechnologyProfile::bnw && profile != TechnologyProfile::vp542)
        throw std::runtime_error("Unknown research rules profile");
    ResearchRules result;
    result.profile = profile;
    result.handicaps.report = rules.prefetch_info("HandicapInfos", [&](auto slot, auto, const RulesRowView &row) {
        ResearchHandicapInfo info;
        info.base = read_info_metadata(row);
        info.percent = row.integer("ResearchPercent");
        if (profile == TechnologyProfile::vp542) {
            info.per_era_modifier = row.integer("ResearchPerEraModifier");
            info.ai_percent = row.integer("AIResearchPercent");
            info.ai_per_era_modifier = row.integer("AIResearchPerEraModifier");
            info.tech_catch_up = row.integer("TechCatchUpMod");
            info.ai_tech_catch_up = row.integer("AITechCatchUpMod");
        }
        result.handicaps.entries.emplace(slot, std::move(info));
        return true;
    }, budget);
    const auto percents = [&](const char *table, auto &collection) {
        collection.report = rules.prefetch_info(table, [&](auto slot, auto, const RulesRowView &row) {
            collection.entries.emplace(slot, ResearchPercentInfo{read_info_metadata(row), row.integer("ResearchPercent")});
            return true;
        }, budget);
    };
    percents("GameSpeeds", result.speeds);
    percents("Eras", result.eras);
    return result;
}
} // namespace civ5::content
