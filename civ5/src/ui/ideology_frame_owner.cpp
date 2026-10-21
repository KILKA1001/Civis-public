#include "civ5/ui/ideology_frame_owner.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>

namespace civ5::ui {

LiveUiHost::FrameProvider make_bnw_ideology_layout_frame_provider(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    LiveSessionUiSources sources, const ProgressionScreenSource &selected_source,
    IdeologyAdopterReader adopters, IdeologyTenetHelpReader tenet_help,
    IdeologyFreePolicyReader free_policies) {
    auto base = make_live_session_frame_provider(session, active_player_id,
                                                  std::move(sources));
    const auto selected_path = selected_source.selected_source_path;
    const bool selected_bnw = selected_source.requested_key ==
            "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseIdeologyPopup.xml" &&
        selected_path.find("Expansion2") != std::string::npos &&
        selected_source.document.name == "Context" &&
        selected_source.document.attribute("ID") == "ChooseIdeologyPopup";
    return [&session, active_player_id, base = std::move(base),
            selected_path, selected_bnw, adopters = std::move(adopters),
            tenet_help = std::move(tenet_help),
            free_policies = std::move(free_policies)]() mutable {
        auto frame = base();
        if (!selected_bnw || !adopters || !frame.active_player_id() ||
            *frame.active_player_id() != active_player_id || !frame.turn() ||
            frame.frame() == 0)
            return frame;
        const auto &live = session.state();
        const auto player = live.setup.players.find(active_player_id);
        const auto *event = frame.find<ProgressionIdeologyPopupEvent>();
        const auto *progression = progression_for_active_player(frame);
        if (live.setup.profile != content::TechnologyProfile::bnw ||
            *frame.turn() != live.setup.game_turn ||
            player == live.setup.players.end() || !player->second.alive ||
            !event || event->player_id != active_player_id || !event->token ||
            !progression || progression->team_id != player->second.team)
            return frame;
        constexpr std::array<std::string_view, 3> lua_order{
            "POLICY_BRANCH_AUTOCRACY", "POLICY_BRANCH_FREEDOM", "POLICY_BRANCH_ORDER"};
        std::map<std::string, std::int64_t> selected_ids;
        for (const auto &branch : progression->branches)
            if (branch.ideology) {
                if (branch.id < 0 ||
                    std::find(lua_order.begin(), lua_order.end(), branch.type) ==
                        lua_order.end() ||
                    !selected_ids.emplace(branch.type, branch.id).second)
                    return frame;
            }
        std::set<std::int64_t> expected;
        for (const auto type : lua_order) {
            const auto found = selected_ids.find(std::string(type));
            if (found == selected_ids.end() || !expected.insert(found->second).second)
                return frame;
        }
        if (selected_ids.size() != lua_order.size()) return frame;
        std::optional<std::vector<IdeologyAdopterCount>> counts;
        try {
            counts = adopters(session, *event);
        } catch (...) {
            return frame;
        }
        if (!counts || counts->size() != lua_order.size()) return frame;
        const auto current_player = live.setup.players.find(active_player_id);
        if (live.setup.profile != content::TechnologyProfile::bnw ||
            live.setup.game_turn != *frame.turn() ||
            current_player == live.setup.players.end() ||
            !current_player->second.alive ||
            current_player->second.team != progression->team_id)
            return frame;
        std::set<std::int64_t> observed;
        std::uint64_t total = 0;
        for (const auto &count : *counts) {
            if (!expected.contains(count.branch_id) ||
                !observed.insert(count.branch_id).second)
                return frame;
            total += count.alive_adopters;
        }
        const auto alive = std::count_if(live.setup.players.begin(),
            live.setup.players.end(), [](const auto &entry) {
                return entry.second.alive;
            });
        if (total > static_cast<std::uint64_t>(alive)) return frame;
        std::optional<std::vector<IdeologyTenetHelpRow>> help_rows;
        if (tenet_help) {
            try {
                help_rows = tenet_help(session, *event);
            } catch (...) {
                help_rows.reset();
            }
            const auto &after = session.state();
            const auto current = after.setup.players.find(active_player_id);
            if (after.setup.game_turn != *frame.turn() ||
                current == after.setup.players.end() || !current->second.alive ||
                current->second.team != progression->team_id)
                return frame;
            if (help_rows && help_rows->size() > 256) help_rows.reset();
            if (help_rows) {
                std::set<std::int64_t> observed_policies;
                std::int32_t previous_group = -1;
                bool valid_help = true;
                for (const auto &row : *help_rows) {
                    const auto branch = selected_ids.find(row.branch_type);
                    const auto policy = std::find_if(progression->policies.begin(),
                        progression->policies.end(), [&](const auto &candidate) {
                            return candidate.id == row.policy_id &&
                                candidate.type == row.policy_type &&
                                candidate.branch_type == row.branch_type;
                        });
                    const auto rank = std::find(lua_order.begin(), lua_order.end(),
                                                row.branch_type);
                    const auto group = static_cast<std::int32_t>(
                        std::distance(lua_order.begin(), rank)) * 3 + row.level;
                    if (branch == selected_ids.end() ||
                        rank == lua_order.end() || row.level < 1 || row.level > 3 ||
                        group < previous_group || policy == progression->policies.end() ||
                        !observed_policies.insert(row.policy_id).second ||
                        row.help_key.empty() || row.localized_help.empty() ||
                        row.label_width <= 0 || row.label_width > 532 ||
                        row.label_height <= 0 || row.label_height > 4096) {
                        valid_help = false;
                        break;
                    }
                    previous_group = group;
                }
                if (!valid_help) help_rows.reset();
            }
        }
        std::optional<std::vector<IdeologyFreePolicyCount>> free_counts;
        if (free_policies) {
            try {
                free_counts = free_policies(session, *event);
            } catch (...) {
                free_counts.reset();
            }
            const auto &after = session.state();
            const auto current = after.setup.players.find(active_player_id);
            if (after.setup.game_turn != *frame.turn() ||
                current == after.setup.players.end() || !current->second.alive ||
                current->second.team != progression->team_id)
                return frame;
            if (free_counts) {
                std::set<std::int64_t> seen;
                bool valid = free_counts->size() == lua_order.size();
                if (valid) for (const auto &entry : *free_counts)
                    if (!expected.contains(entry.branch_id) ||
                        !seen.insert(entry.branch_id).second) {
                        valid = false;
                        break;
                    }
                if (!valid) free_counts.reset();
            }
        }
        IdeologyLayoutEvidence evidence;
        evidence.player_id = active_player_id;
        evidence.turn = *frame.turn();
        evidence.frame = frame.frame();
        evidence.event_token = event->token;
        evidence.selected_source_path = selected_path;
        evidence.branches = std::move(*counts);
        evidence.tenet_help = std::move(help_rows);
        evidence.free_policy_counts = std::move(free_counts);
        return frame.with<IdeologyLayoutEvidence>(
            std::make_shared<const IdeologyLayoutEvidence>(std::move(evidence)));
    };
}

} // namespace civ5::ui
