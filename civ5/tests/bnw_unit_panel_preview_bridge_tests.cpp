#include "civ5/ui/bnw_unit_panel_preview_bridge.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/bnw_info_default_form.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_unit_finish_moves.hpp"
#include "civ5/runtime/bnw_busy_state.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
std::int64_t &scalar(saves::UnitCoreObservation &core, std::string_view name) {
    for (auto &field : core.fields) if (field.name && std::string_view(field.name) == name && field.values.size() == 1)
        return field.values.front();
    throw std::runtime_error("Missing current unit scalar");
}
ui::TopPanelLocalizationCatalog locale(const std::filesystem::path &root, bool russian) {
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
    auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
    const auto language = russian ? "RU_RU" : "en_US";
    base->scan({root / "Assets/Gameplay/XML/NewText" / (russian ? "RU_RU" : "EN_US"), {}, false});
    expansion->scan({root / "Assets/DLC/Expansion/Gameplay/XML/Text" / language, {}, false});
    bnw->scan({root / "Assets/DLC/Expansion2/Gameplay/XML/Text" / language, {}, false});
    assets::ResourceMountTable resources(assets::resource_lookup(base), {}, assets::resource_keys(base));
    resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, assets::resource_keys(expansion)});
    resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, assets::resource_keys(bnw)});
    return ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources,
        russian ? "Language_RU_RU" : "Language_en_US");
}
void run(int argc, char **argv) {
    check(argc == 3 || argc == 4, "Expected actual save/game root/optional installed SDK root");
    if (argc == 4) {
        for (const auto &source : runtime::kBnwUnitOrderOwnerSources)
            check(runtime::identify_file(std::filesystem::path(argv[3]) / source.path).sha256 == source.sha256,
                "Actual native getter/FinishMoves source pin differs");
        for (const auto &source : ui::kBnwInfoDefaultFormSources)
            check(runtime::identify_file(std::filesystem::path(argv[3]) / source.path).sha256 == source.sha256,
                "Actual default Info form source pin differs");
    }
    auto bootstrap = ui::bootstrap_selected_bnw_preview(argv[1], argv[2], 0);
    check(bootstrap.preview.host && !bootstrap.preview.blocker, "Actual selected Gandhi host unavailable");
    auto &host = *bootstrap.preview.host;
    const runtime::BnwUnitOrderKey key{0, 16385}; // Explicit source-tested warrior selection input, not first-unit inference.
    auto client = runtime::start_bnw_unit_presentation_client(host.session().state(), host.identity(), host.database());
    runtime::select_bnw_unit_presentation(host.session().state(), client, host.database(), key, client.client_revision());
    ui::BnwUnitPanelPreviewBridge bridge(host.session(), host.database(), host.types(), host.identity(), 0);
    const auto initial = bridge.capture(host.session(), key.second, 21);
    check(initial && initial->movement == "2/2 [ICON_MOVES]" && initial->strength == "8 [ICON_STRENGTH]" &&
        initial->movement_blocker.empty() && initial->strength_blocker.empty() &&
        bridge.matches(host.session(), *initial, key.second, 21), "Actual source native unit getters failed");
    auto input = host.preview_input();
    check(input.frame_sources.selected_unit_read_stats && input.frame_sources.selected_unit_read_stats_current,
        "Production host did not hand off native UnitPanel bridge");
    input.frame_sources.selected_unit = [&] {
        return client.selected_unit() ? std::optional<std::int64_t>(client.selected_unit()->second) : std::nullopt;
    };
    const auto initial_catalog = locale(argv[2], false);
    std::string selected_name_key;
    const auto name_query=host.database().query_rows({"SELECT Description FROM Units WHERE ID=?",
        {host.session().state().maintenance.players.at(0).units.at(key.second).type}},
        [&](const content::RulesRowView &row) { selected_name_key=row.text("Description").value_or(""); },2);
    check(name_query.all_succeeded() && !selected_name_key.empty(),"Actual selected Info Description key unavailable");
    input.frame_sources.localize_info = [&](std::string_view name) { return ui::bnw_info_default_form(initial_catalog,name); };
    ui::BnwClientFrameOwner frame_owner(host.session(), host.identity().payload_sha256, 0, input.frame_sources);
    frame_owner.select({key.second, std::nullopt});
    const auto rendered_frame = frame_owner.capture({64, 64, 64, 64, 96}, {});
    const auto game_root = std::filesystem::path(argv[2]);
    const auto registry = ui::UiContextRegistry::from_documents(
        content::read_xml(game_root / "Assets/DLC/Expansion2/UI/InGame/InGame.xml"),
        content::read_xml(game_root / "Assets/UI/InGame/WorldView/WorldView.xml"));
    ui::LiveUiHost ui_host(registry, frame_owner.ui_provider(), nullptr);
    const auto live_ui_frame = ui_host.begin_frame(64, 64);
    check(rendered_frame && frame_owner.is_current(rendered_frame) && ui_host.is_current_frame(live_ui_frame) &&
        rendered_frame->ui.find<ui::LiveWorldViewUnitReadStatsFeed>() && ui::ui_owners_current(rendered_frame->ui),
        "Actual retained client/UI hosts did not accept native stats owner fences");
    for (bool russian : {false, true}) {
        const auto catalog = locale(argv[2], russian);
        input.frame_sources.localize_info = [&](std::string_view name) { return ui::bnw_info_default_form(catalog,name); };
        const auto frame = ui::capture_live_session_ui_frame(host.session(), 21, 0, input.frame_sources);
        const auto *world = frame.find<ui::WorldViewState>();
        check(world && world->selected_unit && world->selected_unit->movement == *initial->movement &&
            world->selected_unit->strength == *initial->strength && world->selected_unit->actions.empty(),
            "Real EN/RU selected frame consumer kept N/A or invented legal actions");
        check(world->selected_unit->name == ui::bnw_info_default_form(catalog,selected_name_key) &&
            world->selected_unit->name.find('|')==std::string::npos,
            "Actual selected default Info form differs");
        auto stale_sources = input.frame_sources;
        stale_sources.selected_unit_read_stats = [&](const runtime::LiveSession &, std::uint64_t, std::int64_t) {
            auto wrong = *initial; --wrong.frame_number; return std::optional{wrong};
        };
        const auto stale = ui::capture_live_session_ui_frame(host.session(), 21, 0, stale_sources);
        const auto *stale_world = stale.find<ui::WorldViewState>();
        check(stale_world && stale_world->selected_unit && stale_world->selected_unit->movement.empty() &&
            stale_world->selected_unit->strength.empty(), "Shared consumer accepted stale native stats frame");
        std::cout << (russian ? "RU" : "EN") << " actual_selected=" << world->selected_unit->name << ' '
            << world->selected_unit->movement << ' ' << world->selected_unit->strength << '\n';
    }
    input.frame_sources.localize_info = [&](std::string_view key_name) {
        return ui::bnw_info_default_form(initial_catalog,key_name);
    };
    runtime::BnwGameActivationOwner activation; activation.identity = host.identity();
    const auto binding = runtime::bind_bnw_end_turn_import(host.session().source(), activation);
    activation.busy_state = runtime::bind_bnw_busy_state_original_import(host.session().source(), host.identity());
    activation.pathfinder_cache = runtime::make_bnw_pathfinder_cache_control_import_owner(host.session().source(), host.identity());
    const auto &before = host.session().state();
    const auto cancelled = runtime::execute_bnw_end_turn_request_draft(before, activation, binding, host.database(),
        {0, before.setup.game_turn, false, runtime::bnw_end_turn_input_revision(before, activation, binding, host.database(), 0)});
    check(bool(cancelled.draft), "Actual source request owner for FinishMoves unavailable");
    const auto finished = runtime::execute_bnw_finish_moves_draft(*cancelled.draft, binding, client, host.database(),
        {key, before.setup.game_turn, runtime::bnw_finish_moves_input_revision(*cancelled.draft, binding, client,
            host.database(), key), client.client_revision()});
    check(finished.draft && finished.draft->movement_changed && !host.session().readiness().can_continue(),
        "Genuine native FinishMoves transaction unavailable or full turn readiness invented");
    // Caller publication of the actual completed native draft, not an inferred
    // cache difference or a synthetic move value. The original game/save stay immutable.
    host.session().mutable_state() = finished.draft->request.state;
    client = finished.draft->presentation;
    const auto current = bridge.capture(host.session(), key.second, 22);
    check(current && current->movement == "0/2 [ICON_MOVES]" && current->strength == "8 [ICON_STRENGTH]" &&
        !bridge.matches(host.session(), *initial, key.second, 21), "Native FinishMoves did not reach same live read owner");
    check(!frame_owner.is_current(rendered_frame) && !ui_host.is_current_frame(live_ui_frame),
        "Retained native client/UI host accepted stale actually changed unit stats");
    bool composed = false;
    check(!frame_owner.present(rendered_frame, [&](const ui::BnwClientFrame &) { composed = true; }) && !composed &&
        frame_owner.dispatch(ui_host, live_ui_frame, rendered_frame, ui::UiOpenScreen{"GameMenu"}).status ==
            ui::UiCommandStatus::invalid, "Stale native unit stats allowed presentation or UI dispatch");
    const auto updated_frame = ui::capture_live_session_ui_frame(host.session(), 22, 0, input.frame_sources);
    const auto *updated = updated_frame.find<ui::WorldViewState>();
    check(updated && updated->selected_unit && updated->selected_unit->movement == *current->movement,
        "Real frame consumer did not adopt actually published FinishMoves");
    auto &state = host.session().mutable_state();
    const auto team = state.setup.players.at(0).team;
    saves::PlotArrayObservation *extra = nullptr;
    for (auto &array : state.world.teams->teams.at(team).arrays)
        if (array.name && std::string_view(array.name) == "extra_moves") extra = &array;
    check(extra && extra->values.size() == 5, "Actual source team move array absent");
    // Labeled same-frame fault probe. This operand is deliberately outside
    // bnw_unit_order_world_revision; native requery must still reject the old label.
    ++extra->values.at(2);
    check(runtime::bnw_unit_order_world_revision(state, host.identity(), host.database()) == current->source_world_revision &&
        !bridge.matches(host.session(), *current, key.second, 22) && !ui::ui_owners_current(updated_frame),
        "Omitted team movement operand evaded native requery/retained UI fence gate");
    --extra->values.at(2);
    saves::UnitCoreObservation *selected_core=nullptr;
    for (auto &owner : state.world.all_players->players) if (owner.saved_player_id==key.first)
        for (auto &candidate : owner.units.units) if (scalar(candidate.core,"m_iID")==key.second)
            selected_core=&candidate.core;
    check(selected_core,"Current selected source core is absent");
    auto &core=*selected_core;
    scalar(core, "m_bEmbarked") = 1;
    const auto unsupported = bridge.capture(host.session(), key.second, 23);
    check(unsupported && !unsupported->movement && !unsupported->strength &&
        !unsupported->movement_blocker.empty() && !unsupported->strength_blocker.empty(), "Unsupported unit branches became zero stats");
    scalar(core, "m_bEmbarked") = 0;
    check(!bridge.capture(host.session(), key.second, 0) && !bridge.capture(host.session(), -1, 24), "Invalid selection/frame accepted");
    const auto generation=host.session().state_generation();
    const auto checkpoint = host.session().checkpoint(); host.session().restore(checkpoint);
    host.session().restore(checkpoint); // Repeated same-valued replacement can reuse an older address.
    check(host.session().state_generation()>generation+1 &&
        !bridge.capture(host.session(), key.second, 24) && !ui::ui_owners_current(updated_frame),
        "Repeated replaced borrowed state/UI frame accepted");
    ui::BnwUnitPanelPreviewBridge rebound(host.session(),host.database(),host.types(),host.identity(),0);
    check(rebound.capture(host.session(),key.second,24).has_value(),"Explicit native unit read owner rebind failed");
    check(!host.session().readiness().can_continue(), "UnitPanel enabled a full turn");
    std::cout << "PASS actual native UnitPanel -> EN/RU current frame; real FinishMoves0/2; requery/stamp/lifecycle guards\n";
}
}
int main(int argc, char **argv) {
    try { run(argc, argv); } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
