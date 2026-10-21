#include "civ5/ui/frontend_process_backend.hpp"
#include "civ5/saves/civis_save.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace ui;
namespace fs = std::filesystem;
unsigned checks = 0;
void need(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
UiCommandResult ready() { return {UiCommandStatus::ready, {}}; }
FrontendRequest action(std::string operation) {
    FrontendRequest request; request.operation = std::move(operation); return request;
}
template<class Callable> bool refused_connection(Callable &&call) {
    try { call(); } catch (const std::logic_error &) { return true; }
    return false;
}
// Routing fixture only. Counters/local mutations prove adapter forwarding and
// refusal semantics, not real settings persistence, a party, Source22, native
// rendering, or any connected production application owner's availability.
struct Application final : FrontendBackend {
    mutable unsigned probes = 0;
    unsigned effects = 0;
    UiCommandResult capability = ready();
    UiCommandResult result{UiCommandStatus::performed, {}};
    mutable std::function<void()> probe;
    std::function<void(const FrontendRequest &)> effect;
    FrontendRequest last;
    UiCommandResult availability(const FrontendRequest &) const override {
        ++probes; if (probe) probe(); return capability;
    }
    UiCommandResult execute(const FrontendRequest &request) override {
        ++effects; last = request; if (effect) effect(request); return result;
    }
};
void imported(const fs::path &path) {
    // Valid content-container framing only. These three bytes deliberately
    // provide no game codec/scene/client authority or playable-party proof.
    saves::CivisSaveDocument document;
    document.flags = 1;
    document.sections.push_back({static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload), 0, {1, 2, 3}});
    saves::SaveHeader header; header.turn = 12; header.civilization = "Process fixture";
    document.sections.push_back(saves::make_save_metadata_section({std::string(64, 'a'), header}));
    saves::write_civis_save(path, document);
}
FrontendRequest load(const FrontendCatalogContentAdapter &content) {
    auto request = action("load_game");
    request.save = content.data().saves.front(); request.draft.save = request.save->id;
    return request;
}
void empty_and_settings(const FrontendCatalogContentAdapter &content) {
    FrontendProcessBackend empty(content);
    const auto loaded = load(content);
    const auto unavailable_load = empty.availability(loaded);
    need(unavailable_load.status == UiCommandStatus::unavailable &&
        unavailable_load.reason.find("party window and decoded-scene executor") != std::string::npos,
        "Missing application owner did not expose its concrete party-window blocker");
    need(empty.availability(action("start_game")).reason.find("new-game generator") != std::string::npos,
        "Missing game generator was hidden behind a generic capability");
    need(empty.availability(action("apply_options")).reason.find("profile persistence") != std::string::npos,
        "Missing settings/persistence owner was not explained");
    need(empty.execute(loaded).status == UiCommandStatus::unavailable && !empty.should_close,
        "Empty delegate fabricated a party effect or closed the process");
    need(refused_connection([&] { empty.connect_application({}); }), "Null application was accepted");
    auto self = std::shared_ptr<FrontendBackend>(&empty, [](FrontendBackend *) {});
    need(refused_connection([&] { empty.connect_application(self); }), "Process adapter accepted itself as its application");

    FrontendProcessBackend backend(content);
    auto owner = std::make_shared<Application>();
    bool applied = false;
    owner->effect = [&](const auto &request) {
        applied = request.operation == "apply_options" && request.draft.values.at("volume") == FrontendValue(std::int64_t(73));
    };
    backend.connect_application(owner);
    auto apply = action("apply_options"); apply.draft.values.emplace("volume", std::int64_t(73));
    need(backend.availability(apply).status == UiCommandStatus::ready && !applied && owner->effects == 0,
        "Capability query performed settings work");
    need(backend.execute(apply).status == UiCommandStatus::performed && applied && owner->effects == 1,
        "Mock settings-routing effect was not forwarded once with its requested value");
    need(!backend.should_close, "Settings apply incorrectly closed the frontend process");
    need(refused_connection([&] { backend.connect_application(std::make_shared<Application>()); }),
        "An occupied application connection was replaced");

    FrontendProcessBackend bad_capability(content);
    auto bad = std::make_shared<Application>();
    bad_capability.connect_application(bad);
    for (const auto status : {UiCommandStatus::performed, UiCommandStatus::navigation, UiCommandStatus::uncertain}) {
        bad->capability = {status, {}};
        need(bad_capability.execute(action("apply_options")).status == UiCommandStatus::unavailable && bad->effects == 0,
            "A capability query forged an effect result without any application effect");
    }
}
void uncertain_results(const FrontendCatalogContentAdapter &content) {
    FrontendProcessBackend backend(content);
    auto owner = std::make_shared<Application>();
    int persisted = 0;
    owner->effect = [&](const auto &) { ++persisted; throw std::runtime_error("Persisted before reporting failure"); };
    backend.connect_application(owner);
    const auto result = backend.execute(action("apply_options"));
    need(result.status == UiCommandStatus::uncertain && persisted == 1 && owner->effects == 1,
        "Effectful exception was reported as a safely retryable refusal");
    need(backend.execute(action("apply_options")).status == UiCommandStatus::unavailable && persisted == 1 && owner->effects == 1,
        "An uncertain settings effect was executed twice");
    need(refused_connection([&] { backend.connect_application(std::make_shared<Application>()); }),
        "Uncertain execution permitted a replacement application owner");
    need(backend.availability(action("exit_game")).status == UiCommandStatus::ready &&
        backend.execute(action("exit_game")).status == UiCommandStatus::performed && backend.should_close && owner->effects == 1,
        "Local process exit depended on an uncertain delegate or repeated its effect");
    need(backend.execute(action("exit_game")).status == UiCommandStatus::unavailable,
        "Closing process accepted another operation");

    FrontendProcessBackend explicit_uncertain(content);
    auto declared = std::make_shared<Application>(); declared->result = {UiCommandStatus::uncertain, "Pending reconciliation"};
    explicit_uncertain.connect_application(declared);
    need(explicit_uncertain.execute(action("apply_options")).status == UiCommandStatus::uncertain &&
        explicit_uncertain.execute(action("apply_options")).status == UiCommandStatus::unavailable && declared->effects == 1,
        "Explicit uncertain result did not latch against duplicate effects");

    FrontendProcessBackend invalid_completion(content);
    auto still_ready = std::make_shared<Application>(); still_ready->result = ready();
    invalid_completion.connect_application(still_ready);
    need(invalid_completion.execute(action("apply_options")).status == UiCommandStatus::uncertain &&
        invalid_completion.execute(action("apply_options")).status == UiCommandStatus::unavailable &&
        still_ready->effects == 1 && !invalid_completion.should_close,
        "An executor returned readiness as completion and allowed its effect to repeat");

    FrontendProcessBackend foreign_exception(content);
    auto throws_unknown = std::make_shared<Application>(); throws_unknown->effect = [](const auto &) { throw 7; };
    foreign_exception.connect_application(throws_unknown);
    need(foreign_exception.execute(action("apply_options")).status == UiCommandStatus::uncertain &&
        foreign_exception.execute(action("apply_options")).status == UiCommandStatus::unavailable && throws_unknown->effects == 1,
        "Non-standard exception escaped effect reconciliation or permitted a retry");
}
void callbacks_and_thread(const FrontendCatalogContentAdapter &content) {
    FrontendProcessBackend backend(content);
    auto owner = std::make_shared<Application>();
    backend.connect_application(owner);
    bool nested_query = false, nested_effect = false, reconnect = false, nested_selection = false, nested_admission = false;
    owner->probe = [&] {
        nested_query = backend.availability(action("apply_options")).status == UiCommandStatus::unavailable;
        nested_effect = backend.execute(action("exit_game")).status == UiCommandStatus::unavailable;
        reconnect = refused_connection([&] { backend.connect_application(std::make_shared<Application>()); });
        nested_selection = backend.check_selection(action("start_game")).result.status == UiCommandStatus::unavailable;
        nested_admission = backend.check_content(action("start_game")).result.status == UiCommandStatus::unavailable;
    };
    need(backend.availability(action("apply_options")).status == UiCommandStatus::ready && nested_query && nested_effect && reconnect,
        "Application capability callback reentered the adapter or replaced its owner");
    need(nested_selection && nested_admission && owner->effects == 0 && !backend.should_close,
        "Nested selection validation bypassed the process callback guard");
    owner->probe = {};
    UiCommandStatus foreign_query = UiCommandStatus::invalid, foreign_effect = UiCommandStatus::invalid;
    UiCommandStatus foreign_selection = UiCommandStatus::invalid;
    UiCommandStatus foreign_admission = UiCommandStatus::invalid;
    bool foreign_connect = false;
    std::thread foreign([&] {
        foreign_query = backend.availability(action("apply_options")).status;
        foreign_effect = backend.execute(action("exit_game")).status;
        foreign_selection = backend.check_selection(action("start_game")).result.status;
        foreign_admission = backend.check_content(action("start_game")).result.status;
        foreign_connect = refused_connection([&] { backend.connect_application(std::make_shared<Application>()); });
    });
    foreign.join();
    need(foreign_query == UiCommandStatus::unavailable && foreign_effect == UiCommandStatus::unavailable && foreign_connect,
        "Foreign thread obtained a process operation or connection");
    need(foreign_selection == UiCommandStatus::unavailable && foreign_admission == UiCommandStatus::unavailable &&
        !backend.should_close && owner->effects == 0,
        "Foreign selection query bypassed creator-thread custody");
}
void blocked_services(const FrontendCatalogContentAdapter &content) {
    FrontendProcessBackend backend(content);
    auto owner = std::make_shared<Application>(); backend.connect_application(owner);
    for (const auto *operation : {"start_multiplayer", "reconnect", "open_workshop", "open_beyond_earth_store"}) {
        const auto before = owner->probes;
        need(backend.availability(action(operation)).status == UiCommandStatus::unavailable &&
            backend.execute(action(operation)).status == UiCommandStatus::unavailable &&
            owner->probes == before && owner->effects == 0,
            "Unavailable network/Steam action escaped to the application delegate");
    }
}
void load_admission(const fs::path &path, std::shared_ptr<const content::Catalog> catalog) {
    FrontendCatalogContentAdapter content(catalog, {path});
    FrontendProcessBackend backend(content);
    auto owner = std::make_shared<Application>(); backend.connect_application(owner);
    auto selected = load(content);
    need(backend.availability(selected).status == UiCommandStatus::ready && owner->effects == 0,
        "Selected-slot capability query performed a load");
    selected.save->path = path.parent_path() / "unselected.CivisSave";
    need(backend.execute(selected).status == UiCommandStatus::unavailable && owner->effects == 0 && !backend.should_close,
        "Direct caller replaced the selected save path before an application effect");
    selected = load(content); selected.draft.save = "another-slot";
    need(backend.execute(selected).status == UiCommandStatus::unavailable && owner->effects == 0,
        "Direct caller bypassed selected-slot identity validation");
    selected = load(content);
    std::ofstream(path, std::ios::binary | std::ios::trunc) << "changed after selection";
    need(backend.execute(selected).status == UiCommandStatus::unavailable && owner->effects == 0 && !backend.should_close,
        "Changed selected payload passed direct-call full admission");

    const auto fresh_path = path.parent_path() / "fresh.CivisSave";
    imported(fresh_path);
    FrontendCatalogContentAdapter fresh(catalog, {fresh_path});
    FrontendProcessBackend legacy(fresh);
    auto new_game = std::make_shared<Application>(); legacy.connect_application(new_game);
    auto request = load(fresh); request.operation = "start_game";
    need(legacy.execute(request).status == UiCommandStatus::invalid && new_game->effects == 0,
        "Legacy new-game request carrying a load slot escaped to an application effect");
    request.save.reset();
    need(legacy.execute(request).status == UiCommandStatus::performed && new_game->effects == 1 &&
        !new_game->last.save && new_game->last.draft.save == fresh.data().saves.front().id && !legacy.should_close,
        "Independent retained Load draft prevented a legitimate new-game request or attached its old slot");
}
} // namespace
int main() {
    const auto base = fs::absolute(fs::current_path() / "frontend-process-fixtures").lexically_normal();
    const auto root = base / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    try {
        if (root.parent_path() != base) throw std::runtime_error("Fixture path escaped its explicit directory");
        fs::create_directories(root);
        const auto path = root / "selected.CivisSave";
        imported(path);
        const auto catalog = std::make_shared<content::Catalog>();
        FrontendCatalogContentAdapter content(catalog, {path});
        empty_and_settings(content); uncertain_results(content); callbacks_and_thread(content); blocked_services(content);
        load_admission(path, catalog);
        fs::remove_all(root);
        std::cout << "Frontend process backend: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
