#include "civ5/ui/frontend_existing.hpp"
#include "civ5/ui/top_panel.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
unsigned checks = 0;
void need(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
assets::ResourceMountTable resources_at(const std::filesystem::path &root) {
    const auto layer = [&](const std::filesystem::path &base) {
        auto index = std::make_shared<assets::LooseResourceIndex>(root);
        for (const auto *name : {"UI", "Gameplay", "Shared"})
            if (std::filesystem::is_directory(base / name)) index->scan({base / name, {}, true});
        return index;
    };
    const auto base = layer(root / "Assets");
    assets::ResourceMountTable resources(assets::resource_lookup(base),
        assets::resource_lookup_all(base), assets::resource_keys(base));
    for (const auto *name : {"Expansion", "Expansion2", "Shared"}) {
        auto index = layer(root / "Assets/DLC" / name);
        resources.mount({static_cast<std::uint64_t>(resources.mounts().size() + 1), 0, 0,
            assets::resource_lookup(index), true, true,
            assets::resource_lookup_all(index), assets::resource_keys(index)});
    }
    return resources;
}
const ui::SystemPediaAction *action(const ui::FrontendHit &hit) {
    if (!hit.command) return nullptr;
    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&*hit.command);
    return gameplay ? gameplay->as<ui::SystemPediaAction>() : nullptr;
}
ui::UiCommand find_action(const ui::FrontendMenuPlan &plan, ui::SystemPediaActionKind kind) {
    for (const auto &hit : plan.hits)
        if (const auto *value = action(hit); value && value->kind == kind && hit.enabled)
            return *hit.command;
    throw std::runtime_error("Missing enabled actual Civilopedia action");
}
ui::UiCommand search_focus(const ui::FrontendMenuPlan &plan) {
    for (const auto &hit : plan.hits) {
        if (!hit.command || !hit.enabled) continue;
        const auto *value = std::get_if<ui::UiGameplayCommand>(&*hit.command);
        if (value && value->source_id() == "CivisFrontEndCivilopedia") return *hit.command;
    }
    throw std::runtime_error("Actual Civilopedia SearchEditBox focus hit missing");
}
std::string query(const ui::UiCommand &command) {
    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&command);
    const auto *value = gameplay ? gameplay->as<ui::SystemPediaAction>() : nullptr;
    need(value && value->kind == ui::SystemPediaActionKind::search, "Text input did not produce existing typed search action");
    return value->article_id;
}
void input_tests(ui::FrontendExistingScreens &existing, const ui::FrontendScreen &screen,
                 const ui::FrontendDraft &draft) {
    auto plan = existing.present(screen, draft, 1280, 960);
    need(plan.has_value(), "Civilopedia presenter did not accept its own screen");
    need(existing.dispatch(screen, draft, search_focus(*plan)).status == ui::UiCommandStatus::performed,
        "SearchEditBox could not obtain local text focus");
    need(existing.text_focused(), "Civilopedia search text focus was not retained");
    const auto type = [&](std::uint32_t unit) {
        const auto command = existing.character(unit);
        need(command.has_value(), "Valid Unicode search input was dropped");
        need(existing.dispatch(screen, draft, *command).status == ui::UiCommandStatus::performed,
            "Existing owner rejected valid text input");
        return query(*command);
    };
    need(type(0x0410) == "А", "Cyrillic UTF16 input was not converted to UTF8");
    need(type(' ') == "А ", "Editing buffer dropped an authored word separator");
    need(type('B') == "А B", "Multiword search joined adjacent words");
    auto backspace = existing.key_down(0x08);
    need(backspace && query(*backspace) == "А ", "Backspace did not preserve preceding word separator");
    existing.dispatch(screen, draft, *backspace);
    backspace = existing.key_down(0x08);
    need(backspace && query(*backspace) == "А", "Backspace did not remove one UTF8 codepoint");
    existing.dispatch(screen, draft, *backspace);
    need(!existing.character(0xdc00), "Unpaired low surrogate was accepted");
    need(!existing.character(0xd83d), "High surrogate emitted an incomplete UTF8 character");
    need(type(0xde00) == "А😀", "Surrogate pair did not produce the expected Unicode codepoint");
    need(!existing.character(0xd83d), "Pending high surrogate emitted a command");
    backspace = existing.key_down(0x08);
    need(backspace && query(*backspace) == "А", "Backspace split a four-byte Unicode character");
    existing.dispatch(screen, draft, *backspace);
    need(!existing.character(0xde00), "Backspace retained a stale high surrogate");
    existing.character(0xd83d); existing.cancel_text_input();
    need(!existing.character(0xde00), "Native cancellation retained a stale high surrogate");
    existing.character(0xd83d); existing.character(13);
    need(!existing.character(0xde00), "Control input retained a stale high surrogate");
    existing.dispatch(screen, draft, ui::civilopedia_search_command(""));
    for (unsigned i = 0; i < 50; ++i) type(0x0416);
    need(!existing.character(0x0416), "Search exceeded the existing owner's authored 50-character limit");
    backspace = existing.key_down(0x08);
    need(backspace && query(*backspace).size() == 98, "Cyrillic maximum-length Backspace removed a partial character");
    existing.dispatch(screen, draft, *backspace);
    need(type(0x0416).size() == 100, "Search character limit was incorrectly counted in UTF8 bytes");
    existing.dispatch(screen, draft, ui::civilopedia_search_command(""));
    auto submit = existing.key_down(13);
    need(submit && existing.dispatch(screen, draft, *submit).status == ui::UiCommandStatus::unavailable,
        "Empty native Enter search fabricated a successful article lookup");
}
void actual(const std::filesystem::path &root) {
    auto resources = resources_at(root);
    const auto texts = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources, "Language_en_US");
    const auto localize = [&texts](std::string_view key) -> std::optional<std::string> {
        const auto found = texts.rows.find(std::string(key));
        return found != texts.rows.end() ? std::optional<std::string>(found->second) : std::nullopt;
    };
    const auto entries = ui::read_civilopedia_entries(resources, localize);
    need(entries.size() > 100, "Actual mounted gameplay did not supply a Civilopedia catalog");
    ui::FrontendExistingScreens existing(resources, localize);
    ui::FrontendMenuSession session(ui::FrontendRegistry::original(),
        ui::FrontendSourceCatalog::from_resources(resources), nullptr, localize);
    session.set_external_presenter([&](const auto &screen, const auto &draft, int w, int h) {
        return existing.present(screen, draft, w, h);
    });
    session.set_external_dispatcher([&](const auto &screen, const auto &draft, const auto &command) {
        return existing.dispatch(screen, draft, command);
    });
    need(session.activate("other").status == ui::UiCommandStatus::navigation &&
        session.activate("pedia").status == ui::UiCommandStatus::navigation,
        "Actual pre-world Civilopedia route is unavailable");
    auto plan = session.compose(1280, 960);
    need(plan.unresolved.empty(), "Actual portable Civilopedia reported a presentation blocker");
    need(std::any_of(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        const auto *value = action(hit);
        return hit.enabled && value && value->kind == ui::SystemPediaActionKind::select_category;
    }), "Existing Civilopedia category owner was not ready without a player or turn");
    need(std::any_of(plan.hits.begin(), plan.hits.end(), [](const auto &hit) { return hit.id == "back" && !hit.command; }),
        "Authored CloseButton was not adapted to frontend navigation");
    const auto selected = find_action(plan, ui::SystemPediaActionKind::select_article);
    const auto selected_id = std::get<ui::UiGameplayCommand>(selected).as<ui::SystemPediaAction>()->article_id;
    const auto entry = std::find_if(entries.begin(), entries.end(), [&](const auto &value) { return value.id == selected_id; });
    need(entry != entries.end(), "Authored article hit did not reference an actual catalog row");
    need(session.dispatch(selected).status == ui::UiCommandStatus::performed, "Actual article selection was rejected");
    plan = session.compose(1280, 960);
    const auto article_visible = [&](const auto &paint) {
        return paint.kind == ui::WorldViewPaintKind::text && paint.style == "TwCenMT24" && paint.text == entry->title;
    };
    need(std::any_of(plan.paints.begin(), plan.paints.end(), article_visible), "Actual selected article title was not presented");
    need(session.back().status == ui::UiCommandStatus::navigation && session.screen() == "other", "Close did not return to Extras");
    need(session.dispatch(selected).status == ui::UiCommandStatus::invalid,
        "An article action escaped the current frontend screen guard");
    need(session.activate("pedia").status == ui::UiCommandStatus::navigation, "Civilopedia could not reopen");
    plan = session.compose(1920, 1080);
    need(std::any_of(plan.paints.begin(), plan.paints.end(), article_visible), "Closing or resizing lost the selected article");
    for (const auto width : {320, 480}) {
        auto compact = session.compose(width, 240);
        need(!compact.paints.empty() && compact.unresolved.empty(), "Compact viewport did not explain the original Civilopedia size requirement");
        need(std::all_of(compact.hits.begin(), compact.hits.end(), [&](const auto &hit) {
            return hit.id == "back" && hit.enabled && !hit.command && hit.bounds.width > 0 && hit.bounds.height > 0 &&
                hit.bounds.x >= 0 && hit.bounds.y >= 0 && hit.bounds.x + hit.bounds.width <= width &&
                hit.bounds.y + hit.bounds.height <= 240;
        }), "Compact Civilopedia exposed an invalid geometry or hidden typed action");
        need(!existing.character('x') && !existing.wheel(-120), "Compact screen accepted hidden search or article actions");
        need(session.dispatch(selected).status == ui::UiCommandStatus::unavailable,
            "Compact screen accepted a retained article command");
        compact = session.compose(width, 240);
        const auto back = std::find_if(compact.hits.begin(), compact.hits.end(), [](const auto &hit) { return hit.id == "back"; });
        need(back != compact.hits.end() && session.click(compact, back->bounds.x + 1, back->bounds.y + 1,
            back->bounds.x + 1, back->bounds.y + 1).status == ui::UiCommandStatus::navigation,
            "Compact frontend-owned Back did not return to Extras");
        need(session.activate("pedia").status == ui::UiCommandStatus::navigation, "Compact Civilopedia could not reopen");
        plan = session.compose(1280, 960);
        need(std::any_of(plan.paints.begin(), plan.paints.end(), article_visible), "Compact fallback or return navigation lost article selection");
    }
    const auto category = find_action(plan, ui::SystemPediaActionKind::select_category);
    need(session.dispatch(category).status == ui::UiCommandStatus::performed, "Actual category action did not reach the existing owner");
    plan = session.compose(1280, 960);
    auto second = find_action(plan, ui::SystemPediaActionKind::select_article);
    for (const auto &hit : plan.hits)
        if (const auto *value = action(hit); hit.enabled && value &&
            value->kind == ui::SystemPediaActionKind::select_article && value->article_id != selected_id) {
            second = *hit.command; break;
        }
    need(session.dispatch(second).status == ui::UiCommandStatus::performed, "Category-filtered article could not open");
    need(std::get<ui::UiGameplayCommand>(second).as<ui::SystemPediaAction>()->article_id != selected_id,
        "Actual category did not contain a second article for history validation");
    need(session.dispatch(ui::UiGameplayCommand::make("CivilopediaScreen",
        ui::SystemPediaAction{ui::SystemPediaActionKind::history_back, {}})).status == ui::UiCommandStatus::performed,
        "Existing Civilopedia history did not preserve article navigation");
    plan = session.compose(1280, 960);
    need(std::any_of(plan.paints.begin(), plan.paints.end(), article_visible), "History Back did not restore the previous article");
    ui::FrontendExistingScreens editing(resources, localize);
    const ui::FrontendScreen pedia{"pedia", {}, {}, true};
    const ui::FrontendDraft draft;
    input_tests(editing, pedia, draft);
    const auto multiword = std::find_if(entries.begin(), entries.end(), [&](const auto &value) {
        return value.title.size() < 50 && value.title.find(' ') != std::string::npos &&
            std::all_of(value.title.begin(), value.title.end(), [](unsigned char ch) { return ch >= 32 && ch < 127; }) &&
            std::count_if(entries.begin(), entries.end(), [&](const auto &candidate) { return candidate.title == value.title; }) == 1;
    });
    need(multiword != entries.end(), "Actual localization has no unique multiword article for native search validation");
    for (const unsigned char ch : multiword->title) {
        const auto command = editing.character(ch);
        need(command && editing.dispatch(pedia, draft, *command).status == ui::UiCommandStatus::performed,
            "Native search could not build an actual multiword article title");
    }
    const auto enter = editing.key_down(13);
    need(enter && editing.dispatch(pedia, draft, *enter).status == ui::UiCommandStatus::performed,
        "Native Enter did not open the actual matching multiword article");
    auto searched = editing.present(pedia, draft, 1280, 960);
    need(searched && std::any_of(searched->paints.begin(), searched->paints.end(), [&](const auto &paint) {
        return paint.style == "TwCenMT24" && paint.text == multiword->title;
    }), "Successful native search did not present the actual selected article");
    need(!editing.present({"options", {}, {}, true}, draft, 1280, 960), "Civilopedia adapter took ownership of settings");
    need(editing.dispatch({"options", {}, {}, true}, draft, selected).status == ui::UiCommandStatus::unavailable,
        "Civilopedia dispatcher accepted an action on another screen");
}
void missing() {
    assets::ResourceMountTable resources([](const assets::AssetKey &) -> assets::ResourceMountHit { return {}; });
    ui::FrontendExistingScreens existing(resources, {});
    const ui::FrontendScreen screen{"pedia", {}, {}, true};
    const ui::FrontendDraft draft;
    const auto plan = existing.present(screen, draft, 320, 240);
    need(plan && !plan->unresolved.empty() && !plan->paints.empty(), "Missing assets were fatal or produced an unexplained blank screen");
    need(existing.dispatch(screen, draft, ui::civilopedia_search_command("x")).status == ui::UiCommandStatus::unavailable,
        "Missing Civilopedia owner fabricated action readiness");
    need(!existing.character('x') && !existing.key_down(13), "Missing owner accepted native text input");
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: frontend_existing_tests ASSETS_ROOT");
        missing(); actual(std::filesystem::path(argv[1]));
        std::cout << "frontend_existing_tests: PASS " << checks << " checks\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
