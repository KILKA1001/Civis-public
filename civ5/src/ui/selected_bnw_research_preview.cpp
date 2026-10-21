#include "civ5/ui/selected_bnw_research_preview.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_research_choice.hpp"
#include <climits>
#include <exception>
#include <span>

namespace civ5::ui {
namespace {

class PreviewServices final : public runtime::ResearchSelectionServices {
  public:
    std::optional<bool> test_all(std::string_view, std::int64_t,
                                 std::int64_t) override {
        return std::nullopt;
    }
    void research_queue_dirty(std::int64_t) override {
        throw runtime::ResearchSelectionUnavailable(
            "Read-only research preview attempted to mutate the queue");
    }

};

std::vector<std::int64_t> candidates(
    runtime::LiveSessionState &detached,
    const runtime::BnwResearchChoiceRules &rules,
    std::int64_t player_id) {
    PreviewServices services;
    runtime::ResearchSelectionContext context{
        detached.research_selection, detached.research_turn,
        detached.player_research, detached.setup, rules.technologies, nullptr,
        std::span<const runtime::TeamTechnologies>(detached.technologies),
        detached.maintenance.team_eras,
        runtime::research_prerequisite_limits(rules.defines), services,
        rules.active_team, detached.science.no_science};
    std::vector<std::int64_t> result;
    for (const auto &[slot, info] : rules.technologies.entries) {
        (void)info;
        if (slot > static_cast<std::uint64_t>(INT64_MAX))
            throw runtime::ResearchSelectionUnavailable(
                "Selected BNW technology ID exceeds the live research range");
        const auto id = static_cast<std::int64_t>(slot);
        if (runtime::can_research(context, player_id, id)) result.push_back(id);
    }
    return result;
}

} // namespace

SelectedBnwResearchPreview preview_selected_bnw_research(
    const SelectedBnwPreviewHost &selected) {
    const auto &session = selected.session();
    const auto &technologies = selected.technologies();
    const auto &defines = selected.defines();
    const auto &database = selected.database();
    const auto &identity = selected.identity();
    const auto player_id = selected.active_player_id();
    SelectedBnwResearchPreview result;
    result.player_id = player_id;
    const auto &state = session.state();
    result.turn = state.setup.game_turn;
    const auto player = state.setup.players.find(player_id);
    if (player_id < 0 || player == state.setup.players.end() ||
        !player->second.alive || state.setup.game_turn < 0 ||
        state.setup.game_turn > INT32_MAX ||
        state.setup.profile != content::TechnologyProfile::bnw ||
        state.research_selection.profile != content::TechnologyProfile::bnw ||
        technologies.profile != content::TechnologyProfile::bnw ||
        defines.profile() != content::GlobalDefineProfile::bnw ||
        !technologies.all_succeeded()) {
        result.blocker = "Selected BNW research player, turn or rules are incomplete";
        return result;
    }
    result.team_id = player->second.team;
    try {
        if (identity.payload_sha256.size() != 64 ||
            identity.rules_sha256.size() != 64 ||
            runtime::identify_bytes(session.source().source.bytes).sha256 !=
                identity.payload_sha256 ||
            runtime::identify_bytes(database.gameplay_fingerprint_data()).sha256 !=
                identity.rules_sha256) {
            result.blocker = "Selected save or activated gameplay rules identity changed";
            return result;
        }
        auto detached = state;
        const runtime::BnwResearchChoiceRules rules{
            technologies, defines, result.team_id};
        result.native_candidates = candidates(detached, rules, player_id);
        result.blocker_code = SelectedBnwResearchBlocker::stock_ui_contexts_unverified;
        result.blocker = "Selected BNW observational Lua TestAll is unavailable";
    } catch (const std::exception &error) {
        result.native_candidates.clear();
        result.blocker = error.what();
    } catch (...) {
        result.native_candidates.clear();
        result.blocker = "Selected BNW research preview failed";
    }
    return result;
}

} // namespace civ5::ui
