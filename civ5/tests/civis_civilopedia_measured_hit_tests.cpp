#include "civ5/ui/civis_civilopedia_measured_hit.hpp"
#include <iostream>
#include <stdexcept>

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
    measured.frame_number = 51;
    measured.owner_token = "CivilopediaScreen:home-5";
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
        sources, identity, 0, 123, 51, measured);
    std::string blocker;
    auto controls = civ5::ui::CivisCivilopediaControlTree::open(
        resources, sources, plan, measured, blocker);
    if (!controls) throw std::runtime_error(blocker);
    check(controls->apply_ordered_plan(plan.owner).completed_operations == 9,
          "Installed Civilopedia setter plan changed");
    auto home = civ5::ui::CivisHomeArticleSubtree::open(
        sources, *controls, plan.owner, 1, 1280, 960, blocker);
    if (!home) throw std::runtime_error(blocker);
    check(!civ5::ui::measure_civis_home_button_envelope(
              *home, plan.owner).available &&
          !civ5::ui::hit_test_civis_home_article(
              *home, plan.owner, 28, 31).matched_button,
          "Missing owner measurements must not create extent or hits");
    civ5::ui::CivisHomeArticleRowsMeasurement rows;
    rows.owner = plan.owner;
    rows.left_panel = {20, 30, 294, 834};
    rows.list_stack = {27, 30, 64, 64};
    for (int i = 0; i < 16; ++i)
        rows.button_bounds.push_back({27, 30 + 22 * i, 256, 22});
    check(home->attach_measured_rows(plan.owner, rows, blocker),
          "Owner-measured HomePage rows must bind to selected source");
    const auto extent = civ5::ui::measure_civis_home_button_envelope(
        *home, plan.owner);
    check(extent.available && extent.screen_bounds.x == 27 &&
          extent.screen_bounds.y == 30 && extent.screen_bounds.width == 256 &&
          extent.screen_bounds.height == 352 &&
          extent.height_below_stack_top == 352 &&
          !extent.native_internal_size_available &&
          extent.blocker.find("native") != std::string::npos,
          "Measured button envelope must not claim native ScrollPanel extent");
    const auto row5 = civ5::ui::hit_test_civis_home_article(
        *home, plan.owner, 28, 30 + 22 * 4 + 1);
    check(row5.inside_panel && row5.matched_button && row5.entry_id == 5 &&
          row5.text_key == "TXT_KEY_PEDIA_CATEGORY_5_LABEL" &&
          !row5.native_callback_available,
          "Measured hit must identify current row without enabling action");
    check(!civ5::ui::hit_test_civis_home_article(
              *home, plan.owner, 283, 31).matched_button &&
          !civ5::ui::hit_test_civis_home_article(
              *home, plan.owner, 28, 382).matched_button,
          "Button right/bottom edges must be exclusive");
    auto stale = plan.owner;
    stale.owner_token = "CivilopediaScreen:home-6";
    check(!civ5::ui::measure_civis_home_button_envelope(
              *home, stale).available &&
          !civ5::ui::hit_test_civis_home_article(
              *home, stale, 28, 31).matched_button,
          "Previous owner measurements must expire with token");
    std::cout << "Civis HomePage envelope=256x352 hit=category5 "
                 "native_internal_size=blocked callback=blocked\n";
}
