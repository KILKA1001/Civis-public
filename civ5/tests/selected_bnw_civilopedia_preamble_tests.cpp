#include "civ5/ui/selected_bnw_civilopedia_preamble.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

std::string read(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Selected Civilopedia fixture source is missing");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void source_only(const std::filesystem::path &root) {
    const auto ui = root / "Assets/DLC/Expansion2/UI/Civilopedia";
    auto source = read(ui / "CivilopediaScreen.lua");
    const auto xml = civ5::content::read_xml(ui / "CivilopediaScreen.xml");
    auto shape = civ5::ui::inspect_civilopedia_preamble_source(source, xml);
    check(shape.status == civ5::ui::SelectedBnwCivilopediaPreambleStatus::source_verified &&
          shape.first_native_control == "ListOfArticles" &&
          shape.first_native_line == 78 &&
          shape.authored_later_event_registrations == 5 &&
          shape.blocker.find("Native Controls") != std::string::npos,
          "Installed Expansion2 Civilopedia boundary or event order changed");
    const auto event = source.find("Events.SearchForPediaEntry.Add( SearchForPediaEntry );");
    check(event != std::string::npos, "Installed event registration missing");
    source.erase(event, std::string("Events.SearchForPediaEntry.Add( SearchForPediaEntry );").size());
    check(civ5::ui::inspect_civilopedia_preamble_source(source, xml).status ==
              civ5::ui::SelectedBnwCivilopediaPreambleStatus::source_blocked,
          "Missing authored event must block source-order proof");
    source = read(ui / "CivilopediaScreen.lua");
    const auto boundary = source.find("local g_ListItemManager = InstanceManager:new");
    check(boundary != std::string::npos, "Installed native boundary missing");
    source.insert(boundary, "Controls.UnverifiedNative:SetText('x');\n");
    check(civ5::ui::inspect_civilopedia_preamble_source(source, xml).status ==
              civ5::ui::SelectedBnwCivilopediaPreambleStatus::source_blocked,
          "Earlier native dependency must block source-order execution");
    source = read(ui / "CivilopediaScreen.lua");
    auto instance = read(root / "Assets/UI/InstanceManager.lua");
    const auto article = civ5::ui::inspect_civilopedia_article_list_control(
        source, xml, instance);
    if (!article.verified) throw std::runtime_error(article.blocker);
    check(article.verified && article.control_id == "ListOfArticles" &&
          article.control_kind == "Stack" && article.parent_id == "LeftScrollPanel" &&
          article.item_instance == "ListItemInstance" &&
          article.item_root == "ListItemButton" &&
          article.heading_instance == "ListHeadingInstance" &&
          article.heading_root == "ListHeadingButton" &&
          article.next_native_line == 80,
          "Installed Expansion2 article-list Controls contract changed");
    const auto heading = source.find("Controls.ListOfArticles );", boundary + 1);
    check(heading != std::string::npos, "Second article-list manager missing");
    source.replace(heading, std::string("Controls.ListOfArticles );").size(),
                   "Controls.OtherParent );");
    check(!civ5::ui::inspect_civilopedia_article_list_control(
              source, xml, instance).verified,
          "Changed authored parent control must block binding");
    source = read(ui / "CivilopediaScreen.lua");
    const auto constructor = instance.find("o.m_ParentControl    = ParentControl;");
    check(constructor != std::string::npos, "Stock InstanceManager constructor missing");
    instance.replace(constructor,
                     std::string("o.m_ParentControl    = ParentControl;").size(),
                     "ContextPtr:BuildInstance('unexpected',{});");
    check(!civ5::ui::inspect_civilopedia_article_list_control(
              source, xml, instance).verified,
          "Native effect in constructor must block identity-only token");
    instance = read(root / "Assets/UI/InstanceManager.lua");
    const auto prereq = civ5::ui::inspect_civilopedia_prereq_tech_control(
        source, xml, instance);
    if (!prereq.verified) throw std::runtime_error(prereq.blocker);
    check(prereq.control_id == "PrereqTechInnerFrame" &&
          prereq.control_kind == "Grid" &&
          prereq.parent_id == "PrereqTechFrame" &&
          prereq.instance_name == "PrereqTechInstance" &&
          prereq.instance_root == "PrereqTechButton" &&
          prereq.next_native_control == "GreatWorksInnerFrame" &&
          prereq.next_native_line == 81,
          "Installed Expansion2 prerequisite-tech Controls contract changed");
    const auto prereq_parent = source.find("Controls.PrereqTechInnerFrame );");
    check(prereq_parent != std::string::npos, "Prerequisite-tech manager missing");
    source.replace(prereq_parent,
                   std::string("Controls.PrereqTechInnerFrame );").size(),
                   "Controls.UnverifiedFrame );");
    check(!civ5::ui::inspect_civilopedia_prereq_tech_control(
              source, xml, instance).verified,
          "Changed prerequisite-tech parent must block binding");
    source = read(ui / "CivilopediaScreen.lua");
    const auto managers = civ5::ui::inspect_civilopedia_manager_block(
        source, xml, instance);
    if (!managers.verified) throw std::runtime_error(managers.blocker);
    check(managers.bindings.size() == 33 &&
          managers.bindings.front().lua_name == "g_ListItemManager" &&
          managers.bindings.front().parent_control == "ListOfArticles" &&
          managers.bindings.front().xml_owner_path.find("LeftScrollPanel") !=
              std::string::npos &&
          managers.bindings.back().lua_name == "g_ImprovementsManager" &&
          managers.next_native_line == 113,
          "Installed Expansion2 manager block or XML ownership changed");
    const auto middle = source.find("Controls.UnlockedUnitsInnerFrame );");
    check(middle != std::string::npos, "Middle manager control missing");
    source.replace(middle,
                   std::string("Controls.UnlockedUnitsInnerFrame );").size(),
                   "Controls.UnverifiedFrame );");
    check(!civ5::ui::inspect_civilopedia_manager_block(
              source, xml, instance).verified,
          "Unowned manager control must block full block binding");
    source = read(ui / "CivilopediaScreen.lua");
    const auto callback = civ5::ui::inspect_civilopedia_first_callback(
        source, xml, instance);
    if (!callback.source_verified) throw std::runtime_error(callback.blocker);
    check(callback.first_callback_line == 169 &&
          callback.authored_category_buttons == 16 &&
          callback.required_mouse_event == "Mouse.eLClick" &&
          callback.blocker.find("native VertSlider width") != std::string::npos,
          "Selected Civilopedia callback/native layout boundary changed");
    const auto registration = source.find(
        "Controls[buttonName]:RegisterCallback( Mouse.eLClick, "
        "CivilopediaCategory[i].buttonClicked );");
    check(registration != std::string::npos, "Selected first callback missing");
    source.replace(registration, std::string("Controls[buttonName]").size(),
                   "Controls[otherName]");
    check(!civ5::ui::inspect_civilopedia_first_callback(
              source, xml, instance).source_verified,
          "Changed callback target must block source proof");
}
}

