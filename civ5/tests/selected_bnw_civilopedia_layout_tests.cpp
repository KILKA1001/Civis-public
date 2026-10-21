#include "civ5/ui/selected_bnw_civilopedia_layout.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
civ5::content::XmlNode *find_id(civ5::content::XmlNode &node,
                                 const std::string &id) {
    if (node.attribute("ID") == id) return &node;
    for (auto &child : node.children)
        if (auto *found = find_id(child, id)) return found;
    return nullptr;
}
}

int main(int argc, char **argv) {
    if (argc != 2) throw std::runtime_error("Expected installed game root");
    const auto sources = civ5::ui::inspect_bnw_civilopedia_layout_sources(argv[1]);
    if (!sources.verified) throw std::runtime_error(sources.blocker);
    check(sources.control_tree.verified && sources.control_tree.controls.size() == 8,
          "Selected Civilopedia native control tree must be source-owned");
    const auto root = std::filesystem::path(argv[1]);
    auto base_styles = std::make_shared<civ5::assets::LooseResourceIndex>(root);
    base_styles->add_file(root / "Assets/UI/Styles.xml");
    civ5::assets::ResourceMountTable selected_styles(
        civ5::assets::resource_lookup(base_styles),
        civ5::assets::resource_lookup_all(base_styles));
    const auto vfs_style = civ5::ui::resolve_bnw_civilopedia_vert_slider_style(
        selected_styles, sources);
    check(vfs_style.selected_source_verified &&
          vfs_style.sha256 == sources.styles_sha256 &&
          vfs_style.authored_vert_slider_width == 18 &&
          vfs_style.source_relative_path == "Assets/UI/Styles.xml" &&
          vfs_style.blocker.find("GetSize") != std::string::npos,
          "Selected base VFS must pin VertSlider source without native GetSize");
    const auto temp = std::filesystem::temp_directory_path() /
        ("civis_civilopedia_styles_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto override_file = temp / "Assets/UI/Styles.xml";
    std::filesystem::create_directories(override_file.parent_path());
    {
        std::ofstream output(override_file);
        output << "<StyleSheet><VertSlider Size=\"24,24\" Vertical=\"1\"/>"
                  "</StyleSheet>";
    }
    auto override_styles = std::make_shared<civ5::assets::LooseResourceIndex>(temp);
    override_styles->add_file(override_file);
    selected_styles.mount({1, 0, 0, civ5::assets::resource_lookup(override_styles),
                           true, true,
                           civ5::assets::resource_lookup_all(override_styles),
                           civ5::assets::resource_keys(override_styles)});
    check(!civ5::ui::resolve_bnw_civilopedia_vert_slider_style(
              selected_styles, sources).selected_source_verified,
          "Unverified selected UI style override must block native width");
    std::filesystem::remove(override_file);
    std::filesystem::remove(override_file.parent_path());
    std::filesystem::remove(temp / "Assets");
    std::filesystem::remove(temp);
    auto xml = civ5::content::read_xml(
        root /
        "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml");
    check(civ5::ui::inspect_bnw_civilopedia_native_control_tree(xml).verified,
          "Installed native control tree changed");
    auto duplicate = xml;
    auto *panel = find_id(duplicate, "ScrollPanel");
    auto *bar = find_id(duplicate, "ScrollBar");
    check(panel && bar, "Installed scrollbar controls are missing");
    const auto bar_copy = *bar;
    panel->children.push_back(bar_copy);
    check(!civ5::ui::inspect_bnw_civilopedia_native_control_tree(duplicate).verified,
          "Duplicate XML control ID must block native binding");
    auto wrong_style = xml;
    bar = find_id(wrong_style, "ScrollBar");
    check(bar, "Installed scrollbar style control is missing");
    for (auto &attribute : bar->attributes)
        if (attribute.first == "Style") attribute.second = "UnverifiedStyle";
    check(!civ5::ui::inspect_bnw_civilopedia_native_control_tree(wrong_style).verified,
          "Unknown scrollbar style must block native binding");
    auto wrong_parent = xml;
    panel = find_id(wrong_parent, "ScrollPanel");
    auto *left_panel = find_id(wrong_parent, "LeftScrollPanel");
    check(panel && left_panel, "Installed scroll panels are missing");
    const auto moved = std::find_if(panel->children.begin(), panel->children.end(),
        [](const auto &child) { return child.attribute("ID") == "DownButton"; });
    check(moved != panel->children.end(), "Installed DownButton is missing");
    const auto moved_button = *moved;
    panel->children.erase(moved);
    left_panel->children.push_back(moved_button);
    check(!civ5::ui::inspect_bnw_civilopedia_native_control_tree(wrong_parent).verified,
          "XML control reparenting must block native binding");
    check(sources.civilopedia_sha256.size() == 64 &&
          sources.xml_sha256.size() == 64 &&
          sources.instance_manager_sha256.size() == 64 &&
          sources.styles_sha256.size() == 64 &&
          sources.authored_vert_slider_width == 18 &&
          sources.blocker.find("unverified") != std::string::npos,
          "Installed Civilopedia/Styles provenance changed");
    civ5::runtime::SessionCheckpointIdentity identity{"payload", "rules"};
    civ5::ui::CivilopediaMeasuredLayout observed;
    observed.frame_number = 17;
    observed.owner_token = "CivilopediaScreen:event-91";
    observed.player_id = 0;
    observed.game_turn = 42;
    observed.payload_sha256 = identity.payload_sha256;
    observed.rules_sha256 = identity.rules_sha256;
    observed.resolved_styles_sha256 = sources.styles_sha256;
    observed.viewport_width = 1280;
    observed.viewport_height = 960;
    observed.scrollbar_width = 23; // Explicit synthetic observation, not XML Size.
    observed.scroll_panel_width = 1002;
    observed.scroll_panel_height = 980;
    observed.left_panel_width = 294;
    observed.left_panel_height = 980;
    const auto plan = civ5::ui::plan_bnw_civilopedia_native_layout(
        sources, identity, 0, 42, 17, observed);
    check(plan.source_and_owner_verified && !plan.native_layout_executed &&
          !plan.callbacks_registered && plan.instructions.size() == 11 &&
          plan.instructions[0].control_id == "ScrollBar" &&
          plan.instructions[0].x == 23 && plan.instructions[0].y == 788 &&
          plan.instructions[0].authored_line == 117 &&
          plan.instructions[4].control_id == "DownButton" &&
          plan.instructions[4].y == 816 &&
          plan.instructions[7].control_id == "ScrollPanel" &&
          plan.instructions[7].y == 834 &&
          plan.instructions[9].operation ==
              civ5::ui::CivilopediaLayoutOperation::calculate_internal_size &&
          plan.instructions[9].control_id == "ScrollPanel" &&
          plan.instructions[10].control_id == "LeftScrollPanel" &&
          plan.blocker.find("CalculateInternalSize") != std::string::npos,
          "Measured-layout plan must preserve authored order and native blocker");
    auto stale = observed;
    stale.game_turn = 41;
    check(!civ5::ui::plan_bnw_civilopedia_native_layout(
              sources, identity, 0, 42, 17, stale).source_and_owner_verified,
          "Stale owner turn must block layout preflight");
    stale = observed;
    stale.frame_number = 16;
    check(!civ5::ui::plan_bnw_civilopedia_native_layout(
              sources, identity, 0, 42, 17, stale).source_and_owner_verified,
          "Stale measured frame must block layout preflight");
    stale = observed;
    stale.resolved_styles_sha256 = "other-style";
    check(!civ5::ui::plan_bnw_civilopedia_native_layout(
              sources, identity, 0, 42, 17, stale).source_and_owner_verified,
          "Different selected style provenance must block preflight");
    stale = observed;
    stale.owner_token.clear();
    check(!civ5::ui::plan_bnw_civilopedia_native_layout(
              sources, identity, 0, 42, 17, stale).source_and_owner_verified,
          "Unowned viewport measurement must block layout preflight");
    civ5::ui::CivilopediaNativeLayoutReceipt receipt;
    receipt.owner = plan.owner;
    for (const auto &instruction : plan.instructions)
        receipt.operations.push_back({instruction, true});
    receipt.internal_size_results = {{{"ScrollPanel", true},
                                      {"LeftScrollPanel", true}}};
    const auto valid = civ5::ui::validate_bnw_civilopedia_layout_receipt(
        plan, plan.owner, receipt);
    check(valid.receipt_matches_plan && !valid.native_controls_available &&
          valid.blocker.find("remain unavailable") != std::string::npos,
          "Exact owner receipt must remain a structural check only");
    auto current = plan.owner;
    current.frame_number = 18;
    check(!civ5::ui::validate_bnw_civilopedia_layout_receipt(
              plan, current, receipt).receipt_matches_plan,
          "Stale frame receipt must be rejected");
    current = plan.owner;
    current.owner_token = "CivilopediaScreen:event-92";
    check(!civ5::ui::validate_bnw_civilopedia_layout_receipt(
              plan, current, receipt).receipt_matches_plan,
          "Wrong owner token must be rejected");
    auto wrong_receipt = receipt;
    wrong_receipt.owner.player_id = 1;
    check(!civ5::ui::validate_bnw_civilopedia_layout_receipt(
              plan, plan.owner, wrong_receipt).receipt_matches_plan,
          "Receipt issued for another player must be rejected");
    auto reordered = receipt;
    std::swap(reordered.operations[0], reordered.operations[1]);
    check(!civ5::ui::validate_bnw_civilopedia_layout_receipt(
              plan, plan.owner, reordered).receipt_matches_plan,
          "Reordered native calls must be rejected");
    auto incomplete = receipt;
    incomplete.internal_size_results[1].completed = false;
    check(!civ5::ui::validate_bnw_civilopedia_layout_receipt(
              plan, plan.owner, incomplete).receipt_matches_plan,
          "Both CalculateInternalSize results are required");
    std::cout << "Civilopedia layout source=" << sources.civilopedia_sha256
              << " style_candidate=" << sources.styles_sha256
              << " native_layout=blocked callbacks=blocked\n";
}
