#include "civ5/ui/frontend_menu.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace ui;
int checks = 0;
void need(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
UiCommandResult ready() { return {UiCommandStatus::ready, {}}; }
FrontendSourceCatalog sources(const std::filesystem::path &root) {
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    base->scan({"Assets/UI", {}, false});
    assets::ResourceMountTable resources(assets::resource_lookup(base));
    std::uint64_t id = 1;
    for (const auto *layer : {"Expansion", "Expansion2", "Shared"}) {
        auto index = std::make_shared<assets::LooseResourceIndex>(root);
        const auto directory = std::filesystem::path("Assets/DLC") / layer / "UI";
        if (!std::filesystem::is_directory(root / directory)) continue;
        index->scan({directory, {}, false});
        resources.mount({id++, 0, 0, assets::resource_lookup(index), true, true, {}, {}});
    }
    return FrontendSourceCatalog::from_resources(resources);
}
struct Backend final : FrontendBackend {
    mutable int probes = 0, preflights = 0;
    int executions = 0;
    mutable std::function<void()> during_query;
    std::function<void()> during_execute;
    UiCommandResult next{UiCommandStatus::performed, {}};
    UiCommandResult capability = ready();
    FrontendContentCheck content{ready(), {}};
    FrontendRequest last;
    UiCommandResult availability(const FrontendRequest &) const override {
        ++probes;
        if (during_query) during_query();
        return capability;
    }
    FrontendContentCheck check_content(const FrontendRequest &) const override {
        ++preflights;
        if (during_query) during_query();
        return content;
    }
    UiCommandResult execute(const FrontendRequest &request) override {
        ++executions; last = request;
        if (during_execute) during_execute();
        return next;
    }
};
struct SelectionBackend final : FrontendBackend {
    mutable int selections = 0, preflights = 0;
    int executions = 0;
    mutable std::vector<std::string> order;
    FrontendContentCheck selection{ready(), {"bnw"}};
    FrontendContentCheck admission{ready(), {"bnw"}};
    FrontendRequest last;
    UiCommandResult availability(const FrontendRequest &) const override { return ready(); }
    FrontendContentCheck check_selection(const FrontendRequest &) const override {
        ++selections; order.push_back("selection"); return selection;
    }
    FrontendContentCheck check_content(const FrontendRequest &) const override {
        ++preflights; order.push_back("admission"); return admission;
    }
    UiCommandResult execute(const FrontendRequest &request) override {
        ++executions; order.push_back("execute"); last = request;
        return {UiCommandStatus::performed, {}};
    }
};
FrontendData data() {
    FrontendData d;
    d.packages = {{"bnw", "Brave New World", false, 0, ready()},
                  {"mod@2", "Selected mod", true, 2, ready()}};
    d.saves = {{"ordinary", "Ordinary game", "Turn 12", "ordinary.Civ5Save", ready()},
               {"modded", "Modded game", "Turn 24", "modded.CivisSave", ready()},
               {"broken", "Broken save", {}, "broken.Civ5Save", {UiCommandStatus::unavailable, "Corrupt saved game"}}};
    FrontendField speed;
    speed.id = "speed"; speed.screen = "setup"; speed.text_key = "Game speed";
    speed.choices = {{"Standard", std::string("standard")}, {"Epic", std::string("epic")}};
    speed.initial = std::string("standard"); speed.availability = ready();
    speed.additional_screens = {"advanced"};
    speed.controls = {{"setup", "GameSpeedButton"}, {"advanced", "GameSpeedPullDown"}};
    d.fields.push_back(speed);
    FrontendField turns;
    turns.id = "turns"; turns.screen = "advanced"; turns.text_key = "Maximum turns";
    turns.kind = FrontendFieldKind::integer; turns.minimum = 0; turns.maximum = 999;
    turns.initial = std::int64_t(500); turns.availability = ready(); d.fields.push_back(turns);
    return d;
}
void open_setup(FrontendMenuSession &s) {
    need(s.activate("single").status == UiCommandStatus::navigation, "Single player route absent");
    need(s.activate("setup").status == UiCommandStatus::navigation, "Basic setup route absent");
}
void state_and_routes(const FrontendSourceCatalog &catalog) {
    Backend backend;
    FrontendMenuSession s(FrontendRegistry::original(), catalog, &backend);
    s.refresh(data());
    need(s.registry().items("main").size() == 7, "Original main sections changed");
    for (const auto *item : s.registry().items("main"))
        need(s.availability(item->id).status == UiCommandStatus::ready, "Real original main route unavailable");
    need(s.availability("news").status == UiCommandStatus::unavailable, "Missing news source fabricated");
    need(s.activate("advanced").status == UiCommandStatus::invalid, "Other screen item accepted");
    s.activate("multiplayer"); s.activate("mp_lan");
    need(s.screen() == "lobby" && std::get<std::string>(s.draft().values.at("multiplayer_mode")) == "lan",
         "Shared lobby lost LAN mode");
    s.back(); s.activate("mp_internet");
    need(std::get<std::string>(s.draft().values.at("multiplayer_mode")) == "internet", "Internet reused stale LAN mode");
    s.back(); s.back();
    need(s.activate("mods").status == UiCommandStatus::navigation, "Mods route absent");
    need(s.toggle_package("mod@2").status == UiCommandStatus::navigation, "Mod selection failed");
    need(s.activate("mods_setup").status == UiCommandStatus::navigation && s.screen() == "setup", "Modded setup duplicated");
    need(s.set_value("speed", std::string("epic")).status == UiCommandStatus::navigation, "Setting failed");
    need(s.activate("advanced").status == UiCommandStatus::navigation, "Advanced route absent");
    const auto advanced = s.compose(1280, 960);
    need(std::any_of(advanced.hits.begin(), advanced.hits.end(), [](const auto &hit) {
        return hit.id == "field:speed";
    }), "Basic setting missing from advanced editor");
    need(std::any_of(advanced.hits.begin(), advanced.hits.end(), [](const auto &hit) {
        return hit.id == "field:turns" && !hit.enabled &&
            hit.unavailable_reason == "Value requires the existing editor adapter";
    }), "A valid value enabled a missing numeric editor");
    need(s.activate("field:speed").status == UiCommandStatus::navigation &&
         s.activate("choice:0").status == UiCommandStatus::navigation, "Shared advanced choice unavailable");
    need(std::get<std::string>(s.draft().values.at("speed")) == "standard", "Advanced choice updated another draft");
    s.set_value("speed", std::string("epic"));
    need(s.set_value("turns", std::int64_t(999)).status == UiCommandStatus::navigation, "Advanced setting failed");
    need(s.set_value("turns", std::int64_t(1000)).status == UiCommandStatus::invalid, "Invalid turn limit accepted");
    need(s.set_value("speed", std::string("invalid")).status == UiCommandStatus::invalid, "Unknown metadata choice accepted");
    need(s.back().status == UiCommandStatus::navigation && s.screen() == "setup", "Esc/back lost predecessor");
    s.back(); s.back();
    need(s.screen() == "main", "Return to main failed");
    need(s.draft().packages == std::vector<std::string>{"mod@2"}, "MainMenu silently deactivated mods");
    need(std::get<std::string>(s.draft().values.at("speed")) == "epic" &&
         std::get<std::int64_t>(s.draft().values.at("turns")) == 999, "Navigation discarded setup draft");
    need(s.request_exit().status == UiCommandStatus::navigation && s.confirming(), "Close did not request confirmation");
    s.back(); open_setup(s);
    s.request_exit();
    need(s.confirming() && s.screen() == "setup", "Submenu close lost original screen");
    s.back(); need(s.screen() == "setup", "Cancel exit should return to current submenu");
    const auto supplied = s.compose(1280, 960);
    need(std::none_of(supplied.hits.begin(), supplied.hits.end(), [](const auto &hit) {
        return hit.id == "setup_speed" || hit.id == "authored:GameSpeedButton";
    }), "Owner-provided speed duplicated the source selector");
    need(s.activate("setup_speed").status == UiCommandStatus::navigation && s.screen() == "select_speed",
         "Original speed selector has no route");
    s.back();
    need(std::get<std::string>(s.draft().values.at("speed")) == "epic", "Selector Back discarded existing draft");
    auto changed = data(); changed.fields.front().choices.erase(changed.fields.front().choices.begin() + 1);
    s.refresh(changed);
    need(s.availability("start").status == UiCommandStatus::unavailable, "Stale setting launched after choices changed");
    need(backend.executions == 0, "Navigation or invalid edits executed gameplay");
    auto bad_initial = data(); bad_initial.fields.front().initial = std::string("bogus");
    bool rejected = false;
    try { s.refresh(bad_initial); } catch (const std::invalid_argument &) { rejected = true; }
    need(rejected, "Invalid default from owner was accepted");
}
void loading(const FrontendSourceCatalog &catalog) {
    Backend backend;
    FrontendMenuSession s(FrontendRegistry::original(), catalog, &backend);
    s.refresh(data()); s.activate("single"); s.activate("load");
    need(s.availability("load_start").status == UiCommandStatus::unavailable, "No-selection load accepted");
    s.select_save("broken");
    need(s.activate("load_start").status == UiCommandStatus::unavailable && backend.executions == 0, "Corrupt save invoked loader");
    s.select_save("modded"); backend.content.required_packages = {"bnw", "mod@2"};
    backend.capability = {UiCommandStatus::unavailable, "Game loader unavailable"};
    const auto checked = backend.preflights;
    s.compose(1280, 960); s.compose(1280, 960);
    need(backend.preflights == checked, "Disabled loader repeatedly read save payload during presentation");
    backend.capability = ready();
    need(s.activate("load_start").status == UiCommandStatus::unavailable && s.draft().packages.empty(), "Save switched content silently");
    s.toggle_package("mod@2"); s.toggle_package("bnw");
    backend.content.result = {UiCommandStatus::unavailable, "Missing mod dependency"};
    need(s.activate("load_start").status == UiCommandStatus::unavailable && backend.executions == 0, "Failed preflight invoked loader");
    backend.content.result = ready(); backend.next = {UiCommandStatus::unavailable, "Loader rejected payload"};
    need(s.activate("load_start").status == UiCommandStatus::unavailable && !s.handed_off(), "Failed loader claimed gameplay handoff");
    need(s.screen() == "load" && s.draft().save == "modded", "Failed loading discarded selection");
    backend.next = {UiCommandStatus::performed, {}};
    need(s.activate("load_start").status == UiCommandStatus::performed && s.handed_off(), "Actual loader handoff absent");
    need(backend.last.save && backend.last.save->id == "modded" && backend.last.draft.packages.size() == 2, "Typed load request lost inputs");
    need(s.activate("load_start").status == UiCommandStatus::unavailable && backend.executions == 2, "Retired frontend executed twice");
    FrontendMenuSession empty(FrontendRegistry::original(), catalog);
    empty.activate("single"); empty.activate("load");
    need(empty.availability("load_start").status == UiCommandStatus::unavailable, "Missing backend made game ready");
}
void selection_and_execution_admission(const FrontendSourceCatalog &catalog) {
    SelectionBackend backend;
    FrontendMenuSession session(FrontendRegistry::original(), catalog, &backend);
    session.refresh(data()); session.activate("single"); session.activate("load");
    session.select_save("ordinary"); session.toggle_package("bnw");
    for (int i = 0; i < 6; ++i) session.compose(1280, 960);
    need(backend.selections >= 6 && backend.preflights == 0 && backend.executions == 0,
        "Ready loader presentation decoded a save or executed a launch instead of checking cheap selection");
    backend.order.clear();
    need(session.activate("load_start").status == UiCommandStatus::performed && session.handed_off(),
        "Validated selected save did not hand off to the executor");
    need(backend.preflights == 1 && backend.executions == 1 && backend.order.size() >= 2 &&
        backend.order[backend.order.size() - 2] == "admission" && backend.order.back() == "execute",
        "Selected save execution did not run exactly one full admission immediately before its effect");
    need(backend.last.save && backend.last.save->id == "ordinary" &&
        backend.last.save->path == std::filesystem::path("ordinary.Civ5Save") &&
        backend.last.draft.packages == std::vector<std::string>{"bnw"},
        "Launch substituted a different selected save path or package choice");

    SelectionBackend changed;
    changed.admission.result = {UiCommandStatus::unavailable, "Save changed before execution"};
    FrontendMenuSession rejected(FrontendRegistry::original(), catalog, &changed);
    rejected.refresh(data()); rejected.activate("single"); rejected.activate("load");
    rejected.select_save("ordinary"); rejected.toggle_package("bnw");
    need(rejected.availability("load_start").status == UiCommandStatus::ready && changed.preflights == 0,
        "Cheap selection did not stay independent from changed full-admission state");
    need(rejected.activate("load_start").status == UiCommandStatus::unavailable &&
        changed.preflights == 1 && changed.executions == 0 && !rejected.handed_off(),
        "Changed content passed commit admission or retired the frontend");
    need(rejected.screen() == "load" && rejected.draft().save == "ordinary" &&
        rejected.draft().packages == std::vector<std::string>{"bnw"},
        "A failed execution admission discarded the user's selected save or content");

    SelectionBackend missing;
    FrontendMenuSession mismatch(FrontendRegistry::original(), catalog, &missing);
    mismatch.refresh(data()); mismatch.activate("single"); mismatch.activate("load"); mismatch.select_save("ordinary");
    need(mismatch.availability("load_start").status == UiCommandStatus::unavailable &&
        missing.preflights == 0 && missing.executions == 0,
        "Known missing selected content was admitted by a cheap selection check");

    SelectionBackend unknown;
    unknown.selection.required_packages.clear(); unknown.selection.requirements_known = false;
    FrontendMenuSession metadata_unknown(FrontendRegistry::original(), catalog, &unknown);
    metadata_unknown.refresh(data()); metadata_unknown.activate("single"); metadata_unknown.activate("load");
    metadata_unknown.select_save("ordinary"); metadata_unknown.toggle_package("bnw");
    need(metadata_unknown.availability("load_start").status == UiCommandStatus::ready && unknown.preflights == 0,
        "Unverified cheap requirements were treated as an empty verified package set");
    metadata_unknown.compose(1280, 960);
    need(unknown.preflights == 0 && unknown.executions == 0, "Unknown metadata triggered admission during paint");
    need(metadata_unknown.activate("load_start").status == UiCommandStatus::performed && unknown.preflights == 1 &&
        unknown.executions == 1, "Unknown cheap metadata bypassed or prevented the real full admission");

    SelectionBackend incomplete;
    incomplete.selection.requirements_known = false; incomplete.admission.requirements_known = false;
    FrontendMenuSession no_proof(FrontendRegistry::original(), catalog, &incomplete);
    no_proof.refresh(data()); no_proof.activate("single"); no_proof.activate("load"); no_proof.select_save("ordinary");
    const auto refused = no_proof.activate("load_start");
    need(refused.status == UiCommandStatus::unavailable &&
        refused.reason == "Content admission has no verified save requirements" &&
        incomplete.preflights == 1 && incomplete.executions == 0 && !no_proof.handed_off(),
        "Full admission without verified requirements performed a load effect");

    SelectionBackend new_game;
    new_game.selection.required_packages.clear(); new_game.admission.required_packages.clear();
    FrontendMenuSession setup(FrontendRegistry::original(), catalog, &new_game);
    setup.refresh(data()); setup.activate("single"); setup.activate("load"); setup.select_save("ordinary");
    setup.back(); setup.activate("setup");
    need(setup.draft().save == "ordinary", "Returning from Load erased the retained load selection");
    need(setup.activate("start").status == UiCommandStatus::performed && new_game.preflights == 1,
        "New-game execution failed despite valid setup and full admission");
    need(new_game.last.operation == "start_game" && !new_game.last.save && new_game.last.draft.save == "ordinary",
        "New-game request attached an old load slot or corrupted the independent retained draft");
}

void package_source_selection(const FrontendSourceCatalog &catalog) {
    Backend backend;
    FrontendMenuSession s(FrontendRegistry::original(), catalog, &backend);
    auto rows = data();
    rows.packages = {
        {"eui0", "Enhanced User Interface", false, 1, ready(), "shared-id", "UI_bc1/EUI_0.Civ5Pkg (setBaseGame)"},
        {"eui1", "Enhanced User Interface", false, 1, ready(), "shared-id", "UI_bc1/EUI_1.Civ5Pkg (setExpansion1)"},
        {"eui2", "Enhanced User Interface", false, 1, {UiCommandStatus::unavailable, "Manifest unavailable"},
            "shared-id", "UI_bc1/EUI_2.Civ5Pkg (setExpansion2)"},
        {"official", "Required DLC", false, 1, ready(), "other-id", "DLC_01/Required.Civ5Pkg"},
        {"same-id-mod", "Selected mod", true, 1, ready(), "shared-id", "Mods/Selected.modinfo"},
        {"legacy0", "Legacy provider", false, 1, ready()},
        {"legacy1", "Legacy provider", false, 2, ready()}};
    s.refresh(rows); s.activate("dlc");
    s.toggle_package("official"); s.toggle_package("eui0"); s.toggle_package("same-id-mod");
    const auto selected = s.toggle_package("eui1");
    need(selected.status == UiCommandStatus::navigation && selected.reason.starts_with("Replaced source:"),
         "Explicit replacement did not explain the selected source");
    need(s.draft().packages == std::vector<std::string>{"official", "same-id-mod", "eui1"},
         "Same-ID replacement discarded another identity/kind or retained a duplicate source");
    const auto before = s.draft().packages;
    need(s.toggle_package("eui2").status == UiCommandStatus::unavailable && s.draft().packages == before,
         "Unavailable source removed the current source");
    const auto plan = s.compose(1280, 2048);
    for (const auto *path : {"UI_bc1/EUI_0.Civ5Pkg", "UI_bc1/EUI_1.Civ5Pkg"})
        need(std::any_of(plan.paints.begin(), plan.paints.end(), [&](const auto &paint) {
            return paint.text.find("Enhanced User Interface v1 [") != std::string::npos &&
                   paint.text.find(path) != std::string::npos;
        }), "Same-name sources are still indistinguishable");
    s.toggle_package("eui1");
    need(s.draft().packages == std::vector<std::string>{"official", "same-id-mod"},
         "Selected source cannot be cleared explicitly");
    s.toggle_package("legacy0"); s.toggle_package("legacy1");
    need(s.draft().packages.size() == 4, "Legacy provider without identity metadata was guessed as an alternative");
    s.toggle_package("eui0"); const auto retained = s.draft().packages;
    rows.packages.erase(rows.packages.begin()); s.refresh(rows);
    need(s.draft().packages == retained, "Refresh silently switched a disappeared selected source");
    need(backend.executions == 0 && backend.preflights == 0, "Source editing executed a content effect");
}
struct SavedSelectionBackend final : FrontendBackend {
    mutable int selections = 0, preflights = 0;
    int executions = 0;
    std::vector<std::string> required{"bnw"};
    bool known = true, refuse_confirmation = false, change_confirmation = false;
    mutable std::function<void()> during_query;
    mutable std::vector<FrontendRequest> requests;
    UiCommandResult availability(const FrontendRequest &) const override { return ready(); }
    FrontendContentCheck check_selection(const FrontendRequest &request) const override {
        ++selections; requests.push_back(request);
        if (during_query) during_query();
        if (!known) return {ready(), {}, false};
        if (request.draft.packages != required)
            return {{UiCommandStatus::unavailable, "Selected content differs from save requirements"}, required};
        if (refuse_confirmation) return {{UiCommandStatus::unavailable, "Required source ambiguous"}, required};
        if (change_confirmation) return {ready(), {"mod@2"}};
        return {ready(), required};
    }
    FrontendContentCheck check_content(const FrontendRequest &) const override {
        ++preflights; return {{UiCommandStatus::unavailable, "Full admission is not part of selection"}, {}};
    }
    UiCommandResult execute(const FrontendRequest &) override {
        ++executions; return {UiCommandStatus::unavailable, "Selection must not execute"};
    }
};
void select_saved_content(const FrontendSourceCatalog &catalog) {
    SavedSelectionBackend backend;
    FrontendMenuSession s(FrontendRegistry::original(), catalog, &backend);
    s.refresh(data()); s.activate("single"); s.activate("load");
    need(s.activate("load_select_saved_content").status == UiCommandStatus::unavailable && backend.selections == 0,
         "Save-content selection queried without a selected save");
    s.toggle_package("mod@2"); s.select_save("ordinary");
    need(s.draft().packages == std::vector<std::string>{"mod@2"}, "Selecting a save silently changed content");
    bool guarded = true;
    backend.during_query = [&] {
        guarded = guarded && s.toggle_package("mod@2").status == UiCommandStatus::unavailable &&
            s.select_saved_content().status == UiCommandStatus::unavailable;
    };
    need(s.activate("load_select_saved_content").status == UiCommandStatus::navigation &&
         s.draft().packages == std::vector<std::string>{"bnw"}, "Explicit known save requirements were not selected");
    need(guarded && backend.selections == 2 && backend.requests.front().draft.packages.empty() &&
         backend.requests.back().draft.packages == std::vector<std::string>{"bnw"},
         "Save-content selection did not earn a fresh exact metadata confirmation");
    need(backend.requests.front().operation == "load_game" && backend.requests.front().save &&
         backend.requests.front().save->id == "ordinary" && backend.requests.front().control_id == "StartButton",
         "Save-content selection lost the genuine save/operation request");
    backend.during_query = {};
    const auto before = s.draft().packages;
    backend.known = false;
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "Unknown requirements were treated as known empty content");
    backend.known = true; backend.required = {"outside-catalog"};
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "An unknown source was committed");
    backend.required = {"bnw", "bnw"};
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "Repeated requirements were committed");
    backend.required = {"bnw"}; backend.refuse_confirmation = true;
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "A refused confirmation changed the retained draft");
    backend.refuse_confirmation = false; backend.change_confirmation = true;
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "Requirements changed between queries without refusal");
    backend.change_confirmation = false;
    auto changed = data();
    changed.packages[1].mod = false;
    changed.packages[0].selection_identity = changed.packages[1].selection_identity = "same-id";
    s.refresh(changed); backend.required = {"bnw", "mod@2"};
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "Ambiguous same-ID source requirements were committed");
    changed = data(); changed.packages[0].availability = {UiCommandStatus::unavailable, "Manifest missing"};
    s.refresh(changed); backend.required = {"bnw"};
    need(s.select_saved_content().status == UiCommandStatus::unavailable && s.draft().packages == before,
         "Unavailable required source changed the draft");
    need(backend.preflights == 0 && backend.executions == 0,
         "Metadata content selection performed full admission or execution");
}

