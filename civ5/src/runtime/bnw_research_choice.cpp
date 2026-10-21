#include "civ5/runtime/bnw_research_choice.hpp"
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace civ5::runtime {
namespace {
class BufferedServices final : public ResearchSelectionServices {
  public:
    BufferedServices(const BnwResearchChoiceServices &source,
                     const LiveSessionState &state,
                     std::vector<std::int64_t> &dirty)
        : source_(source), state_(state), dirty_(dirty) {}

    std::optional<bool> test_all(std::string_view event, std::int64_t player,
                                 std::int64_t technology) override {
        if (!source_.lua_ready())
            throw ResearchSelectionUnavailable("Research Lua host became unavailable");
        return source_.lua_test_all_read_only(event, player, technology, state_);
    }

    void research_queue_dirty(std::int64_t team) override {
        dirty_.push_back(team);
    }

  private:
    const BnwResearchChoiceServices &source_;
    const LiveSessionState &state_;
    std::vector<std::int64_t> &dirty_;
};
} // namespace

LiveActionResult push_bnw_research_choice(
    LiveSession &session, std::int64_t player, std::int64_t technology,
    bool clear, const BnwResearchChoiceRules &rules,
    const BnwResearchChoiceServices &services) {
    if (!services.lua_ready || !services.lua_test_all_read_only ||
        !services.dirty_after_commit)
        return {LiveActionOutcome::rejected,
                "BNW research choice requires Lua checks and dirty service"};
    if (!services.lua_ready())
        return {LiveActionOutcome::rejected, "BNW research Lua host is unavailable"};
    if (rules.technologies.profile != content::TechnologyProfile::bnw ||
        rules.defines.profile() != content::GlobalDefineProfile::bnw ||
        !rules.technologies.all_succeeded())
        return {LiveActionOutcome::rejected, "BNW research rules are incomplete"};

    const auto limits = research_prerequisite_limits(rules.defines);
    std::vector<std::int64_t> pending_dirty;
    auto result = apply_live_action(session, [&](LiveSessionState &state) {
        if (state.setup.profile != content::TechnologyProfile::bnw ||
            state.research_selection.profile != content::TechnologyProfile::bnw ||
            !state.setup.players.contains(player) ||
            !state.research_selection.players.contains(player))
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "BNW research player state is incomplete"};
        BufferedServices buffered(services, state, pending_dirty);
        ResearchSelectionContext context{
            state.research_selection, state.research_turn,
            state.player_research, state.setup, rules.technologies, nullptr,
            state.technologies, state.maintenance.team_eras, limits, buffered,
            rules.active_team, state.science.no_science};
        if (!push_research(context, player, technology, clear))
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "BNW research choice is unavailable"};
        return LiveActionResult{LiveActionOutcome::committed, {}};
    });
    if (result.outcome != LiveActionOutcome::committed)
        return result;
    for (const auto team : pending_dirty) {
        try {
            services.dirty_after_commit(team);
        } catch (const std::exception &error) {
            result.reason = std::string("Research queue committed; dirty notification failed: ") + error.what();
            break;
        } catch (...) {
            result.reason = "Research queue committed; dirty notification failed";
            break;
        }
    }
    return result;
}

} // namespace civ5::runtime
