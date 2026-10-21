#include "civ5/runtime/vp_research_selection.hpp"
#include "civ5/runtime/research_selection.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace civ5::runtime {
namespace {
struct Prerequisite { std::uint64_t index; std::int64_t technology; };

// VP stops at the first empty prerequisite slot rather than scanning holes.
std::optional<Prerequisite> next(const content::RuleValues &values,
    std::int64_t limit, std::uint64_t &cursor) {
    if (limit <= 0 || cursor >= static_cast<std::uint64_t>(limit)) return {};
    if (values.default_value != -1)
        throw std::runtime_error("Research prerequisite default is not NO_TECH");
    const auto bound = std::min(values.extent, static_cast<std::uint64_t>(limit));
    while (cursor < bound) {
        const auto entry = values.values.lower_bound(cursor);
        if (entry == values.values.end() || entry->first >= bound ||
            entry->first != cursor || entry->second == -1) {
            cursor = bound;
            return {};
        }
        cursor = entry->first + 1;
        return Prerequisite{entry->first, entry->second};
    }
    if (static_cast<std::uint64_t>(limit) > values.extent)
        throw std::out_of_range("Research prerequisite limit exceeds source extent");
    return {};
}
} // namespace

std::int64_t vp_research_path_length(const content::TechnologyRules &rules,
    const TeamTechnologies &team, const ResearchPrerequisiteLimits &limits,
    std::int64_t technology) {
    using Set = std::set<std::int64_t>;
    std::map<std::pair<std::int64_t, int>, Set> memo;
    const auto visit = [&](auto &&self, std::int64_t id, int depth) -> const Set & {
        const auto key = std::pair{id, depth};
        if (memo.contains(key)) return memo.at(key);
        auto &result = memo[key];
        if (team.has_tech(id) || depth == 0) return result;
        const auto *info = id < 0 ? nullptr : rules.find(static_cast<std::uint64_t>(id));
        if (!info) return result;
        result.insert(id);
        std::uint64_t cursor = 0;
        while (auto pre = next(info->and_prerequisites, limits.and_count, cursor))
            if (!team.has_tech(pre->technology)) {
                const auto &child = self(self, pre->technology, depth - 1);
                result.insert(child.begin(), child.end());
            }
        Set best;
        cursor = 0;
        while (auto pre = next(info->or_prerequisites, limits.or_count, cursor))
            if (!team.has_tech(pre->technology)) {
                const auto &child = self(self, pre->technology, depth - 1);
                if (pre->index == 0 || child.size() < best.size()) best = child;
            }
        result.insert(best.begin(), best.end());
        return result;
    };
    const auto count = visit(visit, technology, 23).size();
    if (count > INT64_MAX) throw std::overflow_error("Research path exceeds int64");
    return static_cast<std::int64_t>(count);
}
bool vp_ai_prerequisites_met(const content::TechnologyRules &rules, const TeamTechnologies &team,
    std::int64_t tech, ResearchPrerequisiteLimits limits, const std::set<std::int64_t> &extra_techs) {
    if (rules.profile != team.profile() || rules.collection.slots != team.slots() || !rules.all_succeeded())
        throw std::invalid_argument("Research requires matching complete technology rules and team state");
    if (team.profile() != content::TechnologyProfile::vp542)
        throw std::invalid_argument("Research operation has a different source profile");
    const auto *info = tech < 0 ? nullptr : rules.find(static_cast<std::uint64_t>(tech));
    if (!info) return false;
    bool all = true, any = false;
    std::uint64_t cursor = 0;
    while (auto prereq = next(info->and_prerequisites, limits.and_count, cursor)) {
        all = team.has_tech(prereq->technology) || extra_techs.contains(prereq->technology);
        if (!all) break;
    }
    cursor = 0;
    while (auto prereq = next(info->or_prerequisites, limits.or_count, cursor)) {
        any = team.has_tech(prereq->technology) || extra_techs.contains(prereq->technology);
        if (any) break;
    }
    return all || any;
}
std::vector<std::int64_t> vp_tech_frontier(const content::TechnologyRules &rules, const TeamTechnologies &team,
    ResearchPrerequisiteLimits limits) {
    if (rules.profile != team.profile() || rules.collection.slots != team.slots() || !rules.all_succeeded())
        throw std::invalid_argument("Research requires matching complete technology rules and team state");
    if (team.profile() != content::TechnologyProfile::vp542)
        throw std::invalid_argument("Research operation has a different source profile");
    std::set<std::int64_t> first, second;
    const auto visit = [&](auto &&callback) {
        for (const auto &[slot, info] : rules.entries) {
            if (slot > INT64_MAX) throw std::overflow_error("Technology ID exceeds int64");
            callback(static_cast<std::int64_t>(slot));
        }
    };
    visit([&](auto tech) {
        if (!team.has_tech(tech) && vp_ai_prerequisites_met(rules, team, tech, limits)) first.insert(tech);
    });
    visit([&](auto tech) {
        if (!team.has_tech(tech) && !first.contains(tech) && vp_ai_prerequisites_met(rules, team, tech, limits, first))
            second.insert(tech);
    });
    first.insert(second.begin(), second.end());
    return {first.begin(), first.end()};
}
} // namespace civ5::runtime