void localized_saved_game_controls(const FrontendSourceCatalog &catalog) {
    SavedSelectionBackend backend;
    const std::map<std::string, std::string, std::less<>> translations{
        {"Selected saved game", "Выбранное сохранение"},
        {"Select content for saved game", "Выбрать моды и DLC для сохранения"},
        {"Selected packages required by this saved game", "Выбраны необходимые моды и DLC"},
        {"Selected source", "Выбранный источник"},
        {"Replaced source", "Источник заменён"},
        {"Unavailable", "Недоступно"},
        {"Turn 12", "Ход 12"},
        {"Corrupt saved game", "Сохранение повреждено"},
        {"Selected content differs from save requirements", "Выбранные моды и DLC не совпадают с требованиями сохранения"}};
    FrontendMenuSession *observed = nullptr;
    bool guarded = true;
    const auto localize = [&](std::string_view key) -> std::optional<std::string> {
        if (observed) guarded = guarded && observed->back().status == UiCommandStatus::unavailable;
        const auto found = translations.find(key);
        return found == translations.end() ? std::nullopt : std::optional<std::string>(found->second);
    };
    FrontendMenuSession s(FrontendRegistry::original(), catalog, &backend, localize);
    observed = &s;
    s.refresh(data()); s.activate("single"); s.activate("load");
    s.toggle_package("mod@2"); s.select_save("ordinary");
    need(s.message() == "Выбранное сохранение: Ordinary game · Ход 12" &&
         s.draft().packages == std::vector<std::string>{"mod@2"},
         "Localized saved-game selection either lacked its explanation or selected packages implicitly");
    const auto has_text = [](const FrontendMenuPlan &plan, const std::string &text) {
        return std::any_of(plan.paints.begin(), plan.paints.end(), [&](const auto &paint) { return paint.text == text; });
    };
    auto plan = s.compose(1280, 2048);
    need(has_text(plan, "[x] Выбранное сохранение: Ordinary game") &&
         has_text(plan, "Выбрать моды и DLC для сохранения"),
         "Selected-save marker or explicit content action bypassed the active localizer");
    need(has_text(plan, "Broken save · Недоступно"), "Unusable save rows lack a localized availability marker");
    const auto broken = std::find_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        return hit.id == "save:broken";
    });
    need(broken != plan.hits.end() && has_text(s.compose(1280, 2048,
         WorldViewPoint{broken->bounds.x + 1, broken->bounds.y + 1}), "Сохранение повреждено"),
         "Unusable save did not expose its owner's localized metadata failure");
    const auto blocked = std::find_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        return hit.id == "load_start";
    });
    need(blocked != plan.hits.end() && !blocked->enabled &&
         blocked->unavailable_reason == "Selected content differs from save requirements",
         "Disabled load lost the actual selection owner's reason");
    need(std::any_of(plan.paints.begin(), plan.paints.end(), [&](const auto &paint) {
        return paint.destination.x == blocked->bounds.x && paint.destination.y == blocked->bounds.y &&
               paint.text.ends_with(" · Недоступно");
    }), "Unavailable action only changed color without a localized explanation");
    const auto point = WorldViewPoint{blocked->bounds.x + 1, blocked->bounds.y + 1};
    const auto hovered = s.compose(1280, 2048, point);
    need(has_text(hovered, translations.at("Selected content differs from save requirements")),
         "Hover did not expose the localized owner refusal");
    need(s.click(hovered, point.x, point.y, point.x, point.y).status == UiCommandStatus::unavailable &&
         backend.executions == 0 && !s.handed_off(), "Disabled load executed an unavailable owner");
    const auto selected = s.activate("load_select_saved_content");
    need(selected.status == UiCommandStatus::navigation && selected.reason == "Выбраны необходимые моды и DLC" &&
         s.draft().packages == std::vector<std::string>{"bnw"} && guarded,
         "Content selection skipped localization or allowed reentrant mutation");

    auto content = data();
    content.packages = {{"source-a", "Example", false, 1, ready(), "same-id", "C:/long/installed/SourceA/Pack.Civ5Pkg", "SourceA/Pack.Civ5Pkg"},
                        {"source-b", "Example", false, 1, ready(), "same-id", "C:/long/installed/SourceB/Pack.Civ5Pkg", "SourceB/Pack.Civ5Pkg"}};
    s.refresh(content); s.back(); s.back(); s.activate("dlc");
    need(s.toggle_package("source-a").reason.starts_with("Выбранный источник: ") &&
         s.toggle_package("source-b").reason.starts_with("Источник заменён: "),
         "Source selection/replacement messages bypassed the active localizer");
    plan = s.compose(1280, 2048);
    need(has_text(plan, "[x] Example v1 [SourceB/Pack.Civ5Pkg]") &&
         !has_text(plan, "[x] Example v1 [C:/long/installed/SourceB/Pack.Civ5Pkg]"),
         "Absolute source paths dominate package row captions");
    const auto source = std::find_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        return hit.id == "package:source-a";
    });
    need(source != plan.hits.end() && source->enabled, "Compact source row lost its exact selectable key");
    const auto detail = s.compose(1280, 2048, WorldViewPoint{source->bounds.x + 1, source->bounds.y + 1});
    need(has_text(detail, "Example v1 [C:/long/installed/SourceA/Pack.Civ5Pkg]"),
         "Hovering a compact source row discarded the detailed source identity");
    need(backend.preflights == 0 && backend.executions == 0,
         "Localized source and save editing performed a launch effect");
}

