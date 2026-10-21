#include "civ5/ui/screen_compositor.hpp"
#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/ui/live_session_frame.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}

std::shared_ptr<assets::LooseResourceIndex> source(const fs::path &root,
                                                    const fs::path &directory) {
    auto result = std::make_shared<assets::LooseResourceIndex>(root);
    result->scan({directory, {}, false});
    return result;
}

bool closes(const ui::UiCommand &command, std::string_view id) {
    const auto *close = std::get_if<ui::UiCloseScreen>(&command);
    return close && close->id == id;
}

bool tree_closes(const ui::UiCommand &command) {
    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&command);
    return gameplay && gameplay->source_id() == "TechTree" &&
        gameplay->as<ui::ProgressionCloseTechnologyTree>();
}

std::string policy_text(const fs::path &root, std::string_view language,
                        std::string_view key) {
    const auto document = content::read_xml(root / "Assets/Gameplay/XML/NewText" /
        std::string(language) / "CIV5GameTextInfos_Jon.xml");
    const auto search = [&](auto &&self, const content::XmlNode &node) -> std::string {
        if (node.name == "Row" && node.attribute("Tag") == key) {
            const auto *value = node.child("Text");
            return value ? value->text : std::string{};
        }
        for (const auto &child : node.children)
            if (auto value = self(self, child); !value.empty()) return value;
        return {};
    };
    return search(search, document);
}

std::string base_text(const fs::path &root, std::string_view language,
                      std::string_view key) {
    const auto document = content::read_xml(root / "Assets/Gameplay/XML/NewText" /
        std::string(language) / "CIV5GameTextInfos.xml");
    const auto search = [&](auto &&self, const content::XmlNode &node) -> std::string {
        if (node.name == "Row" && node.attribute("Tag") == key) {
            const auto *value = node.child("Text");
            return value ? value->text : std::string{};
        }
        for (const auto &child : node.children)
            if (auto value = self(self, child); !value.empty()) return value;
        return {};
    };
    return search(search, document);
}

