#include "civ5/ui/progression_screens.hpp"
#include "civ5/content/info_metadata.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {

std::string_view source_key(ProgressionScreen screen) {
    switch (screen) {
    case ProgressionScreen::technologies: return "Assets/UI/InGame/TechTree/TechTree.xml";
    case ProgressionScreen::policies: return "Assets/UI/InGame/Popups/SocialPolicyPopup.xml";
    case ProgressionScreen::ideology: return "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseIdeologyPopup.xml";
    case ProgressionScreen::technology_popup: return "Assets/UI/InGame/TechPopup.xml";
    }
    throw std::runtime_error("Unknown progression screen");
}

std::optional<std::string> localized(const content::InfoText &text,
                                     const content::InfoLocalizer &localizer) {
    if (text.value) return text.value;
    if (localizer && !text.key.empty()) return localizer(text.key);
    return std::nullopt;
}

std::int64_t integer_or(const content::TechnologyInfo &info, std::string_view key) {
    const auto it = info.integers.find(std::string(key));
    return it == info.integers.end() ? 0 : it->second;
}

const content::XmlNode *find_control(const content::XmlNode &node, std::string_view id) {
    if (node.attribute("ID") == id) return &node;
    for (const auto &child : node.children)
        if (const auto *found = find_control(child, id)) return found;
    return nullptr;
}

const content::XmlNode *find_instance(const content::XmlNode &node, std::string_view name) {
    if (node.name == "Instance" && node.attribute("Name") == name) return &node;
    for (const auto &child : node.children)
        if (const auto *found = find_instance(child, name)) return found;
    return nullptr;
}

