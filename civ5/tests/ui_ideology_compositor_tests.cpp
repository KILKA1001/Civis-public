#include "civ5/ui/ideology_frame_owner.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

namespace {
void check(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        check(argc == 3, "Supply selected Gandhi save and original game root");
        const std::filesystem::path game_root(argv[2]);
        auto bootstrap = ui::bootstrap_selected_bnw_preview(argv[1], game_root, 0);
        if (bootstrap.preview.blocker)
            throw std::runtime_error("Selected BNW bootstrap: " +
                                     bootstrap.preview.blocker->reason);
        check(bootstrap.preview.host != nullptr, "Selected Gandhi host is absent");
        auto &selected = *bootstrap.preview.host;
        auto input = selected.preview_input();
        const auto scan = [&](const std::filesystem::path &directory) {
            auto index = std::make_shared<assets::LooseResourceIndex>(game_root);
            index->scan({directory, {}, false});
            return index;
        };
        auto base = scan(game_root / "Assets/UI");
        auto expansion = scan(game_root / "Assets/DLC/Expansion/UI");
        auto bnw = scan(game_root / "Assets/DLC/Expansion2/UI");
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
        const auto source = ui::read_progression_screen_source(
            resources, ui::ProgressionScreen::ideology);
        const auto registry = ui::UiContextRegistry::from_resources(resources);
        const auto *context = registry.find("ChooseIdeology");
        check(context && context->modal,
              "Selected ideology popup does not own modal input focus");
        const auto initial = ui::capture_live_session_ui_frame(selected.session(), 1,
            selected.active_player_id(), input.frame_sources);
        const auto *progression = ui::progression_for_active_player(initial);
        check(progression && !initial.find<ui::ProgressionIdeologyPopupEvent>(),
              "Selected Gandhi unexpectedly owns an ideology event");
        std::map<std::string, std::int64_t> ids;
        for (const auto &branch : progression->branches)
            if (branch.ideology) ids.emplace(branch.type, branch.id);
        check(ids.size() == 3, "Selected BNW ideology branch IDs are unresolved");
        const auto owner_tenet = std::find_if(progression->policies.begin(),
            progression->policies.end(), [](const auto &policy) {
                return policy.branch_type == "POLICY_BRANCH_AUTOCRACY";
            });
        check(owner_tenet != progression->policies.end(),
              "Selected rules have no owner tenet identity");

        // The test owner explicitly supplies a current popup event and alive
        // adopter counts. The selected Gandhi save itself has no such event.
        std::optional<ui::ProgressionIdeologyPopupEvent> event{
            ui::ProgressionIdeologyPopupEvent{selected.active_player_id(), 401}};
        auto sources = input.frame_sources;
        sources.active_ideology_popup = [&](const runtime::LiveSession &) {
            return event;
        };
        auto provider = ui::make_bnw_ideology_layout_frame_provider(
            selected.session(), selected.active_player_id(), sources, source,
            [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyAdopterCount>>{
                    {{ids.at("POLICY_BRANCH_AUTOCRACY"), 1},
                     {ids.at("POLICY_BRANCH_FREEDOM"), 0},
                     {ids.at("POLICY_BRANCH_ORDER"), 0}}};
            },
            [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyTenetHelpRow>>{
                    {{owner_tenet->id, owner_tenet->type,
                      owner_tenet->branch_type, "OWNER_HELP_KEY",
                      "Owner localized tenet Help", 1, 220, 20}}};
            });
        ui::ProgressionGameplayBackend backend;
        ui::LiveUiSources ui_sources;
        ui_sources.progression[static_cast<std::size_t>(ui::ProgressionScreen::ideology)] =
            &source;
        ui::LiveUiHost host(registry, std::move(provider), &backend, ui_sources);
        ui::UiScreenCompositor compositor(resources, registry, &backend,
            [](std::string_view key) -> std::optional<std::string> {
                if (key == "TXT_KEY_POLICYSCREEN_L1_TENET") return "Level one";
                if (key == "TXT_KEY_POLICYSCREEN_L2_TENET") return "Level two";
                if (key == "TXT_KEY_POLICYSCREEN_L3_TENET") return "Level three";
                return std::nullopt;
            });
        const auto opening = host.begin_frame(1280, 960);
        check(host.dispatch(ui::progression_open_command(ui::ProgressionScreen::ideology),
                            opening).status == ui::UiCommandStatus::navigation,
              "Owner event did not open selected ideology popup");
        const auto presented = ui::present_live_ui_frame(host, compositor, 1280, 960);
        check(presented.composed.focused_context == "ChooseIdeology" &&
              presented.live.snapshot.find<ui::IdeologyLayoutEvidence>(),
              "Ideology modal lost owner-scoped frame evidence");
        const auto &hits = presented.composed.hits;
        const auto first = std::find_if(hits.begin(), hits.end(), [&](const auto &hit) {
            const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
            const auto *choice = gameplay ? gameplay->template as<ui::IdeologyUiAction>()
                                          : nullptr;
            return choice && choice->kind == ui::IdeologyUiActionKind::select_branch &&
                   choice->branch_id == ids.at("POLICY_BRANCH_AUTOCRACY");
        });
        const auto show = std::find_if(hits.begin(), hits.end(), [&](const auto &hit) {
            const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
            const auto *choice = gameplay ? gameplay->template as<ui::IdeologyUiAction>()
                                          : nullptr;
            return choice && choice->kind == ui::IdeologyUiActionKind::view_tenets &&
                   choice->branch_id == ids.at("POLICY_BRANCH_AUTOCRACY");
        });
        const auto close = std::find_if(hits.begin(), hits.end(), [](const auto &hit) {
            const auto *command = std::get_if<ui::UiCloseScreen>(&hit.command);
            return command && command->id == "ChooseIdeology";
        });
        check(first != hits.end() && show != hits.end() && close != hits.end() &&
              presented.composed.ideology_tooltip_keys.size() == 3 &&
              presented.composed.ideology_tooltip_keys.front().key ==
                  "TXT_KEY_VP_DIPLO_DETAILS" &&
              first->bounds.x == 18 && first->bounds.y == 198 &&
              first->bounds.width == 418 && first->bounds.height == 202 &&
              first->enabled && show->enabled &&
              show->bounds.x == 402 && show->bounds.y == 200 &&
              close->enabled &&
              close->bounds.x == 80 && close->bounds.y == 919,
              "Compositor did not use selected XML/Lua ideology geometry");
        const auto wheel = ui::wheel_composed_ui(host, presented.live,
            presented.composed, 100, 300, -120);
        check(wheel.consumed && wheel.command.status == ui::UiCommandStatus::unavailable,
              "Unverified native ideology wheel step changed list position");
        const auto chosen = ui::click_composed_ui(host, presented.live,
            presented.composed, first->bounds.x + 2, first->bounds.y + 2,
            first->bounds.x + 2, first->bounds.y + 2);
        check(chosen.consumed &&
              chosen.command.status == ui::UiCommandStatus::navigation &&
              host.screens().focused_screen() == "ChooseIdeology" &&
              !selected.session().state().setup.players.at(selected.active_player_id())
                   .policies_resolved,
              "Ideology row did not open local confirmation without changing policy");
        const auto confirm = ui::present_live_ui_frame(host, compositor, 1280, 960);
        const auto yes = std::find_if(confirm.composed.hits.begin(),
            confirm.composed.hits.end(), [&](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay ? gameplay->template as<ui::ProgressionChooseIdeology>()
                                              : nullptr;
                return choice && choice->branch == ids.at("POLICY_BRANCH_AUTOCRACY") &&
                       hit.bounds.width == 400;
            });
        const auto no = std::find_if(confirm.composed.hits.begin(),
            confirm.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay ? gameplay->template as<ui::IdeologyUiAction>()
                                              : nullptr;
                return choice && choice->kind == ui::IdeologyUiActionKind::close_overlay;
            });
        check(yes != confirm.composed.hits.end() && no != confirm.composed.hits.end() &&
              !yes->enabled && no->enabled && yes->bounds.x == 440 &&
              yes->bounds.y == 447 && no->bounds.y == 513,
              "Selected confirmation geometry or disabled Yes is wrong");
        const auto blocked = ui::click_composed_ui(host, confirm.live,
            confirm.composed, yes->bounds.x + 2, yes->bounds.y + 2);
        check(blocked.consumed && blocked.command.status == ui::UiCommandStatus::unavailable,
              "Disabled confirmation Yes reached GameCore");
        const auto dismissed = ui::click_composed_ui(host, confirm.live,
            confirm.composed, no->bounds.x + 2, no->bounds.y + 2);
        check(dismissed.consumed &&
              dismissed.command.status == ui::UiCommandStatus::navigation,
              "Confirmation No did not dismiss overlay");
        const auto again = ui::present_live_ui_frame(host, compositor, 1280, 960);
        const auto tenets = ui::click_composed_ui(host, again.live,
            again.composed, show->bounds.x + 2, show->bounds.y + 2);
        check(tenets.consumed && tenets.command.status == ui::UiCommandStatus::navigation,
              "Authored ShowTenets control did not open overlay");
        const auto tenet_frame = ui::present_live_ui_frame(host, compositor, 1280, 960);
        const auto tenet_close = std::find_if(tenet_frame.composed.hits.begin(),
            tenet_frame.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay ? gameplay->template as<ui::IdeologyUiAction>()
                                              : nullptr;
                return choice && choice->kind == ui::IdeologyUiActionKind::close_overlay;
            });
        check(tenet_close != tenet_frame.composed.hits.end() &&
              tenet_close->bounds.x == 565 && tenet_close->bounds.y == 723 &&
              tenet_frame.composed.ideology_scroll_region &&
              tenet_frame.composed.ideology_scroll_region->max_offset == 0,
              "Selected tenet overlay geometry or no-owner-data scroll is wrong");
        const auto level_one = std::find_if(tenet_frame.composed.hits.begin(),
            tenet_frame.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *action = gameplay
                    ? gameplay->template as<ui::IdeologyUiAction>() : nullptr;
                return action &&
                    action->kind == ui::IdeologyUiActionKind::toggle_tenet_level &&
                    action->level == 1;
            });
        check(level_one != tenet_frame.composed.hits.end() && level_one->enabled &&
              tenet_frame.composed.ideology_tenet_headers.size() == 3 &&
              tenet_frame.composed.ideology_tenet_headers[0].icon_markup ==
                  "[ICON_MINUS]" &&
              tenet_frame.composed.ideology_tenet_headers[0].localized_title ==
                  "Level one" &&
              level_one->bounds.x == 360 && level_one->bounds.y == 199,
              "Current owner did not expose selected level-one header");
        const auto collapsed_click = ui::click_composed_ui(host, tenet_frame.live,
            tenet_frame.composed, level_one->bounds.x + 2, level_one->bounds.y + 2);
        check(collapsed_click.consumed &&
              collapsed_click.command.status == ui::UiCommandStatus::navigation,
              "Header click did not collapse current tenet panel");
        const auto collapsed_frame = ui::present_live_ui_frame(host, compositor, 1280, 960);
        check(collapsed_frame.live.ideology_view &&
              collapsed_frame.live.ideology_view->tenet_collapsed[0] &&
              collapsed_frame.composed.ideology_tenet_headers.size() == 3 &&
              collapsed_frame.composed.ideology_tenet_headers[0].icon_markup ==
                  "[ICON_PLUS]" &&
              std::none_of(collapsed_frame.composed.paints.begin(),
                  collapsed_frame.composed.paints.end(), [](const auto &paint) {
                      return paint.text == "Owner localized tenet Help";
                  }), "Collapsed tenet level retained owner Help row");
        const auto stale_header = ui::click_composed_ui(host, tenet_frame.live,
            tenet_frame.composed, level_one->bounds.x + 2, level_one->bounds.y + 2);
        check(stale_header.consumed &&
              stale_header.command.status == ui::UiCommandStatus::invalid,
              "Old tenet header frame accepted a second toggle");
        const auto expanded_click = ui::click_composed_ui(host, collapsed_frame.live,
            collapsed_frame.composed, level_one->bounds.x + 2, level_one->bounds.y + 2);
        check(expanded_click.consumed &&
              expanded_click.command.status == ui::UiCommandStatus::navigation,
              "Header click did not re-expand current tenet panel");
        const auto expanded_frame = ui::present_live_ui_frame(host, compositor, 1280, 960);
        check(expanded_frame.live.ideology_view &&
              !expanded_frame.live.ideology_view->tenet_collapsed[0] &&
              std::any_of(expanded_frame.composed.paints.begin(),
                  expanded_frame.composed.paints.end(), [](const auto &paint) {
                      return paint.text == "Owner localized tenet Help";
                  }), "Expanded tenet level did not restore owner Help row");
        const auto tenet_wheel = ui::wheel_composed_ui(host, expanded_frame.live,
            expanded_frame.composed, 400, 250, -120);
        check(tenet_wheel.consumed &&
              tenet_wheel.command.status == ui::UiCommandStatus::unavailable,
              "Tenet wheel moved without owner-supplied content");
        const auto tenet_dismissed = ui::click_composed_ui(host, expanded_frame.live,
            expanded_frame.composed, tenet_close->bounds.x + 2,
            tenet_close->bounds.y + 2);
        check(tenet_dismissed.consumed &&
              tenet_dismissed.command.status == ui::UiCommandStatus::navigation,
              "Selected tenet Close did not dismiss overlay");
        const auto before_owner_change = ui::present_live_ui_frame(
            host, compositor, 1280, 960);
        const auto reopened = ui::click_composed_ui(host, before_owner_change.live,
            before_owner_change.composed, show->bounds.x + 2, show->bounds.y + 2);
        check(reopened.consumed &&
              reopened.command.status == ui::UiCommandStatus::navigation,
              "Tenet overlay did not reopen for owner-change test");
        const auto owner_overlay = ui::present_live_ui_frame(
            host, compositor, 1280, 960);
        const auto owner_collapsed = ui::click_composed_ui(host, owner_overlay.live,
            owner_overlay.composed, level_one->bounds.x + 2, level_one->bounds.y + 2);
        check(owner_collapsed.consumed &&
              owner_collapsed.command.status == ui::UiCommandStatus::navigation,
              "Owner-scoped tenet state did not collapse before token change");
        const auto old_owner = ui::present_live_ui_frame(host, compositor, 1280, 960);
        check(old_owner.live.ideology_view &&
              old_owner.live.ideology_view->tenet_collapsed[0],
              "Collapsed tenet state was not retained for current popup owner");
        event->token = 402;
        const auto restored = ui::present_live_ui_frame(host, compositor, 1280, 960);
        check(restored.live.ideology_view &&
              restored.live.ideology_view->event_token == 402 &&
              restored.live.ideology_view->overlay == ui::LiveUiFrame::IdeologyOverlay::none &&
              !restored.live.ideology_view->tenet_collapsed[0],
              "New popup owner reused previous tenet overlay state");
        const auto old_close = ui::click_composed_ui(host, old_owner.live,
            old_owner.composed, tenet_close->bounds.x + 2,
            tenet_close->bounds.y + 2);
        check(old_close.consumed &&
              old_close.command.status == ui::UiCommandStatus::invalid,
              "Old owner frame accepted tenet Close after popup token changed");
        const auto outside = ui::click_composed_ui(host, restored.live,
            restored.composed, 1200, 900, 1200, 900);
        check(outside.consumed && outside.command.status == ui::UiCommandStatus::invalid,
              "Outside modal click reached the world");
        const auto mismatched = ui::click_composed_ui(host, restored.live,
            restored.composed, 1200, 900,
            close->bounds.x + 2, close->bounds.y + 2);
        check(mismatched.consumed &&
              mismatched.command.status == ui::UiCommandStatus::invalid &&
              host.screens().focused_screen() == "ChooseIdeology",
              "Outside press/release over ideology Close activated it");
        const auto closed = ui::click_composed_ui(host, restored.live,
            restored.composed, close->bounds.x + 2, close->bounds.y + 2,
            close->bounds.x + 2, close->bounds.y + 2);
        check(closed.consumed &&
              closed.command.status == ui::UiCommandStatus::navigation &&
              !host.screens().focused_screen(),
              "Selected ideology Close hit did not release modal focus");
        const auto stale = ui::click_composed_ui(host, presented.live,
            presented.composed, first->bounds.x + 2, first->bounds.y + 2);
        check(stale.consumed && stale.command.status == ui::UiCommandStatus::invalid,
              "Old ideology frame accepted a click after close");
        event.reset();
        const auto without_event = host.begin_frame(1280, 960);
        check(!without_event.snapshot.find<ui::IdeologyLayoutEvidence>() &&
              host.dispatch(ui::progression_open_command(ui::ProgressionScreen::ideology),
                            without_event).status == ui::UiCommandStatus::unavailable &&
              !host.screens().focused_screen(),
              "Selected Gandhi opened an ideology popup without a live owner event");
        std::cout << "PASS selected BNW ideology modal and disabled hit routing\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL ideology compositor: " << error.what() << '\n';
        return 1;
    }
}