struct ProductionBackend final : ui::UiGameplayBackend {
    int executions = 0;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &command,
                                      const ui::UiFrameSnapshot &) const override {
        const auto *choice = command.as<ui::CityChooseProduction>();
        return choice && choice->item_id == 3
            ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
            : ui::UiCommandResult{ui::UiCommandStatus::unavailable,
                                  "Production is unavailable"};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &,
                                 const ui::UiFrameSnapshot &) override {
        ++executions;
        return {ui::UiCommandStatus::performed, {}};
    }
};
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2) {
            std::cout << "SKIP: pass the active civ5-assets directory\n";
            return EXIT_SUCCESS;
        }
        const fs::path root(argv[1]);
        auto base = source(root, root / "Assets/UI");
        auto expansion = source(root, root / "Assets/DLC/Expansion/UI");
        auto bnw = source(root, root / "Assets/DLC/Expansion2/UI");
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
        const auto registry = ui::UiContextRegistry::from_resources(resources);
        ui::UiScreenCompositor compositor(resources, registry);
        ui::LiveUiHost host(registry, [] { return ui::UiFrameSnapshot(1, 0, 0); }, nullptr);
        auto frame = host.begin_frame(1280, 960);
        require(compositor.compose(frame, host.screens(), 1280, 960).paints.empty(),
                "missing data must not create world content");

        require(host.dispatch(ui::UiOpenScreen{"GameMenu"}, frame).status ==
                    ui::UiCommandStatus::navigation, "open game menu");
        frame = host.begin_frame(1280, 960);
        auto game_menu = compositor.compose(frame, host.screens(), 1280, 960);
        require(game_menu.focused_context == "GameMenu" && !game_menu.paints.empty() &&
                    !game_menu.hits.empty(), "system screen is visible and interactive");
        const auto close = std::find_if(game_menu.hits.begin(), game_menu.hits.end(),
                                       [](const auto &hit) {
                                           return closes(hit.command, "GameMenu");
                                       });
        require(close != game_menu.hits.end(), "game menu close hit exists");
        const auto closed = host.click(frame, close->bounds.x + 2, close->bounds.y + 2,
                                      std::span<const ui::LiveUiHit>(game_menu.hits));
        require(closed.consumed && closed.command.status == ui::UiCommandStatus::navigation,
                "measured screen hit routes through live host");

        for (const auto *id : {"TechTree", "EconomicOverview", "GenericPopup",
                                "CivilopediaScreen"}) {
            frame = host.begin_frame(1280, 960);
            require(host.dispatch(ui::UiOpenScreen{id}, frame).status ==
                        ui::UiCommandStatus::navigation, "open active screen");
            frame = host.begin_frame(1280, 960);
            const auto composed = compositor.compose(frame, host.screens(), 1280, 960);
            require(composed.focused_context == id && !composed.paints.empty(),
                    "focused U5-U9 screen paints even without game data");
            require(!composed.unresolved.empty(), "unavailable screen data is explicit");
            if (std::string_view(id) == "TechTree") {
                const auto authored_close = std::find_if(composed.hits.begin(),
                    composed.hits.end(), [](const auto &hit) {
                        return tree_closes(hit.command);
                    });
                require(authored_close != composed.hits.end() && !authored_close->enabled &&
                    authored_close->bounds.x == 7 && authored_close->bounds.y == 787 &&
                    authored_close->bounds.width == 100 &&
                    authored_close->bounds.height == 32,
                    "selected BNW TechTree close control at 1280x960");
                const auto hd_frame = host.begin_frame(1920, 1080);
                const auto hd = compositor.compose(hd_frame, host.screens(), 1920, 1080);
                const auto hd_close = std::find_if(hd.hits.begin(), hd.hits.end(),
                    [](const auto &hit) { return tree_closes(hit.command); });
                require(hd_close != hd.hits.end() && hd_close->bounds.x == 7 &&
                    hd_close->bounds.y == 847 && hd_close->bounds.width == 100 &&
                    hd_close->bounds.height == 32,
                    "selected BNW TechTree close control at 1920x1080");
                const auto clicked = ui::click_composed_ui(host, hd_frame, hd,
                    hd_close->bounds.x + 2, hd_close->bounds.y + 2);
                require(clicked.consumed &&
                    clicked.command.status == ui::UiCommandStatus::unavailable &&
                    host.screens().focused_screen() ==
                        std::optional<std::string>{"TechTree"},
                    "authored TechTree close fails closed without native popup owner");
                require(host.dispatch(ui::UiCloseScreen{"TechTree"}, hd_frame).status ==
                    ui::UiCommandStatus::navigation,
                    "synthetic screen can be cleaned up by U0 controller");
                continue;
            }
            require(host.dispatch(ui::UiCloseScreen{id}, frame).status ==
                        ui::UiCommandStatus::navigation, "close active screen");
        }
        {
            const auto tree_source = ui::read_progression_screen_source(
                resources, ui::ProgressionScreen::technologies);
            const ui::ProgressionTechnologyTreeEvent event{0, 10, 991};
            bool owner_active = true;
            int processed = 0;
            std::uint64_t sequence = 0;
            ui::LiveUiSources tree_sources;
            tree_sources.progression[static_cast<std::size_t>(
                ui::ProgressionScreen::technologies)] = &tree_source;
            tree_sources.active_tech_tree_event = [&]()
                -> std::optional<ui::ProgressionTechnologyTreeEvent> {
                return owner_active ? std::optional{event} : std::nullopt;
            };
            tree_sources.tech_tree_processed = [&](const auto &received) {
                require(received.player_id == event.player_id &&
                    received.turn == event.turn && received.token == event.token,
                    "technology tree owner receives the exact popup occurrence");
                ++processed;
                owner_active = false;
                return ui::UiCommandResult{ui::UiCommandStatus::performed, {}};
            };
            ui::LiveUiHost tree_host(registry, [&] {
                auto snapshot = ui::UiFrameSnapshot(++sequence, 0, 10);
                return owner_active ? snapshot.with<ui::ProgressionTechnologyTreeEvent>(
                    std::make_shared<const ui::ProgressionTechnologyTreeEvent>(event))
                    : snapshot;
            }, nullptr, tree_sources);
            auto current = tree_host.begin_frame(1280, 960);
            require(tree_host.dispatch(ui::UiOpenScreen{"GameMenu"}, current).status ==
                ui::UiCommandStatus::navigation, "open parent for owner-backed tree");
            current = tree_host.begin_frame(1280, 960);
            require(tree_host.dispatch(ui::UiOpenScreen{"TechTree"}, current).status ==
                ui::UiCommandStatus::navigation, "open owner-backed tree");
            current = tree_host.begin_frame(1280, 960);
            const auto composed = compositor.compose(current, tree_host.screens(),
                1280, 960);
            const auto hit = std::find_if(composed.hits.begin(), composed.hits.end(),
                [](const auto &entry) { return tree_closes(entry.command); });
            require(hit != composed.hits.end() && hit->enabled,
                "current native tree event enables the authored close control");
            const auto clicked = ui::click_composed_ui(tree_host, current, composed,
                hit->bounds.x + 2, hit->bounds.y + 2);
            require(clicked.consumed &&
                clicked.command.status == ui::UiCommandStatus::navigation &&
                processed == 1 && !tree_host.screens().visible("TechTree") &&
                tree_host.screens().focused_screen() ==
                    std::optional<std::string>{"GameMenu"},
                "confirmed native popup close restores U0 parent focus");
            require(ui::click_composed_ui(tree_host, current, composed,
                hit->bounds.x + 2, hit->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::invalid && processed == 1,
                "stale tree frame cannot acknowledge an event twice");
            owner_active = true;
            auto uncertain_sources = tree_sources;
            uncertain_sources.tech_tree_processed = [&](const auto &) {
                ++processed;
                return ui::UiCommandResult{ui::UiCommandStatus::performed, {}};
            };
            ui::LiveUiHost uncertain_host(registry, [&] {
                auto snapshot = ui::UiFrameSnapshot(++sequence, 0, 10);
                return snapshot.with<ui::ProgressionTechnologyTreeEvent>(
                    std::make_shared<const ui::ProgressionTechnologyTreeEvent>(event));
            }, nullptr, uncertain_sources);
            auto uncertain_frame = uncertain_host.begin_frame(1280, 960);
            require(uncertain_host.dispatch(ui::UiOpenScreen{"TechTree"},
                uncertain_frame).status == ui::UiCommandStatus::navigation,
                "open tree with an unacknowledged native owner");
            uncertain_frame = uncertain_host.begin_frame(1280, 960);
            const auto uncertain = compositor.compose(uncertain_frame,
                uncertain_host.screens(), 1280, 960);
            const auto uncertain_hit = std::find_if(uncertain.hits.begin(),
                uncertain.hits.end(), [](const auto &entry) {
                    return tree_closes(entry.command);
                });
            require(uncertain_hit != uncertain.hits.end() && uncertain_hit->enabled,
                "unprocessed native event initially offers a close attempt");
            const auto unconfirmed = ui::click_composed_ui(uncertain_host,
                uncertain_frame, uncertain, uncertain_hit->bounds.x + 2,
                uncertain_hit->bounds.y + 2);
            require(unconfirmed.consumed &&
                unconfirmed.command.status == ui::UiCommandStatus::unavailable &&
                uncertain_host.screens().focused_screen() ==
                    std::optional<std::string>{"TechTree"} && processed == 2,
                "reported success without token disappearance keeps tree focused");
            uncertain_frame = uncertain_host.begin_frame(1280, 960);
            const auto retry = compositor.compose(uncertain_frame,
                uncertain_host.screens(), 1280, 960);
            const auto retry_hit = std::find_if(retry.hits.begin(), retry.hits.end(),
                [](const auto &entry) { return tree_closes(entry.command); });
            require(retry_hit != retry.hits.end() && !retry_hit->enabled,
                "uncertain acknowledgement cannot be retried from a new frame");
        }
        {
            auto progression = std::make_shared<ui::ProgressionFrameState>();
            progression->player_id = 0;
            progression->team_id = 0;
            auto popup = std::make_shared<ui::ProgressionTechPopupState>();
            popup->player_id = 0;
            popup->token = 481;
            for (std::int64_t id = 0; id < 10; ++id) {
                ui::ProgressionTechnology tech;
                tech.id = id;
                tech.type = "TECH_TEST_" + std::to_string(id);
                tech.description = "Technology " + std::to_string(id);
                progression->technologies.push_back(std::move(tech));
                popup->eligible_technologies.push_back(id);
            }
            std::uint64_t sequence = 0;
            bool owner_active = false;
            std::int32_t observed_offset = 0;
            bool stale_scroll_source = false;
            const auto tech_source = ui::read_progression_screen_source(
                resources, ui::ProgressionScreen::technology_popup);
            ui::LiveUiSources tech_sources;
            tech_sources.progression[static_cast<std::size_t>(
                ui::ProgressionScreen::technology_popup)] = &tech_source;
            tech_sources.tech_popup_scroll = [&](const ui::UiFrameSnapshot &snapshot,
                                                 std::int32_t width, std::int32_t height)
                -> std::optional<ui::TechPopupScrollEvidence> {
                return ui::TechPopupScrollEvidence{0, *snapshot.turn(),
                    snapshot.frame(), popup->token,
                    stale_scroll_source ? "wrong source" : tech_source.selected_source_path,
                    width, height, height - 300, observed_offset,
                    popup->eligible_technologies.size()};
            };
            ui::LiveUiHost tech_host(registry, [&] {
                auto snapshot = ui::UiFrameSnapshot(++sequence, 0, 10)
                    .with<ui::ProgressionFrameState>(progression);
                return owner_active ? snapshot.with<ui::ProgressionTechPopupState>(popup)
                                    : snapshot;
            }, nullptr, tech_sources);
            auto absent = tech_host.begin_frame(1280, 960);
            require(tech_host.dispatch(ui::UiOpenScreen{"TechPopup"}, absent).status ==
                        ui::UiCommandStatus::unavailable,
                    "TechPopup cannot open without a live popup owner");
            owner_active = true;
            auto opening = tech_host.begin_frame(1280, 960);
            require(tech_host.dispatch(ui::UiOpenScreen{"TechPopup"}, opening).status ==
                        ui::UiCommandStatus::navigation,
                    "live technology popup owner can open its screen");
            auto current = tech_host.begin_frame(1280, 960);
            const auto view = compositor.compose(current, tech_host.screens(), 1280, 960);
            require(std::none_of(view.unresolved.begin(), view.unresolved.end(),
                        [](const auto &reason) {
                            return reason == "Progression list uses a temporary non-native layout" ||
                                   reason == "Selected TechPopup XML layout is unresolved";
                        }), "technology chooser uses the selected authored layout");
            const auto tech_hits = std::count_if(view.hits.begin(), view.hits.end(),
                [](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    return gameplay && gameplay->template as<ui::ProgressionChooseTechnology>();
                });
            require(tech_hits == 9, "only authored TechPopup rows inside the conservative XML/Lua viewport have hits");
            const auto first = std::find_if(view.hits.begin(), view.hits.end(),
                [](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *choice = gameplay
                        ? gameplay->template as<ui::ProgressionChooseTechnology>() : nullptr;
                    return choice && choice->technology == 0;
                });
            require(first != view.hits.end() && first->bounds.x == 38 &&
                        first->bounds.y == 310 && first->bounds.width == 262 &&
                        first->bounds.height == 60 && !first->enabled,
                    "selected TechPopup XML and Lua place a disabled first research choice");
            const auto disabled = ui::click_composed_ui(tech_host, current, view, 42, 315);
            require(disabled.consumed &&
                        disabled.command.status == ui::UiCommandStatus::unavailable,
                    "unavailable technology action consumes modal pointer input");
            const auto tree_hit = [](const ui::ComposedUiFrame &composed) {
                return std::find_if(composed.hits.begin(), composed.hits.end(),
                    [](const auto &hit) {
                        const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                        return gameplay &&
                            gameplay->template as<ui::ProgressionOpenTechnologyTree>();
                    });
            };
            const auto no_owner_transition = tree_hit(view);
            require(no_owner_transition != view.hits.end() &&
                        no_owner_transition->bounds.x == 8 &&
                        no_owner_transition->bounds.y == 260 &&
                        no_owner_transition->bounds.width == 310 &&
                        no_owner_transition->bounds.height == 36 &&
                        !no_owner_transition->enabled,
                    "authored OpenTTButton is visible but disabled without its popup owner service");
            const auto blocked_tree = ui::click_composed_ui(tech_host, current, view, 12, 264);
            require(blocked_tree.consumed &&
                        blocked_tree.command.status == ui::UiCommandStatus::unavailable,
                    "missing popup acknowledgement cannot open TechTree");
            const auto outside = ui::click_composed_ui(tech_host, current, view, 700, 500);
            require(outside.consumed, "TechPopup owns focus outside its 340px background");
            require(view.tech_popup_scroll_region &&
                        view.tech_popup_scroll_region->scroll_panel_height == 660 &&
                        view.tech_popup_scroll_region->vertical_offset_pixels == 0,
                    "live chooser publishes selected BNW ScrollPanel bounds");
            const auto wheel = ui::wheel_composed_ui(tech_host, current, view,
                                                      40, 320, -120);
            require(wheel.consumed &&
                        wheel.command.status == ui::UiCommandStatus::unavailable &&
                        observed_offset == 0,
                    "unverified native wheel step cannot move the chooser");
            observed_offset = 30;
            const auto partial_frame = tech_host.begin_frame(1280, 960);
            const auto partial_view = compositor.compose(partial_frame,
                tech_host.screens(), 1280, 960);
            const auto partial_row = std::find_if(partial_view.paints.begin(),
                partial_view.paints.end(), [](const auto &paint) {
                    return paint.kind == ui::WorldViewPaintKind::rectangle &&
                        paint.destination.x == 38 && paint.destination.y == 280;
                });
            const auto partial_text = std::find_if(partial_view.paints.begin(),
                partial_view.paints.end(), [](const auto &paint) {
                    return paint.kind == ui::WorldViewPaintKind::text &&
                        paint.destination.x == 102 && paint.destination.y == 284;
                });
            require(partial_row != partial_view.paints.end() &&
                        partial_row->destination.width == 270 &&
                        partial_row->destination.height == 60 && partial_row->clip &&
                        partial_row->clip->y == 298 &&
                        partial_text != partial_view.paints.end() &&
                        partial_text->destination.height == 32 && partial_text->clip &&
                        partial_text->clip->y == 298,
                    "partial native scroll row retains original Grid9 size and text origin under inherited clip");
            observed_offset = 152; // Explicit owner observation, not wheel math.
            auto scrolled_frame = tech_host.begin_frame(1280, 960);
            const auto scrolled_view = compositor.compose(scrolled_frame,
                tech_host.screens(), 1280, 960);
            const auto next = std::find_if(scrolled_view.hits.begin(),
                scrolled_view.hits.end(), [](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *choice = gameplay
                        ? gameplay->template as<ui::ProgressionChooseTechnology>() : nullptr;
                    return choice && choice->technology == 2;
                });
            require(next != scrolled_view.hits.end() && next->bounds.y == 310,
                    "owner-observed pixel offset places the correct candidate");
            stale_scroll_source = true;
            auto stale_scroll = tech_host.begin_frame(1280, 960);
            const auto stale_view = compositor.compose(stale_scroll,
                tech_host.screens(), 1280, 960);
            require(std::none_of(stale_view.hits.begin(), stale_view.hits.end(),
                        [](const auto &hit) {
                            const auto *command = std::get_if<ui::UiGameplayCommand>(
                                &hit.command);
                            return command &&
                                command->template as<ui::ProgressionChooseTechnology>();
                        }), "mismatched native ScrollPanel owner hides research hits");
            stale_scroll_source = false;
            popup->token = 482;
            auto changed = tech_host.begin_frame(1280, 960);
            const auto reset = compositor.compose(changed, tech_host.screens(),
                1280, 960);
            require(reset.tech_popup_scroll_region &&
                        reset.tech_popup_scroll_region->event_token == 482 &&
                        reset.tech_popup_scroll_region->vertical_offset_pixels == 152,
                    "new popup event only displays its owner-observed offset");
            for (const auto *language : {"EN_US", "RU_RU"}) {
                const auto title = base_text(root, language,
                                             "TXT_KEY_TECH_POPUP_HEADER");
                const auto label = base_text(root, language,
                                             "TXT_KEY_OPEN_TECH_TREE");
                require(!title.empty() && !label.empty(),
                        "selected EN/RU chooser strings exist");
                ui::UiScreenCompositor localized(resources, registry, nullptr,
                    [title, label](std::string_view key)
                        -> std::optional<std::string> {
                        if (key == "TXT_KEY_TECH_POPUP_HEADER") return title;
                        if (key == "TXT_KEY_OPEN_TECH_TREE") return label;
                        return std::nullopt;
                    });
                const auto localized_frame = tech_host.begin_frame(1920, 1080);
                const auto localized_view = localized.compose(localized_frame,
                    tech_host.screens(), 1920, 1080);
                require(localized_view.tech_popup_scroll_region &&
                            localized_view.tech_popup_scroll_region->scroll_panel_height == 780 &&
                            std::any_of(localized_view.paints.begin(),
                                localized_view.paints.end(),
                                [&](const auto &paint) { return paint.text == title; }) &&
                            std::any_of(localized_view.paints.begin(),
                                localized_view.paints.end(),
                                [&](const auto &paint) { return paint.text == label; }),
                        "1920×1080 chooser uses EN/RU labels and live viewport");
            }
            require(ui::click_composed_ui(tech_host, current, view, 42, 315).command.status ==
                        ui::UiCommandStatus::invalid,
                    "old popup geometry cannot dispatch after the owner changes");
            changed = tech_host.begin_frame(1280, 960);
            require(tech_host.dispatch(ui::UiCloseScreen{"TechPopup"}, changed).status ==
                        ui::UiCommandStatus::navigation, "close technology popup");
            auto transition_popup = std::make_shared<ui::ProgressionTechPopupState>(*popup);
            transition_popup->token = 900;
            transition_popup->eligible_technologies.clear();
            transition_popup->unavailable_reason =
                "Live BNW research selection backend is unavailable";
            std::uint64_t transition_frame = 0;
            auto transition_provider = [&] {
                return ui::UiFrameSnapshot(++transition_frame, 0, 10)
                    .with<ui::ProgressionFrameState>(progression)
                    .with<ui::ProgressionTechPopupState>(transition_popup);
            };
            int acknowledged = 0;
            ui::LiveUiSources transition_sources;
            transition_sources.tech_popup_open_tree = [&](const auto &event) {
                require(event.player_id == 0 && event.token == 900,
                        "OpenTTButton passes the current owner identity");
                ++acknowledged;
                return ui::UiCommandResult{ui::UiCommandStatus::performed, {}};
            };
            ui::LiveUiHost transition_host(registry, transition_provider, nullptr,
                                           transition_sources);
            auto before_open = transition_host.begin_frame(1280, 960);
            require(transition_host.dispatch(ui::UiOpenScreen{"TechPopup"}, before_open).status ==
                        ui::UiCommandStatus::navigation, "open owner-backed technology popup");
            auto transition_live = transition_host.begin_frame(1280, 960);
            const auto transition_view = compositor.compose(
                transition_live, transition_host.screens(), 1280, 960);
            require(tree_hit(transition_view) != transition_view.hits.end() &&
                        tree_hit(transition_view)->enabled,
                    "OpenTTButton uses its owner even when candidate service is absent");
            require(std::find(transition_view.unresolved.begin(),
                        transition_view.unresolved.end(),
                        transition_popup->unavailable_reason) !=
                        transition_view.unresolved.end(),
                    "Missing research eligibility remains visible as an unresolved source");
            require(std::none_of(transition_view.hits.begin(), transition_view.hits.end(),
                        [](const auto &hit) {
                            const auto *command = std::get_if<ui::UiGameplayCommand>(
                                &hit.command);
                            return command &&
                                command->template as<ui::ProgressionChooseTechnology>();
                        }), "missing native scroll evidence hides research choice hits");
            const auto opened_tree = ui::click_composed_ui(
                transition_host, transition_live, transition_view, 12, 264);
            require(opened_tree.consumed &&
                        opened_tree.command.status == ui::UiCommandStatus::navigation &&
                        transition_host.screens().focused_screen() ==
                            std::optional<std::string>{"TechTree"} && acknowledged == 1,
                    "confirmed BNW popup transition closes chooser and opens TechTree once");
            require(ui::click_composed_ui(transition_host, transition_live,
                        transition_view, 12, 264).command.status ==
                        ui::UiCommandStatus::invalid && acknowledged == 1,
                    "stale OpenTTButton cannot replay an acknowledged event");

            int uncertain_attempts = 0;
            ui::LiveUiSources uncertain_sources;
            uncertain_sources.tech_popup_open_tree = [&](const auto &)
                -> ui::UiCommandResult {
                ++uncertain_attempts;
                throw std::runtime_error("second native popup event failed");
            };
            ui::LiveUiHost uncertain_host(registry, transition_provider, nullptr,
                                          uncertain_sources);
            auto before_uncertain = uncertain_host.begin_frame(1280, 960);
            require(uncertain_host.dispatch(ui::UiOpenScreen{"TechPopup"}, before_uncertain)
                        .status == ui::UiCommandStatus::navigation,
                    "open chooser for uncertain owner transition");
            auto uncertain_live = uncertain_host.begin_frame(1280, 960);
            const auto uncertain_view = compositor.compose(
                uncertain_live, uncertain_host.screens(), 1280, 960);
            const auto uncertain = ui::click_composed_ui(
                uncertain_host, uncertain_live, uncertain_view, 12, 264);
            require(uncertain.consumed &&
                        uncertain.command.status == ui::UiCommandStatus::unavailable &&
                        uncertain_host.screens().focused_screen() ==
                            std::optional<std::string>{"TechPopup"} &&
                        uncertain_attempts == 1,
                    "partial native transition cannot claim successful navigation");
            auto retry_live = uncertain_host.begin_frame(1280, 960);
            const auto retry_view = compositor.compose(
                retry_live, uncertain_host.screens(), 1280, 960);
            require(tree_hit(retry_view) != retry_view.hits.end() &&
                        !tree_hit(retry_view)->enabled && uncertain_attempts == 1,
                    "same owner token is disabled after uncertain callback outcome");
        }
        for (const auto *language : {"EN_US", "RU_RU"}) {
            auto policy_state = std::make_shared<ui::ProgressionFrameState>();
            policy_state->player_id = 0;
            policy_state->team_id = 0;
            policy_state->policy_ownership_resolved = true;
            const auto label = policy_text(root, language,
                                           "TXT_KEY_POLICY_ARISTOCRACY");
            require(!label.empty(), "active EN/RU policy localization exists");
            policy_state->policies.push_back({77, "POLICY_ARISTOCRACY",
                "POLICY_BRANCH_TRADITION", "TXT_KEY_POLICY_ARISTOCRACY",
                label, 1, 1, false});
            policy_state->policies.push_back({78, "POLICY_RATIONALISM_TEST",
                "POLICY_BRANCH_RATIONALISM", {}, std::nullopt, 1, 1, false});
            std::uint64_t policy_sequence = 0;
            ui::LiveUiHost policy_host(registry, [&] {
                return ui::UiFrameSnapshot(++policy_sequence, 0, 0)
                    .with<ui::ProgressionFrameState>(policy_state);
            }, nullptr);
            auto opening = policy_host.begin_frame(1280, 960);
            require(policy_host.dispatch(ui::UiOpenScreen{"SocialPolicyPopup"}, opening)
                        .status == ui::UiCommandStatus::navigation,
                    "open policy screen for authored geometry");
            const auto policy_source = ui::read_progression_screen_source(
                resources, ui::ProgressionScreen::policies);
            const auto policy_plan = ui::plan_progression_screen(policy_source,
                ui::ProgressionScreen::policies, opening.snapshot);
            require(policy_source.selected_source_path.find("Expansion2") !=
                        std::string::npos && !policy_plan.items.empty() &&
                        policy_plan.items.front().text == label,
                    "active Expansion2 geometry carries selected EN/RU policy text");
            const auto policy_hit = [](const ui::ComposedUiFrame &view,
                                       std::int64_t id) {
                return std::find_if(view.hits.begin(), view.hits.end(),
                    [id](const auto &hit) {
                        const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                        const auto *choice = gameplay
                            ? gameplay->template as<ui::ProgressionChoosePolicy>() : nullptr;
                        return choice && choice->policy == id;
                    });
            };
            for (const auto [screen_width, screen_height, first_x, first_y,
                             lower_x, lower_y] : {
                std::array{1280, 960, 171, 316, 366, 611},
                std::array{1920, 1080, 491, 376, 686, 671}}) {
                auto current = policy_host.begin_frame(screen_width, screen_height);
                const auto view = compositor.compose(current, policy_host.screens(),
                                                     screen_width, screen_height);
                const auto first = policy_hit(view, 77), lower = policy_hit(view, 78);
                require(first != view.hits.end() && lower != view.hits.end() &&
                            first->bounds.x == first_x && first->bounds.y == first_y &&
                            first->bounds.width == 48 && first->bounds.height == 48 &&
                            lower->bounds.x == lower_x && lower->bounds.y == lower_y &&
                            lower->bounds.width == 48 && lower->bounds.height == 48 &&
                            !first->enabled && !lower->enabled,
                        "selected XML branch stacks and BNW Lua grid offset determine disabled 48px hits");
                require(std::none_of(view.unresolved.begin(), view.unresolved.end(),
                            [](const auto &reason) {
                                return reason == "Progression list uses a temporary non-native layout";
                            }), "policy icons do not fall back to temporary list layout");
                const auto blocked = ui::click_composed_ui(policy_host, current, view,
                    first_x + 2, first_y + 2);
                require(blocked.consumed &&
                            blocked.command.status == ui::UiCommandStatus::unavailable,
                        "typed policy hit remains unavailable without gameplay backend");
            }
        }
        frame = host.begin_frame(1280, 960);
        require(host.dispatch(ui::UiOpenScreen{"CityView"}, frame).status ==
                    ui::UiCommandStatus::unavailable,
                "city screen requires a selected owned live city");
        ProductionBackend production_backend;
        ui::UiScreenCompositor production_compositor(resources, registry,
                                                     &production_backend);
        const auto &selected_popup = production_compositor.production_popup_source();
        auto city_state = std::make_shared<ui::CityScreenFrame>();
        city_state->player_id = 0;
        city_state->selected_city_id = 9;
        ui::CityScreenCity selected_city;
        selected_city.owner = 0;
        selected_city.id = 9;
        city_state->cities.push_back(selected_city);
        auto choices = std::make_shared<ui::ProductionPopupChoices>();
        choices->player_id = 0;
        choices->city_id = 9;
        choices->turn = 0;
        choices->backend_owner = &production_backend;
        choices->selected_source_path = selected_popup.selected_source_path;
        choices->visible_rows = ui::production_popup_visible_rows(selected_popup, 960);
        for (std::int64_t id = 3; id <= 12; ++id)
            choices->entries.push_back({1, id, "BUILDING_" + std::to_string(id),
                id == 3
                    ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
                    : ui::UiCommandResult{ui::UiCommandStatus::unavailable,
                                          "Production is unavailable"},
                "Building " + std::to_string(id)});
        std::uint64_t number = 0;
        ui::LiveUiHost production_host(registry, [&] {
            return ui::UiFrameSnapshot(++number, 0, 0)
                .with<ui::CityScreenFrame>(city_state)
                .with<ui::ProductionPopupChoices>(choices);
        }, &production_backend);
        auto before_popup = production_host.begin_frame(1280, 960);
        require(production_host.dispatch(ui::UiOpenScreen{"ProductionPopup"},
                    before_popup).status == ui::UiCommandStatus::navigation,
                "open production popup for measured UI binding");
        auto production_frame = production_host.begin_frame(1280, 960);
        ui::MeasuredProductionPopup measured;
        measured.selected_source_path = selected_popup.selected_source_path;
        measured.scroll_panel = ui::WorldViewRect{600, 300, 340, 400};
        measured.up_button = ui::WorldViewRect{570, 300, 20, 20};
        measured.down_button = ui::WorldViewRect{570, 650, 20, 20};
        measured.rows.push_back({{9, 1, 3}, {610, 310, 300, 42}});
        measured.rows.push_back({{9, 1, 4}, {610, 360, 300, 42}});
        auto production_view = production_compositor.compose(production_frame,
            production_host.screens(), 1280, 960, &measured);
        const auto find_choice = [&](std::int64_t id) {
            return std::find_if(production_view.hits.begin(), production_view.hits.end(),
                [&](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *choice = gameplay ? gameplay->template as<ui::CityChooseProduction>()
                                                  : nullptr;
                    return choice && choice->item_id == id;
                });
        };
        const auto ready = find_choice(3), blocked = find_choice(4);
        require(production_view.production_scroll_panel &&
                    production_view.production_scroll_panel->x == 600 &&
                    production_view.production_scroll_panel->y == 300 &&
                    ready != production_view.hits.end() && ready->enabled &&
                    ready->bounds.x == 610 && ready->bounds.y == 310 &&
                    blocked != production_view.hits.end() && !blocked->enabled &&
                    blocked->bounds.y == 360 && find_choice(5) == production_view.hits.end(),
                "only measured visible choices receive hits; eligibility is unchanged");
        const auto unmeasured = production_compositor.compose(production_frame,
            production_host.screens(), 1280, 960);
        require(!unmeasured.production_scroll_panel &&
                    std::none_of(unmeasured.hits.begin(), unmeasured.hits.end(),
                        [](const auto &hit) {
                            const auto *gameplay =
                                std::get_if<ui::UiGameplayCommand>(&hit.command);
                            return gameplay &&
                                gameplay->template as<ui::CityChooseProduction>();
                        }),
                "no presenter geometry means no clickable production choice");
        auto duplicate = measured;
        duplicate.rows.push_back(duplicate.rows.front());
        const auto ambiguous = production_compositor.compose(production_frame,
            production_host.screens(), 1280, 960, &duplicate);
        require(std::none_of(ambiguous.hits.begin(), ambiguous.hits.end(),
                        [](const auto &hit) {
                            const auto *gameplay =
                                std::get_if<ui::UiGameplayCommand>(&hit.command);
                            const auto *choice = gameplay
                                ? gameplay->template as<ui::CityChooseProduction>() : nullptr;
                            return choice && choice->item_id == 3;
                        }),
                "duplicate presenter measurements cannot authorize a ready row");
        auto clipped_measurement = measured;
        clipped_measurement.rows.front().bounds = {580, 310, 50, 42};
        const auto clipped_view = production_compositor.compose(production_frame,
            production_host.screens(), 1280, 960, &clipped_measurement);
        const auto clipped_hit = std::find_if(clipped_view.hits.begin(), clipped_view.hits.end(),
            [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay
                    ? gameplay->template as<ui::CityChooseProduction>() : nullptr;
                return choice && choice->item_id == 3;
            });
        require(clipped_hit != clipped_view.hits.end() &&
                    clipped_hit->bounds.x == 600 && clipped_hit->bounds.width == 30,
                "production hit is clipped to measured ScrollPanel");
        require(ui::click_composed_ui(production_host, production_frame,
                    production_view, 612, 362).command.status ==
                    ui::UiCommandStatus::unavailable &&
                    production_backend.executions == 0,
                "measured disabled row cannot execute production");
        require(ui::wheel_composed_ui(production_host, production_frame,
                    production_view, 500, 350, -120).consumed,
                "focused popup consumes wheel outside measured panel");
        require(ui::click_composed_ui(production_host, production_frame,
                    production_view, 612, 312).command.status ==
                    ui::UiCommandStatus::unavailable && production_backend.executions == 0,
                "synthetic ready row has no live production source and cannot execute");
        auto next_frame = production_host.begin_frame(1280, 960);
        require(ui::click_composed_ui(production_host, next_frame,
                    production_view, 612, 312).command.status ==
                    ui::UiCommandStatus::invalid,
                "old composed hit plan cannot be replayed on a new live frame");
        measured.selected_source_path = "unselected/ProductionPopup.xml";
        auto wrong_source = production_compositor.compose(next_frame,
            production_host.screens(), 1280, 960, &measured);
        require(!wrong_source.production_scroll_panel &&
                    std::none_of(wrong_source.hits.begin(), wrong_source.hits.end(),
                        [](const auto &hit) {
                            const auto *gameplay =
                                std::get_if<ui::UiGameplayCommand>(&hit.command);
                            return gameplay &&
                                gameplay->template as<ui::CityChooseProduction>();
                        }),
                "different XML provenance cannot authorize choice hit geometry");
        int measured_frames = 0;
        const auto measure_current = [&](const ui::LiveUiFrame &live,
                                         const ui::CityScreenSource &source) {
            ++measured_frames;
            require(source.selected_source_path == selected_popup.selected_source_path,
                    "presenter receives the selected retail BNW XML source");
            const auto *current_choices = live.snapshot.find<ui::ProductionPopupChoices>();
            require(current_choices && current_choices->visible_rows > 1,
                    "presenter receives the current live production choices");
            ui::ProductionPopupControlRects controls;
            controls.frame_owner = live.owner;
            controls.frame_sequence = live.sequence;
            controls.snapshot_frame = live.snapshot.frame();
            controls.selected_source_path = source.selected_source_path;
            controls.scroll_panel = ui::WorldViewRect{600, 300, 340, 400};
            controls.visible_choice_buttons.resize(
                std::min(current_choices->visible_rows,
                    current_choices->entries.size() - current_choices->first_visible));
            controls.visible_choice_buttons[0] = ui::WorldViewRect{610, 310, 300, 42};
            controls.visible_choice_buttons[1] = ui::WorldViewRect{610, 360, 300, 42};
            return std::optional{std::move(controls)};
        };
        auto presented = ui::present_live_ui_frame(production_host,
            production_compositor, 1280, 960, measure_current);
        const auto presented_choice = [&](std::int64_t id) {
            return std::find_if(presented.composed.hits.begin(),
                presented.composed.hits.end(), [&](const auto &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *choice = gameplay
                        ? gameplay->template as<ui::CityChooseProduction>() : nullptr;
                    return choice && choice->item_id == id;
                });
        };
        require(measured_frames == 1 && presented_choice(3) != presented.composed.hits.end() &&
                    presented_choice(3)->enabled &&
                    presented_choice(4) != presented.composed.hits.end() &&
                    !presented_choice(4)->enabled &&
                    presented_choice(5) == presented.composed.hits.end(),
                "presenter measurements bind only current visible typed choices");
        require(ui::click_composed_ui(production_host, presented.live,
                    presented.composed, 612, 362).command.status ==
                    ui::UiCommandStatus::unavailable &&
                    production_backend.executions == 0,
                "presented disabled production remains unavailable");
        auto no_measure = ui::present_live_ui_frame(production_host,
            production_compositor, 1280, 960);
        require(!no_measure.composed.production_scroll_panel &&
                    ui::click_composed_ui(production_host, no_measure.live,
                        no_measure.composed, 612, 312).command.status !=
                        ui::UiCommandStatus::performed,
                "missing presenter callback fails closed");
        auto stale_controls = ui::present_live_ui_frame(production_host,
            production_compositor, 1280, 960,
            [&](const ui::LiveUiFrame &live, const ui::CityScreenSource &source) {
                auto controls = measure_current(live, source);
                --controls->frame_sequence;
                return controls;
            });
        require(!stale_controls.composed.production_scroll_panel,
                "geometry captured for a different frame fails closed");
        auto wrong_controls = ui::present_live_ui_frame(production_host,
            production_compositor, 1280, 960,
            [&](const ui::LiveUiFrame &live, const ui::CityScreenSource &source) {
                auto controls = measure_current(live, source);
                controls->selected_source_path = "unselected/ProductionPopup.xml";
                return controls;
            });
        require(!wrong_controls.composed.production_scroll_panel,
                "geometry from a different selected XML fails closed");
        auto present_again = ui::present_live_ui_frame(production_host,
            production_compositor, 1280, 960, measure_current);
        require(ui::click_composed_ui(production_host, presented.live,
                    presented.composed, 612, 312).command.status ==
                    ui::UiCommandStatus::invalid,
                "presenter result cannot replay into a newer live frame");
        require(ui::click_composed_ui(production_host, present_again.live,
                    present_again.composed, 612, 312).command.status ==
                    ui::UiCommandStatus::unavailable &&
                    production_backend.executions == 0,
                "current geometry cannot turn synthetic choices into live orders");
        const auto authored_layout = ui::layout_production_popup(selected_popup,
            1280, 960);
        require(authored_layout && authored_layout->scroll_panel.x == 38 &&
                    authored_layout->scroll_panel.y == 196 &&
                    authored_layout->scroll_panel.height == 564 &&
                    authored_layout->up_button.x == 35 &&
                    authored_layout->up_button.y == 196 &&
                    authored_layout->down_button.x == 35 &&
                    authored_layout->down_button.y == 742 &&
                    authored_layout->choice_button.width == 340 &&
                    authored_layout->choice_button.height == 42,
                "selected BNW XML/Lua supplies the viewport and scroll control dimensions");
        const auto full_hd_layout = ui::layout_production_popup(selected_popup,
            1920, 1080);
        require(full_hd_layout && full_hd_layout->scroll_panel.x == 38 &&
                    full_hd_layout->scroll_panel.y == 196 &&
                    full_hd_layout->scroll_panel.height == 684 &&
                    full_hd_layout->up_button.y == 196 &&
                    full_hd_layout->down_button.y == 862 &&
                    ui::production_popup_visible_rows(selected_popup, 1080) == 16,
                "active Lua viewport and scroll buttons track 1920x1080 height");
        auto rendered = ui::present_live_ui_frame(production_host,
            production_compositor, 1280, 960,
            [](const ui::LiveUiFrame &live, const ui::CityScreenSource &source) {
                return ui::measure_civis_production_popup(live, source, 1280, 960);
            });
        const auto rendered_row = std::find_if(rendered.composed.hits.begin(),
            rendered.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay
                    ? gameplay->template as<ui::CityChooseProduction>() : nullptr;
                return choice && choice->item_id == 3;
            });
        require(rendered.composed.production_scroll_panel &&
                    rendered_row != rendered.composed.hits.end() &&
                    rendered_row->bounds.x == authored_layout->choice_button.x &&
                    rendered_row->bounds.y == authored_layout->choice_button.y &&
                    std::any_of(rendered.composed.paints.begin(), rendered.composed.paints.end(),
                        [&](const auto &paint) {
                            return paint.text == "Building 3" &&
                                paint.destination.x == rendered_row->bounds.x &&
                                paint.destination.y == rendered_row->bounds.y;
                        }),
                "painted production button and measured live hit share one XML layout");
        const auto scroll_button = [&](ui::ProductionPopupScrollDirection direction) {
            return std::find_if(rendered.composed.hits.begin(),
                rendered.composed.hits.end(), [&](const auto &candidate) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(
                        &candidate.command);
                    const auto *scroll = gameplay
                        ? gameplay->template as<ui::ProductionPopupScroll>() : nullptr;
                    return scroll && scroll->city_id == 9 &&
                           scroll->direction == direction;
                });
        };
        const auto up_scroll = scroll_button(ui::ProductionPopupScrollDirection::up);
        const auto down_scroll = scroll_button(ui::ProductionPopupScrollDirection::down);
        require(up_scroll != rendered.composed.hits.end() &&
                    down_scroll != rendered.composed.hits.end() &&
                    up_scroll->bounds.x == authored_layout->up_button.x &&
                    up_scroll->bounds.y == authored_layout->up_button.y &&
                    down_scroll->bounds.x == authored_layout->down_button.x &&
                    down_scroll->bounds.y == authored_layout->down_button.y &&
                    !up_scroll->enabled && !down_scroll->enabled,
                "typed scroll hits use current XML/Lua controls and disabled edges");
        require(ui::click_composed_ui(production_host, rendered.live,
                    rendered.composed, rendered_row->bounds.x + 2,
                    rendered_row->bounds.y + 2).command.status ==
                    ui::UiCommandStatus::unavailable &&
                    production_backend.executions == 0,
                "drawn BNW production button still requires a live choice source");
        saves::SavedSession restored;
        restored.snapshot.game_state.layout =
            saves::GameStateLayout::expansion2_version1;
        runtime::LiveSessionState state{
            restored.snapshot, runtime::ResearchContext{}, {},
            runtime::PlayerResearchContext{}, runtime::CityYieldContext{},
            runtime::BnwUnhappinessContext{}, runtime::MaintenanceContext{},
            runtime::TreasuryContext{}, runtime::ScienceContext{},
            runtime::ResearchTurnState{},
            {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {5, 0, 0}),
             runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {7, 0, 0})},
            {}, {}, {}, {}};
        state.setup.game_turn = 12;
        state.setup.players[5].alive = true;
        auto &live_city = state.cities.players[5].cities[9];
        live_city.owner = 5;
        live_city.id = 9;
        live_city.production_orders =
            std::vector<runtime::CityYieldState::ProductionOrder>{
                {4, 1, -1, false, false, std::nullopt}};
        live_city.production_progress.emplace();
        auto session = runtime::LiveSession::from_restored_bnw(
            std::move(restored), std::move(state));
        runtime::ProductionProgressRules rules;
        rules.processes.insert(1);
        rules.buildings.insert(3);
        rules.info_identity["Buildings"][3] = "BUILDING_MARKET";
        rules.info_identity["Processes"][1] = "PROCESS_WEALTH";
        int committed = 0;
        runtime::BnwProductionOrderServices services;
        services.lua_ready = [] { return true; };
        services.core_can_order_read_only = [](const auto &, auto, auto,
            const auto &) { return true; };
        services.lua_test_all_read_only = [](auto, auto, auto, auto,
            const auto &) -> std::optional<bool> { return std::nullopt; };
        services.apply_owned_effect = [](auto &, auto, auto, auto, const auto &) {};
        services.dirty_after_commit = [&](auto, auto) { ++committed; };
        ui::BnwProductionPreview live_preview =
            [](const auto &, auto, auto, const auto &choice) {
                return choice.item == 3
                    ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
                    : ui::UiCommandResult{ui::UiCommandStatus::unavailable,
                                          "Process preview unavailable"};
            };
        ui::LiveSessionUiSources frame_sources;
        frame_sources.selected_city = [] { return std::optional<std::int64_t>{9}; };
        auto missing_binding = ui::bind_live_bnw_preview(
            {session, 5, rules, frame_sources, {}, live_preview});
        ui::UiScreenCompositor missing_compositor(resources, registry,
            missing_binding.backend.get());
        ui::LiveUiSources live_sources;
        live_sources.production_backend = missing_binding.backend.get();
        live_sources.production_popup_source =
            &missing_compositor.production_popup_source();
        ui::LiveUiHost missing_host(registry, missing_binding.frames,
            missing_binding.backend.get(), live_sources);
        auto missing_closed = missing_host.begin_frame(1280, 960);
        require(missing_host.dispatch(ui::UiOpenScreen{"ProductionPopup"},
                    missing_closed).status == ui::UiCommandStatus::navigation,
                "live popup can open with unavailable production services");
        auto missing_presented = ui::present_live_ui_frame(missing_host,
            missing_compositor, 1280, 960,
            [](const ui::LiveUiFrame &live, const ui::CityScreenSource &source) {
                return ui::measure_civis_production_popup(live, source, 1280, 960);
            });
        const auto unavailable_market = std::find_if(
            missing_presented.composed.hits.begin(),
            missing_presented.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay
                    ? gameplay->template as<ui::CityChooseProduction>() : nullptr;
                return choice && choice->order_type == 1 && choice->item_id == 3;
            });
        require(unavailable_market != missing_presented.composed.hits.end() &&
                    !unavailable_market->enabled &&
                    ui::click_composed_ui(missing_host, missing_presented.live,
                        missing_presented.composed,
                        unavailable_market->bounds.x + 2,
                        unavailable_market->bounds.y + 2).command.status ==
                        ui::UiCommandStatus::unavailable && committed == 0 &&
                    session.state().cities.players.at(5).cities.at(9)
                        .production_orders->front().item == 1,
                "live preview without Lua/effect services keeps production disabled");
        content::RulesDatabase production_database;
        production_database.apply_sql(
            "CREATE TABLE Buildings (ID INTEGER, Type TEXT, Description TEXT);"
            "CREATE TABLE Processes (ID INTEGER, Type TEXT, Description TEXT);"
            "INSERT INTO Buildings VALUES (3, 'BUILDING_MARKET', 'TXT_KEY_MARKET');"
            "INSERT INTO Processes VALUES (1, 'PROCESS_WEALTH', 'TXT_KEY_WEALTH');");
        const content::InfoLocalizer production_text = [](std::string_view key) {
            if (key == "TXT_KEY_MARKET") return std::string("Market");
            if (key == "TXT_KEY_WEALTH") return std::string("Wealth");
            return std::string(key);
        };
        frame_sources.database = &production_database;
        frame_sources.localize_info = production_text;
        ui::BnwCityProductionUiBackend real_backend(session, rules, services,
            live_preview, &production_database, production_text);
        ui::UiScreenCompositor real_compositor(resources, registry,
            &real_backend);
        live_sources.production_backend = &real_backend;
        live_sources.production_popup_source =
            &real_compositor.production_popup_source();
        ui::LiveUiHost real_host(registry,
            ui::make_live_session_frame_provider(session, 5, frame_sources),
            &real_backend, live_sources);
        auto closed_live = real_host.begin_frame(1280, 960);
        require(real_host.dispatch(ui::UiOpenScreen{"ProductionPopup"},
                    closed_live).status == ui::UiCommandStatus::navigation,
                "real live session popup acquires focus");
        auto real_presented = ui::present_live_ui_frame(real_host,
            real_compositor, 1280, 960,
            [](const ui::LiveUiFrame &live, const ui::CityScreenSource &source) {
                return ui::measure_civis_production_popup(live, source, 1280, 960);
            });
        const auto live_market = std::find_if(real_presented.composed.hits.begin(),
            real_presented.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay
                    ? gameplay->template as<ui::CityChooseProduction>() : nullptr;
                return choice && choice->order_type == 1 && choice->item_id == 3;
            });
        require(live_market != real_presented.composed.hits.end() &&
                    live_market->enabled &&
                    ui::click_composed_ui(real_host, real_presented.live,
                        real_presented.composed, live_market->bounds.x + 20,
                        live_market->bounds.y + 2).command.status ==
                        ui::UiCommandStatus::performed && committed == 1 &&
                    session.state().cities.players.at(5).cities.at(9)
                        .production_orders->front().item == 3,
                "drawn selected BNW XML choice commits one live production order");
        std::cout << "PASS active BNW U4-U9 composition and measured-hit routing\n";
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
