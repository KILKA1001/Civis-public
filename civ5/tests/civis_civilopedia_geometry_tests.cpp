#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char **argv) {
    if (argc != 2) throw std::runtime_error("Expected installed game root");
    const auto root = std::filesystem::path(argv[1]);
    const auto sources = civ5::ui::inspect_bnw_civilopedia_layout_sources(root);
    if (!sources.verified) throw std::runtime_error(sources.blocker);
    auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
    base->add_file(root / "Assets/UI/Styles.xml");
    civ5::assets::ResourceMountTable resources(
        civ5::assets::resource_lookup(base),
        civ5::assets::resource_lookup_all(base));
    civ5::runtime::SessionCheckpointIdentity identity{
        std::string(64, 'a'), std::string(64, 'b')};
    civ5::ui::CivilopediaMeasuredLayout measured;
    measured.frame_number = 31;
    measured.owner_token = "CivilopediaScreen:visible-event-7";
    measured.player_id = 0;
    measured.game_turn = 123;
    measured.payload_sha256 = identity.payload_sha256;
    measured.rules_sha256 = identity.rules_sha256;
    measured.resolved_styles_sha256 = sources.styles_sha256;
    measured.viewport_width = 1280;
    measured.viewport_height = 960;
    measured.scrollbar_width = 18;
    measured.scroll_panel_width = 1002;
    measured.scroll_panel_height = 980;
    measured.left_panel_width = 294;
    measured.left_panel_height = 980;
    const auto plan = civ5::ui::plan_bnw_civilopedia_native_layout(
        sources, identity, 0, 123, 31, measured);
    check(plan.source_and_owner_verified && plan.instructions.size() == 11,
          "Installed Civilopedia layout source must admit measured fixture");
    std::string blocker;
    auto tree = civ5::ui::CivisCivilopediaControlTree::open(
        resources, sources, plan, measured, blocker);
    if (!tree) throw std::runtime_error(blocker);
    const auto initial_bar = tree->get_size(plan.owner, "ScrollBar");
    const auto initial_button = tree->get_size(plan.owner, "LeftDownButton");
    check(initial_bar.available && initial_bar.width == 18 &&
          initial_bar.height == 18 && initial_button.available &&
          initial_button.width == 18 && initial_button.height == 18,
          "Civis controls must begin from verified selected style size");
    check(!tree->set_offset(plan.owner, "ScrollBar", 0, 18).applied &&
          tree->completed_operations() == 0,
          "Out-of-order setter must not mutate control geometry");
    check(tree->set_size(plan.owner, "ScrollBar", 18, 788).applied &&
          tree->set_size(plan.owner, "LeftScrollBar", 18, 788).applied &&
          tree->set_offset(plan.owner, "ScrollBar", 0, 18).applied &&
          tree->completed_operations() == 3,
          "Civis SetSize and SetOffsetVal must mutate exact authored controls");
    const auto applied = tree->apply_ordered_plan(plan.owner);
    check(!applied.applied && applied.completed_operations == 9 &&
          applied.blocker.find("CalculateInternalSize") != std::string::npos,
          "Ordered Civis setters must stop at original internal-size boundary");
    const auto bar = tree->get_geometry(plan.owner, "ScrollBar");
    const auto down = tree->get_geometry(plan.owner, "DownButton");
    const auto panel = tree->get_size(plan.owner, "ScrollPanel");
    const auto left = tree->get_size(plan.owner, "LeftScrollPanel");
    check(bar.available && bar.geometry.width == 18 &&
          bar.geometry.height == 788 && bar.geometry.offset_y == 18 &&
          down.available && down.geometry.offset_y == 816 &&
          panel.available && panel.width == 1002 && panel.height == 834 &&
          left.available && left.width == 294 && left.height == 834,
          "Civis control objects must retain all nine authored mutations");
    auto stale = plan.owner;
    stale.frame_number = 32;
    check(!tree->get_size(stale, "ScrollBar").available &&
          !tree->apply_ordered_plan(stale).applied &&
          tree->completed_operations() == 9,
          "Old control tree must reject next-frame access and mutation");
    stale = plan.owner;
    stale.owner_token = "CivilopediaScreen:visible-event-8";
    check(!tree->get_size(stale, "ScrollBar").available &&
          !tree->calculate_internal_size(stale, "ScrollPanel").applied,
          "Control tree must reject a different popup owner token");
    auto reordered = plan;
    std::swap(reordered.instructions[0], reordered.instructions[1]);
    check(!civ5::ui::CivisCivilopediaControlTree::open(
              resources, sources, reordered, measured, blocker),
          "Tampered setter sequence must not open a Civis control tree");
    std::cout << "Civis Civilopedia setters=9 internal_size=blocked "
                 "native_parity=unverified\n";
}
