#include "civ5/ui/civis_civilopedia_articles.hpp"
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
    const auto article_source = civ5::ui::inspect_bnw_home_article_source(sources);
    if (!article_source.verified) throw std::runtime_error(article_source.blocker);
    auto base = std::make_shared<civ5::assets::LooseResourceIndex>(root);
    base->add_file(root / "Assets/UI/Styles.xml");
    civ5::assets::ResourceMountTable resources(
        civ5::assets::resource_lookup(base),
        civ5::assets::resource_lookup_all(base));
    civ5::runtime::SessionCheckpointIdentity identity{
        std::string(64, 'a'), std::string(64, 'b')};
    civ5::ui::CivilopediaMeasuredLayout measured;
    measured.frame_number = 44;
    measured.owner_token = "CivilopediaScreen:home-4";
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
        sources, identity, 0, 123, 44, measured);
    check(plan.source_and_owner_verified, "Civilopedia plan must be source-owned");
    std::string blocker;
    auto controls = civ5::ui::CivisCivilopediaControlTree::open(
        resources, sources, plan, measured, blocker);
    if (!controls) throw std::runtime_error(blocker);
    const auto layout = controls->apply_ordered_plan(plan.owner);
    check(!layout.applied && layout.completed_operations == 9,
          "Civis control tree must stop before native internal-size calculation");
    auto subtree = civ5::ui::CivisHomeArticleSubtree::open(
        sources, *controls, plan.owner, 1, 1280, 960, blocker);
    if (!subtree) throw std::runtime_error(blocker);
    check(subtree->item_count() == 16 && !subtree->measured_rows_available() &&
          !subtree->native_callbacks_available(),
          "HomePage must create 16 Civis instances without invented hit geometry");
    const auto stack = subtree->list_stack(plan.owner);
    check(stack.available && stack.geometry.id == "ListOfArticles" &&
          stack.geometry.parent_id == "LeftScrollPanel" &&
          stack.geometry.width == 64 && stack.geometry.height == 64 &&
          stack.geometry.offset_x == 7 && stack.geometry.offset_y == 0,
          "Civis subtree must own the authored ListOfArticles root Stack");
    for (int id = 1; id <= 16; ++id) {
        const auto entry = subtree->item(plan.owner, id);
        check(entry.available && entry.item.entry_id == id &&
              entry.item.text_key ==
                  "TXT_KEY_PEDIA_CATEGORY_" + std::to_string(id) + "_LABEL" &&
              entry.item.parent_control == "ListOfArticles" &&
              entry.item.button.width == 256 && entry.item.button.height == 22 &&
              entry.item.label.id == "ListItemLabel" &&
              entry.item.label.parent_id == "ListItemButton" &&
              entry.item.label.offset_x == 12 &&
              entry.item.label.offset_y == -2 &&
              !entry.item.measured_button_bounds,
              "Civis Button/Label instances must follow selected HomePage source");
    }
    check(!civ5::ui::CivisHomeArticleSubtree::open(
              sources, *controls, plan.owner, 3, 1280, 960, blocker),
          "A different selected category must not reuse HomePage children");
    auto stale = plan.owner;
    stale.frame_number = 45;
    check(!subtree->item(stale, 1).available,
          "HomePage subtree must expire at the next frame");
    check(!subtree->list_stack(stale).available,
          "ListOfArticles root must expire with its owner frame");
    civ5::ui::CivisHomeArticleRowsMeasurement rows;
    rows.owner = plan.owner;
    rows.left_panel = {20, 30, 294, 834};
    rows.list_stack = {27, 30, 64, 64};
    for (int i = 0; i < 16; ++i)
        rows.button_bounds.push_back({27, 30 + 22 * i, 256, 22});
    check(!subtree->attach_measured_rows(stale, rows, blocker) &&
          !subtree->measured_rows_available(),
          "Stale owner must not publish measured row geometry");
    auto shifted = rows;
    shifted.button_bounds[4].y -= 22;
    check(!subtree->attach_measured_rows(plan.owner, shifted, blocker) &&
          !subtree->measured_rows_available(),
          "Overlapping measured rows must be rejected atomically");
    check(subtree->attach_measured_rows(plan.owner, rows, blocker) &&
          subtree->measured_rows_available() &&
          subtree->item(plan.owner, 16).item.measured_button_bounds->y == 360 &&
          !subtree->native_callbacks_available(),
          "Measured rows may bind geometry without enabling native callbacks");
    std::cout << "Civis HomePage instances=16 measured_rows=16 "
                 "calculate_internal_size=blocked callbacks=blocked\n";
}