std::optional<std::pair<std::int64_t, std::int64_t>> authored_size(const content::XmlNode *control) {
    if (!control) return std::nullopt;
    const auto size = control->attribute("Size");
    const auto separator = size.find_first_of(",.");
    if (separator == std::string::npos) return std::nullopt;
    try {
        return std::pair{std::stoll(size.substr(0, separator)),
                         std::stoll(size.substr(separator + 1))};
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::string branch_panel(std::string_view type) {
    constexpr std::string_view prefix = "POLICY_BRANCH_";
    if (!type.starts_with(prefix)) return {};
    std::string name(type.substr(prefix.size()));
    if (name.empty()) return {};
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    return name + "Panel";
}

bool valid_frame_player(const ProgressionFrameState *state, const UiFrameSnapshot &frame) {
    return state && frame.active_player_id() && state->player_id == *frame.active_player_id();
}

struct PolicyRuleRow {
    std::int64_t id = -1, level = 0;
    std::string type, branch, tech;
};
struct PolicyBranchRow {
    std::string type;
    bool purchase_by_level = false;
};

template <class T> const T *unique_named(const std::vector<T> &values,
                                         std::string_view name) {
    const T *found = nullptr;
    for (const auto &value : values)
        if (value.name && std::string_view(value.name) == name) {
            if (found) throw std::runtime_error("Duplicate BNW policy source field");
            found = &value;
        }
    return found;
}

std::int64_t core_scalar(const saves::PlayerCoreObservation &core,
                         std::string_view name) {
    const auto *field = unique_named(core.fields, name);
    if (!field || field->values.size() != 1)
        throw std::runtime_error("BNW policy core value is unavailable: " + std::string(name));
    return field->values.front();
}

std::vector<std::string> policy_links(const content::RulesDatabase &database,
    std::string_view table, std::string_view target_column, std::string_view policy_type) {
    std::vector<std::string> links;
    const content::DatabaseStatement statement{
        "SELECT PolicyType, " + std::string(target_column) + " FROM " +
            std::string(table) + " WHERE PolicyType = ?",
        {std::string(policy_type)}};
    const auto report = database.query_rows(statement,
        [&](const content::RulesRowView &row) {
            const auto type = row.text(target_column);
            if (!type || type->empty())
                throw std::runtime_error("BNW policy prerequisite has no type");
            links.push_back(*type);
        }, 4096);
    if (!report.all_succeeded())
        throw std::runtime_error("Selected BNW policy link table failed: " +
                                 std::string(table) + ": " + report.error);
    return links;
}

// The current BNW LiveSession import retains these policy bytes in its owned
// world, but has not yet applied the policy load hook to ResearchContext.
// Decode them only for read-only UI state, with selected rule IDs and hashes.
std::set<std::int64_t> saved_policy_ownership(
    const saves::SavedSessionSnapshot &world, std::int64_t player_id,
    const std::map<std::int64_t, PolicyRuleRow> &policies) {
    if (!world.all_players ||
        world.all_players->layout != saves::GameStateLayout::expansion2_version1)
        throw std::runtime_error("Saved BNW policy player source is unavailable");
    std::map<std::uint32_t, std::int64_t> ids_by_hash;
    for (const auto &[id, rule] : policies) {
        if (rule.type.empty() ||
            !ids_by_hash.emplace(content::saved_info_type_hash(rule.type), id).second)
            throw std::runtime_error("Selected BNW policy hashes are ambiguous");
    }
    const saves::SavedPlayerObservation *saved = nullptr;
    for (const auto &candidate : world.all_players->players)
        if (candidate.saved_player_id == player_id) {
            if (saved) throw std::runtime_error("Duplicate saved BNW policy player");
            saved = &candidate;
        }
    if (!saved) throw std::runtime_error("Saved BNW policy player is unavailable");
    const auto *block = unique_named(saved->strategy.blocks, "policies");
    const auto *flags = block ? unique_named(block->hashes, "has_policy") : nullptr;
    if (!flags) throw std::runtime_error("Saved BNW policy ownership flags are unavailable");
    std::map<std::int64_t, bool> observed;
    for (const auto &row : flags->rows) {
        if (!row.type_hash) {
            if (!row.values.empty())
                throw std::runtime_error("Saved BNW policy sentinel has a value");
            continue;
        }
        const auto rule = ids_by_hash.find(row.type_hash);
        if (rule == ids_by_hash.end() || row.values.size() != 1 ||
            row.values.front() < 0 || row.values.front() > 255)
            throw std::runtime_error("Saved BNW policy hash or flag differs from selected rules");
        observed[rule->second] = row.values.front() != 0; // Native assignment: last wins.
    }
    if (observed.size() != policies.size())
        throw std::runtime_error("Saved BNW policy dimensions differ from selected rules");
    std::set<std::int64_t> owned;
    for (const auto &[id, adopted] : observed)
        if (adopted) owned.insert(id);
    return owned;
}

} // namespace

std::string_view progression_context_id(ProgressionScreen screen) {
    switch (screen) {
    case ProgressionScreen::technologies: return "TechTree";
    case ProgressionScreen::policies: return "SocialPolicyPopup";
    case ProgressionScreen::ideology: return "ChooseIdeology";
    case ProgressionScreen::technology_popup: return "TechPopup";
    }
    throw std::runtime_error("Unknown progression screen");
}

ProgressionScreenSource read_progression_screen_source(
    const assets::ResourceMountTable &resources, ProgressionScreen screen) {
    const auto key = source_key(screen);
    const auto resolved = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *asset = std::get_if<assets::AssetHandle>(&resolved.asset);
    if (!asset) throw std::runtime_error("BNW progression XML is missing or ambiguous: " + std::string(key));
    const auto bytes = asset->read_bytes(64ULL * 1024 * 1024);
    const auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
    if (xml.name != "Context") throw std::runtime_error("Progression XML root is not Context");
    ProgressionScreenSource result{std::string(key),
                                   asset->candidate().provenance.source_relative_path, {}, xml};
    auto walk = [&](auto &&self, const content::XmlNode &node) -> void {
        const auto id = node.attribute("ID");
        if (!id.empty()) result.control_ids.push_back(id);
        for (const auto &child : node.children) self(self, child);
    };
    walk(walk, xml);
    const auto required = screen == ProgressionScreen::technologies ? "TechTreeScrollPanel"
        : screen == ProgressionScreen::policies ? "SocialPolicyPane"
        : screen == ProgressionScreen::ideology ? "ItemScrollPanel" : "ButtonStack";
    if (std::find(result.control_ids.begin(), result.control_ids.end(), required) == result.control_ids.end())
        throw std::runtime_error("Selected progression XML lacks required control");
    return result;
}

UiCommand progression_open_command(ProgressionScreen screen) {
    return UiOpenScreen{std::string(progression_context_id(screen))};
}

std::vector<UiCommand> progression_popup_open_tree_commands() {
    // BNW TechPopup.lua closes the chooser before opening the technology tree.
    return {UiCloseScreen{"TechPopup"}, progression_open_command(ProgressionScreen::technologies)};
}

UiCommand progression_top_panel_command(bool science) {
    return progression_open_command(science ? ProgressionScreen::technologies
                                            : ProgressionScreen::policies);
}

std::optional<UiCommand> progression_top_panel_command(TopPanelAction action) {
    if (action == TopPanelAction::open_technology_tree)
        return progression_open_command(ProgressionScreen::technologies);
    if (action == TopPanelAction::open_policy_choice)
        return progression_open_command(ProgressionScreen::policies);
    return std::nullopt;
}

ProgressionRenderPlan plan_progression_screen(const ProgressionScreenSource &source,
    ProgressionScreen screen, const UiFrameSnapshot &frame) {
    ProgressionRenderPlan plan{screen, source.selected_source_path, {}, {}};
    const auto *state = progression_for_active_player(frame);
    if (!state) {
        plan.unresolved.push_back("No progression state for the active player");
        return plan;
    }
    if (!state->unavailable_reason.empty()) plan.unresolved.push_back(state->unavailable_reason);
    if (screen == ProgressionScreen::technology_popup) {
        const auto *popup = frame.find<ProgressionTechPopupState>();
        if (!popup || !frame.active_player_id() || popup->player_id != *frame.active_player_id() ||
            popup->token == 0) {
            plan.unresolved.push_back("No active BNW technology choice event");
            return plan;
        }
        if (!popup->unavailable_reason.empty()) {
            plan.unresolved.push_back(popup->unavailable_reason);
            // OpenTTButton belongs to the popup event, not research eligibility.
            // Keep its authored chrome while withholding candidate rows.
            return plan;
        }
        const auto *instance = find_instance(source.document, "TechButtonInstance");
        const auto size = instance ? authored_size(find_control(*instance, "TechButton")) : std::nullopt;
        const auto container = instance ? authored_size(find_control(*instance, "TechButtonContainer")) : std::nullopt;
        if (!size || !container) {
            plan.unresolved.push_back("Selected TechPopup XML lacks numeric technology button dimensions");
            return plan;
        }
        // BNW TechPopup.lua adds one 70px instance per ButtonStack row; its
        // active XML gives the stack 6px padding.
        std::int64_t row = 0;
        for (const auto id : popup->eligible_technologies) {
            const auto tech = std::find_if(state->technologies.begin(), state->technologies.end(),
                [id](const auto &value) { return value.id == id; });
            if (tech == state->technologies.end()) {
                plan.unresolved.push_back("Eligible technology is absent from active rules: " + std::to_string(id));
                continue;
            }
            if (!tech->description)
                plan.unresolved.push_back("Missing localized technology: " + tech->type);
            plan.items.push_back({ProgressionItemKind::technology, tech->id, "ButtonStack",
                tech->description.value_or(""),
                ProgressionRect{0, row * (container->second + 6), size->first, size->second},
                state->current_research == tech->id, false});
            ++row;
        }
    } else if (screen == ProgressionScreen::technologies) {
        const auto *instance = find_instance(source.document, "TechButtonInstance");
        const auto size = instance ? authored_size(find_control(*instance, "TechButton")) : std::nullopt;
        if (!size) {
            plan.unresolved.push_back("Selected TechTree XML has no numeric TechButton size");
            return plan;
        }
        for (const auto &tech : state->technologies) {
            if (tech.grid_x < 0 || tech.grid_y < 0) continue;
            if (!tech.description) plan.unresolved.push_back("Missing localized technology: " + tech.type);
            plan.items.push_back({ProgressionItemKind::technology, tech.id, "TechTreeScrollPanel",
                tech.description.value_or(""),
                ProgressionRect{tech.grid_x * 366 + 64, (tech.grid_y - 5) * 68 + 32,
                                size->first, size->second},
                state->current_research == tech.id, tech.researched});
        }
    } else if (screen == ProgressionScreen::policies) {
        const auto *instance = find_instance(source.document, "PolicyButton");
        const auto size = instance ? authored_size(find_control(*instance, "PolicyIcon")) : std::nullopt;
        if (!size) {
            plan.unresolved.push_back("Selected SocialPolicyPopup XML has no numeric PolicyIcon size");
            return plan;
        }
        if (!state->policy_ownership_resolved)
            plan.unresolved.push_back("Live policy ownership is unavailable");
        for (const auto &policy : state->policies) {
            const auto parent = branch_panel(policy.branch_type);
            if (policy.grid_x <= 0 || policy.grid_y <= 0 ||
                std::find(source.control_ids.begin(), source.control_ids.end(), parent) ==
                    source.control_ids.end()) continue;
            if (!policy.description) plan.unresolved.push_back("Missing localized policy: " + policy.type);
            plan.items.push_back({ProgressionItemKind::policy, policy.id, parent,
                policy.description.value_or(""),
                ProgressionRect{(policy.grid_x - 1) * 28 + 16,
                                (policy.grid_y - 1) * 68 + 12, size->first, size->second},
                false, state->policy_ownership_resolved && policy.adopted});
        }
        plan.unresolved.push_back("Policy eligibility and BNW tenet layout need a policy GameCore host");
    } else {
        const auto *event = frame.find<ProgressionIdeologyPopupEvent>();
        if (!event || event->player_id != state->player_id || event->token == 0) {
            plan.unresolved.push_back("No active BNW ideology choice event");
            return plan;
        }
        for (const auto &branch : state->branches) {
            if (!branch.ideology) continue;
            if (!branch.description) plan.unresolved.push_back("Missing localized ideology: " + branch.type);
            plan.items.push_back({ProgressionItemKind::ideology, branch.id, "ItemStack",
                                  branch.description.value_or(""), std::nullopt, false, false});
        }
        plan.unresolved.push_back("Ideology row height depends on live adopted-by and tenet stacks");
    }
    return plan;
}

UiCommand progression_choose_technology(std::int64_t id, bool clear_queue) {
    return UiGameplayCommand::make("TechTree", ProgressionChooseTechnology{id, clear_queue});
}
UiCommand progression_choose_popup_technology(std::int64_t id) {
    return UiGameplayCommand::make("TechPopup", ProgressionChooseTechnology{id, false});
}
UiCommand progression_popup_open_tree_command(std::uint64_t event_token) {
    return UiGameplayCommand::make("TechPopup",
        ProgressionOpenTechnologyTree{event_token});
}
UiCommand progression_tree_close_command(std::uint64_t event_token) {
    return UiGameplayCommand::make("TechTree",
        ProgressionCloseTechnologyTree{event_token});
}
UiCommand progression_choose_policy(std::int64_t id) {
    return UiGameplayCommand::make("SocialPolicyPopup", ProgressionChoosePolicy{id});
}
UiCommand progression_choose_ideology(std::int64_t id) {
    return UiGameplayCommand::make("ChooseIdeology", ProgressionChooseIdeology{id});
}

const ProgressionFrameState *progression_for_active_player(const UiFrameSnapshot &frame) {
    const auto *state = frame.find<ProgressionFrameState>();
    return valid_frame_player(state, frame) ? state : nullptr;
}

ProgressionTechPopupState capture_progression_tech_popup(
    runtime::ResearchSelectionContext &research, std::int64_t player_id,
    ProgressionTechPopupMode mode, std::optional<std::int64_t> steal_target_player) {
    ProgressionTechPopupState result;
    result.player_id = player_id;
    result.mode = mode;
    result.steal_target_player = steal_target_player;
    if (mode == ProgressionTechPopupMode::steal_technology && !steal_target_player) {
        result.unavailable_reason = "Technology theft has no target player";
        return result;
    }
    const runtime::TeamTechnologies *opponent = nullptr;
    if (steal_target_player) {
        const auto player = research.setup.players.find(*steal_target_player);
        if (player == research.setup.players.end()) {
            result.unavailable_reason = "Technology theft target is absent";
            return result;
        }
        const auto team = std::find_if(research.teams.begin(), research.teams.end(),
            [&](const auto &value) { return value.team() == player->second.team; });
        if (team == research.teams.end()) {
            result.unavailable_reason = "Technology theft target team is unavailable";
            return result;
        }
        opponent = &*team;
    }
    try {
        for (const auto &[slot, info] : research.technologies.entries) {
            const auto id = static_cast<std::int64_t>(slot);
            const bool eligible = mode == ProgressionTechPopupMode::free_technology
                ? runtime::can_research_for_free(research, player_id, id)
                : runtime::can_research(research, player_id, id);
            if (eligible && (!opponent || opponent->has_tech(id)))
                result.eligible_technologies.push_back(id);
        }
    } catch (const std::exception &error) {
        result.eligible_technologies.clear();
        result.unavailable_reason = error.what();
    }
    return result;
}

ProgressionFrameState capture_progression_frame(
    const runtime::ResearchContext &world, const runtime::PlayerResearchContext &players,
    std::span<const runtime::TeamTechnologies> teams,
    const content::TechnologyRules &technologies, const content::RulesDatabase &database,
    std::int64_t player_id, const content::InfoLocalizer &localizer,
    const saves::SavedSessionSnapshot *saved_world) {
    ProgressionFrameState result;
    result.player_id = player_id;
    const auto player = world.players.find(player_id);
    if (player == world.players.end() || !player->second.alive) {
        result.unavailable_reason = "Active player has no live research state";
        return result;
    }
    result.team_id = player->second.team;
    const auto team = std::find_if(teams.begin(), teams.end(), [&](const auto &value) {
        return value.team() == result.team_id;
    });
    if (team == teams.end() || !technologies.all_succeeded()) {
        result.unavailable_reason = "Active BNW technology state is incomplete";
        return result;
    }
    const auto research = players.players.find(player_id);
    if (research == players.players.end()) {
        result.unavailable_reason = "Active player's research queue is unavailable";
        return result;
    }
    if (research->second.current_research() >= 0)
        result.current_research = research->second.current_research();
    for (const auto &[slot, info] : technologies.entries) {
        const auto id = static_cast<std::int64_t>(slot);
        result.technologies.push_back({id, info.base.type, info.base.description.key,
            localized(info.base.description, localizer), info.era,
            integer_or(info, "GridX"), integer_or(info, "GridY"), integer_or(info, "Cost"),
            team->has_tech(id),
            std::find(research->second.queue.begin(), research->second.queue.end(), id) !=
                research->second.queue.end(), team->progress_times100(id)});
    }
    result.policy_ownership_resolved = player->second.policies_resolved;
    std::set<std::string> ideology_types = {
        "POLICY_BRANCH_FREEDOM", "POLICY_BRANCH_ORDER", "POLICY_BRANCH_AUTOCRACY"};
    const auto branches = database.prefetch_info("PolicyBranchTypes", [&](std::uint64_t slot,
            std::int64_t, const content::RulesRowView &row) {
        const auto metadata = content::read_info_metadata(row, localizer);
        result.branches.push_back({static_cast<std::int64_t>(slot), metadata.type,
            metadata.description.key, localized(metadata.description, localizer),
            ideology_types.contains(metadata.type)});
        return true;
    });
    const auto policies = database.prefetch_info("Policies", [&](std::uint64_t slot,
            std::int64_t, const content::RulesRowView &row) {
        const auto metadata = content::read_info_metadata(row, localizer);
        result.policies.push_back({static_cast<std::int64_t>(slot), metadata.type,
            row.text("PolicyBranchType").value_or(""), metadata.description.key,
            localized(metadata.description, localizer), row.integer("GridX"), row.integer("GridY"),
            result.policy_ownership_resolved && player->second.policies.contains(static_cast<std::int64_t>(slot))});
        return true;
    });
    if (!branches.all_succeeded() || !policies.all_succeeded()) {
        result.branches.clear();
        result.policies.clear();
        result.unavailable_reason = "Active BNW policy rules could not be loaded";
    } else if (!result.policy_ownership_resolved && saved_world) {
        std::map<std::int64_t, PolicyRuleRow> selected;
        for (const auto &policy : result.policies)
            selected.emplace(policy.id, PolicyRuleRow{policy.id, 0,
                policy.type, policy.branch_type, {}});
        try {
            const auto owned = saved_policy_ownership(*saved_world, player_id, selected);
            for (auto &policy : result.policies)
                policy.adopted = owned.contains(policy.id);
            result.policy_ownership_resolved = true;
        } catch (const std::exception &) {
            // Unresolved ownership remains disabled in the frame.
        }
    }
    return result;
}

UiCommandResult ProgressionGameplayBackend::availability(const UiGameplayCommand &command,
                                                          const UiFrameSnapshot &frame) const {
    const auto *state = progression_for_active_player(frame);
    if (!state) return {UiCommandStatus::unavailable, "Progression data for active player is unavailable"};
    if (const auto *choice = command.as<ProgressionChooseTechnology>()) {
        const bool popup_choice = command.source_id() == "TechPopup";
        if ((command.source_id() != "TechTree" && !popup_choice) || choice->technology < 0 ||
            (popup_choice && choice->clear_queue))
            return {UiCommandStatus::invalid, "Invalid technology choice"};
        if (popup_choice)
            return {UiCommandStatus::unavailable,
                    "TechPopup requires a live event owner and post-commit acknowledgement"};
        const auto item = std::find_if(state->technologies.begin(), state->technologies.end(),
            [&](const auto &value) { return value.id == choice->technology; });
        if (item == state->technologies.end())
            return {UiCommandStatus::invalid, "Technology is absent from active rules"};
        if (!research_) return {UiCommandStatus::unavailable, "Research selection backend is not connected"};
        if (research_->setup.profile != content::TechnologyProfile::bnw ||
            !research_->setup.players.contains(state->player_id) ||
            research_->setup.players.at(state->player_id).team != state->team_id ||
            !research_->turn.players.contains(state->player_id) ||
            !research_->state.players.contains(state->player_id))
            return {UiCommandStatus::unavailable, "Active BNW research player is unavailable"};
        if (!research_->turn.players.at(state->player_id).founded_first_city &&
            research_->state.players.at(state->player_id).advanced_start_points < 0)
            return {UiCommandStatus::unavailable, "Research needs a founded city"};
        // Do not invoke PlayerCanEverResearch Lua during a hover/availability
        // query: push_research performs the authoritative selection once.
        if (item->researched)
            return {UiCommandStatus::unavailable, "Technology is already researched"};
        return {UiCommandStatus::ready, {}};
    }
    if (command.as<ProgressionChoosePolicy>())
        return {UiCommandStatus::unavailable, "BNW policy adoption backend is not implemented"};
    if (command.as<ProgressionChooseIdeology>())
        return {UiCommandStatus::unavailable, "BNW ideology adoption backend is not implemented"};
    return {UiCommandStatus::invalid, "Unknown progression command"};
}

UiCommandResult ProgressionGameplayBackend::execute(const UiGameplayCommand &command,
                                                     const UiFrameSnapshot &frame) {
    const auto checked = availability(command, frame);
    if (checked.status != UiCommandStatus::ready) return checked;
    const auto *state = progression_for_active_player(frame);
    const auto *choice = command.as<ProgressionChooseTechnology>();
    try {
        if (runtime::push_research(*research_, state->player_id,
                                   choice->technology, choice->clear_queue))
            return {UiCommandStatus::performed, {}};
        return {UiCommandStatus::unavailable, "Research selection did not complete"};
    } catch (const std::exception &error) {
        return {UiCommandStatus::unavailable, error.what()};
    }
}

BnwPolicyChoicePreview preview_bnw_policy_choice(
    const runtime::LiveSession &session, const content::RulesDatabase &database,
    const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines, std::int64_t player_id,
    std::int64_t policy_id) {
    BnwPolicyChoicePreview result;
    const auto reject = [&](std::string reason) {
        result.blocker = std::move(reason);
        return result;
    };
    if (player_id < 0 || policy_id < 0)
        return reject("Invalid BNW policy player or ID");
    const auto &live = session.state();
    const auto player = live.setup.players.find(player_id);
    if (live.setup.profile != content::TechnologyProfile::bnw ||
        player == live.setup.players.end() || !player->second.alive ||
        !live.world.all_players ||
        live.world.all_players->layout != saves::GameStateLayout::expansion2_version1)
        return reject("Live BNW policy roster or source is unavailable");
    std::map<std::int64_t, PolicyRuleRow> policies;
    std::map<std::string, std::int64_t, std::less<>> policy_types;
    const auto policy_report = database.prefetch_info("Policies",
        [&](std::uint64_t slot, std::int64_t, const content::RulesRowView &row) {
            if (slot > static_cast<std::uint64_t>(INT64_MAX))
                throw std::overflow_error("BNW policy slot exceeds int64");
            PolicyRuleRow value;
            value.id = static_cast<std::int64_t>(slot);
            value.type = row.text("Type").value_or("");
            value.branch = row.text("PolicyBranchType").value_or("");
            value.tech = row.text("TechPrereq").value_or("");
            value.level = row.integer("Level");
            if (value.type.empty() || value.level < 0 ||
                !policy_types.emplace(value.type, value.id).second)
                throw std::runtime_error("Selected BNW policy identity is invalid");
            policies.emplace(value.id, std::move(value));
            return true;
        });
    if (!policy_report.all_succeeded())
        return reject("Selected BNW Policies rules are unavailable");
    const auto target = policies.find(policy_id);
    if (target == policies.end()) return reject("Policy is absent from selected BNW rules");
    result.policy_type = target->second.type;
    result.branch_type = target->second.branch;
    std::set<std::int64_t> saved_owned;
    const std::set<std::int64_t> *owned_policies = &player->second.policies;
    if (!player->second.policies_resolved) {
        try {
            saved_owned = saved_policy_ownership(live.world, player_id, policies);
            owned_policies = &saved_owned;
        } catch (const std::exception &error) {
            return reject(error.what());
        }
    }
    if (owned_policies->contains(policy_id))
        return reject("Policy is already adopted");
    if (target->second.branch.empty())
        return reject("Branch free policy cannot be chosen directly");

    std::map<std::string, PolicyBranchRow, std::less<>> branches;
    std::map<std::uint32_t, std::string> branch_hashes;
    const auto branch_report = database.prefetch_info("PolicyBranchTypes",
        [&](std::uint64_t, std::int64_t, const content::RulesRowView &row) {
            PolicyBranchRow value;
            value.type = row.text("Type").value_or("");
            value.purchase_by_level = row.boolean("PurchaseByLevel");
            if (value.type.empty()) throw std::runtime_error("BNW branch has no type");
            const auto hash = content::saved_info_type_hash(value.type);
            if (!branch_hashes.emplace(hash, value.type).second ||
                !branches.emplace(value.type, std::move(value)).second)
                throw std::runtime_error("Ambiguous selected BNW policy branch hash");
            return true;
        });
    if (!branch_report.all_succeeded())
        return reject("Selected BNW policy branch rules are unavailable");
    const auto branch = branches.find(target->second.branch);
    if (branch == branches.end()) return reject("Policy branch is absent from selected rules");

    const saves::SavedPlayerObservation *saved = nullptr;
    for (const auto &candidate : live.world.all_players->players)
        if (candidate.saved_player_id == player_id) {
            if (saved) return reject("Duplicate saved BNW policy player");
            saved = &candidate;
        }
    if (!saved) return reject("Saved BNW policy player is unavailable");
    try {
        result.culture = core_scalar(saved->core, "m_iJONSCulture");
        result.next_cost = core_scalar(saved->core, "m_iCostNextPolicy");
        result.free_policies = core_scalar(saved->core, "m_iNumFreePolicies");
        result.free_tenets = core_scalar(saved->core, "m_iNumFreeTenets");
        if (*result.culture < 0 || *result.next_cost < 0 ||
            *result.free_policies < 0 || *result.free_tenets < 0)
            return reject("Live BNW policy payment counters are invalid");
        const auto *block = unique_named(saved->strategy.blocks, "policies");
        const auto *flags = block ? unique_named(block->hashes, "branch_unlocked") : nullptr;
        if (!flags) return reject("Saved BNW branch unlock flags are unavailable");
        const auto branch_hash = content::saved_info_type_hash(target->second.branch);
        std::optional<bool> unlocked;
        for (const auto &row : flags->rows)
            if (row.type_hash == branch_hash) {
                if (row.values.size() != 1 || (row.values.front() != 0 && row.values.front() != 1))
                    return reject("Saved BNW branch unlock flag is invalid");
                unlocked = row.values.front() != 0; // Native hash assignment: last wins.
            }
        if (!unlocked) return reject("Live BNW branch unlock state is unavailable");
        if (!*unlocked) return reject("Policy branch is locked");
    } catch (const std::exception &error) {
        return reject(error.what());
    }
    if (*result.next_cost > 0 && *result.culture < *result.next_cost &&
        *result.free_policies == 0 &&
        (target->second.level == 0 || *result.free_tenets == 0))
        return reject("Insufficient culture or free policy/tenet credit");
    if (branch->second.purchase_by_level && target->second.level > 1) {
        std::int64_t current = 0, previous = 0;
        for (const auto owned_id : *owned_policies) {
            const auto owned = policies.find(owned_id);
            if (owned == policies.end())
                return reject("Live BNW policy ownership differs from selected rules");
            if (owned->second.branch != target->second.branch) continue;
            current += owned->second.level == target->second.level;
            previous += owned->second.level == target->second.level - 1;
        }
        if (current + 1 >= previous)
            return reject("BNW tenet level requires more lower-level tenets");
    }

    const auto owned_type = [&](std::string_view type) {
        const auto found = policy_types.find(type);
        if (found == policy_types.end())
            throw std::runtime_error("Policy prerequisite is absent from selected rules");
        return owned_policies->contains(found->second);
    };
    try {
        const auto limits = runtime::research_prerequisite_limits(defines);
        if (defines.profile() != content::GlobalDefineProfile::bnw ||
            limits.or_count < 0 || limits.and_count < 0)
            return reject("Selected BNW policy prerequisite limits are unavailable");
        const auto or_links = policy_links(database, "Policy_PrereqORPolicies",
            "PrereqPolicy", target->second.type);
        const auto or_end = or_links.begin() + std::min<std::size_t>(
            or_links.size(), static_cast<std::size_t>(limits.or_count));
        if (or_end != or_links.begin() &&
            std::none_of(or_links.begin(), or_end, owned_type))
            return reject("BNW OR policy prerequisite is unmet");
        const auto and_links = policy_links(database, "Policy_PrereqPolicies",
            "PrereqPolicy", target->second.type);
        const auto and_end = and_links.begin() + std::min<std::size_t>(
            and_links.size(), static_cast<std::size_t>(limits.and_count));
        if (!std::all_of(and_links.begin(), and_end, owned_type))
            return reject("BNW AND policy prerequisite is unmet");
        bool disabled = false;
        for (const auto owned_id : *owned_policies) {
            const auto owned = policies.find(owned_id);
            if (owned == policies.end())
                return reject("Live BNW policy ownership differs from selected rules");
            const auto disabled_types = policy_links(database, "Policy_Disables",
                "PolicyDisable", owned->second.type);
            const auto end = disabled_types.begin() + std::min<std::size_t>(
                disabled_types.size(), static_cast<std::size_t>(limits.and_count));
            disabled = disabled || std::find(disabled_types.begin(), end,
                                              target->second.type) != end;
        }
        if (disabled) return reject("An adopted policy disables this choice");
    } catch (const std::exception &error) {
        return reject(error.what());
    }
    if (!target->second.tech.empty()) {
        const auto tech = std::find_if(technologies.entries.begin(), technologies.entries.end(),
            [&](const auto &entry) { return entry.second.base.type == target->second.tech; });
        const auto team = std::find_if(live.technologies.begin(), live.technologies.end(),
            [&](const auto &value) { return value.team() == player->second.team; });
        if (tech == technologies.entries.end() || team == live.technologies.end() ||
            !team->has_tech(static_cast<std::int64_t>(tech->first)))
            return reject("BNW policy technology prerequisite is unmet or unresolved");
    }
    result.passes_known_rules = true;
    return result;
}

BnwLiveProgressionBackend::BnwLiveProgressionBackend(
    runtime::LiveSession &session, runtime::BnwResearchChoiceRules rules,
    runtime::BnwResearchChoiceServices services, PopupEventReader popup_event,
    PopupProcessed popup_processed, const content::RulesDatabase *policy_database)
    : session_(session), rules_(rules), services_(std::move(services)),
      popup_event_(std::move(popup_event)),
      popup_processed_(std::move(popup_processed)),
      policy_database_(policy_database) {}

UiCommandResult BnwLiveProgressionBackend::availability(
    const UiGameplayCommand &command, const UiFrameSnapshot &frame) const {
    if (const auto *policy = command.as<ProgressionChoosePolicy>()) {
        if (command.source_id() != "SocialPolicyPopup" || policy->policy < 0)
            return {UiCommandStatus::invalid, "Invalid BNW policy choice"};
        const auto *progression = progression_for_active_player(frame);
        if (!progression)
            return {UiCommandStatus::unavailable, "Active player's policy frame is unavailable"};
        const auto &live = session_.state();
        const auto player = live.setup.players.find(progression->player_id);
        if (!frame.turn() || *frame.turn() != live.setup.game_turn ||
            live.setup.profile != content::TechnologyProfile::bnw ||
            player == live.setup.players.end() || !player->second.alive ||
            player->second.team != progression->team_id)
            return {UiCommandStatus::unavailable, "UI policy frame is stale or player changed"};
        if (!progression->unavailable_reason.empty() ||
            !progression->policy_ownership_resolved || !player->second.policies_resolved)
            return {UiCommandStatus::unavailable, "Live BNW policy rules or ownership are unresolved"};
        const auto item = std::find_if(progression->policies.begin(),
            progression->policies.end(), [&](const ProgressionPolicy &value) {
                return value.id == policy->policy;
            });
        if (item == progression->policies.end())
            return {UiCommandStatus::invalid, "Policy is absent from active UI rules"};
        const bool owned = player->second.policies.contains(policy->policy);
        if (item->adopted != owned)
            return {UiCommandStatus::unavailable, "UI policy ownership is stale"};
        if (owned)
            return {UiCommandStatus::unavailable, "Policy is already adopted"};
        if (item->branch_type.empty())
            return {UiCommandStatus::unavailable, "Branch free policy cannot be chosen directly"};
        if (!policy_database_)
            return {UiCommandStatus::unavailable,
                    "Selected BNW policy rules are not bound to this live backend"};
        BnwPolicyChoicePreview preview;
        try {
            preview = preview_bnw_policy_choice(session_, *policy_database_,
                rules_.technologies, rules_.defines, progression->player_id, policy->policy);
        } catch (const std::exception &error) {
            return {UiCommandStatus::unavailable, error.what()};
        }
        if (!preview.passes_known_rules)
            return {UiCommandStatus::unavailable, preview.blocker};
        if (item->type != preview.policy_type ||
            item->branch_type != preview.branch_type)
            return {UiCommandStatus::unavailable, "UI policy rules have changed"};
        // BNW CvPlayer::doAdoptPolicy invokes CanAdoptPolicy (including Lua),
        // pays culture/free-policy or tenet cost, SetPolicy/processPolicies,
        // branch/finisher effects, PlayerAdoptPolicy Lua and updateYield.
        // No transactional owner for all those effects exists in LiveSession.
        return {UiCommandStatus::unavailable,
                "BNW CanAdoptPolicy Lua and transactional DoAdoptPolicy effects are unavailable"};
    }
    if (const auto *ideology = command.as<ProgressionChooseIdeology>()) {
        if (command.source_id() != "ChooseIdeology" || ideology->branch < 0)
            return {UiCommandStatus::invalid, "Invalid BNW ideology choice"};
        const auto *progression = progression_for_active_player(frame);
        if (!progression)
            return {UiCommandStatus::unavailable, "Active player's ideology frame is unavailable"};
        const auto *event = frame.find<ProgressionIdeologyPopupEvent>();
        if (!event || event->player_id != progression->player_id || event->token == 0)
            return {UiCommandStatus::unavailable, "Active BNW ideology choice event is unavailable"};
        const auto &live = session_.state();
        const auto player = live.setup.players.find(progression->player_id);
        if (!frame.turn() || *frame.turn() != live.setup.game_turn ||
            live.setup.profile != content::TechnologyProfile::bnw ||
            player == live.setup.players.end() || !player->second.alive ||
            player->second.team != progression->team_id)
            return {UiCommandStatus::unavailable, "UI ideology frame is stale or player changed"};
        const auto branch = std::find_if(progression->branches.begin(),
            progression->branches.end(), [&](const ProgressionBranch &value) {
                return value.id == ideology->branch && value.ideology;
            });
        if (branch == progression->branches.end())
            return {UiCommandStatus::invalid, "Ideology is absent from active UI rules"};
        // SetPolicyBranchUnlocked grants free tenets and sends met-player
        // notifications; the full choice/revolution owner is not bound.
        return {UiCommandStatus::unavailable,
                "BNW ideology branch effects and notifications are unavailable"};
    }
    const auto *choice = command.as<ProgressionChooseTechnology>();
    if (!choice || (command.source_id() != "TechTree" && command.source_id() != "TechPopup") ||
        choice->technology < 0 || (command.source_id() == "TechPopup" && choice->clear_queue))
        return {UiCommandStatus::invalid, "Invalid technology choice"};
    const auto *progression = progression_for_active_player(frame);
    if (!progression)
        return {UiCommandStatus::unavailable, "Active player's progression state is unavailable"};
    const auto &live = session_.state();
    const auto player = live.setup.players.find(progression->player_id);
    if (!frame.turn() || *frame.turn() != live.setup.game_turn ||
        player == live.setup.players.end() || !player->second.alive ||
        player->second.team != progression->team_id ||
        rules_.active_team != progression->team_id)
        return {UiCommandStatus::unavailable, "UI research frame is stale or player changed"};
    if (rules_.technologies.profile != content::TechnologyProfile::bnw ||
        rules_.defines.profile() != content::GlobalDefineProfile::bnw ||
        !rules_.technologies.all_succeeded() ||
        live.setup.profile != content::TechnologyProfile::bnw ||
        live.research_selection.profile != content::TechnologyProfile::bnw)
        return {UiCommandStatus::unavailable, "Active BNW research rules are incomplete"};
    if (!services_.lua_ready || !services_.lua_test_all_read_only ||
        !services_.dirty_after_commit)
        return {UiCommandStatus::unavailable, "Live BNW research Lua services are unavailable"};
    try {
        if (!services_.lua_ready())
            return {UiCommandStatus::unavailable, "Live BNW research Lua services are unavailable"};
    } catch (const std::exception &error) {
        return {UiCommandStatus::unavailable, error.what()};
    }
    const auto research = live.player_research.players.find(progression->player_id);
    const auto turn = live.research_turn.players.find(progression->player_id);
    const auto selection = live.research_selection.players.find(progression->player_id);
    const auto team = std::find_if(live.technologies.begin(), live.technologies.end(),
        [&](const auto &value) { return value.team() == progression->team_id; });
    if (research == live.player_research.players.end() ||
        turn == live.research_turn.players.end() ||
        selection == live.research_selection.players.end() || team == live.technologies.end())
        return {UiCommandStatus::unavailable, "Live BNW research player is incomplete"};
    if (!turn->second.founded_first_city && selection->second.advanced_start_points < 0)
        return {UiCommandStatus::unavailable, "Research needs a founded city"};
    const auto item = std::find_if(progression->technologies.begin(),
        progression->technologies.end(), [&](const auto &value) {
            return value.id == choice->technology;
        });
    const auto *technology = rules_.technologies.find(
        static_cast<std::uint64_t>(choice->technology));
    if (item == progression->technologies.end() || !technology)
        return {UiCommandStatus::invalid, "Technology is absent from active rules"};
    if (item->type != technology->base.type)
        return {UiCommandStatus::unavailable, "UI technology rules have changed"};
    const bool queued = std::find(research->second.queue.begin(), research->second.queue.end(),
                                  choice->technology) != research->second.queue.end();
    if (item->researched != team->has_tech(choice->technology) ||
        item->queued != queued ||
        progression->current_research !=
            (research->second.current_research() >= 0
                ? std::optional<std::int64_t>(research->second.current_research())
                : std::nullopt))
        return {UiCommandStatus::unavailable, "UI research frame is stale"};
    if (item->researched || queued || turn->second.researching.contains(choice->technology))
        return {UiCommandStatus::unavailable, "Technology is already researched or queued"};
    if (command.source_id() == "TechPopup") {
        const auto *popup = frame.find<ProgressionTechPopupState>();
        if (!popup || !popup_event_ || !popup_processed_ || popup->token == 0 ||
            !popup->unavailable_reason.empty() ||
            popup->player_id != progression->player_id)
            return {UiCommandStatus::unavailable, "Active BNW technology choice event is unavailable"};
        std::optional<ProgressionTechPopupEvent> event;
        try { event = popup_event_(session_); }
        catch (const std::exception &error) {
            return {UiCommandStatus::unavailable, error.what()};
        }
        if (!event || event->player_id != popup->player_id ||
            event->mode != popup->mode ||
            event->steal_target_player != popup->steal_target_player ||
            event->token != popup->token)
            return {UiCommandStatus::unavailable, "BNW technology choice event has changed"};
        if (popup->mode != ProgressionTechPopupMode::research)
            return {UiCommandStatus::unavailable,
                    "Free and stolen technology grants need their BNW gameplay backend"};
        if (std::find(popup->eligible_technologies.begin(),
                      popup->eligible_technologies.end(), choice->technology) ==
            popup->eligible_technologies.end())
            return {UiCommandStatus::unavailable, "Technology is not an eligible popup choice"};
    }
    // The authoritative PlayerCanEverResearch Lua check and queue mutation
    // happen together in push_bnw_research_choice, never in a hover query.
    return {UiCommandStatus::ready, {}};
}

UiCommandResult BnwLiveProgressionBackend::execute(
    const UiGameplayCommand &command, const UiFrameSnapshot &frame) {
    const auto checked = availability(command, frame);
    if (checked.status != UiCommandStatus::ready) return checked;
    const auto *choice = command.as<ProgressionChooseTechnology>();
    try {
        const auto result = runtime::push_bnw_research_choice(session_,
            *frame.active_player_id(), choice->technology, choice->clear_queue,
            rules_, services_);
        if (result.outcome == runtime::LiveActionOutcome::committed) {
            if (command.source_id() == "TechPopup") {
                const auto *popup = frame.find<ProgressionTechPopupState>();
                const ProgressionTechPopupEvent event{popup->player_id, popup->mode,
                    popup->steal_target_player, popup->token};
                try { popup_processed_(event); }
                catch (const std::exception &error) {
                    return {UiCommandStatus::unavailable,
                        std::string("Research queue committed; popup acknowledgement is uncertain: ") +
                        error.what() + "; do not retry"};
                } catch (...) {
                    return {UiCommandStatus::unavailable,
                        "Research queue committed; popup acknowledgement is uncertain; do not retry"};
                }
                try {
                    const auto active = popup_event_(session_);
                    if (active && active->token == event.token)
                        return {UiCommandStatus::unavailable,
                            "Research queue committed; popup acknowledgement was not observed; do not retry"};
                } catch (const std::exception &error) {
                    return {UiCommandStatus::unavailable,
                        std::string("Research queue committed; popup acknowledgement could not be checked: ") +
                        error.what() + "; do not retry"};
                } catch (...) {
                    return {UiCommandStatus::unavailable,
                        "Research queue committed; popup acknowledgement could not be checked; do not retry"};
                }
            }
            return {UiCommandStatus::performed, result.reason};
        }
        return {UiCommandStatus::unavailable, result.reason};
    } catch (const std::exception &error) {
        return {UiCommandStatus::unavailable, error.what()};
    }
}

} // namespace civ5::ui