int main(int argc, char **argv) {
#ifdef CIVIS_SOURCE_ONLY
    if (argc != 2) throw std::runtime_error("Expected game root");
    try { source_only(argv[1]); }
    catch (const std::exception &error) {
        std::cerr << "Civilopedia source-only fixture: " << error.what() << '\n';
        return 1;
    }
    return 0;
#else
    if (argc != 2 && argc != 3)
        throw std::runtime_error("Expected game root, or Gandhi save and game root");
    const std::filesystem::path root(argv[argc - 1]);
    source_only(root);
    if (argc == 2) return 0;
    auto selected = civ5::ui::bootstrap_selected_bnw_preview(argv[1], root, 0);
    check(static_cast<bool>(selected.preview), "Selected Gandhi host must restore");
    const auto catalog = civ5::content::discover_packages({root / "Assets/DLC"});
    const auto &state = selected.preview.host->session().state();
    const auto queue_before = state.player_research.players.at(0).queue;
    const auto gameplay_rng_before = state.random.gameplay.state().calls;
    const auto map_rng_before = state.random.map.state().calls;
    auto result = civ5::ui::execute_selected_bnw_manager_block_binding(
        root, catalog, selected.profile, *selected.preview.host);
    check(result.status == civ5::ui::SelectedBnwCivilopediaPreambleStatus::manager_block_bound &&
          result.context_source_sha256.size() == 64 &&
          result.xml_sha256.size() == 64 &&
          result.first_native_control == "ListOfArticles" &&
          result.authored_later_event_registrations == 5 &&
          result.owner_player_id == 0 &&
          result.owner_game_turn == state.setup.game_turn &&
          result.owner_payload_sha256 == selected.preview.host->identity().payload_sha256 &&
          result.owner_rules_sha256 == selected.preview.host->identity().rules_sha256 &&
          result.observed_include_order == std::vector<std::string>{
              "Assets/UI/InstanceManager.lua", "Assets/UI/IconSupport.lua",
              "Assets/Gameplay/Lua/FLuaVector.lua"} &&
          result.bound_article_list_managers == 2 &&
          result.bound_prereq_tech_managers == 1 &&
          result.bound_manager_count == 33 &&
          result.next_native_control == "UIManager.GetScreenSizeVal" &&
          !result.live_context_published &&
          result.blocker.find("UIManager.GetScreenSizeVal") != std::string::npos,
          "Selected Civilopedia manager block must stop before native layout");
    check(state.player_research.players.at(0).queue == queue_before &&
          state.random.gameplay.state().calls == gameplay_rng_before &&
          state.random.map.state().calls == map_rng_before &&
          !selected.preview.host->session().readiness().can_continue(),
          "Diagnostic Civilopedia VM must not mutate live research, RNG or readiness");
    std::cout << "Civilopedia preamble source=" << result.context_source_sha256
              << " xml=" << result.xml_sha256
              << " first_native_line=" << result.first_native_line
              << " bound_article_list_managers=" << result.bound_article_list_managers
              << " bound_prereq_tech_managers=" << result.bound_prereq_tech_managers
              << " bound_manager_count=" << result.bound_manager_count
              << " context_event_registrations=0/"
              << result.authored_later_event_registrations
              << " full_context=blocked\n";
#endif
}
