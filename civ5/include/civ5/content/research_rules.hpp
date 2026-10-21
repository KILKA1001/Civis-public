#pragma once
#include "civ5/content/technology_rules.hpp"

namespace civ5::content {
// The research fields of these Info collections; other mechanics are not yet
// represented here. IDs are PrefetchCollection slots, not raw authored IDs.
struct ResearchHandicapInfo {
    InfoMetadata base;
    std::int64_t percent = 0, per_era_modifier = 0, ai_percent = 0, ai_per_era_modifier = 0;
    std::int64_t tech_catch_up = 0, ai_tech_catch_up = 0;
};
struct ResearchPercentInfo { InfoMetadata base; std::int64_t percent = 0; };
template <class T> struct ResearchInfoCollection {
    InfoCollectionReport report;
    std::map<std::uint64_t, T> entries;
    const T *find(std::int64_t id) const {
        if (id < 0) return nullptr;
        const auto it = entries.find(static_cast<std::uint64_t>(id));
        return it == entries.end() ? nullptr : &it->second;
    }
};
struct ResearchRules {
    TechnologyProfile profile = TechnologyProfile::bnw;
    ResearchInfoCollection<ResearchHandicapInfo> handicaps;
    ResearchInfoCollection<ResearchPercentInfo> speeds, eras;
    bool all_succeeded() const {
        return handicaps.report.all_succeeded() && speeds.report.all_succeeded() && eras.report.all_succeeded();
    }
};
ResearchRules load_research_rules(const RulesDatabase &rules, TechnologyProfile profile,
                                  const InfoCollectionBudget &budget = {});
} // namespace civ5::content
