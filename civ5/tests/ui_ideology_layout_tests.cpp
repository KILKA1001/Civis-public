#include "civ5/ui/ideology_layout.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/top_panel.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
std::size_t ideology_hits(const civ5::ui::IdeologyLayoutPlan &plan) {
    return std::count_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        const auto *gameplay = std::get_if<civ5::ui::UiGameplayCommand>(&hit.command);
        return gameplay && gameplay->template as<civ5::ui::ProgressionChooseIdeology>();
    });
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        check(argc == 3, "Supply selected Gandhi save and original game root");
        const std::filesystem::path game_root(argv[2]);
        const auto scan = [&](const std::filesystem::path &directory) {
            auto index = std::make_shared<assets::LooseResourceIndex>(game_root);
            index->scan({directory, {}, false});
            return index;
        };
        const auto base = scan(game_root / "Assets/UI");
        const auto expansion = scan(game_root / "Assets/DLC/Expansion/UI");
        const auto bnw = scan(game_root / "Assets/DLC/Expansion2/UI");
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
        const auto source = ui::read_progression_screen_source(
            resources, ui::ProgressionScreen::ideology);
        check(source.selected_source_path.find("Expansion2") != std::string::npos,
              "Ideology layout did not select the BNW source");

        ui::ProgressionFrameState state;
        state.player_id = 0;
        state.team_id = 0;
        state.branches = {
            {7, "POLICY_BRANCH_AUTOCRACY", {}, "Autocracy", true},
            {8, "POLICY_BRANCH_FREEDOM", {}, "Freedom", true},
            {9, "POLICY_BRANCH_ORDER", {}, "Order", true}};
        state.policies = {
            {11, "POLICY_AUTOCRACY_TEST_1", "POLICY_BRANCH_AUTOCRACY", {}, {}, 0, 0, false},
            {12, "POLICY_AUTOCRACY_TEST_2", "POLICY_BRANCH_AUTOCRACY", {}, {}, 0, 0, false}};
        auto frame = ui::UiFrameSnapshot(55, 0, 10).with<ui::ProgressionFrameState>(
            std::make_shared<const ui::ProgressionFrameState>(state));
        const auto static_plan = ui::build_ideology_layout(source, frame, 1280, 960);
        check(static_plan.panel && static_plan.scroll_panel && static_plan.close_button &&
              static_plan.hits.size() == 1 && ideology_hits(static_plan) == 0 &&
              static_plan.scroll_panel->x == 0 && static_plan.scroll_panel->y == 198 &&
              static_plan.scroll_panel->width == 436 &&
              static_plan.scroll_panel->height == 710 &&
              static_plan.close_button->x == 80 &&
              static_plan.close_button->y == 919 &&
              static_plan.close_button->width == 250 &&
              static_plan.close_button->height == 36,
              "Selected XML static panel/Close geometry or absent-event gate differs");
        frame = frame.with<ui::ProgressionIdeologyPopupEvent>(
            std::make_shared<const ui::ProgressionIdeologyPopupEvent>(
                ui::ProgressionIdeologyPopupEvent{0, 27}));
        const auto no_counts = ui::build_ideology_layout(source, frame, 1280, 960);
        check(no_counts.hits.size() == 1 && ideology_hits(no_counts) == 0,
              "Popup event without owner adopter counts created ideology hits");
        ui::IdeologyLayoutEvidence evidence;
        evidence.player_id = 0;
        evidence.turn = 10;
        evidence.frame = 55;
        evidence.event_token = 27;
        evidence.selected_source_path = source.selected_source_path;
        evidence.branches = {{7, 0}, {8, 13}, {9, 24}};
        auto owned = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(evidence));
        const auto laid_out = ui::build_ideology_layout(source, owned, 1280, 960);
        check(ideology_hits(laid_out) == 3 && laid_out.hits.size() == 4,
              "Owner-scoped event did not place the three BNW ideology branches");
        check(std::none_of(laid_out.paints.begin(), laid_out.paints.end(),
            [](const auto &paint) {
                return paint.text.find("Early Adopter Free Tenets") !=
                    std::string::npos;
            }) && std::any_of(laid_out.unresolved.begin(), laid_out.unresolved.end(),
            [](const auto &reason) {
                return reason.find("Game.GetNumFreePolicies") != std::string::npos;
            }), "Missing GameCore free-policy values were treated as zero");
        auto with_free = evidence;
        with_free.free_policy_counts = std::vector<ui::IdeologyFreePolicyCount>{
            {9, 1}, {7, 0}, {8, 2}};
        const auto free_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(with_free));
        const auto localize_free = [](std::string_view key)
            -> std::optional<std::string> {
            if (key == "TXT_KEY_CHOOSE_IDEOLOGY_NUM_FREE_POLICIES")
                return "Early Adopter Free Tenets: {1_NumFreePolicies}";
            return std::nullopt;
        };
        const auto free_plan = ui::build_ideology_layout(source, free_frame,
            1280, 960, nullptr, localize_free);
        const auto free_labels = std::count_if(free_plan.paints.begin(),
            free_plan.paints.end(), [](const auto &paint) {
                return paint.text.find("Early Adopter Free Tenets") !=
                    std::string::npos;
            });
        check(free_labels == 3 && std::any_of(free_plan.paints.begin(),
            free_plan.paints.end(), [](const auto &paint) {
                return paint.text == "Early Adopter Free Tenets: 0" &&
                    paint.destination.x == 28 && paint.destination.y == 228 &&
                    paint.destination.width == 380 &&
                    paint.destination.height == 28 &&
                    paint.style == "TwCenMT16";
            }) && std::any_of(free_plan.paints.begin(), free_plan.paints.end(),
            [](const auto &paint) {
                return paint.text == "[COLOR_POSITIVE_TEXT]Early Adopter Free Tenets: 2[ENDCOLOR]";
            }), "Owner GameCore values did not use selected FreePolicies label geometry");
        auto wrong_free = with_free;
        wrong_free.free_policy_counts->back().branch_id = 7;
        const auto wrong_free_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(wrong_free));
        const auto wrong_free_plan = ui::build_ideology_layout(source,
            wrong_free_frame, 1280, 960, nullptr, localize_free);
        check(std::none_of(wrong_free_plan.paints.begin(), wrong_free_plan.paints.end(),
            [](const auto &paint) {
                return paint.text.find("Early Adopter Free Tenets") !=
                    std::string::npos;
            }), "Duplicate free-policy branch published labels");
        ui::LiveUiFrame::IdeologyView view{0, -1, 10, 1280, 960, 27,
            source.selected_source_path, 0, 0,
            ui::LiveUiFrame::IdeologyOverlay::none, {}};
        const auto navigable = ui::build_ideology_layout(source, owned, 1280, 960, &view);
        check(navigable.scroll_region && navigable.hits.size() == 7 &&
              navigable.tooltip_keys.size() == 3 &&
              navigable.tooltip_keys.front().key == "TXT_KEY_VP_DIPLO_DETAILS" &&
              navigable.hits[1].enabled && navigable.hits[2].enabled,
              "Current owner view did not expose row and authored ShowTenets hits");
        view.overlay = ui::LiveUiFrame::IdeologyOverlay::confirmation;
        view.branch_id = 7;
        const auto confirm = ui::build_ideology_layout(source, owned, 1280, 960, &view);
        check(!confirm.scroll_region && confirm.hits.size() == 7 &&
              !confirm.hits[confirm.hits.size() - 2].enabled &&
              confirm.hits.back().enabled,
              "Authored confirmation did not keep Yes disabled and No enabled");
        view.overlay = ui::LiveUiFrame::IdeologyOverlay::tenets;
        const auto tenets = ui::build_ideology_layout(source, owned, 1280, 960, &view);
        check(tenets.scroll_region && tenets.scroll_region->max_offset == 0 &&
              tenets.hits.back().enabled,
              "Authored tenet overlay did not preserve read-only empty body and Close");
        auto with_help = evidence;
        with_help.tenet_help = std::vector<ui::IdeologyTenetHelpRow>{
            {11, "POLICY_AUTOCRACY_TEST_1", "POLICY_BRANCH_AUTOCRACY",
             "TXT_KEY_HELP_1", "Localized level one Help", 1, 190, 20},
            {12, "POLICY_AUTOCRACY_TEST_2", "POLICY_BRANCH_AUTOCRACY",
             "TXT_KEY_HELP_2", "Localized level two Help", 2, 220, 40}};
        const auto help_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(with_help));
        const auto with_rows = ui::build_ideology_layout(
            source, help_frame, 1280, 960, &view);
        check(with_rows.scroll_region && with_rows.hits.size() == 11 &&
              with_rows.tenet_headers.size() == 3 &&
              with_rows.tenet_headers[0].icon_markup == "[ICON_MINUS]" &&
              !with_rows.tenet_headers[0].localized_title &&
              std::any_of(with_rows.unresolved.begin(), with_rows.unresolved.end(),
                  [](const auto &reason) {
                      return reason.find("localization is unavailable") !=
                          std::string::npos;
                  }) &&
              std::count_if(with_rows.paints.begin(), with_rows.paints.end(),
                  [](const auto &paint) {
                      return paint.kind == ui::WorldViewPaintKind::text &&
                          paint.text.find("Localized level") != std::string::npos;
                  }) == 2 &&
              std::count_if(with_rows.hits.begin(), with_rows.hits.end(),
                  [](const auto &hit) {
                      return hit.unavailable_reason == "BNW tenet Help row is read only" &&
                          !hit.enabled;
                  }) == 2,
              "Measured owner Help rows did not render as disabled BNW tenets");
        auto text_base = scan(game_root / "Assets/Gameplay/XML/NewText/EN_US");
        auto text_expansion = scan(game_root /
            "Assets/DLC/Expansion/Gameplay/XML/Text/en_US");
        auto text_bnw = scan(game_root /
            "Assets/DLC/Expansion2/Gameplay/XML/Text/en_US");
        assets::ResourceMountTable text_resources(assets::resource_lookup(text_base),
            assets::resource_lookup_all(text_base), assets::resource_keys(text_base));
        text_resources.mount({1, 0, 0, assets::resource_lookup(text_expansion),
                              true, true, assets::resource_lookup_all(text_expansion),
                              assets::resource_keys(text_expansion)});
        text_resources.mount({2, 0, 0, assets::resource_lookup(text_bnw),
                              true, true, assets::resource_lookup_all(text_bnw),
                              assets::resource_keys(text_bnw)});
        const auto selected_text = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
            text_resources, "Language_en_US");
        const auto localize_header = [&](std::string_view key)
            -> std::optional<std::string> {
            const auto found = selected_text.rows.find(std::string(key));
            return found == selected_text.rows.end() ? std::nullopt
                : std::optional<std::string>(found->second);
        };
        const auto localized = ui::build_ideology_layout(
            source, help_frame, 1280, 960, &view, localize_header);
        check(localized.tenet_headers.size() == 3 &&
              localized.tenet_headers[0].localized_title ==
                  localize_header("TXT_KEY_POLICYSCREEN_L1_TENET") &&
              localized.tenet_headers[1].localized_title ==
                  localize_header("TXT_KEY_POLICYSCREEN_L2_TENET") &&
              localized.tenet_headers[2].localized_title ==
                  localize_header("TXT_KEY_POLICYSCREEN_L3_TENET") &&
              std::all_of(localized.tenet_headers.begin(),
                  localized.tenet_headers.end(), [](const auto &header) {
                      return header.localized_title &&
                          !header.localized_title->empty();
                  }), "Selected BNW localization did not resolve tenet level names");
        const auto selected_en_free = ui::build_ideology_layout(source,
            free_frame, 1280, 960, nullptr, localize_header);
        check(std::any_of(selected_en_free.paints.begin(),
            selected_en_free.paints.end(), [](const auto &paint) {
                return paint.text == "Early Adopter Free Tenets: 0";
            }), "Active EN BNW resources did not format the zero award");
        auto ru_base = scan(game_root / "Assets/Gameplay/XML/NewText/RU_RU");
        auto ru_expansion = scan(game_root /
            "Assets/DLC/Expansion/Gameplay/XML/Text/RU_RU");
        auto ru_bnw = scan(game_root /
            "Assets/DLC/Expansion2/Gameplay/XML/Text/RU_RU");
        assets::ResourceMountTable ru_resources(assets::resource_lookup(ru_base),
            assets::resource_lookup_all(ru_base), assets::resource_keys(ru_base));
        ru_resources.mount({1, 0, 0, assets::resource_lookup(ru_expansion),
            true, true, assets::resource_lookup_all(ru_expansion),
            assets::resource_keys(ru_expansion)});
        ru_resources.mount({2, 0, 0, assets::resource_lookup(ru_bnw),
            true, true, assets::resource_lookup_all(ru_bnw),
            assets::resource_keys(ru_bnw)});
        const auto selected_ru = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
            ru_resources, "Language_RU_RU");
        const auto localize_ru = [&](std::string_view key)
            -> std::optional<std::string> {
            const auto found = selected_ru.rows.find(std::string(key));
            return found == selected_ru.rows.end() ? std::nullopt
                : std::optional<std::string>(found->second);
        };
        const auto selected_ru_free = ui::build_ideology_layout(source,
            free_frame, 1280, 960, nullptr, localize_ru);
        check(std::any_of(selected_ru_free.paints.begin(),
            selected_ru_free.paints.end(), [](const auto &paint) {
                return paint.text ==
                    "[COLOR_POSITIVE_TEXT]Бесплатные догматы за раннее принятие: 2[ENDCOLOR]";
            }), "Active RU BNW resources did not format positive award");
        const auto header = std::find_if(with_rows.hits.begin(), with_rows.hits.end(),
            [](const auto &hit) {
                const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = command
                    ? command->template as<ui::IdeologyUiAction>() : nullptr;
                return action &&
                    action->kind == ui::IdeologyUiActionKind::toggle_tenet_level &&
                    action->level == 1;
            });
        check(header != with_rows.hits.end() && header->enabled &&
              header->bounds.x == 360 && header->bounds.y == 199 &&
              header->bounds.width == 542 && header->bounds.height == 27,
              "Selected XML level-one tenet header was not measured");
        view.tenet_collapsed[0] = true;
        const auto collapsed = ui::build_ideology_layout(
            source, help_frame, 1280, 960, &view);
        check(collapsed.hits.size() == 10 &&
              collapsed.tenet_headers.size() == 3 &&
              collapsed.tenet_headers[0].collapsed &&
              collapsed.tenet_headers[0].icon_markup == "[ICON_PLUS]" &&
              std::none_of(collapsed.paints.begin(), collapsed.paints.end(),
                  [](const auto &paint) {
                      return paint.text == "Localized level one Help";
                  }) &&
              std::any_of(collapsed.paints.begin(), collapsed.paints.end(),
                  [](const auto &paint) {
                      return paint.text == "Localized level two Help";
                  }), "Collapsed level hid the wrong owner tenet rows");
        auto tall_help = with_help;
        tall_help.tenet_help->front().label_height = 600;
        const auto tall_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(tall_help));
        const auto tall_collapsed = ui::build_ideology_layout(
            source, tall_frame, 1280, 960, &view);
        view.tenet_collapsed[0] = false;
        const auto tall_expanded = ui::build_ideology_layout(
            source, tall_frame, 1280, 960, &view);
        check(tall_collapsed.scroll_region && tall_expanded.scroll_region &&
              tall_collapsed.scroll_region->max_offset == 0 &&
              tall_expanded.scroll_region->max_offset > 0,
              "Header collapse did not recalculate tenet ScrollPanel internal size");
        auto bad_help = with_help;
        bad_help.tenet_help->back().policy_id = -1;
        const auto bad_help_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(bad_help));
        const auto bad_rows = ui::build_ideology_layout(
            source, bad_help_frame, 1280, 960, &view);
        check(std::none_of(bad_rows.paints.begin(), bad_rows.paints.end(),
            [](const auto &paint) {
                return paint.text.find("Localized level") != std::string::npos;
            }) && bad_rows.hits.size() == 6 && bad_rows.tenet_headers.empty(),
              "Invalid later Help row leaked partially painted tenet data");
        view.overlay = ui::LiveUiFrame::IdeologyOverlay::none;
        auto missing_confirm = source;
        std::erase_if(missing_confirm.document.children, [](const auto &node) {
            return node.attribute("ID") == "ChooseConfirm";
        });
        const auto fail_closed = ui::build_ideology_layout(
            missing_confirm, owned, 1280, 960, &view);
        check(std::none_of(fail_closed.hits.begin(), fail_closed.hits.end(),
            [](const auto &hit) {
                const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = command
                    ? command->template as<ui::IdeologyUiAction>() : nullptr;
                return action && action->kind == ui::IdeologyUiActionKind::select_branch &&
                    hit.enabled;
            }), "Missing confirmation XML exposed branch selection");
        const std::array<std::int32_t, 3> expected_y{198, 368, 602};
        const std::array<std::int32_t, 3> expected_height{170, 234, 234};
        for (std::size_t i = 0; i < 3; ++i) {
            const auto &hit = laid_out.hits[i + 1];
            const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
            const auto *choice = gameplay ? gameplay->as<ui::ProgressionChooseIdeology>() : nullptr;
            check(choice && choice->branch == static_cast<std::int64_t>(i + 7) &&
                  hit.bounds.x == 18 && hit.bounds.y == expected_y[i] &&
                  hit.bounds.width == 418 && hit.bounds.height == expected_height[i] &&
                  !hit.enabled && !hit.unavailable_reason.empty(),
                  "Lua row order/height or disabled branch hit differs");
        }
        auto stale = evidence;
        stale.event_token = 28;
        const auto stale_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(stale));
        check(ideology_hits(ui::build_ideology_layout(source, stale_frame, 1280, 960)) == 0,
              "Stale adopter event supplied ideology hits");
        auto duplicated = evidence;
        duplicated.branches[2].branch_id = 8;
        const auto duplicated_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(duplicated));
        check(ideology_hits(ui::build_ideology_layout(source, duplicated_frame, 1280, 960)) == 0,
              "Ambiguous adopter IDs supplied ideology hits");
        auto wrong_xml = source;
        wrong_xml.selected_source_path = "Assets/UI/InGame/Popups/ChooseIdeologyPopup.xml";
        check(ui::build_ideology_layout(wrong_xml, owned, 1280, 960).hits.empty(),
              "Non-BNW source supplied authored ideology hits");
        auto missing_stack = source;
        for (auto &top : missing_stack.document.children)
            if (top.name == "Instance" && top.attribute("Name") == "ItemInstance")
                for (auto &control : top.children)
                    if (control.name == "Button" && control.attribute("ID") == "Button")
                        std::erase_if(control.children, [](const auto &node) {
                            return node.attribute("ID") == "AdoptedByStacks";
                        });
        check(ui::build_ideology_layout(missing_stack, owned, 1280, 960).hits.empty(),
              "Missing authored adopter stack supplied ideology hits");
        auto one_row = evidence;
        one_row.branches[2].alive_adopters = 12;
        const auto one_row_frame = frame.with<ui::IdeologyLayoutEvidence>(
            std::make_shared<const ui::IdeologyLayoutEvidence>(one_row));
        const auto one_row_plan = ui::build_ideology_layout(source, one_row_frame, 1280, 960);
        check(ideology_hits(one_row_plan) == 3 &&
              one_row_plan.hits[3].bounds.height == 202,
              "Twelve alive adopters must use one 32-pixel CivInstance row");
        const auto compact = ui::build_ideology_layout(source, owned, 800, 600);
        check(compact.scroll_panel && compact.scroll_panel->height == 350 &&
              ideology_hits(compact) == 2 && compact.hits[2].bounds.height == 180 &&
              std::any_of(compact.unresolved.begin(), compact.unresolved.end(),
                  [](const auto &reason) {
                      return reason.find("wheel step") != std::string::npos;
                  }),
              "Compact ideology rows were not clipped to the authored ScrollPanel");
        std::cout << "PASS selected BNW ideology XML synthetic event/count layout\n";

        auto bootstrap = ui::bootstrap_selected_bnw_preview(argv[1], game_root, 0);
        if (bootstrap.preview.blocker)
            throw std::runtime_error("Selected BNW bootstrap: " +
                                     bootstrap.preview.blocker->reason);
        check(bootstrap.preview.host != nullptr, "Selected Gandhi host is absent");
        auto &host = *bootstrap.preview.host;
        auto input = host.preview_input();
        const auto gandhi = ui::capture_live_session_ui_frame(host.session(), 1,
            host.active_player_id(), input.frame_sources);
        check(!gandhi.find<ui::ProgressionIdeologyPopupEvent>(),
              "Selected Gandhi unexpectedly has an ideology popup owner");
        const auto gandhi_plan = ui::build_ideology_layout(source, gandhi, 1280, 960);
        check(gandhi_plan.hits.size() == 1 && ideology_hits(gandhi_plan) == 0 &&
              std::any_of(gandhi_plan.unresolved.begin(), gandhi_plan.unresolved.end(),
                  [](const auto &reason) {
                      return reason.find("No active BNW ideology choice event") !=
                             std::string::npos;
                  }),
              "Selected Gandhi rules invented an ideology choice event");
        std::cout << "PASS selected Gandhi ideology event remains absent\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL ideology layout: " << error.what() << '\n';
        return 1;
    }
}
