#include "civ5/ui/system_screens.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/glyphgen.hpp"
#include "civ5/ui/top_panel.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace {
using namespace civ5;
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
std::set<std::uint32_t> code_points(std::string_view text) {
    std::set<std::uint32_t> result;
    for (std::size_t at = 0; at < text.size();) {
        const auto first = static_cast<unsigned char>(text[at++]);
        std::uint32_t value = first;
        int remaining = 0;
        if (first >= 0xf0) { value &= 0x07; remaining = 3; }
        else if (first >= 0xe0) { value &= 0x0f; remaining = 2; }
        else if (first >= 0xc0) { value &= 0x1f; remaining = 1; }
        while (remaining--) {
            require(at < text.size() &&
                    (static_cast<unsigned char>(text[at]) & 0xc0) == 0x80,
                    "Civilopedia localized text contains malformed UTF-8");
            value = (value << 6) | (static_cast<unsigned char>(text[at++]) & 0x3f);
        }
        if (value >= 32) result.insert(value);
    }
    return result;
}
}

int main(int argc, char **argv) {
    try {
        require(argc == 2, "Pass civ5-assets root");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->scan({"Assets/UI"});
        expansion->scan({"Assets/DLC/Expansion/UI"});
        bnw->scan({"Assets/DLC/Expansion2/UI"});
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion)});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw)});
        const auto registry = ui::UiContextRegistry::from_resources(resources);
        const auto sources = ui::read_system_sources(resources, registry);
        require(sources.size() == 12, "All twelve active BNW system contexts");
        const auto source = [&](std::string_view id) -> const ui::SystemScreenSource & {
            const auto found = std::find_if(sources.begin(), sources.end(),
                [id](const auto &entry) { return entry.context_id == id; });
            if (found == sources.end()) throw std::runtime_error("Missing system source");
            return *found;
        };
        require(source("GameMenu").has("ReturnButton") &&
                source("GameMenu").selected_source_path.find("Expansion2") != std::string::npos,
                "Selected GameMenu uses BNW override");
        require(source("GameMenu").has("DetailsButton"),
                "Selected GameMenu authors DetailsButton");
        require(source("GameMenu").has("ModsButton"),
                "Selected BNW GameMenu authors ModsButton");
        std::ifstream game_menu_lua(root / "Assets/UI/InGame/Menus/GameMenu.lua",
                                    std::ios::binary);
        require(game_menu_lua.good(), "Active GameMenu Lua is installed");
        const std::string game_menu_script(std::istreambuf_iterator<char>{game_menu_lua}, {});
        require(game_menu_script.find("function OnGameDetails()") != std::string::npos &&
                game_menu_script.find("Controls.DetailsPanel:SetHide(") !=
                    std::string::npos &&
                game_menu_script.find("PreGame.IsVictory(row.ID)") !=
                    std::string::npos,
                "Active GameMenu Lua toggles a details pane from real game settings");
        require(game_menu_script.find("function OnGameMods()") != std::string::npos &&
                game_menu_script.find("Modding.GetActivatedMods()") !=
                    std::string::npos &&
                game_menu_script.find("Controls.ModsButton:SetHide(#activatedMods == 0)") !=
                    std::string::npos,
                "Active GameMenu Lua hides ModsButton for an empty mod list");
        require(source("CivilopediaScreen").has("CloseButton") &&
                source("CivilopediaScreen").selected_source_path.find("Expansion2") != std::string::npos,
                "Selected Civilopedia uses BNW override");
        require(source("CivilopediaScreen").has("SearchEditBox"),
                "Selected BNW Civilopedia exposes its authored edit box");
        for (int tab = 1; tab <= 16; ++tab)
            require(source("CivilopediaScreen").has(
                "CategoryButton" + std::to_string(tab)),
                "Selected BNW Civilopedia exposes sixteen authored tabs");
        require(source("CivilopediaScreen").has("ScrollPanel") &&
                source("CivilopediaScreen").has("UpButton") &&
                source("CivilopediaScreen").has("DownButton"),
                "Selected BNW Civilopedia exposes authored article scroll controls");
        require(source("CivilopediaScreen").has("SearchButton") &&
                std::any_of(source("CivilopediaScreen").controls.begin(),
                    source("CivilopediaScreen").controls.end(),
                    [](const auto &control) {
                        return control.id == "SearchButton" &&
                               control.texture == "Civilopedia_SearchIcon.dds";
                    }), "Selected BNW SearchButton retains its authored icon");
        require(source("EndGameMenu").has("ReplayButton"), "Selected EndGameMenu controls");
        require(source("PlayerChange").has("SaveButton") &&
                source("PlayerChange").selected_source_path.find(
                    "Assets/UI/InGame/PlayerChange.xml") != std::string::npos,
                "Selected PlayerChange authors save control");
        require(registry.find("PlayerChange") && registry.find("PlayerChange")->modal &&
                registry.find("SaveMenu") && registry.find("SaveMenu")->modal,
                "PlayerChange and its SaveMenu own modal focus");
        std::ifstream player_change_lua(root / "Assets/UI/InGame/PlayerChange.lua",
                                        std::ios::binary);
        require(player_change_lua.good(), "Selected PlayerChange Lua is installed");
        const std::string player_change_script(
            std::istreambuf_iterator<char>{player_change_lua}, {});
        require(player_change_script.find(
                    "UIManager:PushModal( Controls.SaveMenu, false )") !=
                    std::string::npos,
                "Installed PlayerChange Lua opens nested SaveMenu modally");
        require(source("EndGameMenu").has("BeyondButton") &&
                source("EndGameMenu").selected_source_path.find("Expansion2") !=
                    std::string::npos,
                "Selected BNW EndGameMenu authors Beyond Earth control");
        std::ifstream end_game_lua(root / "Assets/UI/InGame/Popups/EndGameMenu.lua",
                                   std::ios::binary);
        require(end_game_lua.good(), "Selected base EndGameMenu Lua is installed");
        const std::string end_game_script(std::istreambuf_iterator<char>{end_game_lua}, {});
        require(end_game_script.find("team == Game.GetActiveTeam()") != std::string::npos &&
                end_game_script.find("type == EndGameTypes.Technology") !=
                    std::string::npos &&
                end_game_script.find("Steam.ActivateGameOverlayToStore(65980)") !=
                    std::string::npos,
                "Installed EndGameMenu Lua gates the authored store action by Technology victory");
        const ui::UiFrameSnapshot frame(1, 0, 0);
        ui::UiScreenController controller(registry);
        require(!controller.visible("GameMenu") && !controller.visible("ExitConfirm"),
                "Unopened system screens are hidden");
        require(controller.dispatch(ui::UiOpenScreen{"GameMenu"}, frame).status ==
                ui::UiCommandStatus::navigation, "Open GameMenu");
        const auto save_route = ui::system_control_command("GameMenu", "SaveGameButton");
        require(save_route && controller.dispatch(*save_route, frame).status ==
                ui::UiCommandStatus::navigation && controller.visible("SaveMenu"),
                "Save screen transition");
        require(controller.dispatch(ui::UiCloseScreen{"SaveMenu"}, frame).status ==
                ui::UiCommandStatus::navigation && !controller.visible("SaveMenu"),
                "Close focused SaveMenu");
        ui::UiScreenController player_change_controller(registry);
        require(player_change_controller.dispatch(ui::UiOpenScreen{"PlayerChange"}, frame)
                    .status == ui::UiCommandStatus::navigation,
                "Open PlayerChange");
        const auto player_save_route =
            ui::system_control_command("PlayerChange", "SaveButton");
        require(player_save_route &&
                std::get_if<ui::UiOpenScreen>(&*player_save_route) &&
                std::get<ui::UiOpenScreen>(*player_save_route).id == "SaveMenu" &&
                player_change_controller.dispatch(*player_save_route, frame).status ==
                    ui::UiCommandStatus::navigation &&
                player_change_controller.focused_screen() ==
                    std::optional<std::string>{"SaveMenu"},
                "PlayerChange SaveButton opens the selected SaveMenu");
        const auto blocked_background = ui::system_control_command(
            "GameMenu", "QuickSaveButton");
        ui::SystemGameplayBackend player_backend;
        require(blocked_background &&
                player_change_controller.dispatch(*blocked_background, frame,
                                                  &player_backend).status ==
                    ui::UiCommandStatus::invalid,
                "Focused SaveMenu blocks gameplay commands from GameMenu");
        const auto player_plan = ui::plan_system_screen(
            source("PlayerChange"), frame, 1920, 1080);
        require(std::any_of(player_plan.hits.begin(), player_plan.hits.end(),
                    [](const auto &hit) {
                        const auto *open = std::get_if<ui::UiOpenScreen>(&hit.command);
                        return hit.enabled && open && open->id == "SaveMenu";
                    }), "PlayerChange save control has a live navigation hit");
        require(player_change_controller.dispatch(ui::UiCloseScreen{"SaveMenu"}, frame)
                    .status == ui::UiCommandStatus::navigation &&
                player_change_controller.focused_screen() ==
                    std::optional<std::string>{"PlayerChange"},
                "Closing SaveMenu restores PlayerChange focus");
        const auto command = ui::system_control_command("GameMenu", "QuickSaveButton");
        require(command && std::holds_alternative<ui::UiGameplayCommand>(*command),
                "Quick save is typed gameplay operation");
        ui::SystemGameplayBackend backend;
        require(controller.dispatch(*command, frame, &backend).status ==
                ui::UiCommandStatus::unavailable,
                "No false save success without a backend");
        const auto exit_route = ui::system_control_command("GameMenu", "ExitGameButton");
        require(exit_route && controller.dispatch(*exit_route, frame).status ==
                ui::UiCommandStatus::navigation, "Open exit confirmation");
        const auto yes = ui::system_control_command("ExitConfirm", "Yes");
        require(yes && controller.dispatch(*yes, frame, &backend).status ==
                ui::UiCommandStatus::unavailable,
                "No false exit success without pending intent");
        const auto no = ui::system_control_command("ExitConfirm", "No");
        require(no && controller.dispatch(*no, frame).status ==
                ui::UiCommandStatus::navigation, "Cancel exit confirmation");
        ui::SystemMenuSession session;
        require(session.activate("GameMenu", "ExitGameButton", frame, controller, &backend)
                    .status == ui::UiCommandStatus::navigation &&
                session.pending_exit() == ui::SystemOperationKind::exit_game,
                "Exit intent survives shared confirmation window");
        require(session.activate("ExitConfirm", "Yes", frame, controller, &backend)
                    .status == ui::UiCommandStatus::unavailable &&
                session.pending_exit() == ui::SystemOperationKind::exit_game &&
                controller.visible("ExitConfirm"),
                "Denied exit remains pending and visible");
        require(session.activate("ExitConfirm", "No", frame, controller, &backend)
                    .status == ui::UiCommandStatus::navigation &&
                !session.pending_exit() && !controller.visible("ExitConfirm"),
                "Cancel clears exit intent");
        const auto plan = ui::plan_system_screen(source("GameMenu"), frame, 1920, 1080);
        require(!plan.paints.empty() && !plan.hits.empty(), "2D GameMenu shell and hits");
        const auto game_details = ui::system_control_command("GameMenu", "DetailsButton");
        const auto *game_details_action = game_details ?
            std::get_if<ui::UiGameplayCommand>(&*game_details) : nullptr;
        const auto *game_details_op = game_details_action ?
            game_details_action->as<ui::SystemOperation>() : nullptr;
        require(game_details_op &&
                game_details_op->kind == ui::SystemOperationKind::view_game_details &&
                backend.availability(*game_details_action, frame).status ==
                    ui::UiCommandStatus::unavailable &&
                controller.dispatch(*game_details, frame, &backend).status ==
                    ui::UiCommandStatus::unavailable,
                "Game details stays unavailable without live setup and victory settings");
        const auto details_plan = ui::plan_system_screen(
            source("GameMenu"), frame, 1920, 1080, {}, &backend);
        require(std::any_of(details_plan.hits.begin(), details_plan.hits.end(),
                    [](const auto &hit) {
                        const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *op = gameplay ?
                            gameplay->template as<ui::SystemOperation>() : nullptr;
                        return op && op->kind ==
                            ui::SystemOperationKind::view_game_details && !hit.enabled;
                    }), "DetailsButton is painted disabled without live details");
        const auto mods_route = ui::system_control_command("GameMenu", "ModsButton");
        const auto *mods_action = mods_route ?
            std::get_if<ui::UiGameplayCommand>(&*mods_route) : nullptr;
        const auto *mods_op = mods_action ? mods_action->as<ui::SystemOperation>() : nullptr;
        require(mods_op && mods_op->kind == ui::SystemOperationKind::view_active_mods &&
                backend.availability(*mods_action, frame).status ==
                    ui::UiCommandStatus::unavailable,
                "Unowned active mods panel has a typed unavailable route");
        const auto has_mods_hit = [](const auto &plan, bool enabled) {
            return std::any_of(plan.hits.begin(), plan.hits.end(),
                [enabled](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *op = gameplay ?
                        gameplay->template as<ui::SystemOperation>() : nullptr;
                    return op && op->kind == ui::SystemOperationKind::view_active_mods &&
                        hit.enabled == enabled;
                });
        };
        require(!has_mods_hit(details_plan, false) &&
                !has_mods_hit(details_plan, true),
                "Missing mod list keeps the authored tab hidden");
        ui::SystemScreenFrame one_mod;
        one_mod.game_menu_active_mod_count = 1;
        const auto one_mod_frame = frame.with<ui::SystemScreenFrame>(
            std::make_shared<const ui::SystemScreenFrame>(one_mod));
        const auto one_mod_plan = ui::plan_system_screen(
            source("GameMenu"), one_mod_frame, 1920, 1080, {}, &backend);
        require(has_mods_hit(one_mod_plan, false) &&
                controller.dispatch(*mods_route, one_mod_frame, &backend).status ==
                    ui::UiCommandStatus::unavailable,
                "Nonempty mod count cannot claim an unowned panel opened");
        const auto has_quick_save_hit = [](const auto &screen) {
            return std::any_of(screen.hits.begin(), screen.hits.end(),
                [](const auto &hit) {
                    const auto *action = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *op = action ? action->template as<ui::SystemOperation>() : nullptr;
                    return op && op->kind == ui::SystemOperationKind::quick_save;
                });
        };
        require(!has_quick_save_hit(plan),
                "Missing GameMenu owner keeps Quick Save hidden");
        ui::SystemScreenFrame quick_save_visible;
        quick_save_visible.game_menu_quick_save_visible = true;
        const auto quick_save_frame = frame.with<ui::SystemScreenFrame>(
            std::make_shared<const ui::SystemScreenFrame>(quick_save_visible));
        const auto quick_save_plan = ui::plan_system_screen(
            source("GameMenu"), quick_save_frame, 1920, 1080, {}, &backend);
        const auto quick_save_route = ui::system_control_command(
            "GameMenu", "QuickSaveButton");
        require(has_quick_save_hit(quick_save_plan) &&
                backend.availability(std::get<ui::UiGameplayCommand>(
                    *quick_save_route), quick_save_frame).status ==
                    ui::UiCommandStatus::unavailable,
                "Owned Quick Save is visible but has no mutation backend");
        const auto has_load_game_hit = [](const auto &screen) {
            return std::any_of(screen.hits.begin(), screen.hits.end(),
                [](const auto &hit) {
                    const auto *action = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *op = action ? action->template as<ui::SystemOperation>() : nullptr;
                    return op && op->kind == ui::SystemOperationKind::open_load_menu;
                });
        };
        require(!has_load_game_hit(plan) &&
                session.activate("GameMenu", "LoadGameButton", frame,
                    controller, &backend).status == ui::UiCommandStatus::unavailable,
                "Unowned BNW LoadGameButton stays hidden and cannot navigate");
        ui::SystemScreenFrame load_game_visible;
        load_game_visible.game_menu_load_game_visible = true;
        const auto load_game_frame = frame.with<ui::SystemScreenFrame>(
            std::make_shared<const ui::SystemScreenFrame>(load_game_visible));
        const auto load_game_plan = ui::plan_system_screen(
            source("GameMenu"), load_game_frame, 1920, 1080, {}, &backend);
        require(has_load_game_hit(load_game_plan) &&
                session.activate("GameMenu", "LoadGameButton", load_game_frame,
                    controller, &backend).status == ui::UiCommandStatus::unavailable &&
                !controller.visible("LoadMenu"),
                "Owned LoadGameButton needs the native QueuePopup transition");
        const auto beyond_route = ui::system_control_command("EndGameMenu", "BeyondButton");
        require(beyond_route && std::holds_alternative<ui::UiGameplayCommand>(*beyond_route) &&
                std::get<ui::UiGameplayCommand>(*beyond_route).as<ui::SystemOperation>() &&
                std::get<ui::UiGameplayCommand>(*beyond_route)
                    .as<ui::SystemOperation>()->kind ==
                    ui::SystemOperationKind::open_beyond_earth_store,
                "Beyond Earth store route is typed");
        const auto hidden_beyond = ui::plan_system_screen(
            source("EndGameMenu"), frame, 1920, 1080, {}, &backend);
        require(std::none_of(hidden_beyond.hits.begin(), hidden_beyond.hits.end(),
                    [](const auto &hit) {
                        const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *op = gameplay ?
                            gameplay->template as<ui::SystemOperation>() : nullptr;
                        return op && op->kind ==
                            ui::SystemOperationKind::open_beyond_earth_store;
                    }), "Beyond Earth stays hidden without an owned Technology victory");
        require(backend.availability(std::get<ui::UiGameplayCommand>(*beyond_route), frame)
                    .status == ui::UiCommandStatus::unavailable,
                "Missing victory state cannot enable the store route");
        ui::SystemScreenFrame technology_victory;
        technology_victory.end_game_technology_victory = true;
        const auto victory_frame = frame.with<ui::SystemScreenFrame>(
            std::make_shared<const ui::SystemScreenFrame>(technology_victory));
        const auto visible_beyond = ui::plan_system_screen(
            source("EndGameMenu"), victory_frame, 1920, 1080, {}, &backend);
        require(std::any_of(visible_beyond.hits.begin(), visible_beyond.hits.end(),
                    [](const auto &hit) {
                        const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *op = gameplay ?
                            gameplay->template as<ui::SystemOperation>() : nullptr;
                        return op && op->kind ==
                            ui::SystemOperationKind::open_beyond_earth_store &&
                            !hit.enabled;
                    }), "Technology victory shows disabled store control without Steam backend");
        require(controller.dispatch(ui::UiOpenScreen{"EndGameMenu"}, victory_frame).status ==
                ui::UiCommandStatus::navigation &&
                controller.dispatch(*beyond_route, victory_frame, &backend).status ==
                ui::UiCommandStatus::unavailable,
                "Store route never reports success without Steam backend");
        const auto loose = std::make_shared<assets::LooseSource>(root);
        assets::ResourceMountTable pedia_resources(assets::resource_lookup(loose),
            assets::resource_lookup_all(loose), assets::resource_keys(loose));
        const auto ru = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
            pedia_resources, "Language_RU_RU");
        const auto entries = ui::read_civilopedia_entries(pedia_resources,
            [&](std::string_view key) -> std::optional<std::string> {
                const auto it = ru.rows.find(std::string(key));
                if (it == ru.rows.end()) return std::nullopt;
                return it->second;
            });
        const auto en = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
            pedia_resources, "Language_en_US");
        const auto english_entries = ui::read_civilopedia_entries(pedia_resources,
            [&](std::string_view key) -> std::optional<std::string> {
                const auto it = en.rows.find(std::string(key));
                if (it == en.rows.end()) return std::nullopt;
                return it->second;
            });
        const auto pottery = std::find_if(entries.begin(), entries.end(),
            [](const auto &entry) { return entry.id == "TECH_POTTERY"; });
        const auto warrior = std::find_if(entries.begin(), entries.end(),
            [](const auto &entry) { return entry.id == "UNIT_WARRIOR"; });
        const auto article = [](const auto &catalog, std::string_view id,
                                std::string_view category) {
            return std::find_if(catalog.begin(), catalog.end(), [&](const auto &entry) {
                return entry.id == id && entry.category == category &&
                    !entry.title.empty() && !entry.description.empty() &&
                    !entry.selected_source_path.empty();
            }) != catalog.end();
        };
        for (const auto &[id, category] : {
                 std::pair{"BUILDING_MONUMENT", "Buildings"},
                 std::pair{"POLICY_TRADITION", "Policies"},
                 std::pair{"IMPROVEMENT_FARM", "Improvements"},
                 std::pair{"RELIGION_BUDDHISM", "Religions"},
                 std::pair{"RESOLUTION_DIPLOMATIC_VICTORY", "Resolutions"},
                 std::pair{"LEAGUE_PROJECT_WORLD_FAIR", "LeagueProjects"},
                 std::pair{"RESOURCE_IRON", "Resources"},
                 std::pair{"TERRAIN_GRASS", "Terrains"},
                 std::pair{"FEATURE_FOREST", "Features"}}) {
            require(article(entries, id, category) &&
                    article(english_entries, id, category),
                    "Real EN/RU articles cover every expanded BNW category");
        }
        require(std::count_if(entries.begin(), entries.end(), [](const auto &entry) {
                    return entry.id == "BUILDING_MONUMENT";
                }) == 1, "Layered sources do not duplicate an article Type");
        require(entries.size() == 537 && english_entries.size() == 537 &&
                pottery != entries.end() && warrior != entries.end(),
                "All 537 visible selected EN/RU XML articles loaded");
        for (const auto *catalog : {&entries, &english_entries}) {
            const auto count = [&](std::string_view category) {
                return std::count_if(catalog->begin(), catalog->end(),
                    [category](const auto &entry) { return entry.category == category; });
            };
            require(count("Buildings") == 80 && count("Wonders") == 58 &&
                    count("Projects") == 2 && count("Religions") == 13 &&
                    count("Resolutions") == 18 && count("LeagueProjects") == 3 &&
                    count("UnclassifiedBuildings") == 0 &&
                    std::none_of(catalog->begin(), catalog->end(),
                        [](const auto &entry) { return entry.id == "RELIGION_PANTHEON"; }),
                    "Visible BNW Religion rows exclude the Lua pantheon sentinel");
            for (const auto &[id, expected_category, class_source] : {
                    std::tuple{"BUILDING_MONUMENT", "Buildings",
                               "CIV5BuildingClasses.xml"},
                    std::tuple{"BUILDING_STONEHENGE", "Wonders",
                               "CIV5BuildingClasses.xml"},
                    std::tuple{"BUILDING_NATIONAL_COLLEGE", "Wonders",
                               "CIV5BuildingClasses.xml"},
                    std::tuple{"BUILDING_UFFIZI", "Wonders",
                               "CIV5BuildingClasses_Expansion2.xml"}}) {
                const auto found = std::find_if(catalog->begin(), catalog->end(),
                    [id](const auto &entry) { return entry.id == id; });
                require(found != catalog->end() &&
                        found->category == expected_category &&
                        found->category_source_path.find(class_source) != std::string::npos,
                        "Building/Wonder classification retains selected class source");
            }
        }
        require(warrior->prerequisite_tech_ids.empty(),
            "A unit without authored PrereqTech gets no synthetic cross-link");
        for (const auto *catalog : {&entries, &english_entries}) {
            for (const auto &[id, tech] : {
                     std::pair{"UNIT_ARCHER", "TECH_ARCHERY"},
                     std::pair{"BUILDING_LIBRARY", "TECH_WRITING"}}) {
                const auto found = std::find_if(catalog->begin(), catalog->end(),
                    [&](const auto &entry) { return entry.id == id; });
                require(found != catalog->end() &&
                    found->selected_source_path.find("Expansion2") != std::string::npos &&
                    found->prerequisite_tech_ids == std::vector<std::string>{tech},
                    "Active BNW source row yields authored EN/RU prerequisite link");
            }
        }
        for (const auto *catalog : {&entries, &english_entries}) {
            const auto uffizi = std::find_if(catalog->begin(), catalog->end(),
                [](const auto &entry) { return entry.id == "BUILDING_UFFIZI"; });
            require(uffizi != catalog->end() &&
                    uffizi->selected_source_path.find(
                        "CIV5Buildings_Expansion2.xml") != std::string::npos &&
                    !uffizi->title.empty() && !uffizi->description.empty(),
                    "Uffizi article resolves from the selected BNW supplemental XML");
            for (const auto &[id, tech] : {
                     std::pair{"PROJECT_MANHATTAN_PROJECT", "TECH_ATOMIC_THEORY"},
                     std::pair{"PROJECT_APOLLO_PROGRAM", "TECH_ROCKETRY"}}) {
                const auto project = std::find_if(catalog->begin(), catalog->end(),
                    [id](const auto &entry) { return entry.id == id; });
                require(project != catalog->end() && project->category == "Projects" &&
                        project->selected_source_path.find(
                            "Assets/DLC/Expansion2/Gameplay/XML/GameInfo/CIV5Projects.xml") !=
                            std::string::npos &&
                        project->prerequisite_tech_ids == std::vector<std::string>{tech} &&
                        !project->title.empty() && !project->description.empty(),
                        "Selected BNW Project article retains source and prerequisite");
            }
            for (const auto *ignored : {"PROJECT_SS_COCKPIT", "PROJECT_SS_STASIS_CHAMBER",
                                        "PROJECT_SS_ENGINE", "PROJECT_SS_BOOSTER",
                                        "PROJECT_UTOPIA_PROJECT"})
                require(std::none_of(catalog->begin(), catalog->end(),
                    [ignored](const auto &entry) { return entry.id == ignored; }),
                    "Authored hidden parts and base-only Utopia stay out of selected BNW XML catalog");
        }
        require(english_entries.size() > 80 &&
            std::any_of(english_entries.begin(), english_entries.end(),
                [](const auto &entry) {
                    return entry.id == "TECH_POTTERY" && entry.title == "Pottery";
                }), "English active BNW article localization");
        require(pottery->title != "TXT_KEY_TECH_POTTERY_TITLE" &&
                !pottery->description.empty() &&
                pottery->selected_source_path.find("Expansion2") != std::string::npos,
                "Selected BNW technology table and localized article text");
        auto pedia_state = std::make_shared<ui::SystemScreenFrame>();
        pedia_state->pedia_entries = entries;
        pedia_state->pedia_article_layout = ui::civilopedia_article_layout(1280, 960);
        const auto pedia_frame = frame.with<ui::SystemScreenFrame>(pedia_state);
        require(controller.dispatch(ui::UiOpenScreen{"CivilopediaScreen"}, pedia_frame).status ==
                ui::UiCommandStatus::navigation, "Open Civilopedia");
        const auto pedia_plan = ui::plan_system_screen(source("CivilopediaScreen"),
            pedia_frame, 1280, 960);
        require(!pedia_plan.hits.empty() && pedia_plan.unresolved.empty(),
                "Civilopedia article list has a usable 2D plan");
        for (const auto size : {std::pair{1280, 960}, std::pair{1920, 1080}}) {
            const auto tabs = ui::plan_system_screen(source("CivilopediaScreen"),
                pedia_frame, size.first, size.second);
            std::size_t authored_tabs = 0;
            bool technology_unwired = false, promotions_unavailable = false;
            for (const auto &hit : tabs.hits) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay ? gameplay->as<ui::SystemPediaAction>() : nullptr;
                if (!action || action->kind != ui::SystemPediaActionKind::select_category)
                    continue;
                ++authored_tabs;
                if (action->article_id == "3")
                    technology_unwired = !hit.enabled &&
                        hit.unavailable_reason.find("no live session owner") !=
                            std::string::npos;
                if (action->article_id == "5")
                    promotions_unavailable = !hit.enabled && !hit.unavailable_reason.empty();
            }
            require(authored_tabs == 16 && technology_unwired && promotions_unavailable,
                    "Unwired live tab controls paint without false availability");
        }
        ui::SystemMenuSession category_session;
        require(category_session.activate("CivilopediaScreen", "CategoryButton3",
            pedia_frame, controller).status == ui::UiCommandStatus::performed &&
            category_session.pedia_category_tab() == 3,
            "Technology tab uses a typed local action");
        const auto technology_frame = category_session.with_pedia_state(pedia_frame);
        const auto technology_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), technology_frame, 1280, 960);
        require(std::any_of(technology_plan.hits.begin(), technology_plan.hits.end(),
                    [](const auto &hit) {
                        const auto *gameplay =
                            std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *action = gameplay ?
                            gameplay->template as<ui::SystemPediaAction>() : nullptr;
                        return hit.enabled && action &&
                            action->kind == ui::SystemPediaActionKind::select_category &&
                            action->article_id == "4";
                    }), "Owned local session can navigate to another supported tab");
        require(std::any_of(technology_plan.paints.begin(), technology_plan.paints.end(),
                    [](const auto &paint) {
                        return paint.source_key == "CivilopediaTopButtonsTechnology.dds" &&
                               paint.source.y == 50 && paint.source.width == 46 &&
                               paint.source.height == 50;
                    }), "Selected category uses the authored lower texture cell");
        std::size_t technology_hits = 0;
        bool only_technologies = true;
        for (const auto &hit : technology_plan.hits) {
            const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
            const auto *action = gameplay ? gameplay->as<ui::SystemPediaAction>() : nullptr;
            if (!action || action->kind != ui::SystemPediaActionKind::select_article)
                continue;
            ++technology_hits;
            const auto found = std::find_if(entries.begin(), entries.end(),
                [&](const auto &entry) { return entry.id == action->article_id; });
            only_technologies &= found != entries.end() &&
                                 found->category == "Technologies";
        }
        require(technology_hits > 0 && only_technologies,
                "Technology tab filters the selected BNW article catalog");
        require(category_session.activate("CivilopediaScreen", "CategoryButton5",
                    technology_frame, controller).status == ui::UiCommandStatus::unavailable &&
                category_session.pedia_category_tab() == 3,
                "Unsupported authored category cannot report success");
        require(category_session.activate("CivilopediaScreen", "CategoryButton6",
                    technology_frame, controller).status == ui::UiCommandStatus::performed &&
                category_session.pedia_category_tab() == 6,
                "Authored Buildings tab uses typed local navigation");
        const auto buildings_frame = category_session.with_pedia_state(pedia_frame);
        const auto buildings_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), buildings_frame, 1280, 960);
        const auto contains_entry = [](const auto &plan, std::string_view id) {
            return std::any_of(plan.hits.begin(), plan.hits.end(),
                [id](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = gameplay ?
                        gameplay->template as<ui::SystemPediaAction>() : nullptr;
                    return action && action->kind ==
                        ui::SystemPediaActionKind::select_article &&
                        action->article_id == id;
                });
        };
        require(std::any_of(buildings_plan.hits.begin(), buildings_plan.hits.end(),
                    [&](const auto &hit) {
                        const auto *gameplay =
                            std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *action = gameplay ?
                            gameplay->template as<ui::SystemPediaAction>() : nullptr;
                        if (!action || action->kind !=
                            ui::SystemPediaActionKind::select_article) return false;
                        const auto found = std::find_if(entries.begin(), entries.end(),
                            [&](const auto &entry) { return entry.id == action->article_id; });
                        return found != entries.end() && found->category == "Buildings";
                    }) && !contains_entry(buildings_plan, "BUILDING_STONEHENGE"),
                "Buildings tab displays source-classified buildings");
        require(category_session.activate("CivilopediaScreen", "CategoryButton7",
                    buildings_frame, controller).status == ui::UiCommandStatus::performed &&
                category_session.pedia_category_tab() == 7,
                "Authored Wonders tab uses typed local navigation");
        const auto wonders_frame = category_session.with_pedia_state(pedia_frame);
        const auto wonders_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), wonders_frame, 1280, 960);
        require(!contains_entry(wonders_plan, "BUILDING_MONUMENT") &&
                std::any_of(wonders_plan.hits.begin(), wonders_plan.hits.end(),
                    [&](const auto &hit) {
                        const auto *gameplay =
                            std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *action = gameplay ?
                            gameplay->template as<ui::SystemPediaAction>() : nullptr;
                        if (!action || action->kind !=
                            ui::SystemPediaActionKind::select_article) return false;
                        const auto found = std::find_if(entries.begin(), entries.end(),
                            [&](const auto &entry) { return entry.id == action->article_id; });
                        return found != entries.end() && found->category == "Wonders";
                    }),
                "Wonders tab shows only source-classified wonder articles");
        require(category_session.activate("CivilopediaScreen", "PediaEntry:BUILDING_MONUMENT",
                    wonders_frame, controller).status == ui::UiCommandStatus::performed &&
                category_session.pedia_category_tab() == 6,
                "Article navigation moves to its authored Buildings tab");
        require(category_session.activate("CivilopediaScreen", "PediaEntry:BUILDING_STONEHENGE",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                category_session.pedia_category_tab() == 7,
                "Article navigation moves to its authored Wonders tab");
        require(category_session.activate("CivilopediaScreen", "BackButton",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                category_session.pedia_category_tab() == 6 &&
                category_session.selected_pedia_id() ==
                    std::optional<std::string>{"BUILDING_MONUMENT"},
                "History restores the previous Building category");

        ui::SystemMenuSession project_session;
        require(project_session.activate("CivilopediaScreen", "CategoryButton7",
                    pedia_frame, controller).status == ui::UiCommandStatus::performed &&
                project_session.activate("CivilopediaScreen", "PediaEntry:BUILDING_STONEHENGE",
                    project_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed,
                "Project navigation begins in the authored Wonders tab");
        for (int page = 1; page <= 4; ++page)
            require(project_session.activate("CivilopediaScreen", "PediaNext",
                        project_session.with_pedia_state(pedia_frame), controller).status ==
                        ui::UiCommandStatus::performed &&
                    project_session.pedia_page() == static_cast<std::size_t>(page),
                    "Projects follow the 58 source-classified wonders in tab 7");
        const auto project_frame = project_session.with_pedia_state(pedia_frame);
        const auto project_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), project_frame, 1280, 960);
        require(contains_entry(project_plan, "PROJECT_MANHATTAN_PROJECT") &&
                contains_entry(project_plan, "PROJECT_APOLLO_PROGRAM") &&
                !contains_entry(project_plan, "PROJECT_SS_COCKPIT") &&
                !contains_entry(project_plan, "PROJECT_UTOPIA_PROJECT"),
                "Final Wonders page shows only the two visible BNW projects");
        require(project_session.activate("CivilopediaScreen",
                    "PediaEntry:PROJECT_APOLLO_PROGRAM", project_frame, controller).status ==
                    ui::UiCommandStatus::performed &&
                project_session.pedia_category_tab() == 7 &&
                project_session.selected_pedia_id() ==
                    std::optional<std::string>{"PROJECT_APOLLO_PROGRAM"},
                "Typed Project selection keeps the Wonders tab");
        const auto project_article_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), project_session.with_pedia_state(pedia_frame),
            1280, 960);
        require(std::any_of(project_article_plan.hits.begin(),
                    project_article_plan.hits.end(), [](const auto &hit) {
                        const auto *gameplay =
                            std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *action = gameplay ?
                            gameplay->template as<ui::SystemPediaAction>() : nullptr;
                        return hit.enabled && action &&
                            action->kind == ui::SystemPediaActionKind::follow_link &&
                            action->article_id == "TECH_ROCKETRY";
                    }), "Selected Apollo article links to its authored prerequisite tech");
        require(project_session.activate("CivilopediaScreen", "BackButton",
                    project_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                project_session.selected_pedia_id() ==
                    std::optional<std::string>{"BUILDING_STONEHENGE"},
                "Project history restores the previous wonder article");

        ui::SystemMenuSession religion_session;
        require(religion_session.activate("CivilopediaScreen", "CategoryButton15",
                    pedia_frame, controller).status == ui::UiCommandStatus::performed &&
                religion_session.pedia_category_tab() == 15,
                "Active BNW Religions section routes to Beliefs tab 15 locally");
        const auto religion_frame = religion_session.with_pedia_state(pedia_frame);
        const auto religion_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), religion_frame, 1280, 960);
        require(contains_entry(religion_plan, "RELIGION_BUDDHISM") &&
                !contains_entry(religion_plan, "RELIGION_PANTHEON") &&
                religion_session.activate("CivilopediaScreen", "PediaEntry:RELIGION_BUDDHISM",
                    religion_frame, controller).status == ui::UiCommandStatus::performed &&
                religion_session.pedia_category_tab() == 15,
                "Visible religion article remains in authored tab 15");
        require(religion_session.activate("CivilopediaScreen", "PediaEntry:TECH_POTTERY",
                    religion_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                religion_session.pedia_category_tab() == 3 &&
                religion_session.activate("CivilopediaScreen", "BackButton",
                    religion_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                religion_session.pedia_category_tab() == 15 &&
                religion_session.selected_pedia_id() ==
                    std::optional<std::string>{"RELIGION_BUDDHISM"},
                "Typed history restores the Religion section after another tab");
        const auto buddhism = std::find_if(entries.begin(), entries.end(),
            [](const auto &entry) { return entry.id == "RELIGION_BUDDHISM"; });
        require(buddhism != entries.end() &&
                buddhism->selected_source_path.find("Civ5Religions.xml") !=
                    std::string::npos &&
                religion_session.activate("CivilopediaScreen",
                    "PediaSearch:" + buddhism->title,
                    religion_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                religion_session.activate("CivilopediaScreen", "SearchButton",
                    religion_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                religion_session.pedia_category_tab() == 15,
                "Localized Religion search uses selected XML and restores tab 15");

        ui::SystemMenuSession congress_session;
        require(congress_session.activate("CivilopediaScreen", "CategoryButton16",
                    pedia_frame, controller).status == ui::UiCommandStatus::performed &&
                congress_session.pedia_category_tab() == 16,
                "Active World Congress Resolutions section routes to tab 16 locally");
        const auto congress_frame = congress_session.with_pedia_state(pedia_frame);
        const auto congress_plan = ui::plan_system_screen(
            source("CivilopediaScreen"), congress_frame, 1280, 960);
        require(std::any_of(congress_plan.hits.begin(), congress_plan.hits.end(),
                    [&](const auto &hit) {
                        const auto *gameplay =
                            std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *action = gameplay ?
                            gameplay->template as<ui::SystemPediaAction>() : nullptr;
                        if (!action || action->kind !=
                            ui::SystemPediaActionKind::select_article) return false;
                        const auto found = std::find_if(entries.begin(), entries.end(),
                            [&](const auto &entry) {
                                return entry.id == action->article_id;
                            });
                        return found != entries.end() &&
                            found->category == "Resolutions";
                    }) && !contains_entry(congress_plan, "PROJECT_APOLLO_PROGRAM"),
                "World Congress tab shows selected resolution XML, not ordinary projects");
        const auto resolution = std::find_if(entries.begin(), entries.end(),
            [](const auto &entry) {
                return entry.id == "RESOLUTION_DIPLOMATIC_VICTORY";
            });
        require(resolution != entries.end() &&
                resolution->selected_source_path.find("CIV5Resolutions.xml") !=
                    std::string::npos &&
                congress_session.activate("CivilopediaScreen",
                    "PediaEntry:RESOLUTION_DIPLOMATIC_VICTORY",
                    congress_frame, controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.pedia_category_tab() == 16 &&
                congress_session.activate("CivilopediaScreen", "PediaEntry:TECH_POTTERY",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.activate("CivilopediaScreen", "BackButton",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.pedia_category_tab() == 16,
                "Source-bound resolution article and typed history restore tab 16");
        require(congress_session.activate("CivilopediaScreen",
                    "PediaSearch:" + resolution->title,
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.activate("CivilopediaScreen", "SearchButton",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.pedia_category_tab() == 16,
                "Localized resolution search returns to World Congress tab");
        for (const auto *catalog : {&entries, &english_entries}) {
            for (const auto *id : {"LEAGUE_PROJECT_INTERNATIONAL_SPACE_STATION",
                                   "LEAGUE_PROJECT_WORLD_FAIR",
                                   "LEAGUE_PROJECT_WORLD_GAMES"}) {
                const auto project = std::find_if(catalog->begin(), catalog->end(),
                    [id](const auto &entry) { return entry.id == id; });
                require(project != catalog->end() &&
                        project->category == "LeagueProjects" &&
                        project->selected_source_path.find("CIV5Resolutions.xml") !=
                            std::string::npos &&
                        project->description.find("[ICON_TROPHY_GOLD]") == 0 &&
                        project->description.find("[ICON_TROPHY_SILVER]") !=
                            std::string::npos &&
                        project->description.find("[ICON_TROPHY_BRONZE]") !=
                            std::string::npos &&
                        project->description.find("{1_TrophyIcon}") ==
                            std::string::npos &&
                        project->description.find("TXT_KEY_") == std::string::npos,
                        "Authored LeagueProject rewards are localized in Lua tier order");
            }
        }
        const auto world_fair = std::find_if(entries.begin(), entries.end(),
            [](const auto &entry) {
                return entry.id == "LEAGUE_PROJECT_WORLD_FAIR";
            });
        require(world_fair != entries.end() &&
                congress_session.activate("CivilopediaScreen",
                    "PediaEntry:LEAGUE_PROJECT_WORLD_FAIR",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.pedia_category_tab() == 16 &&
                congress_session.selected_pedia_id() ==
                    std::optional<std::string>{"LEAGUE_PROJECT_WORLD_FAIR"} &&
                congress_session.activate("CivilopediaScreen", "BackButton",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.activate("CivilopediaScreen", "ForwardButton",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                congress_session.selected_pedia_id() ==
                    std::optional<std::string>{"LEAGUE_PROJECT_WORLD_FAIR"},
                "Typed World Congress article history includes LeagueProjects");
        require(congress_session.activate("CivilopediaScreen",
                    "PediaSearch:" + world_fair->title,
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                std::count_if(entries.begin(), entries.end(),
                    [&](const auto &entry) {
                        return entry.title == world_fair->title;
                    }) == 2 &&
                congress_session.activate("CivilopediaScreen", "SearchButton",
                    congress_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::unavailable &&
                congress_session.selected_pedia_id() ==
                    std::optional<std::string>{"LEAGUE_PROJECT_WORLD_FAIR"},
                "Duplicate authored World Fair titles fail closed in global search");

        require(category_session.activate("CivilopediaScreen",
                    "PediaSearch:" + pottery->title, technology_frame, controller).status ==
                    ui::UiCommandStatus::performed &&
                category_session.pedia_category_tab() == 0,
                "Owned local search restores the complete article catalog");
        require(category_session.activate("CivilopediaScreen", "SearchButton",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                category_session.selected_pedia_id() ==
                    std::optional<std::string>{"TECH_POTTERY"} &&
                category_session.pedia_category_tab() == 3,
                "Authored SearchButton selects an exact localized article");
        require(category_session.activate("CivilopediaScreen", "PediaSearch:о",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                category_session.activate("CivilopediaScreen", "SearchButton",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::unavailable &&
                category_session.selected_pedia_id() ==
                    std::optional<std::string>{"TECH_POTTERY"},
                "Ambiguous partial search cannot claim article navigation");
        require(category_session.activate("CivilopediaScreen", "PediaSearch:",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::performed &&
                category_session.activate("CivilopediaScreen", "SearchButton",
                    category_session.with_pedia_state(pedia_frame), controller).status ==
                    ui::UiCommandStatus::unavailable,
                "Empty authored search cannot claim navigation");
        ui::LiveUiSources tab_sources;
        tab_sources.pedia_entries = &entries;
        ui::LiveUiHost tab_host(registry, [&] { return frame; }, nullptr, tab_sources);
        auto tab_frame = tab_host.begin_frame(1280, 960);
        require(tab_host.dispatch(ui::UiOpenScreen{"CivilopediaScreen"}, tab_frame).status ==
                ui::UiCommandStatus::navigation, "Open live Civilopedia for tab input");
        tab_frame = tab_host.begin_frame(1280, 960);
        require(tab_frame.snapshot.find<ui::SystemScreenFrame>()->
                    pedia_category_session_owned,
                "Live host publishes its persistent Civilopedia category owner");
        const auto tab_command = ui::system_control_command("CivilopediaScreen",
            "CategoryButton3");
        require(tab_command && tab_host.dispatch(*tab_command, tab_frame).status ==
                ui::UiCommandStatus::performed &&
                tab_host.dispatch(*tab_command, tab_frame).status ==
                    ui::UiCommandStatus::invalid,
                "Live category command changes owned state once per frame");
        tab_frame = tab_host.begin_frame(1280, 960);
        require(tab_frame.snapshot.find<ui::SystemScreenFrame>()->pedia_category_tab == 3,
                "Next live frame publishes the selected Technologies tab");
        ui::UiScreenCompositor tab_compositor(resources, registry);
        auto tab_view = tab_compositor.compose(tab_frame, tab_host.screens(), 1280, 960);
        const auto congress_hit = std::find_if(tab_view.hits.begin(), tab_view.hits.end(),
            [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay ?
                    gameplay->template as<ui::SystemPediaAction>() : nullptr;
                return hit.enabled && action &&
                    action->kind == ui::SystemPediaActionKind::select_category &&
                    action->article_id == "16";
            });
        require(congress_hit != tab_view.hits.end() &&
                ui::click_composed_ui(tab_host, tab_frame, tab_view,
                    congress_hit->bounds.x + 2, congress_hit->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::performed,
                "Measured active BNW category hit changes the live host");
        tab_frame = tab_host.begin_frame(1280, 960);
        tab_view = tab_compositor.compose(tab_frame, tab_host.screens(), 1280, 960);
        const auto resolution_hit = std::find_if(tab_view.hits.begin(),
            tab_view.hits.end(), [&](const auto &hit) {
                const auto *gameplay =
                    std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay ?
                    gameplay->template as<ui::SystemPediaAction>() : nullptr;
                if (!action || action->kind !=
                    ui::SystemPediaActionKind::select_article) return false;
                const auto found = std::find_if(entries.begin(), entries.end(),
                    [&](const auto &entry) {
                        return entry.id == action->article_id;
                    });
                return found != entries.end() && found->category == "Resolutions";
            });
        require(tab_frame.snapshot.find<ui::SystemScreenFrame>()->pedia_category_tab == 16 &&
                resolution_hit != tab_view.hits.end() &&
                tab_host.dispatch(*ui::system_control_command("CivilopediaScreen",
                    "CategoryButton16"), tab_frame).status ==
                    ui::UiCommandStatus::unavailable,
                "Visible live World Congress list follows the selected tab");
        const auto resolution_id = std::get<ui::UiGameplayCommand>(
            resolution_hit->command).as<ui::SystemPediaAction>()->article_id;
        require(ui::click_composed_ui(tab_host, tab_frame, tab_view,
                    resolution_hit->bounds.x + 2,
                    resolution_hit->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::performed,
                "Measured resolution article hit changes the live selection");
        tab_frame = tab_host.begin_frame(1280, 960);
        tab_view = tab_compositor.compose(tab_frame, tab_host.screens(), 1280, 960);
        require(tab_frame.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id ==
                    std::optional<std::string>{resolution_id} &&
                std::any_of(tab_view.paints.begin(), tab_view.paints.end(),
                    [&](const auto &paint) {
                        const auto found = std::find_if(entries.begin(), entries.end(),
                            [&](const auto &entry) { return entry.id == resolution_id; });
                        return found != entries.end() &&
                            paint.text.find(found->description.substr(0, 12)) !=
                                std::string::npos;
                    }),
                "Next composed frame visibly paints the source-bound resolution");
        const auto search_submit = ui::system_control_command("CivilopediaScreen",
            "SearchButton", tab_frame.snapshot.find<ui::SystemScreenFrame>());
        require(search_submit &&
                tab_host.dispatch(*search_submit, tab_frame).status ==
                    ui::UiCommandStatus::unavailable,
                "Empty live SearchButton cannot claim navigation");
        require(tab_host.dispatch(ui::UiCloseScreen{"CivilopediaScreen"}, tab_frame).status ==
                ui::UiCommandStatus::navigation,
                "Authored Civilopedia close route clears focused screen");
        tab_frame = tab_host.begin_frame(1280, 960);
        require(tab_host.dispatch(*search_submit, tab_frame).status ==
                ui::UiCommandStatus::invalid,
                "Closed Civilopedia refuses a stale SearchButton action");
        ui::LiveUiHost no_catalog_host(registry, [&] { return frame; }, nullptr);
        auto no_catalog_frame = no_catalog_host.begin_frame(1280, 960);
        require(no_catalog_host.dispatch(ui::UiOpenScreen{"CivilopediaScreen"},
                    no_catalog_frame).status == ui::UiCommandStatus::navigation,
                "Civilopedia shell opens without inventing a catalog");
        no_catalog_frame = no_catalog_host.begin_frame(1280, 960);
        require(no_catalog_host.dispatch(*tab_command, no_catalog_frame).status ==
                    ui::UiCommandStatus::unavailable,
                "Missing selected article catalog leaves live category unavailable");
        ui::SystemMenuSession pedia_session;
        require(pedia_session.activate("CivilopediaScreen", "PediaEntry:TECH_POTTERY",
            pedia_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.selected_pedia_id() == "TECH_POTTERY",
            "Typed article selection through U0 controller");
        const auto selected_frame = pedia_session.with_pedia_state(pedia_frame);
        const auto selected_plan = ui::plan_system_screen(source("CivilopediaScreen"),
            selected_frame, 1280, 960);
        require(std::any_of(selected_plan.paints.begin(), selected_plan.paints.end(),
            [&](const auto &paint) {
                return paint.text.find(pottery->description.substr(0, 12)) !=
                    std::string::npos;
            }) && selected_plan.article_scroll_panel.has_value(),
            "Selected article body is drawn from real localization in authored ScrollPanel");
        require(pedia_session.activate("CivilopediaScreen", "PediaNext",
            selected_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.pedia_page() == 1, "Typed page navigation");
        require(pedia_session.activate("CivilopediaScreen", "PediaEntry:TECH_MISSING",
            selected_frame, controller).status == ui::UiCommandStatus::unavailable &&
            pedia_session.selected_pedia_id() == "TECH_POTTERY",
            "Missing article cannot report success");
        require(pedia_session.activate("CivilopediaScreen", "PediaEntry:BUILDING_MONUMENT",
            selected_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.selected_pedia_id() == "BUILDING_MONUMENT",
            "Expanded article category uses typed selection");
        ui::SystemMenuSession link_session;
        require(link_session.activate("CivilopediaScreen", "PediaEntry:UNIT_ARCHER",
            pedia_frame, controller).status == ui::UiCommandStatus::performed,
            "Open source article for authored link");
        for (const auto [link_width, link_height] : {
                 std::pair{1280, 960}, std::pair{1920, 1080}}) {
            auto link_frame = link_session.with_pedia_state(pedia_frame);
            auto link_state = *link_frame.find<ui::SystemScreenFrame>();
            link_state.pedia_article_layout = ui::civilopedia_article_layout(
                link_width, link_height);
            link_frame = link_frame.with<ui::SystemScreenFrame>(
                std::make_shared<const ui::SystemScreenFrame>(std::move(link_state)));
            const auto link_plan = ui::plan_system_screen(source("CivilopediaScreen"),
                link_frame, link_width, link_height,
                [&](std::string_view key) -> std::optional<std::string> {
                    const auto it = ru.rows.find(std::string(key));
                    return it == ru.rows.end() ? std::nullopt :
                        std::optional<std::string>{it->second};
                });
            require(std::any_of(link_plan.hits.begin(), link_plan.hits.end(),
                [](const auto &hit) {
                    const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = command ? command->template as<ui::SystemPediaAction>() :
                        nullptr;
                    return hit.enabled && action &&
                        action->kind == ui::SystemPediaActionKind::follow_link &&
                        action->article_id == "TECH_ARCHERY";
                }), "Authored prerequisite link has a typed hit at both resolutions");
        }
        const auto archer_frame = link_session.with_pedia_state(pedia_frame);
        for (const auto &[catalog, localized, expected] : {
                 std::tuple{&entries, &ru, std::string{"TECH_ARCHERY"}},
                 std::tuple{&english_entries, &en, std::string{"TECH_ARCHERY"}}}) {
            auto localized_state = *archer_frame.find<ui::SystemScreenFrame>();
            localized_state.pedia_entries = *catalog;
            const auto localized_frame = archer_frame.with<ui::SystemScreenFrame>(
                std::make_shared<const ui::SystemScreenFrame>(std::move(localized_state)));
            const auto localized_plan = ui::plan_system_screen(
                source("CivilopediaScreen"), localized_frame, 1280, 960,
                [&](std::string_view key) -> std::optional<std::string> {
                    const auto it = localized->rows.find(std::string(key));
                    return it == localized->rows.end() ? std::nullopt :
                        std::optional<std::string>{it->second};
                });
            const auto target = std::find_if(catalog->begin(), catalog->end(),
                [&](const auto &entry) { return entry.id == expected; });
            require(target != catalog->end() && std::any_of(localized_plan.paints.begin(),
                localized_plan.paints.end(), [&](const auto &paint) {
                    return paint.text.find(target->title) != std::string::npos &&
                        paint.text.find(localized->rows.at(
                            "TXT_KEY_PEDIA_PREREQ_TECH_LABEL")) != std::string::npos;
                }), "Link title and relation label use active EN/RU localization");
        }
        require(link_session.activate("CivilopediaScreen", "PediaLink:TECH_WRITING",
            archer_frame, controller).status == ui::UiCommandStatus::unavailable &&
            link_session.selected_pedia_id() == "UNIT_ARCHER",
            "Unrelated Civilopedia target cannot masquerade as an authored link");
        require(link_session.activate("CivilopediaScreen", "PediaLink:TECH_ARCHERY",
            archer_frame, controller).status == ui::UiCommandStatus::performed &&
            link_session.selected_pedia_id() == "TECH_ARCHERY" &&
            link_session.activate("CivilopediaScreen", "BackButton",
                link_session.with_pedia_state(pedia_frame), controller).status ==
                ui::UiCommandStatus::performed &&
            link_session.selected_pedia_id() == "UNIT_ARCHER",
            "Authored link follows typed local history and Back returns to source");
        ui::LiveUiSources live_sources;
        live_sources.pedia_entries = &entries;
        ui::LiveUiHost host(registry, [&] { return frame; }, nullptr, live_sources);
        auto live = host.begin_frame(1280, 960);
        require(live.snapshot.find<ui::SystemScreenFrame>() &&
            live.snapshot.find<ui::SystemScreenFrame>()->pedia_entries.size() == entries.size(),
            "Live host publishes immutable Civilopedia entries in UiFrameSnapshot");
        require(host.dispatch(ui::UiOpenScreen{"CivilopediaScreen"}, live).status ==
                ui::UiCommandStatus::navigation, "Live host opens Civilopedia");
        live = host.begin_frame(1280, 960);
        const auto live_selection = ui::system_control_command("CivilopediaScreen",
            "PediaEntry:UNIT_WARRIOR");
        require(live_selection && host.dispatch(*live_selection, live).status ==
                ui::UiCommandStatus::performed, "Live host routes typed selection");
        live = host.begin_frame(1280, 960);
        require(live.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id ==
                std::optional<std::string>{"UNIT_WARRIOR"} &&
                host.dispatch(*live_selection, live).status ==
                    ui::UiCommandStatus::unavailable,
                "Next live snapshot contains article; repeated selection is not success");
        const auto live_archer = ui::system_control_command("CivilopediaScreen",
            "PediaEntry:UNIT_ARCHER");
        require(live_archer && host.dispatch(*live_archer, live).status ==
            ui::UiCommandStatus::performed,
            "Live host opens prerequisite source article");
        live = host.begin_frame(1280, 960);
        const auto live_link = ui::system_control_command("CivilopediaScreen",
            "PediaLink:TECH_ARCHERY");
        require(live_link && host.dispatch(*live_link, live).status ==
            ui::UiCommandStatus::performed,
            "Live host follows typed authored prerequisite");
        live = host.begin_frame(1280, 960);
        require(live.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id ==
                std::optional<std::string>{"TECH_ARCHERY"} &&
            host.dispatch(*live_link, live).status == ui::UiCommandStatus::unavailable,
            "Following a link twice cannot claim success from a different article");
        const auto live_back = ui::system_control_command("CivilopediaScreen",
            "BackButton");
        const auto live_forward = ui::system_control_command("CivilopediaScreen",
            "ForwardButton");
        require(live_back && live_forward &&
                host.dispatch(*live_back, live).status == ui::UiCommandStatus::performed,
                "Live BackButton commits the previous authored article");
        live = host.begin_frame(1280, 960);
        require(live.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id ==
                    std::optional<std::string>{"UNIT_ARCHER"} &&
                live.snapshot.find<ui::SystemScreenFrame>()->pedia_category_tab == 4 &&
                host.dispatch(*live_forward, live).status ==
                    ui::UiCommandStatus::performed,
                "Live history publishes the source article and accepts Forward");
        live = host.begin_frame(1280, 960);
        require(live.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id ==
                    std::optional<std::string>{"TECH_ARCHERY"} &&
                live.snapshot.find<ui::SystemScreenFrame>()->pedia_category_tab == 3,
                "Live ForwardButton publishes the target article and tab");
        require(pedia_session.activate("CivilopediaScreen", "BackButton",
            selected_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.selected_pedia_id() == "TECH_POTTERY",
            "History back restores the prior real article");
        require(pedia_session.activate("CivilopediaScreen", "ForwardButton",
            selected_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.selected_pedia_id() == "BUILDING_MONUMENT",
            "History forward restores the real article");
        require(pedia_session.activate("CivilopediaScreen", "BackButton",
            selected_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.activate("CivilopediaScreen", "PediaEntry:UNIT_WARRIOR",
            selected_frame, controller).status == ui::UiCommandStatus::performed &&
            pedia_session.activate("CivilopediaScreen", "ForwardButton",
            selected_frame, controller).status == ui::UiCommandStatus::unavailable,
            "Selecting a new article truncates forward history");
        const auto ru_search = std::string("PediaSearch:") + pottery->title;
        const auto ru_search_result = pedia_session.activate("CivilopediaScreen", ru_search,
            selected_frame, controller);
        require(ru_search_result.status == ui::UiCommandStatus::performed &&
            pedia_session.pedia_search() == pottery->title && pedia_session.pedia_page() == 0,
            "RU authored article title search is a typed command");
        const auto filtered_frame = pedia_session.with_pedia_state(pedia_frame);
        const auto filtered_plan = ui::plan_system_screen(source("CivilopediaScreen"),
            filtered_frame, 1280, 960);
        require(std::any_of(filtered_plan.paints.begin(), filtered_plan.paints.end(),
            [&](const auto &paint) { return paint.text == pottery->title; }) &&
            pedia_session.activate("CivilopediaScreen", "PediaNext",
            filtered_frame, controller).status == ui::UiCommandStatus::unavailable,
            "Search filters the displayed list and its pagination");
        require(pedia_session.activate("CivilopediaScreen",
            "PediaSearch:ГОНЧАРНОЕ", filtered_frame, controller).status ==
                ui::UiCommandStatus::performed,
            "RU uppercase search is accepted");
        const auto folded_ru = ui::plan_system_screen(source("CivilopediaScreen"),
            pedia_session.with_pedia_state(pedia_frame), 1280, 960);
        require(std::any_of(folded_ru.paints.begin(), folded_ru.paints.end(),
            [&](const auto &paint) { return paint.text == pottery->title; }),
            "RU title search folds Cyrillic case");
        require(pedia_session.activate("CivilopediaScreen", "PediaSearch:absent article xyz",
            filtered_frame, controller).status == ui::UiCommandStatus::performed,
            "Zero-result search is a valid local action");
        const auto no_results = ui::plan_system_screen(source("CivilopediaScreen"),
            pedia_session.with_pedia_state(pedia_frame), 1280, 960);
        require(std::any_of(no_results.paints.begin(), no_results.paints.end(),
            [](const auto &paint) { return paint.text == "0"; }) &&
            pedia_session.activate("CivilopediaScreen", "PediaNext",
            pedia_session.with_pedia_state(pedia_frame), controller).status ==
                ui::UiCommandStatus::unavailable,
            "Zero-result search gives no phantom page");
        require(pedia_session.activate("CivilopediaScreen",
            "PediaSearch:" + std::string(51, 'x'), selected_frame, controller).status ==
                ui::UiCommandStatus::invalid,
            "Authored SearchEditBox length is enforced");
        auto missing_history = *filtered_frame.find<ui::SystemScreenFrame>();
        missing_history.pedia_history = {"TECH_MISSING", "BUILDING_MONUMENT"};
        missing_history.pedia_history_index = 1;
        missing_history.selected_pedia_id = "BUILDING_MONUMENT";
        require(ui::apply_civilopedia_action(missing_history,
            {ui::SystemPediaActionKind::history_back, {}}).status ==
                ui::UiCommandStatus::unavailable &&
            missing_history.selected_pedia_id == "BUILDING_MONUMENT",
            "History refuses missing catalog articles without changing selection");
        const auto stale_live = live;
        require(host.search_civilopedia(pottery->title, live).status ==
                ui::UiCommandStatus::performed,
            "Live host accepts selected SearchEditBox text");
        require(host.search_civilopedia(pottery->title, stale_live).status ==
                ui::UiCommandStatus::invalid,
            "Stale live search frame is rejected");
        live = host.begin_frame(1280, 960);
        require(live.snapshot.find<ui::SystemScreenFrame>()->pedia_search == pottery->title,
            "Filtered query persists in the next immutable live frame");
        require(host.dispatch(ui::UiCloseScreen{"CivilopediaScreen"}, live).status ==
                ui::UiCommandStatus::navigation,
            "Civilopedia closes");
        live = host.begin_frame(1280, 960);
        require(host.search_civilopedia(pottery->title, live).status ==
                ui::UiCommandStatus::invalid,
            "Search cannot bypass Civilopedia input focus");
        ui::LiveUiSources en_sources;
        en_sources.pedia_entries = &english_entries;
        ui::LiveUiHost en_host(registry, [&] { return frame; }, nullptr, en_sources);
        auto en_live = en_host.begin_frame(1280, 960);
        require(en_host.dispatch(ui::UiOpenScreen{"CivilopediaScreen"}, en_live).status ==
                ui::UiCommandStatus::navigation, "Open English Civilopedia");
        en_live = en_host.begin_frame(1280, 960);
        require(en_host.search_civilopedia("POTTERY", en_live).status ==
                ui::UiCommandStatus::performed, "English title search folds case");
        en_live = en_host.begin_frame(1280, 960);
        const auto en_plan = ui::plan_system_screen(source("CivilopediaScreen"),
            en_live.snapshot, 1280, 960);
        require(std::any_of(en_plan.paints.begin(), en_plan.paints.end(),
            [](const auto &paint) { return paint.text == "Pottery"; }),
            "English filtered article is displayed");
        auto preview_ru_keys = pedia_resources.keys(
            "Assets/Gameplay/XML/NewText/RU_RU");
        std::sort(preview_ru_keys.begin(), preview_ru_keys.end());
        auto preview_ru = ui::TopPanelLocalizationCatalog::read_resources(
            pedia_resources, preview_ru_keys, "Language_RU_RU");
        for (const auto *name : {
                 "CIV5GameTextInfos_InGameScreens_Inherited_Expansion2.xml",
                 "CIV5GameTextInfos_InGameScreens_Expansion2.xml",
                 "CIV5GameTextInfos_WorldView_Expansion2.xml"}) {
            const auto extra = ui::TopPanelLocalizationCatalog::read_resources(
                pedia_resources,
                {std::string("Assets/DLC/Expansion2/Gameplay/XML/Text/RU_RU/") +
                 name}, "Language_RU_RU");
            for (const auto &[key, value] : extra.rows)
                preview_ru.rows.insert_or_assign(key, value);
        }
        const auto authored_right_font = ui::read_glyphgen(pedia_resources,
            "Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml");
        const auto right_geometry = ui::glyphgen_geometry(authored_right_font);
        std::set<std::uint32_t> prepared_right_glyphs;
        for (const auto &glyph : right_geometry.glyphs)
            prepared_right_glyphs.insert(glyph.code_point);
        const auto base_right_glyph_count = prepared_right_glyphs.size();
        for (const auto &[key, value] : preview_ru.rows) {
            (void)key;
            const auto points = code_points(value);
            prepared_right_glyphs.insert(points.begin(), points.end());
        }
        require(prepared_right_glyphs.size() - base_right_glyph_count == 66,
            "Pinned RU base preview adds 66 glyphs to the right-panel font");
        std::map<std::string, std::uint32_t> uncovered_ru_articles;
        for (const auto &entry : entries) {
            auto points = code_points(entry.title);
            const auto body_points = code_points(entry.description);
            points.insert(body_points.begin(), body_points.end());
            for (const auto point : points)
                if (!prepared_right_glyphs.contains(point)) {
                    uncovered_ru_articles.emplace(entry.id, point);
                    std::cout << "Civilopedia RU uncovered article=" << entry.id
                              << " glyph=U+" << std::hex << point << std::dec << '\n';
                    break;
                }
        }
        require(uncovered_ru_articles ==
                (std::map<std::string, std::uint32_t>{
                    {"BUILDING_CANDI", 0x2013},
                    {"BUILDING_GLOBE_THEATER", 0x2013},
                    {"BUILDING_IKANDA", 0x2013},
                    {"BUILDING_PARTHENON", 0x2013},
                    {"BUILDING_ROYAL_LIBRARY", 0x2013},
                    {"BUILDING_TOURIST_CENTER", 0x2013},
                    {"BUILDING_UFFIZI", 0x2013},
                    {"BUILDING_WRITERS_GUILD", 0x2013},
                    {"POLICY_GUNBOAT_DIPLOMACY", 0x2026},
                    {"POLICY_NEW_DEAL", 0x2014},
                    {"POLICY_MERCHANT_CONFEDERACY", 0x2013}}),
            "Authored RU building and policy glyphs extend the selected preview atlas");
        for (const auto &entry : entries) {
            const auto title_points = code_points(entry.title);
            const auto body_points = code_points(entry.description);
            prepared_right_glyphs.insert(title_points.begin(), title_points.end());
            prepared_right_glyphs.insert(body_points.begin(), body_points.end());
        }
        require(prepared_right_glyphs.size() - base_right_glyph_count == 69,
            "Civilopedia-aware RU right-panel atlas needs 69 added glyphs");
        for (const auto &[id, point] : uncovered_ru_articles)
            require(prepared_right_glyphs.contains(point) &&
                    std::any_of(entries.begin(), entries.end(),
                        [&](const auto &entry) { return entry.id == id; }),
                "Each authored RU article outside the base atlas is covered");
        std::cout << "Civilopedia RU prepared right font covers " << entries.size()
                  << " articles with 69 added glyphs\n";
        for (const auto *catalog : {&entries, &english_entries}) {
            ui::SystemScreenFrame probe;
            probe.pedia_entries = *catalog;
            probe.pedia_article_layout = ui::civilopedia_article_layout(1280, 960);
            const auto long_article = std::find_if(catalog->begin(), catalog->end(),
                [&](const auto &entry) {
                    probe.selected_pedia_id = entry.id;
                    return ui::civilopedia_article_max_scroll(probe) > 2;
                });
            require(long_article != catalog->end(),
                    "Active EN/RU Civilopedia has a genuinely scrollable article");
            if (catalog == &entries) {
                auto article_glyphs = code_points(long_article->title);
                const auto body_glyphs = code_points(long_article->description);
                article_glyphs.insert(body_glyphs.begin(), body_glyphs.end());
                std::size_t missing = 0;
                for (const auto point : article_glyphs)
                    if (!prepared_right_glyphs.contains(point)) ++missing;
                std::cout << "Civilopedia RU scroll article=" << long_article->id
                          << " preparedRightAddedGlyphs=69 missingGlyphs="
                          << missing << '\n';
                require(!missing,
                    "Actual long RU article is covered by the preview's prepared font");
            }
            ui::LiveUiSources scroll_sources;
            scroll_sources.pedia_entries = catalog;
            ui::LiveUiHost scroll_host(registry, [&] { return frame; }, nullptr,
                                      scroll_sources);
            ui::UiScreenCompositor scroll_compositor(resources, registry);
            auto scroll_frame = scroll_host.begin_frame(1280, 960);
            require(scroll_host.dispatch(ui::UiOpenScreen{"CivilopediaScreen"},
                        scroll_frame).status == ui::UiCommandStatus::navigation,
                    "Open EN/RU article scroll view");
            scroll_frame = scroll_host.begin_frame(1280, 960);
            const auto select_long = ui::system_control_command("CivilopediaScreen",
                "PediaEntry:" + long_article->id);
            require(select_long && scroll_host.dispatch(*select_long, scroll_frame).status ==
                    ui::UiCommandStatus::performed,
                    "Select authored long article with typed command");
            scroll_frame = scroll_host.begin_frame(1280, 960);
            auto scroll_view = scroll_compositor.compose(scroll_frame,
                scroll_host.screens(), 1280, 960);
            require(scroll_view.civilopedia_article_scroll_panel &&
                    scroll_frame.snapshot.find<ui::SystemScreenFrame>()->
                        pedia_article_first_line == 0,
                    "Live frame publishes measured article scroll panel and initial line");
            const auto article_body = [](const ui::ComposedUiFrame &view) {
                const auto &bounds = *view.civilopedia_article_scroll_panel;
                const auto found = std::find_if(view.paints.begin(), view.paints.end(),
                    [&](const auto &paint) {
                        return paint.kind == ui::WorldViewPaintKind::text &&
                            paint.destination.x == bounds.x &&
                            paint.destination.y == bounds.y;
                    });
                return found == view.paints.end() ? std::string{} : found->text;
            };
            const auto first_text = article_body(scroll_view);
            require(!first_text.empty() && first_text != long_article->description,
                    "Long authored article is bounded to the visible text viewport");
            const auto down = std::find_if(scroll_view.hits.begin(),
                scroll_view.hits.end(), [](const auto &hit) {
                    const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = command ?
                        command->template as<ui::SystemPediaAction>() : nullptr;
                    return action && action->kind ==
                        ui::SystemPediaActionKind::article_scroll_down;
                });
            require(down != scroll_view.hits.end() && down->enabled &&
                    ui::click_composed_ui(scroll_host, scroll_frame, scroll_view,
                        down->bounds.x + 2, down->bounds.y + 2).command.status ==
                        ui::UiCommandStatus::performed,
                    "Authored DownButton routes typed local article scroll");
            require(ui::click_composed_ui(scroll_host, scroll_frame, scroll_view,
                        down->bounds.x + 2, down->bounds.y + 2).command.status ==
                        ui::UiCommandStatus::invalid,
                    "Article button cannot replay a stale composed frame");
            scroll_frame = scroll_host.begin_frame(1280, 960);
            scroll_view = scroll_compositor.compose(scroll_frame,
                scroll_host.screens(), 1280, 960);
            require(scroll_frame.snapshot.find<ui::SystemScreenFrame>()->
                        pedia_article_first_line == 1 &&
                    article_body(scroll_view) != first_text,
                    "Next immutable frame advances the visible authored article text");
            const auto body = *scroll_view.civilopedia_article_scroll_panel;
            require(ui::wheel_composed_ui(scroll_host, scroll_frame, scroll_view,
                        body.x + 2, body.y + 2, -120).command.status ==
                        ui::UiCommandStatus::performed,
                    "Wheel inside authored article panel scrolls down");
            require(ui::wheel_composed_ui(scroll_host, scroll_frame, scroll_view,
                        body.x + 2, body.y + 2, -120).command.status ==
                        ui::UiCommandStatus::invalid,
                    "Article wheel cannot replay a stale composed frame");
            scroll_frame = scroll_host.begin_frame(1280, 960);
            scroll_view = scroll_compositor.compose(scroll_frame,
                scroll_host.screens(), 1280, 960);
            require(scroll_frame.snapshot.find<ui::SystemScreenFrame>()->
                        pedia_article_first_line == 2 &&
                    ui::wheel_composed_ui(scroll_host, scroll_frame, scroll_view,
                        body.x - 2, body.y + 2, -120).command.status ==
                        ui::UiCommandStatus::invalid,
                    "Wheel outside focused article does not reach world camera");
            auto end_state = *scroll_frame.snapshot.find<ui::SystemScreenFrame>();
            end_state.pedia_article_first_line =
                ui::civilopedia_article_max_scroll(end_state);
            require(ui::apply_civilopedia_action(end_state,
                        {ui::SystemPediaActionKind::article_scroll_down, {}}).status ==
                        ui::UiCommandStatus::unavailable &&
                    ui::apply_civilopedia_action(end_state,
                        {ui::SystemPediaActionKind::article_scroll_up, {}}).status ==
                        ui::UiCommandStatus::performed,
                    "Article scroll reports unavailable at end and can move upward");
            end_state.pedia_article_layout = {};
            require(ui::apply_civilopedia_action(end_state,
                        {ui::SystemPediaActionKind::article_scroll_down, {}}).status ==
                        ui::UiCommandStatus::unavailable &&
                    ui::civilopedia_article_layout(1920, 1080).visible_lines >
                        ui::civilopedia_article_layout(1280, 960).visible_lines,
                    "Missing layout cannot report success; taller viewport exposes more lines");
            const auto other = std::find_if(catalog->begin(), catalog->end(),
                [&](const auto &entry) { return entry.id != long_article->id; });
            require(other != catalog->end() &&
                    scroll_host.dispatch(*ui::system_control_command("CivilopediaScreen",
                        "PediaEntry:" + other->id), scroll_frame).status ==
                        ui::UiCommandStatus::performed,
                    "Changing article succeeds locally");
            scroll_frame = scroll_host.begin_frame(1280, 960);
            require(scroll_frame.snapshot.find<ui::SystemScreenFrame>()->
                        pedia_article_first_line == 0,
                    "Changing article resets its scroll position");
            require(scroll_host.dispatch(ui::UiCloseScreen{"CivilopediaScreen"},
                        scroll_frame).status == ui::UiCommandStatus::navigation,
                    "Close article view");
            scroll_frame = scroll_host.begin_frame(1280, 960);
            require(!scroll_host.wheel_civilopedia_article(scroll_frame,
                        body.x + 2, body.y + 2, -120, body).consumed,
                    "Hidden Civilopedia cannot own the wheel");
        }
        ui::LiveUiSources search_sources;
        search_sources.pedia_entries = &entries;
        ui::LiveUiHost search_host(registry, [&] { return frame; }, nullptr,
                                   search_sources);
        auto search_frame = search_host.begin_frame(1280, 960);
        require(search_host.dispatch(ui::UiOpenScreen{"CivilopediaScreen"},
                    search_frame).status == ui::UiCommandStatus::navigation,
                "Open Civilopedia for native text input");
        ui::UiScreenCompositor search_compositor(resources, registry);
        search_frame = search_host.begin_frame(1280, 960);
        auto search_view = search_compositor.compose(search_frame,
            search_host.screens(), 1280, 960);
        require(search_view.civilopedia_search_box.has_value(),
                "Visible SearchEditBox publishes its screen-space bounds");
        const auto box = *search_view.civilopedia_search_box;
        ui::CivilopediaSearchEditor editor;
        require(editor.select(search_host, search_frame, search_view,
                    box.x + 2, box.y + 2, box.x + 2, box.y + 2),
                "Press and release inside the search field grants edit focus");
        const std::array<char16_t, 3> russian{u'г', u'о', u'н'};
        require(editor.input(search_host, search_frame, search_view, russian).status ==
                    ui::UiCommandStatus::performed,
                "Russian WM_CHAR text routes through the typed live search command");
        require(editor.input(search_host, search_frame, search_view, russian).status ==
                    ui::UiCommandStatus::invalid,
                "Stale input frame cannot replay search text");
        const auto stale_button = std::find_if(search_view.hits.begin(),
            search_view.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay ? gameplay->template as<ui::SystemPediaAction>() : nullptr;
                return action && action->kind == ui::SystemPediaActionKind::submit_search;
            });
        require(stale_button != search_view.hits.end() &&
                ui::click_composed_ui(search_host, search_frame, search_view,
                    stale_button->bounds.x + 2,
                    stale_button->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::invalid,
                "SearchButton cannot replay a stale composed frame");
        require(!editor.select(search_host, search_frame, search_view,
                    box.x + 2, box.y + 2, box.x + 2, box.y + 2),
                "Stale composed frame cannot grant search focus");
        search_frame = search_host.begin_frame(1280, 960);
        search_view = search_compositor.compose(search_frame,
            search_host.screens(), 1280, 960);
        require(search_frame.snapshot.find<ui::SystemScreenFrame>()->pedia_search == "гон" &&
                std::any_of(search_view.paints.begin(), search_view.paints.end(),
                    [&](const auto &paint) { return paint.text == pottery->title; }),
                "Russian input filters articles in the next composed frame");
        const auto current_button = std::find_if(search_view.hits.begin(),
            search_view.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay ? gameplay->template as<ui::SystemPediaAction>() : nullptr;
                return action && action->kind == ui::SystemPediaActionKind::submit_search;
            });
        require(current_button != search_view.hits.end() && !current_button->enabled &&
                current_button->unavailable_reason.find("multiple authored matches") !=
                    std::string::npos,
                "Ambiguous live SearchButton is visibly unavailable");
        const auto *button_action = std::get<ui::UiGameplayCommand>(
            current_button->command).as<ui::SystemPediaAction>();
        require(button_action && button_action->article_id.empty(),
                "SearchButton submits the query from its immutable UI frame");
        require(!editor.select(search_host, search_frame, search_view,
                    current_button->bounds.x + 2, current_button->bounds.y + 2,
                    current_button->bounds.x + 2, current_button->bounds.y + 2) &&
                !editor.focused(), "Clicking SearchButton releases edit focus");
        require(ui::click_composed_ui(search_host, search_frame, search_view,
                    current_button->bounds.x + 2,
                    current_button->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::unavailable,
                "Ambiguous live SearchButton cannot claim navigation");
        search_frame = search_host.begin_frame(1280, 960);
        search_view = search_compositor.compose(search_frame,
            search_host.screens(), 1280, 960);
        require(search_frame.snapshot.find<ui::SystemScreenFrame>()->pedia_search == "гон" &&
                !search_frame.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id,
                "Ambiguous search preserves the visible filter without selecting an article");
        require(!editor.select(search_host, search_frame, search_view,
                    box.x + 2, box.y + 2, box.x + box.width + 1, box.y + 2) &&
                !editor.focused(),
                "Release outside SearchEditBox removes edit focus");
        require(editor.input(search_host, search_frame, search_view, russian).status ==
                    ui::UiCommandStatus::invalid,
                "Text input without edit focus is rejected");
        require(editor.select(search_host, search_frame, search_view,
                    box.x + 2, box.y + 2, box.x + 2, box.y + 2),
                "SearchEditBox can regain focus");
        const std::array<char16_t, 1> erase{u'\b'};
        require(editor.input(search_host, search_frame, search_view, erase).status ==
                    ui::UiCommandStatus::performed,
                "Backspace routes a complete UTF-8 character deletion");
        search_frame = search_host.begin_frame(1280, 960);
        require(search_frame.snapshot.find<ui::SystemScreenFrame>()->pedia_search == "го",
                "Backspace preserves Russian UTF-8 boundaries");
        require(search_host.search_civilopedia(pottery->title, search_frame).status ==
                    ui::UiCommandStatus::performed,
                "Complete localized title enters the live SearchEditBox state");
        search_frame = search_host.begin_frame(1280, 960);
        search_view = search_compositor.compose(search_frame,
            search_host.screens(), 1280, 960);
        const auto exact_button = std::find_if(search_view.hits.begin(),
            search_view.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay ?
                    gameplay->template as<ui::SystemPediaAction>() : nullptr;
                return action && action->kind ==
                    ui::SystemPediaActionKind::submit_search;
            });
        require(exact_button != search_view.hits.end() && exact_button->enabled &&
                ui::click_composed_ui(search_host, search_frame, search_view,
                    exact_button->bounds.x + 2,
                    exact_button->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::performed,
                "Measured SearchButton commits a unique active BNW title");
        search_frame = search_host.begin_frame(1280, 960);
        search_view = search_compositor.compose(search_frame,
            search_host.screens(), 1280, 960);
        require(search_frame.snapshot.find<ui::SystemScreenFrame>()->selected_pedia_id ==
                    std::optional<std::string>{"TECH_POTTERY"} &&
                search_frame.snapshot.find<ui::SystemScreenFrame>()->pedia_category_tab == 3 &&
                std::any_of(search_view.paints.begin(), search_view.paints.end(),
                    [&](const auto &paint) {
                        return paint.text.find(pottery->description.substr(0, 12)) !=
                            std::string::npos;
                    }),
                "Next composed frame visibly shows the selected article and tab");
        require(search_host.dispatch(ui::UiCloseScreen{"CivilopediaScreen"},
                    search_frame).status == ui::UiCommandStatus::navigation,
                "Close Civilopedia after native search");
        search_frame = search_host.begin_frame(1280, 960);
        search_view = search_compositor.compose(search_frame,
            search_host.screens(), 1280, 960);
        require(editor.input(search_host, search_frame, search_view, russian).status ==
                    ui::UiCommandStatus::invalid,
                "Closed Civilopedia cannot receive native text input");
        std::cout << "Civilopedia RU articles=" << entries.size()
                  << " technologies=" << std::count_if(entries.begin(), entries.end(),
                      [](const auto &entry) { return entry.category == "Technologies"; })
                  << " units=" << std::count_if(entries.begin(), entries.end(),
                      [](const auto &entry) { return entry.category == "Units"; }) << '\n';
        std::cout << "Civilopedia EN articles=" << english_entries.size()
                  << " technologies=" << std::count_if(english_entries.begin(), english_entries.end(),
                      [](const auto &entry) { return entry.category == "Technologies"; })
                  << " units=" << std::count_if(english_entries.begin(), english_entries.end(),
                      [](const auto &entry) { return entry.category == "Units"; }) << '\n';
        const auto linked = [](const auto &catalog) {
            return std::count_if(catalog.begin(), catalog.end(),
                [](const auto &entry) { return !entry.prerequisite_tech_ids.empty(); });
        };
        std::cout << "Civilopedia authored prerequisite links RU=" << linked(entries)
                  << " EN=" << linked(english_entries) << '\n';
        for (const auto &category : {"Buildings", "Wonders", "Projects", "Policies", "Improvements", "Religions", "Resolutions", "LeagueProjects",
                                     "Resources", "Terrains", "Features"}) {
            const auto count = [category](const auto &catalog) {
                return std::count_if(catalog.begin(), catalog.end(),
                    [category](const auto &entry) { return entry.category == category; });
            };
            std::cout << "Civilopedia " << category << " RU=" << count(entries)
                      << " EN=" << count(english_entries) << '\n';
        }
        std::cout << "PASS twelve active BNW system sources, routing, focus and no false success\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