void load_actions_and_footer_layout(const FrontendSourceCatalog &catalog) {
    const auto *load_source = catalog.find("LoadMenu.xml");
    const auto *authored_start = load_source ? load_source->find_control("StartButton") : nullptr;
    need(authored_start && authored_start->style == "BaseButton" &&
         std::find(authored_start->ancestor_ids.begin(), authored_start->ancestor_ids.end(), "ScrollPanel") ==
             authored_start->ancestor_ids.end(), "Selected original Load action is no longer outside its file ScrollPanel");
    struct LoadOnlyBackend final : FrontendBackend {
        SavedSelectionBackend selection;
        UiCommandResult availability(const FrontendRequest &request) const override {
            return request.operation == "load_game" ? ready() : UiCommandResult{UiCommandStatus::unavailable,
                "Secondary action owner unavailable: a long explanation must remain inside the status clip"};
        }
        FrontendContentCheck check_selection(const FrontendRequest &request) const override {
            return selection.check_selection(request);
        }
        FrontendContentCheck check_content(const FrontendRequest &request) const override {
            return selection.check_content(request);
        }
        UiCommandResult execute(const FrontendRequest &request) override { return selection.execute(request); }
    };
    const auto same_bounds = [](const WorldViewRect &a, const WorldViewRect &b) {
        return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
    };
    const auto overlaps = [](const WorldViewRect &a, const WorldViewRect &b) {
        return a.x < b.x + b.width && b.x < a.x + a.width &&
               a.y < b.y + b.height && b.y < a.y + a.height;
    };
    const auto inspect = [&](const FrontendMenuPlan &plan, int width, int height) {
        for (std::size_t i = 0; i < plan.hits.size(); ++i) {
            const auto &bounds = plan.hits[i].bounds;
            need(bounds.x >= 0 && bounds.y >= 0 && bounds.width > 0 && bounds.height > 0 &&
                 bounds.x + bounds.width <= width && bounds.y + bounds.height <= height,
                 "Load/DLC layout placed a target outside the viewport");
            for (std::size_t j = 0; j < i; ++j)
                need(!overlaps(bounds, plan.hits[j].bounds), "Fixed actions overlap the scrolling list or Back");
        }
        const auto back = std::find_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) { return hit.id == "back"; });
        need(back != plan.hits.end() && back->enabled, "Load/DLC layout lost its Back action");
        const auto range = std::to_string(plan.first_row + 1) + "-" +
            std::to_string(plan.first_row + plan.visible_rows) + "/" + std::to_string(plan.total_rows);
        const auto counter = std::find_if(plan.paints.begin(), plan.paints.end(), [&](const auto &paint) {
            return paint.kind == WorldViewPaintKind::text && paint.text == range;
        });
        need(counter != plan.paints.end() && !overlaps(counter->destination, back->bounds),
             "Scrolling row counter paints over Back in a compact viewport");
        for (const auto &paint : plan.paints) if (paint.kind == WorldViewPaintKind::text)
            need(paint.clip && paint.clip->x >= 0 && paint.clip->y >= 0 && paint.clip->width > 0 &&
                 paint.clip->height > 0 && paint.clip->x + paint.clip->width <= width &&
                 paint.clip->y + paint.clip->height <= height,
                 "Long save captions or owner reasons paint outside their viewport clip");
    };
    for (const auto size : {WorldViewPoint{320, 240}, WorldViewPoint{480, 360}, WorldViewPoint{640, 480},
                            WorldViewPoint{800, 600}, WorldViewPoint{1280, 960}}) {
        LoadOnlyBackend backend;
        auto definitions = data();
        definitions.saves.clear();
        for (int i = 0; i < 48; ++i)
            definitions.saves.push_back({"slot" + std::to_string(i),
                "AutoSave_0013 г. до н.э. 3480 with a long saved-game name " + std::to_string(i),
                "Long selected-save metadata that must not overlap the fixed action area",
                "slot" + std::to_string(i) + ".Civ5Save", ready()});
        FrontendMenuSession loader(FrontendRegistry::original(), catalog, &backend);
        loader.refresh(definitions); loader.activate("single"); loader.activate("load"); loader.select_save("slot0");
        auto plan = loader.compose(size.x, size.y);
        inspect(plan, size.x, size.y);
        need(std::any_of(plan.hits.begin(), plan.hits.end(), [](const auto &hit) { return hit.id == "save:slot0"; }),
             "Responsive Load layout leaves no selectable saved game");
        const auto find = [](const FrontendMenuPlan &frame, std::string_view id) {
            return std::find_if(frame.hits.begin(), frame.hits.end(), [&](const auto &hit) { return hit.id == id; });
        };
        if (size.x >= 640 && size.y >= 480) {
            const auto helper = find(plan, "load_select_saved_content"), start = find(plan, "load_start");
            need(helper != plan.hits.end() && helper->enabled && start != plan.hits.end() && !start->enabled,
                 "Many saves hide the content action or enable Load before required content is selected");
            const auto helper_bounds = helper->bounds, start_bounds = start->bounds;
            need(loader.click(plan, helper_bounds.x + 1, helper_bounds.y + 1,
                 helper_bounds.x + 1, helper_bounds.y + 1).status == UiCommandStatus::navigation,
                 "Fixed content action did not reach the existing metadata owner");
            loader.scroll(20); plan = loader.compose(size.x, size.y); inspect(plan, size.x, size.y);
            const auto scrolled_helper = find(plan, "load_select_saved_content"), scrolled_start = find(plan, "load_start");
            need(plan.first_row == 20 && scrolled_helper != plan.hits.end() && scrolled_start != plan.hits.end() &&
                 same_bounds(scrolled_helper->bounds, helper_bounds) && same_bounds(scrolled_start->bounds, start_bounds) &&
                 scrolled_start->enabled, "Scrolling moved the two Load actions or lost their current availability");
            const auto save = std::find_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
                return hit.id.starts_with("save:");
            });
            need(save != plan.hits.end(), "Scrolling lost the save list");
            const auto selected_save = save->id.substr(5);
            const auto saved_bounds = save->bounds;
            need(loader.click(plan, saved_bounds.x + 1, saved_bounds.y + 1,
                 saved_bounds.x + 1, saved_bounds.y + 1).status == UiCommandStatus::navigation &&
                 loader.draft().save == selected_save, "A visible save row no longer selects the exact clicked slot");
            plan = loader.compose(size.x, size.y);
            need(find(plan, "load_start") != plan.hits.end() && find(plan, "load_start")->enabled,
                 "Selecting a scrolled save hides the fixed Load action");
        } else {
            std::set<std::string> reached;
            for (int row = 0; row < 80; ++row) {
                plan = loader.compose(size.x, size.y); inspect(plan, size.x, size.y);
                for (const auto &hit : plan.hits) reached.insert(hit.id);
                loader.scroll(1);
            }
            need(reached.contains("load_select_saved_content") && reached.contains("load_start") &&
                 reached.contains("save:slot0") && reached.contains("save:slot47"),
                 "Compact Load fallback makes a save or one of the existing actions unreachable");
        }
        loader.scroll(1000000); plan = loader.compose(size.x, size.y); inspect(plan, size.x, size.y);
        const auto blocked = std::find_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
            return !hit.enabled && !hit.id.starts_with("save:") && hit.id != "load_start";
        });
        need(blocked != plan.hits.end() && !blocked->unavailable_reason.empty(),
             "Secondary unavailable functions lost their owner's refusal");
        const auto bounds = blocked->bounds;
        need(loader.click(plan, bounds.x + 1, bounds.y + 1, bounds.x + 1, bounds.y + 1).status ==
             UiCommandStatus::unavailable, "Layout enabled an unsupported secondary action");
        inspect(loader.compose(size.x, size.y), size.x, size.y);
        need(backend.selection.preflights == 0 && backend.selection.executions == 0,
             "Load layout/selection performed full admission or an application effect");

        definitions.packages.clear();
        for (int i = 0; i < 48; ++i)
            definitions.packages.push_back({"dlc" + std::to_string(i), "Long DLC source caption " + std::to_string(i),
                false, 1, ready(), "id" + std::to_string(i), "detail/" + std::to_string(i), "caption/" + std::to_string(i)});
        FrontendMenuSession dlc(FrontendRegistry::original(), catalog, &backend);
        dlc.refresh(definitions); dlc.activate("dlc"); inspect(dlc.compose(size.x, size.y), size.x, size.y);
        dlc.scroll(1000000); inspect(dlc.compose(size.x, size.y), size.x, size.y);
    }
}

