#include "civ5/ui/progression_screens.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5::ui;
        require(std::get<UiOpenScreen>(progression_top_panel_command(true)).id == "TechTree",
                "science opens the technology tree");
        require(std::get<UiOpenScreen>(progression_top_panel_command(false)).id == "SocialPolicyPopup",
                "culture opens social policies");
        require(std::get<UiOpenScreen>(progression_open_command(ProgressionScreen::ideology)).id ==
                    "ChooseIdeology", "ideology context");
        require(std::get<UiOpenScreen>(progression_open_command(ProgressionScreen::technology_popup)).id ==
                    "TechPopup", "technology chooser context");
        const auto popup_to_tree = progression_popup_open_tree_commands();
        require(popup_to_tree.size() == 2 &&
                std::get<UiCloseScreen>(popup_to_tree[0]).id == "TechPopup" &&
                std::get<UiOpenScreen>(popup_to_tree[1]).id == "TechTree",
                "technology chooser closes before tree opens");
        require(progression_top_panel_command(TopPanelAction::open_technology_tree).has_value() &&
                progression_top_panel_command(TopPanelAction::open_policy_choice).has_value() &&
                !progression_top_panel_command(TopPanelAction::open_game_menu).has_value(),
                "existing TopPanel action bridge");
        ProgressionFrameState state;
        state.player_id = 7;
        state.technologies.push_back({3, "TECH_POTTERY", "TXT_KEY_TECH_POTTERY", std::nullopt,
                                      0, 0, 0, 35, false, false, 0});
        const auto frame = UiFrameSnapshot(12, 7, 1)
            .with<ProgressionFrameState>(std::make_shared<const ProgressionFrameState>(state));
        require(progression_for_active_player(frame) != nullptr, "active live player payload");
        require(progression_for_active_player(UiFrameSnapshot(13, 8, 1)
            .with<ProgressionFrameState>(std::make_shared<const ProgressionFrameState>(state))) == nullptr,
            "stale player payload rejected");
        const auto tech = std::get<UiGameplayCommand>(progression_choose_technology(3));
        require(tech.source_id() == "TechTree" && tech.as<ProgressionChooseTechnology>() &&
                tech.as<ProgressionChooseTechnology>()->technology == 3,
                "typed technology command");
        ProgressionGameplayBackend backend;
        require(backend.availability(tech, frame).status == UiCommandStatus::unavailable,
                "research requires connected game mechanics");
        const auto popup_command = std::get<UiGameplayCommand>(progression_choose_popup_technology(3));
        require(popup_command.source_id() == "TechPopup" &&
                popup_command.as<ProgressionChooseTechnology>() &&
                !popup_command.as<ProgressionChooseTechnology>()->clear_queue,
                "technology chooser has its own typed source and queue behavior");
        require(backend.availability(popup_command, frame).status == UiCommandStatus::unavailable,
                "technology chooser requires an active candidate event");
        require(backend.execute(tech, frame).status == UiCommandStatus::unavailable,
                "missing research backend never reports success");
        require(backend.availability(std::get<UiGameplayCommand>(progression_choose_policy(4)), frame)
                    .status == UiCommandStatus::unavailable,
                "policy adoption is explicitly unavailable");
        require(backend.availability(std::get<UiGameplayCommand>(progression_choose_ideology(8)), frame)
                    .status == UiCommandStatus::unavailable,
                "ideology adoption is explicitly unavailable");
        civ5::content::RulesDatabase rules;
        rules.apply_sql("CREATE TABLE PolicyBranchTypes (ID INTEGER, Type TEXT, Description TEXT);"
                        "CREATE TABLE Policies (ID INTEGER, Type TEXT, Description TEXT, PolicyBranchType TEXT, GridX INTEGER, GridY INTEGER);"
                        "INSERT INTO PolicyBranchTypes VALUES (0, 'POLICY_BRANCH_FREEDOM', 'TXT_KEY_FREEDOM');"
                        "INSERT INTO PolicyBranchTypes VALUES (1, 'POLICY_BRANCH_TRADITION', 'TXT_KEY_TRADITION');"
                        "INSERT INTO Policies VALUES (0, 'POLICY_TEST', 'TXT_KEY_TEST', 'POLICY_BRANCH_TRADITION', 1, 1);");
        civ5::runtime::ResearchContext world;
        auto &player = world.players[7];
        player.alive = true;
        player.team = 2;
        player.policies_resolved = true;
        player.policies.insert(0);
        civ5::runtime::PlayerResearchContext players;
        players.players[7].queue.push_back(3);
        civ5::content::TechnologyRules technologies;
        technologies.collection.slots = 4;
        auto &pottery = technologies.entries[3];
        pottery.base.type = "TECH_POTTERY";
        pottery.base.description.key = "TXT_KEY_TECH_POTTERY";
        pottery.integers["Cost"] = 35;
        pottery.integers["GridX"] = 1;
        pottery.integers["GridY"] = 5;
        const civ5::runtime::TeamTechnologies team(civ5::content::TechnologyProfile::bnw, 2, 4);
        const auto captured = capture_progression_frame(world, players,
            std::span<const civ5::runtime::TeamTechnologies>(&team, 1), technologies, rules, 7);
        require(captured.unavailable_reason.empty() && captured.current_research == 3 &&
                captured.technologies.size() == 1 && captured.technologies[0].queued &&
                captured.technologies[0].cost == 35, "live research and active technology rules");
        require(captured.policies.size() == 1 && captured.policies[0].adopted &&
                captured.branches.size() == 2 && captured.branches[0].ideology,
                "saved policy ownership and active ideology rules");
        const auto live_frame = UiFrameSnapshot(14, 7, 1)
            .with<ProgressionFrameState>(std::make_shared<const ProgressionFrameState>(captured));
        const auto ideology_frame = live_frame.with<ProgressionIdeologyPopupEvent>(
            std::make_shared<const ProgressionIdeologyPopupEvent>(
                ProgressionIdeologyPopupEvent{7, 19}));
        ProgressionTechPopupState popup;
        popup.player_id = 7;
        popup.token = 7;
        popup.eligible_technologies = {3};
        const auto popup_frame = live_frame.with<ProgressionTechPopupState>(
            std::make_shared<const ProgressionTechPopupState>(popup));
        require(backend.availability(popup_command, popup_frame).status == UiCommandStatus::unavailable,
                "technology chooser never reports success without research backend");
        popup.mode = ProgressionTechPopupMode::steal_technology;
        const auto steal_frame = live_frame.with<ProgressionTechPopupState>(
            std::make_shared<const ProgressionTechPopupState>(popup));
        require(backend.availability(popup_command, steal_frame).status == UiCommandStatus::unavailable,
                "technology theft is unavailable without grant backend");
        if (argc == 2) {
            const std::filesystem::path root(argv[1]);
            const char *paths[] = {
                "Assets/DLC/Expansion2/UI/InGame/TechTree/TechTree.xml",
                "Assets/DLC/Expansion2/UI/InGame/Popups/SocialPolicyPopup.xml",
                "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseIdeologyPopup.xml",
                "Assets/DLC/Expansion2/UI/InGame/TechPopup.xml"};
            for (int index = 0; index < 4; ++index) {
                auto source = std::make_shared<civ5::assets::LooseResourceIndex>(root);
                source->add_file(root / paths[index]);
                civ5::assets::ResourceMountTable resources(civ5::assets::resource_lookup(source));
                const auto selected = read_progression_screen_source(
                    resources, static_cast<ProgressionScreen>(index));
                require(!selected.control_ids.empty() &&
                        selected.selected_source_path.find("Expansion2") != std::string::npos,
                        "selected BNW progression XML controls and provenance");
                const auto plan = plan_progression_screen(selected,
                    static_cast<ProgressionScreen>(index),
                    index == 3 ? popup_frame : index == 2 ? ideology_frame : live_frame);
                require(!plan.items.empty(), "progression screen has projected live items");
                if (index == 0)
                    require(plan.items.front().rect && plan.items.front().rect->x == 430 &&
                            plan.items.front().rect->y == 32 && plan.items.front().rect->width == 270,
                            "technology placement follows active BNW Lua and XML");
                if (index == 1)
                    require(plan.items.front().rect && plan.items.front().rect->x == 16 &&
                            plan.items.front().rect->y == 12 && plan.items.front().rect->width == 48,
                            "policy placement follows active BNW Lua and XML");
                if (index == 2)
                    require(!plan.items.front().rect && !plan.unresolved.empty() &&
                            plan_progression_screen(selected, ProgressionScreen::ideology,
                                                    live_frame).items.empty(),
                            "ideology rows require real popup event; dynamic height remains explicit");
                if (index == 3)
                    require(plan.items.front().rect &&
                            plan.items.front().parent_control_id == "ButtonStack" &&
                            plan.items.front().rect->width == 270 &&
                            plan.items.front().rect->height == 60,
                            "technology chooser uses active BNW instance and stack dimensions");
            }
        }
        std::cout << "ui progression tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
