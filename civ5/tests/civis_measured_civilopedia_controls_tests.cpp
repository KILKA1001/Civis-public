#include "civ5/ui/civis_measured_civilopedia_controls.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5::ui;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
}
int main(int argc, char **argv) {
    if (argc != 2) throw std::runtime_error("Expected installed game root");
    const auto root = std::filesystem::path(argv[1]);
    const auto sources = inspect_bnw_civilopedia_layout_sources(root);
    if (!sources.verified) throw std::runtime_error(sources.blocker);
    auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
    base->add_file(root / "Assets/UI/Styles.xml");
    civ5::assets::ResourceMountTable resources(
        civ5::assets::resource_lookup(base), civ5::assets::resource_lookup_all(base));
    const civ5::runtime::SessionCheckpointIdentity identity{std::string(64, 'a'), std::string(64, 'b')};
    CivilopediaMeasuredLayout measured;
    measured.frame_number = 31; measured.owner_token = "CivilopediaScreen:test-resolved-child-owner";
    measured.player_id = 0; measured.game_turn = 123;
    measured.payload_sha256 = identity.payload_sha256; measured.rules_sha256 = identity.rules_sha256;
    measured.resolved_styles_sha256 = sources.styles_sha256;
    measured.viewport_width = 1280; measured.viewport_height = 960;
    measured.scrollbar_width = 18; measured.scroll_panel_width = 1002; measured.scroll_panel_height = 980;
    measured.left_panel_width = 294; measured.left_panel_height = 980;
    const auto plan = plan_bnw_civilopedia_native_layout(sources, identity, 0, 123, 31, measured);
    std::string blocker;
    auto tree = CivisMeasuredCivilopediaControlTree::open(resources, sources, plan, measured,
        CivisUiExecutionScope::diagnostic, blocker);
    if (!tree) throw std::runtime_error(blocker);
    check(!tree->calculate_internal_size(plan.owner, "ScrollPanel").applied &&
          tree->completed_operations() == 0, "Measured call cannot skip source setters");
    const auto prefix = tree->apply_ordered_plan(plan.owner);
    check(!prefix.applied && prefix.completed_operations == 9 &&
          prefix.blocker.find("renderer measurements") != std::string::npos,
          "Source setters execute and missing measurements remain an actual blocker");
    // Resolved renderer inputs below are deliberate test measurements. They do
    // not assert the original game's child anchoring or visibility defaults.
    for (const auto id : {"ScrollPanel", "LeftScrollPanel"}) {
        const auto size = tree->get_size(plan.owner, id);
        CivisMeasuredScrollSnapshot snapshot;
        snapshot.owner = plan.owner; snapshot.panel_id = id; snapshot.geometry_revision = 1;
        snapshot.viewport_width = size.width; snapshot.viewport_height = size.height;
        snapshot.participants_complete = true;
        snapshot.children = {{"ResolvedVisibleRoot", true, CivisMeasuredRect{0, 0, 200, 1200}},
                             {"ResolvedHiddenRoot", false, {}}};
        auto wrong_size = snapshot; ++wrong_size.viewport_height;
        check(!tree->publish(plan.owner, wrong_size).applied,
              "Source-measured viewport mismatch cannot publish child geometry");
        check(tree->publish(plan.owner, std::move(snapshot)).applied,
              "Current complete measured children must publish");
    }
    check(!tree->calculate_internal_size(plan.owner, "LeftScrollPanel").applied &&
          tree->completed_operations() == 9 &&
          !tree->scroll_state(plan.owner, "LeftScrollPanel").applied,
          "Second internal calculation cannot execute before first");
    const auto applied = tree->apply_ordered_plan(plan.owner);
    check(applied.applied && applied.completed_operations == 11 &&
          tree->geometry_prefix().completed_operations() == 9 && !tree->native_parity_verified(),
          "Both measured calculations execute; diagnostic native tree stays honest");
    check(tree->scroll_state(plan.owner, "ScrollPanel").state.content_height == 1200 &&
          tree->scroll_state(plan.owner, "ScrollPanel").state.max_scroll_y == 366 &&
          tree->set_scroll(plan.owner, "ScrollPanel", 0, 999).applied &&
          tree->scroll_state(plan.owner, "ScrollPanel").state.scroll_y == 366,
          "Bridge must retain calculated content and executable scroll state");
    auto stale = plan.owner; ++stale.frame_number;
    check(!tree->apply_ordered_plan(stale).applied &&
          !tree->calculate_internal_size(stale, "ScrollPanel").applied &&
          !tree->scroll_state(stale, "ScrollPanel").applied,
          "Completed measured adapter still rejects stale frames");
    std::cout << "Installed BNW source layout setters=9 measured_calculations=2 completed=11 native_parity=unverified\n";
}