void input_and_extensions(const FrontendSourceCatalog &catalog) {
    Backend backend;
    auto registry = FrontendRegistry::original();
    registry.add_screen({"extension", "Extension", {}});
    registry.add_item({"extension", "main", "Extension", 7, {}, {}, "extension", {}, false});
    bool duplicate = false;
    try { registry.add_item({"single", "main", "Duplicate", 8, {}, {}, "extension", {}, false}); }
    catch (const std::invalid_argument &) { duplicate = true; }
    need(duplicate, "Registry accepted duplicate action ID");
    FrontendMenuSession s(registry, catalog, &backend);
    s.refresh(data());
    auto frame = s.compose(1280, 960);
    const auto hit = std::find_if(frame.hits.begin(), frame.hits.end(), [](const auto &h) { return h.id == "single"; });
    need(hit != frame.hits.end(), "Main single player hit absent");
    const auto x = hit->bounds.x + 2, y = hit->bounds.y + 2;
    need(s.click(frame, x, y, 0, 0).status == UiCommandStatus::invalid, "Mismatched press/release activated");
    s.scroll(1);
    need(s.click(frame, x, y, x, y).status == UiCommandStatus::invalid, "Stale frame activated after scroll");
    FrontendMenuSession other(registry, catalog, &backend);
    need(other.click(frame, x, y, x, y).status == UiCommandStatus::invalid, "Foreign session frame accepted");
    need(s.activate("extension").status == UiCommandStatus::navigation && s.screen() == "extension", "Extension required controller modification");
    s.back();
    for (const auto [width, height] : {std::pair{1280, 960}, {1920, 1080}, {800, 600}, {480, 360}, {320, 240}}) {
        const auto plan = s.compose(width, height);
        need(plan.visible_rows > 0, "Small viewport cannot reach any menu section");
        for (const auto &target : plan.hits)
            need(target.bounds.x >= 0 && target.bounds.y >= 0 && target.bounds.width > 0 &&
                 target.bounds.height > 0 && target.bounds.x + target.bounds.width <= width &&
                 target.bounds.y + target.bounds.height <= height, "Responsive hit outside viewport");
    }
    s.request_exit();
    auto compact = s.compose(320, 240);
    need(compact.hits.size() == 2 && compact.hits[0].id == "confirm" && compact.hits[1].id == "cancel", "Small modal cannot be resolved");
    backend.next = {UiCommandStatus::unavailable, "Exit owner denied"};
    need(s.activate("confirm").status == UiCommandStatus::unavailable && s.confirming(), "Denied confirmation closed modal");
    s.back();
    backend.during_query = [&] {
        need(s.back().status == UiCommandStatus::unavailable, "Backend query reentered navigation");
        need(s.toggle_package("bnw").status == UiCommandStatus::unavailable, "Backend query changed content");
    };
    need(s.availability("exit").status == UiCommandStatus::ready, "Query guard incorrectly blocks original query");
    backend.during_query = {};
    backend.during_execute = [&] { need(s.back().status == UiCommandStatus::unavailable, "Executor reentered navigation"); };
    s.request_exit(); backend.next = {UiCommandStatus::performed, {}};
    need(s.activate("confirm").status == UiCommandStatus::performed && s.handed_off(), "Confirmed process exit did not retire frontend");
}
void external_adapters(const FrontendSourceCatalog &catalog) {
    struct ArticleAction { int article; };
    Backend backend;
    FrontendMenuSession s(FrontendRegistry::original(), catalog, &backend);
    s.activate("other"); s.activate("pedia");
    int article = 0;
    s.set_external_presenter([&](const auto &, const auto &, int, int) -> std::optional<FrontendMenuPlan> {
        need(s.back().status == UiCommandStatus::unavailable, "Presenter mutated frontend history");
        need(!s.compose(1280, 960).unresolved.empty(), "Recursive presenter was not rejected");
        FrontendMenuPlan p;
        p.hits.push_back({{20, 20, 120, 30}, "article", true, {},
            UiGameplayCommand::make("CivilopediaScreen", ArticleAction{42})});
        return p;
    });
    auto plan = s.compose(1280, 960);
    need(!plan.hits.front().enabled, "Presenter without action owner enabled typed controls");
    s.set_external_dispatcher([&](const auto &screen, const auto &, const UiCommand &command) {
        need(screen.id == "pedia", "External dispatch lost focused screen");
        need(s.back().status == UiCommandStatus::unavailable, "External owner reentered navigation");
        const auto *typed = std::get_if<UiGameplayCommand>(&command);
        if (!typed || !typed->as<ArticleAction>()) return UiCommandResult{UiCommandStatus::invalid, "Wrong article command"};
        article = typed->as<ArticleAction>()->article;
        return UiCommandResult{UiCommandStatus::navigation, {}};
    });
    plan = s.compose(1280, 960);
    need(s.click(plan, 22, 22, 22, 22).status == UiCommandStatus::navigation && article == 42,
         "Existing screen typed action did not reach its owner");
    need(s.click(plan, 22, 22, 22, 22).status == UiCommandStatus::invalid,
         "Old article frame accepted after owner navigation");
    s.set_external_presenter([](const auto &, const auto &, int, int) -> std::optional<FrontendMenuPlan> {
        throw std::runtime_error("Article data changed");
    });
    need(!s.compose(1280, 960).unresolved.empty(), "Failed screen owner removed all frontend presentation");
    need(s.back().status == UiCommandStatus::navigation && s.screen() == "other", "Failed external viewer trapped navigation");
    FrontendMenuSession missing(FrontendRegistry::original(), FrontendSourceCatalog{}, &backend);
    need(missing.availability("exit").status == UiCommandStatus::unavailable, "Missing authored exit fabricated");
    need(missing.request_exit().status == UiCommandStatus::navigation && missing.confirming(),
         "Native close cannot recover from absent authored main menu");
    missing.back();
    need(!missing.confirming(), "Native close cancellation failed without assets");
}
void unavailable_original_settings(const FrontendSourceCatalog &catalog) {
    // These real controls receive their labels from Lua or sibling XML labels.
    // Without a value owner they must remain discoverable, with an explanation.
    const auto inspect = [&](std::initializer_list<const char *> route,
                             std::initializer_list<const char *> controls) {
        FrontendMenuSession session(FrontendRegistry::original(), catalog);
        for (const auto *item : route)
            need(session.activate(item).status == UiCommandStatus::navigation,
                 "Original unavailable-setting screen cannot be reached");
        std::map<std::string, FrontendHit> observed;
        std::size_t previous = std::numeric_limits<std::size_t>::max();
        for (int page = 0; page < 200; ++page) {
            const auto plan = session.compose(1280, 960);
            for (const auto &hit : plan.hits) observed.insert_or_assign(hit.id, hit);
            if (plan.first_row == previous) break;
            previous = plan.first_row;
            session.scroll(1);
        }
        for (const auto *id : controls) {
            const auto found = observed.find(id);
            need(found != observed.end(), "Original Lua-labelled function silently disappeared");
            need(!found->second.enabled && !found->second.unavailable_reason.empty(),
                 "Unimplemented original setting lacks a blocking explanation");
        }
    };
    inspect({"single", "setup", "advanced"}, {"authored:MapTypePullDown", "authored:MapSizePullDown",
        "authored:HandicapPullDown", "authored:GameSpeedPullDown", "authored:EraPullDown",
        "authored:MinorCivsSlider", "authored:MaxTurnsEdit", "advanced_victories", "advanced_options", "advanced_map_options"});
    inspect({"single", "setup", "setup_names"}, {"authored:EditNickName", "authored:EditCivLeader",
        "authored:EditCivName", "authored:EditCivShortName", "authored:EditCivAdjective",
        "authored:EditPassword", "authored:EditRetypePassword"});
    inspect({"options", "options_audio"}, {"authored:MusicVolumeSlider", "authored:EffectsVolumeSlider",
        "authored:SpeechVolumeSlider", "authored:AmbienceVolumeSlider"});
    inspect({"options", "options_multiplayer"}, {"authored:TurnNotifySmtpPassRetypeEdit"});
    inspect({"single", "load"}, {"authored:SortByPullDown"});
    inspect({"other", "replays"}, {"authored:SortByPullDown"});
    inspect({"multiplayer", "mp_lan"}, {"authored:ListTypePulldown"});
    inspect({"multiplayer", "mp_hotseat"}, {"hotseat_victories", "hotseat_options", "hotseat_map_options",
        "authored:MapTypePullDown", "authored:MapSizePullDown", "authored:GameSpeedPullDown",
        "authored:EraPull", "authored:TurnModePull", "authored:MinorCivsSlider", "authored:MaxTurnsEdit",
        "authored:TurnTimerEdit"});
    inspect({"mods"}, {"mods_details", "mods_options", "mods_sort_title", "mods_sort_enabled", "mods_update"});
    FrontendMenuSession toggles(FrontendRegistry::original(), catalog);
    auto definitions = data();
    FrontendField toggle;
    toggle.id = "unfinished_toggle"; toggle.screen = "setup";
    toggle.text_key = "Test toggle"; toggle.kind = FrontendFieldKind::toggle;
    toggle.availability = ready(); definitions.fields.push_back(toggle);
    toggles.refresh(std::move(definitions)); open_setup(toggles);
    auto plan = toggles.compose(1280, 960);
    need(std::any_of(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        return hit.id == "field:unfinished_toggle" && !hit.enabled &&
            hit.unavailable_reason == "Toggle value unavailable";
    }), "An absent toggle value enabled a nonfunctional button");
    need(toggles.set_value("unfinished_toggle", true).status == UiCommandStatus::navigation,
         "Authoritative toggle value cannot populate the retained draft");
    plan = toggles.compose(1280, 960);
    need(std::any_of(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        return hit.id == "field:unfinished_toggle" && hit.enabled;
    }), "A populated toggle remains blocked despite a ready editor");
}
void conditional_dialog_presentation(const FrontendSourceCatalog &catalog) {
    const auto *main = catalog.find("MainMenu.xml");
    need(main && main->find_control("Civ5Logo"), "Real main logo source absent");
    const auto texture = main->find_control("Civ5Logo")->texture;
    const auto has_logo = [&](const FrontendMenuPlan &plan) {
        return std::any_of(plan.paints.begin(), plan.paints.end(), [&](const auto &paint) {
            return paint.kind == WorldViewPaintKind::texture && paint.source_key == texture;
        });
    };
    Backend backend;
    FrontendMenuSession session(FrontendRegistry::original(), catalog, &backend);
    need(has_logo(session.compose(1280, 960)), "Main menu lost its authored logo");
    need(session.activate("single").status == UiCommandStatus::navigation, "Single player unavailable");
    for (const auto size : {WorldViewPoint{1280, 960}, WorldViewPoint{1920, 1080}, WorldViewPoint{320, 240}})
        need(!has_logo(session.compose(size.x, size.y)), "Main logo overlaps a submenu header");
    session.back();
    need(session.request_exit().status == UiCommandStatus::navigation, "Exit confirmation unavailable");
    const auto confirmation = session.compose(1280, 960);
    need(!has_logo(confirmation), "Main logo overlaps a confirmation header");
    for (const auto *id : {"confirm", "cancel"})
        need(std::any_of(confirmation.hits.begin(), confirmation.hits.end(), [&](const auto &hit) {
            return hit.id == id && hit.enabled;
        }), "Real pending confirmation lost its buttons");
    for (const auto replay : {false, true}) {
        const auto *source = catalog.find(replay ? "LoadReplayMenu.xml" : "LoadMenu.xml");
        need(source != nullptr, "Real load/replay source absent");
        for (const auto *id : {"Yes", "No", "CloseReferencedPackagesButton"})
            need(source->find_control(id) && source->find_control(id)->static_hidden,
                 "Modal source evidence was removed or changed");
        FrontendMenuSession loader(FrontendRegistry::original(), catalog);
        loader.activate(replay ? "other" : "single");
        loader.activate(replay ? "replays" : "load");
        std::set<std::string> observed;
        std::size_t previous = std::numeric_limits<std::size_t>::max();
        for (int page = 0; page < 200; ++page) {
            const auto plan = loader.compose(1280, 960);
            for (const auto &hit : plan.hits) observed.insert(hit.id);
            if (plan.first_row == previous) break;
            previous = plan.first_row;
            loader.scroll(1);
        }
        for (const auto *id : {"authored:Yes", "authored:No", "authored:CloseReferencedPackagesButton"})
            need(!observed.contains(id), "A hidden owner dialog became a permanent menu row");
        need(observed.contains("authored:SortByPullDown"), "Conditional filtering removed a normal Lua-labelled control");
        if (!replay) for (const auto *id : {"load_delete", "load_show_dlc", "load_show_mods", "authored:AutoCheck", "authored:CloudCheck"})
            need(observed.contains(id), "Conditional filtering removed a normal save operation/filter");
    }
    FrontendMenuSession hotseat(FrontendRegistry::original(), catalog);
    hotseat.activate("multiplayer"); hotseat.activate("mp_hotseat");
    std::set<std::string> hotseat_hits;
    std::size_t previous = std::numeric_limits<std::size_t>::max();
    for (int page = 0; page < 200; ++page) {
        const auto plan = hotseat.compose(1280, 960);
        for (const auto &hit : plan.hits) hotseat_hits.insert(hit.id);
        if (plan.first_row == previous) break;
        previous = plan.first_row; hotseat.scroll(1);
    }
    need(!hotseat_hits.contains("authored:ExitButton"), "Dedicated-server-only exit became a permanent hotseat control");
    for (const auto *id : {"authored:MaxTurnsEdit", "authored:TurnTimerEdit"})
        need(hotseat_hits.contains(id), "Conditional filtering removed a hidden ordinary setup field");
    const auto registry = FrontendRegistry::original();
    need(registry.item("advanced_default")->text_key == "TXT_KEY_AD_SETUP_DEFAULT", "Reset displays a tooltip as a button");
    need(registry.item("advanced_add_ai")->text_key == "TXT_KEY_AD_SETUP_ADD_AI_PLAYER", "Add AI displays a tooltip as a button");
}
} // namespace

struct KeyboardBackend final : FrontendBackend {
    mutable int selections = 0, admissions = 0;
    int executions = 0;
    std::function<void()> during_query;
    UiCommandResult next{UiCommandStatus::unavailable, "Loader rejected the selected save"};
    FrontendRequest last;
    UiCommandResult availability(const FrontendRequest &request) const override {
        if (during_query) during_query();
        return request.operation == "load_game" || request.operation == "exit_game" ? ready() :
            UiCommandResult{UiCommandStatus::unavailable, "This action has no retained game owner"};
    }
    FrontendContentCheck check_selection(const FrontendRequest &request) const override {
        ++selections;
        if (during_query) during_query();
        const std::vector<std::string> required{"bnw"};
        return {request.draft.packages == required ? ready() :
            UiCommandResult{UiCommandStatus::unavailable, "Selected content differs from save requirements"}, required};
    }
    FrontendContentCheck check_content(const FrontendRequest &) const override { ++admissions; return {ready(), {"bnw"}}; }
    UiCommandResult execute(const FrontendRequest &request) override { ++executions; last = request; return next; }
};
FrontendMenuPlan keyboard_focus(FrontendMenuSession &session, std::string_view id, int width=1280, int height=800) {
    for (std::size_t n=0;n<256;++n) {
        auto plan=session.compose(width,height);
        if (const auto *hit=session.focused_hit(plan); hit && hit->id==id) return plan;
        need(session.focus_next(plan).status==UiCommandStatus::navigation, "Tab could not move to a real available button");
    }
    throw std::runtime_error("Requested keyboard target was unreachable");
}
void keyboard_existing_route(const FrontendSourceCatalog &catalog) {
    KeyboardBackend backend;
    FrontendMenuSession s(FrontendRegistry::original(),catalog,&backend);
    s.refresh(data());
    auto plan=s.compose(1280,800);
    need(s.focus_next(plan,true).status==UiCommandStatus::navigation &&
        s.focused_hit(s.compose(1280,800))->id=="exit", "Shift-Tab did not start at the last displayed button");
    need(s.focus_next(s.compose(1280,800)).status==UiCommandStatus::navigation &&
        s.focused_hit(s.compose(1280,800))->id=="single", "Tab did not wrap from the last to first button");
    need(s.activate_focused(s.compose(1280,800)).status==UiCommandStatus::navigation && s.screen()=="single",
        "Keyboard Single Player did not use the existing route");
    need(!s.focused_hit(s.compose(1280,800)), "Navigation carried the old screen's keyboard focus");
    plan=keyboard_focus(s,"load");
    need(s.activate_focused(plan).status==UiCommandStatus::navigation && s.screen()=="load", "Keyboard Load menu failed");
    plan=keyboard_focus(s,"save:ordinary");
    need(s.activate_focused(plan).status==UiCommandStatus::navigation && s.draft().save=="ordinary" && s.draft().packages.empty(),
        "Keyboard saved-game selection silently changed packages");
    plan=keyboard_focus(s,"load_select_saved_content");
    need(s.activate_focused(plan).status==UiCommandStatus::navigation && s.draft().packages==std::vector<std::string>{"bnw"} &&
        backend.admissions==0 && backend.executions==0, "Keyboard explicit DLC selection bypassed metadata-only behavior");
    plan=keyboard_focus(s,"back");s.activate_focused(plan);s.back();
    need(s.screen()=="main" && s.draft().save=="ordinary", "Back/Escape discarded the selected save");
    s.activate_focused(keyboard_focus(s,"dlc"));
    need(s.screen()=="dlc", "Keyboard DLC route absent");
    s.activate_focused(keyboard_focus(s,"package:bnw"));
    need(s.draft().packages.empty(), "Space/Enter-style activation did not deselect the focused package");
    s.activate_focused(keyboard_focus(s,"package:bnw"));
    s.activate_focused(keyboard_focus(s,"back"));
    s.activate_focused(keyboard_focus(s,"single"));s.activate_focused(keyboard_focus(s,"load"));
    plan=keyboard_focus(s,"load_start");
    bool highlighted=false;
    const auto bounds=s.focused_hit(plan)->bounds;
    for(const auto &paint:plan.paints) highlighted |= paint.style=="Grid9FrameTurnsHL" &&
        paint.destination.x==bounds.x && paint.destination.y==bounds.y;
    need(highlighted, "Keyboard focused button has no visible existing highlight");
    need(s.activate_focused(plan).status==UiCommandStatus::unavailable && !s.handed_off() &&
        backend.admissions==1 && backend.executions==1 && backend.last.operation=="load_game" && backend.last.save &&
        backend.last.save->id=="ordinary", "Keyboard Load bypassed admission/owner refusal or fabricated handoff");
    need(s.message()=="Loader rejected the selected save", "Keyboard owner failure was not retained for presentation");
    need(s.activate_focused(plan).status==UiCommandStatus::invalid && backend.executions==1,
        "An old keyboard frame replayed its failed action");
    backend.next={UiCommandStatus::performed,{}};
    need(s.activate_focused(s.compose(1280,800)).status==UiCommandStatus::performed && s.handed_off() && backend.executions==2,
        "A genuine completed keyboard load was not handed off");
    need(s.activate_focused(s.compose(1280,800)).status==UiCommandStatus::unavailable && backend.executions==2,
        "Retired keyboard controls repeated an effect");
}
void keyboard_scroll_and_guards(const FrontendSourceCatalog &catalog) {
    for(const auto size:{WorldViewPoint{1280,800},WorldViewPoint{320,240}}) {
        KeyboardBackend backend;
        FrontendMenuSession s(FrontendRegistry::original(),catalog,&backend);
        auto rows=data();rows.saves.clear();
        for(int n=0;n<48;++n) rows.saves.push_back({"slot"+std::to_string(n),"Save "+std::to_string(n),{},
            "save"+std::to_string(n)+".Civ5Save",ready()});
        s.refresh(rows);s.activate("single");s.activate("load");s.select_save("slot0");s.select_saved_content();
        std::set<std::string> reached;bool scrolled=false;
        for(int n=0;n<55;++n) {
            auto plan=s.compose(size.x,size.y);
            need(s.focus_next(plan).status==UiCommandStatus::navigation, "Tab could not traverse a long saved-game list");
            plan=s.compose(size.x,size.y);const auto *hit=s.focused_hit(plan);
            need(hit && hit->enabled && hit->bounds.x>=0 && hit->bounds.y>=0 &&
                hit->bounds.x+hit->bounds.width<=size.x && hit->bounds.y+hit->bounds.height<=size.y,
                "Tab focused an offscreen or disabled action");
            reached.insert(hit->id);scrolled |= plan.first_row!=0;
        }
        for(int n=0;n<48;++n) need(reached.contains("save:slot"+std::to_string(n)), "Tab skipped an offscreen saved game");
        need(scrolled && reached.contains("load_select_saved_content") && reached.contains("load_start") && reached.contains("back") &&
            !reached.contains("load_delete") && !reached.contains("load_show_dlc"),
            "Keyboard traversal trapped actions or included unavailable secondary owners");
        reached.clear();
        for(int n=0;n<55;++n) {
            auto plan=s.compose(size.x,size.y);s.focus_next(plan,true);
            plan=s.compose(size.x,size.y);const auto *hit=s.focused_hit(plan);
            need(hit && hit->enabled && hit->bounds.y>=0 && hit->bounds.y+hit->bounds.height<=size.y,
                "Shift-Tab focused a hidden or disabled action");
            reached.insert(hit->id);
        }
        for(int n=0;n<48;++n) need(reached.contains("save:slot"+std::to_string(n)), "Shift-Tab skipped an offscreen saved game");
        need(reached.contains("load_select_saved_content") && reached.contains("load_start") && reached.contains("back"),
            "Reverse traversal trapped a fixed Load or Back action");
        auto stale=s.compose(size.x,size.y);s.focus_next(stale);
        need(s.focus_next(stale).status==UiCommandStatus::invalid && s.activate_focused(stale).status==UiCommandStatus::invalid,
            "Stale Tab/activation plan was accepted");
        FrontendMenuSession other(FrontendRegistry::original(),catalog,&backend);
        need(other.focus_next(s.compose(size.x,size.y)).status==UiCommandStatus::invalid,
            "Foreign session supplied keyboard authority");
        s.scroll(3);auto page=s.compose(size.x,size.y);
        need(!s.focused_hit(page), "Manual scroll left an invisible keyboard activation armed");
        const auto first_visible=std::find_if(page.hits.begin(),page.hits.end(),[](const auto &hit){return hit.enabled;});
        need(first_visible!=page.hits.end(), "Scrolled Load page has no actual available button");
        const auto first_id=first_visible->id;s.focus_next(page);
        need(s.focused_hit(s.compose(size.x,size.y))->id==first_id, "Tab after wheel reset to an unrelated hidden row");
        bool refused=false;
        backend.during_query=[&]{refused |= s.focus_next(s.compose(size.x,size.y)).status==UiCommandStatus::unavailable;};
        s.compose(size.x,size.y);backend.during_query={};
        need(refused && backend.admissions==0 && backend.executions==0, "Owner query reentered keyboard navigation or invoked gameplay");
        keyboard_focus(s,"load_start",size.x,size.y);
        rows.packages[0].availability={UiCommandStatus::unavailable,"Manifest missing"};s.refresh(rows);
        auto disabled=s.compose(size.x,size.y);
        need(!s.focused_hit(disabled) && s.activate_focused(disabled).status==UiCommandStatus::unavailable &&
            backend.admissions==0 && backend.executions==0,
            "Refreshed unavailable content retained an armed Load keyboard action");
    }
}
void keyboard_modal_and_choices(const FrontendSourceCatalog &catalog) {
    KeyboardBackend backend;
    FrontendMenuSession s(FrontendRegistry::original(),catalog,&backend,[](std::string_view key)->std::optional<std::string>{
        if(key=="Select a button with Tab first") return "Сначала выберите кнопку клавишей Tab";
        return {};
    });
    s.refresh(data());
    need(s.activate_focused(s.compose(1280,800)).status==UiCommandStatus::unavailable &&
        s.message()=="Сначала выберите кнопку клавишей Tab" && backend.executions==0,
        "Unfocused Enter silently executed a default action or bypassed localization");
    s.request_exit();
    auto modal=s.compose(1280,800);
    need(modal.keyboard_order.size()==2 && modal.keyboard_order[0].id=="confirm" && modal.keyboard_order[1].id=="cancel",
        "Confirmation exposed controls from the underlying screen");
    s.activate_focused(keyboard_focus(s,"cancel"));
    need(!s.confirming() && backend.executions==0 && !s.focused_hit(s.compose(1280,800)),
        "Keyboard Cancel executed an owner or retained modal focus");
    open_setup(s);
    s.activate_focused(keyboard_focus(s,"field:speed"));
    auto choices=s.compose(1280,800);
    need(choices.keyboard_order.size()==3 && choices.keyboard_order[0].id=="choice:0" && choices.keyboard_order.back().id=="back",
        "Choice popup exposed unrelated underlying controls");
    s.activate_focused(keyboard_focus(s,"choice:1"));
    need(std::get<std::string>(s.draft().values.at("speed"))=="epic" && backend.executions==0,
        "Keyboard choice did not use the actual draft value editor");
    s.activate_focused(keyboard_focus(s,"field:speed"));s.back();
    need(s.screen()=="setup" && !s.focused_hit(s.compose(1280,800)), "Escape did not cancel choice focus in place");
}

int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Pass the original assets root");
        const auto catalog = sources(argv[1]);
        state_and_routes(catalog); loading(catalog); selection_and_execution_admission(catalog);
        package_source_selection(catalog); select_saved_content(catalog);
        localized_saved_game_controls(catalog);
        load_actions_and_footer_layout(catalog);
        keyboard_existing_route(catalog); keyboard_scroll_and_guards(catalog); keyboard_modal_and_choices(catalog);
        input_and_extensions(catalog); external_adapters(catalog);
        unavailable_original_settings(catalog); conditional_dialog_presentation(catalog);
        std::cout << "Frontend menu: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
