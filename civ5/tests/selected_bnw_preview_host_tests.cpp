#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

using namespace civ5;

namespace {
void verify_actual_debug_read_owner(runtime::LiveSession& session) {
    const auto refuses=[&] {
        bool caught=false;
        try { (void)session.bnw_game_debug_mode(); }
        catch (const std::invalid_argument&) { caught=true; }
        assert(caught);
    };
    assert(!session.bnw_game_debug_mode());
    const auto original=session.checkpoint();
    const auto* allocation=&session.state();
    const auto generation=session.state_generation();
    session.mutable_state().setup.game_turn+=1;
    refuses();
    const auto dirty=session.checkpoint();
    session.restore(original);
    assert(!session.bnw_game_debug_mode() && &session.state()!=allocation &&
           session.state_generation()>generation);
    session.restore(dirty);
    refuses();
    session.restore(original);
    auto diagnostic=runtime::LiveSession::from_restored_bnw(session.source(),session.state());
    bool refused=false;
    try { (void)diagnostic.bnw_game_debug_mode(); }
    catch (const std::invalid_argument&) { refused=true; }
    assert(refused);
    refused=false;
    try { session.restore(diagnostic.checkpoint()); }
    catch (const std::invalid_argument&) { refused=true; }
    assert(refused && !session.bnw_game_debug_mode());
    const auto rejected=runtime::apply_live_action(session,[](runtime::LiveSessionState& state) {
        ++state.setup.game_turn;
        return runtime::LiveActionResult{runtime::LiveActionOutcome::rejected,"fixture rejection"};
    });
    assert(rejected.outcome==runtime::LiveActionOutcome::rejected && !session.bnw_game_debug_mode());
    try {
        (void)runtime::apply_live_action(session,[](runtime::LiveSessionState&)->runtime::LiveActionResult {
            throw std::invalid_argument("fixture trial throws");
        });
        assert(false);
    } catch (const std::invalid_argument&) {}
    assert(!session.bnw_game_debug_mode());
    const auto committed=runtime::apply_live_action(session,[](runtime::LiveSessionState& state) {
        ++state.setup.game_turn;
        return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,"fixture commit"};
    });
    assert(committed.outcome==runtime::LiveActionOutcome::committed);
    refuses();
    session.restore(original);
    auto moved=std::move(session);
    assert(!moved.bnw_game_debug_mode());
    refuses(); // a moved-from session cannot lend its old owner's cache
    session=std::move(moved);
    assert(!session.bnw_game_debug_mode() && !session.readiness().can_continue());
}
struct SyntheticBnwPreviewOwner {
    runtime::LiveSession session;
    runtime::ProductionProgressRules production;
    int *destructions;
    ~SyntheticBnwPreviewOwner() { ++*destructions; }
    ui::LiveBnwPreviewInput preview_input() {
        return {session, 5, production, {}, {}, {}};
    }
};

// Verify the current producer crosses the frame boundary. Later numeric
// ownership replaced the old assumption that every selected stat is absent.
void verify_bound_unit_read_stats(const ui::UiFrameSnapshot& frame,
                                  const ui::LiveBnwPreviewInput& input,
                                  std::int64_t unit_id) {
    const auto* world = frame.find<ui::WorldViewState>();
    const auto* read = frame.find<ui::LiveWorldViewUnitReadStatsFeed>();
    assert(world && world->selected_unit && read && read->player_id == 0 &&
           read->unit_id == unit_id && read->turn == frame.turn() &&
           read->frame_number == frame.frame());
    assert(input.frame_sources.selected_unit_read_stats_current &&
           input.frame_sources.selected_unit_read_stats_current(
               input.session, *read, unit_id, frame.frame()));
    assert(read->movement || read->strength);
    assert(read->movement ? !read->movement->empty() : !read->movement_blocker.empty());
    assert(read->strength ? !read->strength->empty() : !read->strength_blocker.empty());
    assert(world->selected_unit->movement == read->movement.value_or("") &&
           world->selected_unit->strength == read->strength.value_or(""));
    std::cout << "BOUND_UNIT_READ frame=" << frame.frame() << " unit=" << unit_id
              << " movement=" << read->movement.value_or("<unavailable>")
              << " strength=" << read->strength.value_or("<unavailable>") << std::endl;
}

std::shared_ptr<assets::LooseResourceIndex> ui_source(
    const std::filesystem::path &root, const std::filesystem::path &directory) {
    auto result = std::make_shared<assets::LooseResourceIndex>(root);
    result->scan({directory, {}, false});
    return result;
}

void verify_owned_live_frame_path(const std::filesystem::path &assets_root) {
    auto base = ui_source(assets_root, assets_root / "Assets/UI");
    auto expansion = ui_source(assets_root, assets_root / "Assets/DLC/Expansion/UI");
    auto bnw = ui_source(assets_root, assets_root / "Assets/DLC/Expansion2/UI");
    assets::ResourceMountTable resources(assets::resource_lookup(base));
    resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
    resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
    const auto registry = ui::UiContextRegistry::from_resources(resources);
    auto skin = ui::WorldViewSkin::from_resources(resources, registry);
    saves::SavedSession saved;
    saved.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        saved.snapshot, runtime::ResearchContext{}, {},
        runtime::PlayerResearchContext{}, runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{}, runtime::MaintenanceContext{},
        runtime::TreasuryContext{}, runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {5, 0, 0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {7, 0, 0})},
        {}, {}, {}, {}};
    state.setup.game_turn = 28;
    state.setup.players[5].alive = true;
    int destructions = 0;
    auto owner = std::shared_ptr<SyntheticBnwPreviewOwner>(
        new SyntheticBnwPreviewOwner{
            runtime::LiveSession::from_restored_bnw(std::move(saved), std::move(state)),
            {}, &destructions});
    std::weak_ptr<SyntheticBnwPreviewOwner> weak = owner;
    ui::with_owned_bnw_camera_input(owner,
        [&](const ui::LiveBnwPreviewInput *input) {
            owner.reset();
            assert(!weak.expired() && destructions == 0);
            auto binding = ui::bind_live_bnw_preview(*input);
            ui::LiveUiSources sources;
            sources.world_skin = &skin;
            ui::LiveUiHost host(registry, binding.frames, binding.backend.get(), sources);
            ui::UiScreenCompositor compositor(resources, registry, binding.backend.get());
            auto first = host.begin_frame(1280, 960);
            const auto *world = first.snapshot.find<ui::WorldViewState>();
            assert(first.snapshot.active_player_id() == 5 &&
                   first.snapshot.turn() == 28 && world &&
                   !world->end_turn_available && !input->session.readiness().can_continue());
            assert(first.world_view.has_value());
            const auto composed = compositor.compose(first, host.screens(), 1280, 960);
            assert(!composed.paints.empty());
            input->session.mutable_state().setup.game_turn = 29;
            auto next = host.begin_frame(1280, 960);
            assert(next.snapshot.turn() == 29 &&
                   next.snapshot.frame() == first.snapshot.frame() + 1);
            assert(host.dispatch(ui::UiOpenScreen{"ProductionPopup"}, first).status ==
                   ui::UiCommandStatus::invalid);
        });
    assert(weak.expired() && destructions == 1);
}
} // namespace

int main(int argc, char **argv) {
    assert(argc == 4);
    const std::filesystem::path selected_save(argv[1]);
    content::RulesDatabase inactive_database;
    content::SavedRulesActivationReport activation;
    content::GameplayPostprocessReport postprocess;
    ui::BnwSourceDecodeContract source;

    auto result = ui::make_selected_bnw_preview_host(selected_save / "absent.Civ5Save",
        inactive_database, activation, postprocess, source, 0);
    assert(!result && result.blocker->stage == ui::SelectedBnwPreviewStage::selected_save);

    // The local retail save is genuinely parsed before the activation gate.
    result = ui::make_selected_bnw_preview_host(selected_save, inactive_database,
        activation, postprocess, source, 0);
    assert(!result && result.blocker->stage == ui::SelectedBnwPreviewStage::activation);
    assert(result.blocker->reason.find("ready BNW profile") != std::string::npos);

    activation.profile_kind = runtime::ContentProfileKind::dlc;
    activation.profile_can_activate = true;
    activation.rules_database_ready = true;
    activation.base.schema_applied = true;
    activation.mod_plan.all_sources_present = true;
    result = ui::make_selected_bnw_preview_host(selected_save, inactive_database,
        activation, postprocess, source, 0);
    assert(!result && result.blocker->stage == ui::SelectedBnwPreviewStage::postprocess);

    postprocess.transaction_released = true;
    postprocess.strategic_resources_applied = true;
    postprocess.luxury_resources_applied = true;
    result = ui::make_selected_bnw_preview_host(selected_save, inactive_database,
        activation, postprocess, source, 0);
    assert(!result && result.blocker->stage == ui::SelectedBnwPreviewStage::source_contract);
    assert(result.blocker->reason.find("GameCore") != std::string::npos);

    source.gamecore_executable = selected_save;
    source.gamecore_sha256 = runtime::identify_file(selected_save).sha256;
    source.context.decode_map_regions = true;
    source.context.decode_teams = true;
    source.context.decode_all_players = true;
    source.context.decode_supporting_classes = true;
    source.context.all_players = saves::AllPlayersContext{};
    source.context.permitted_profile_sections.insert(1);
    result = ui::make_selected_bnw_preview_host(selected_save, inactive_database,
        activation, postprocess, source, 0);
    assert(!result && result.blocker->stage == ui::SelectedBnwPreviewStage::source_contract);
    assert(result.blocker->reason.find("profile sections") != std::string::npos);

    activation.dlc.packages.push_back({});
    activation.dlc.packages.back().id = "selected-bnw-dlc";
    activation.dlc.packages.back().missing = 1;
    result = ui::make_selected_bnw_preview_host(selected_save, inactive_database,
        activation, postprocess, source, 0);
    assert(!result && result.blocker->stage == ui::SelectedBnwPreviewStage::activation);
    assert(result.blocker->reason.find("selected-bnw-dlc") != std::string::npos);

    bool rejected = false;
    try { ui::SelectedBnwReadOnlyPreview preview(std::move(result)); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
    verify_owned_live_frame_path(argv[2]);

    auto bootstrap = ui::bootstrap_selected_bnw_preview(selected_save, argv[3], 0);
    if (bootstrap.preview.blocker)
        std::cerr << "probe stage=" << static_cast<int>(bootstrap.preview.blocker->stage)
                  << " reason=" << bootstrap.preview.blocker->reason << '\n';
    if (!std::filesystem::is_regular_file(
            std::filesystem::path(argv[3]) / "CivilizationV.exe")) {
        assert(bootstrap.preview.blocker &&
               bootstrap.preview.blocker->stage == ui::SelectedBnwPreviewStage::content_catalog &&
               bootstrap.preview.blocker->reason ==
                   "installation root requires real CivilizationV.exe and Assets/DLC");
        std::cout << "selected Gandhi content bootstrap blocked by split local installation\n";
        return 0;
    }
    assert(bootstrap.profile.enabled_dlc.size() == 14 &&
           bootstrap.profile.enabled_mods.empty() &&
           bootstrap.profile.all_requirements_located &&
           bootstrap.profile.can_activate);
    assert(bootstrap.gamecore_sha256 ==
           "c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5");
    assert(bootstrap.indexed_resources > 0);
    assert(bootstrap.activation && bootstrap.activation->rules_database_ready);
    assert(bootstrap.postprocess && bootstrap.postprocess->all_succeeded);
    if (bootstrap.preview.blocker)
        std::cout << "selected Gandhi bootstrap blocker stage="
                  << static_cast<int>(bootstrap.preview.blocker->stage)
                  << " reason=" << bootstrap.preview.blocker->reason << '\n';
    assert(bootstrap.preview.host && !bootstrap.preview.blocker);
    {
        ui::SelectedBnwReadOnlyPreview selected(std::move(bootstrap.preview));
        assert(!selected.session().readiness().can_continue());
        selected.with_camera_input([&](const ui::LiveBnwPreviewInput *input) {
            const std::filesystem::path root(argv[2]);
            auto base = ui_source(root, root / "Assets/UI");
            auto expansion = ui_source(root, root / "Assets/DLC/Expansion/UI");
            auto bnw = ui_source(root, root / "Assets/DLC/Expansion2/UI");
            base->scan({root / "Assets/Gameplay/XML/NewText/EN_US", {}, false});
            expansion->scan({root / "Assets/DLC/Expansion/Gameplay/XML/Text/en_US", {}, false});
            bnw->scan({root / "Assets/DLC/Expansion2/Gameplay/XML/Text/en_US", {}, false});
            assets::ResourceMountTable resources(assets::resource_lookup(base), {},
                                                assets::resource_keys(base));
            resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true,
                             {}, assets::resource_keys(expansion)});
            resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true,
                             {}, assets::resource_keys(bnw)});
            const auto catalog = ui::TopPanelLocalizationCatalog::read_active_bnw_resources(
                resources, "Language_en_US");
            auto localized_input = *input;
            localized_input.frame_sources.localize_city =
                [&catalog](std::string_view key) -> std::optional<std::string> {
                    const auto found = catalog.rows.find(std::string(key));
                    return found == catalog.rows.end() ? std::nullopt
                        : std::optional<std::string>(found->second);
                };
            auto binding = ui::bind_live_bnw_preview(std::move(localized_input));
            const auto registry = ui::UiContextRegistry::from_resources(resources);
            auto skin = ui::WorldViewSkin::from_resources(resources, registry);
            ui::LiveUiSources sources;
            sources.world_skin = &skin;
            ui::LiveUiHost host(registry, binding.frames, binding.backend.get(), sources);
            ui::UiScreenCompositor compositor(resources, registry, binding.backend.get());
            auto frame = host.begin_frame(1280, 960);
            const auto *world = frame.snapshot.find<ui::WorldViewState>();
            const auto *top = frame.snapshot.find<ui::TopPanelLiveFrame>();
            const auto *cities = frame.snapshot.find<ui::CityScreenFrame>();
            assert(cities && cities->cities.size() == 1 &&
                   cities->cities.front().id == 8192 &&
                   cities->cities.front().owner == 0 &&
                   cities->cities.front().x == 32 &&
                   cities->cities.front().y == 15 &&
                   cities->cities.front().population == 1 &&
                   cities->cities.front().name == std::optional<std::string>{"Delhi"});
            assert(frame.snapshot.active_player_id() == 0 &&
                   frame.snapshot.turn() == 0 && world && !world->end_turn_available);
            assert(top && top->players.contains(0));
            // The current bridge now supplies completed numeric quotes. Check
            // the SAME session/frame producer instead of requiring absence.
            assert(input->frame_sources.quotes && input->frame_sources.quotes_current);
            const auto top_quotes = input->frame_sources.quotes(input->session,
                                                               frame.snapshot.frame());
            assert(input->frame_sources.quotes_current(input->session, top_quotes,
                                                       frame.snapshot.frame()));
            assert(top_quotes.gold.contains(0) && top_quotes.gold.at(0).expenses &&
                   top_quotes.science.contains(0) &&
                   top_quotes.bnw_native_breakdowns.contains(0));
            const auto &top_player = top->players.at(0);
            assert(top_player.native_gold && top_player.native_gold->expenses &&
                   top_player.available.contains(ui::TopPanelMetric::gold) &&
                   top_player.native_gold->net_times100 == top_quotes.gold.at(0).net_times100 &&
                   top_player.native_gold->expenses->total == top_quotes.gold.at(0).expenses->total &&
                   top_player.native_breakdown &&
                   top_player.native_breakdown->owner_frame_number == frame.snapshot.frame());
            std::cout << "BOUND_TOP_PANEL frame=" << frame.snapshot.frame()
                      << " goldNetTimes100=" << top_player.native_gold->net_times100
                      << " expenses=" << top_player.native_gold->expenses->total << std::endl;
            const auto &saved_options = input->session.state().setup.game_options;
            const auto policy_option = [](const saves::PregameOption &option) {
                const auto name = std::string_view(option.name);
                return name.substr(0, name.find('\0')) == "GAMEOPTION_NO_POLICIES";
            };
            const auto policy = std::find_if(saved_options.begin(), saved_options.end(), policy_option);
            assert(top->no_policies ==
                   (std::count_if(saved_options.begin(), saved_options.end(), policy_option) == 1 &&
                    policy->value == 1));
            const auto turn_panel = ui::present_top_panel_frame(frame.snapshot,
                ui::TopPanelTextLocalizer{.text = [&catalog](std::string_view key,
                    const std::vector<std::string> &args) {
                    return catalog.format(key, args);
                }});
            assert(turn_panel && turn_panel->current_turn_text ==
                   catalog.format("TXT_KEY_TP_TURN_COUNTER", {"0"}) &&
                   turn_panel->current_date_text == "N/A");
            assert(!world->selected_unit && !world->plot_help &&
                   world->resource_icons.empty() && world->yield_icons.empty());
            // The restored notification owner publishes Gandhi's pending
            // choose-research entry; it is no longer an absent preview source.
            assert(world->notifications.size() == 1 &&
                   world->notifications.front().id == 3 &&
                   world->notifications.front().text ==
                       "Вы можете выбрать новое исследование." &&
                   world->notifications.front().requires_decision);
            const auto &owned_units = input->session.state().maintenance.players.at(0).units;
            assert(!owned_units.empty());
            const auto owned_unit_id = owned_units.begin()->first;
            auto selected_sources = input->frame_sources;
            selected_sources.selected_unit = [owned_unit_id] {
                return std::optional<std::int64_t>{owned_unit_id};
            };
            selected_sources.selected_unit_actions = [owned_unit_id](
                const runtime::LiveSession &, std::uint64_t) ->
                std::optional<ui::LiveWorldViewUnitActionFeed> {
                return ui::LiveWorldViewUnitActionFeed{
                    0, owned_unit_id, 0, 500, 77,
                    {{11, "Owner action", {}, {}, true}}};
            };
            selected_sources.localize_info = [&catalog](std::string_view key) {
                const auto found = catalog.rows.find(std::string(key));
                return found == catalog.rows.end() ? std::string(key) : found->second;
            };
            const auto selected_frame = ui::capture_live_session_ui_frame(
                input->session, 500, 0, selected_sources);
            assert(!selected_frame.find<ui::PopupFrame>());
            // Restored Gandhi data and selected BNW XML do not recreate the
            // Lua BUTTONPOPUP_FOUND_RELIGION event or its live choices.
            const auto *selected_world = selected_frame.find<ui::WorldViewState>();
            assert(selected_world && selected_world->selected_unit &&
                   selected_world->selected_unit->id == owned_unit_id &&
                   !selected_world->selected_unit->name.empty() &&
                   !selected_world->selected_unit->stat_boxes &&
                   selected_world->selected_unit->actions.size() == 1);
            verify_bound_unit_read_stats(selected_frame, *input, owned_unit_id);
            const auto stale_action_frame = ui::capture_live_session_ui_frame(
                input->session, 501, 0, selected_sources);
            const auto *stale_action_world =
                stale_action_frame.find<ui::WorldViewState>();
            assert(stale_action_world && stale_action_world->selected_unit &&
                   stale_action_world->selected_unit->actions.empty());
            const auto stale_action_plan = ui::plan_world_view(
                skin, registry, stale_action_frame, 1280, 960,
                binding.backend.get());
            assert(std::none_of(stale_action_plan.hits.begin(),
                stale_action_plan.hits.end(), [](const ui::WorldViewHit &hit) {
                    const auto *command =
                        std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = command
                        ? command->as<ui::WorldViewAction>() : nullptr;
                    return action &&
                        action->kind == ui::WorldViewActionKind::unit_action;
                }));
            selected_sources.selected_unit_actions = [owned_unit_id](
                const runtime::LiveSession &, std::uint64_t frame_number)
                -> std::optional<ui::LiveWorldViewUnitActionFeed> {
                return ui::LiveWorldViewUnitActionFeed{
                    0, owned_unit_id, 0, frame_number, 77,
                    {{11, "Owner action", {}, {}, true}}};
            };
            selected_sources.selected_unit_stat_boxes = [owned_unit_id](
                const runtime::LiveSession &, std::uint64_t)
                -> std::optional<ui::LiveWorldViewUnitStatBoxesFeed> {
                return ui::LiveWorldViewUnitStatBoxesFeed{
                    0, owned_unit_id, 0, 500, {true, true, true, false}};
            };
            const auto stale_stat_frame = ui::capture_live_session_ui_frame(
                input->session, 501, 0, selected_sources);
            const auto *stale_stat_world = stale_stat_frame.find<ui::WorldViewState>();
            assert(stale_stat_world && stale_stat_world->selected_unit &&
                   !stale_stat_world->selected_unit->stat_boxes);
            selected_sources.selected_unit_stat_boxes = [owned_unit_id](
                const runtime::LiveSession &, std::uint64_t frame_number)
                -> std::optional<ui::LiveWorldViewUnitStatBoxesFeed> {
                return ui::LiveWorldViewUnitStatBoxesFeed{
                    0, owned_unit_id, 0, frame_number, {true, true, true, false}};
            };
            const auto current_stat_frame = ui::capture_live_session_ui_frame(
                input->session, 502, 0, selected_sources);
            const auto *current_stat_world = current_stat_frame.find<ui::WorldViewState>();
            assert(current_stat_world && current_stat_world->selected_unit &&
                   current_stat_world->selected_unit->stat_boxes &&
                   current_stat_world->selected_unit->stat_boxes->xp);
            verify_bound_unit_read_stats(current_stat_frame, *input, owned_unit_id);
            const auto selected_plan = ui::plan_world_view(skin, registry,
                selected_frame, 1280, 960, binding.backend.get());
            const auto owner_action = std::find_if(selected_plan.hits.begin(),
                selected_plan.hits.end(), [](const ui::WorldViewHit &hit) {
                    const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = command
                        ? command->as<ui::WorldViewAction>() : nullptr;
                    return action && action->kind == ui::WorldViewActionKind::unit_action &&
                        action->id == 11;
                });
            assert(owner_action != selected_plan.hits.end() &&
                   !owner_action->enabled);
            selected_sources.selected_unit = [] {
                return std::optional<std::int64_t>{-1};
            };
            assert(!ui::capture_live_session_ui_frame(input->session, 501, 0,
                selected_sources).find<ui::WorldViewState>()->selected_unit);
            assert(world->city_list_launcher && frame.world_view);
            const auto launcher = std::find_if(frame.world_view->hits.begin(),
                frame.world_view->hits.end(), [](const ui::WorldViewHit &hit) {
                    const auto *open = std::get_if<ui::UiOpenScreen>(&hit.command);
                    return open && open->id == "CityList";
                });
            assert(launcher != frame.world_view->hits.end() && launcher->enabled);
            assert(host.click(frame, launcher->bounds.x + 1,
                launcher->bounds.y + 1).command.status == ui::UiCommandStatus::navigation);
            auto list_frame = host.begin_frame(1280, 960);
            const auto list = compositor.compose(list_frame, host.screens(), 1280, 960);
            assert(list.focused_context == "CityList" &&
                   std::any_of(list.paints.begin(), list.paints.end(),
                       [](const ui::WorldViewPaint &paint) {
                           return paint.kind == ui::WorldViewPaintKind::text &&
                               paint.text == "Delhi";
                       }));
            const auto city_row = std::find_if(list.hits.begin(), list.hits.end(),
                [](const ui::LiveUiHit &hit) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = gameplay ? gameplay->as<ui::WorldViewAction>() : nullptr;
                    return action && action->kind == ui::WorldViewActionKind::open_city &&
                        action->id == 8192;
                });
            assert(city_row != list.hits.end() && city_row->enabled);
            const auto panel_hit = std::find_if(list.hits.begin(), list.hits.end(),
                [](const ui::LiveUiHit &hit) {
                    return hit.context_id == "CityList" && !hit.enabled;
                });
            assert(panel_hit != list.hits.end());
            const auto blank = ui::click_composed_ui(host, list_frame, list, 20, 20);
            assert(blank.consumed && blank.command.status ==
                ui::UiCommandStatus::unavailable &&
                host.screens().focused_screen() == std::optional<std::string>{"CityList"});
            assert(ui::click_composed_ui(host, list_frame, list,
                city_row->bounds.x + 1, city_row->bounds.y + 1).command.status ==
                ui::UiCommandStatus::navigation);
            assert(host.screens().focused_screen() == std::optional<std::string>{"CityView"});
            auto reopened = host.begin_frame(1280, 960);
            assert(host.dispatch(ui::UiCloseScreen{"CityView"}, reopened).status ==
                   ui::UiCommandStatus::navigation);
            reopened = host.begin_frame(1280, 960);
            const auto reopened_list = compositor.compose(reopened, host.screens(), 1280, 960);
            const auto close_hit = std::find_if(reopened_list.hits.begin(),
                reopened_list.hits.end(), [](const ui::LiveUiHit &hit) {
                    return std::holds_alternative<ui::UiCloseScreen>(hit.command) && hit.enabled;
                });
            assert(close_hit != reopened_list.hits.end() &&
                ui::click_composed_ui(host, reopened, reopened_list,
                    close_hit->bounds.x + 1, close_hit->bounds.y + 1).command.status ==
                    ui::UiCommandStatus::navigation && !host.screens().focused_screen());
            ui::WorldViewState no_city_world;
            no_city_world.player_id = 0;
            ui::CityScreenFrame no_cities;
            no_cities.player_id = 0;
            ui::LiveUiHost empty_host(registry, [no_city_world, no_cities] {
                return ui::UiFrameSnapshot(1, 0, 0)
                    .with(std::make_shared<const ui::WorldViewState>(no_city_world))
                    .with(std::make_shared<const ui::CityScreenFrame>(no_cities));
            }, nullptr, sources);
            auto empty_frame = empty_host.begin_frame(1280, 960);
            assert(empty_frame.snapshot.find<ui::WorldViewState>()->city_list_launcher);
            assert(empty_host.dispatch(ui::UiOpenScreen{"CityList"}, empty_frame).status ==
                   ui::UiCommandStatus::navigation);
            empty_frame = empty_host.begin_frame(1280, 960);
            const auto empty_list = compositor.compose(empty_frame,
                empty_host.screens(), 1280, 960);
            assert(empty_list.focused_context == "CityList" &&
                   std::none_of(empty_list.paints.begin(), empty_list.paints.end(),
                       [](const ui::WorldViewPaint &paint) {
                           return paint.kind == ui::WorldViewPaintKind::text &&
                               paint.text == "Delhi";
                       }));
            assert(!frame.snapshot.find<ui::ProgressionIdeologyPopupEvent>());
            const auto ideology_source = ui::read_progression_screen_source(
                resources, ui::ProgressionScreen::ideology);
            const auto ideology_plan = ui::plan_progression_screen(ideology_source,
                ui::ProgressionScreen::ideology, frame.snapshot);
            assert(ideology_plan.items.empty() &&
                   !ideology_plan.unresolved.empty());
            const auto &owned = input->session.state();
            const auto owned_balance = owned.treasury.players.at(0).balance_times100 / 100;
            assert(top && top->players.size() == 1 &&
                   top->players.at(0).city_count ==
                       static_cast<int>(owned.cities.players.at(0).cities.size()) &&
                   top->players.at(0).gold_balance == owned_balance &&
                   top->players.at(0).outgoing_trade_routes.has_value() &&
                   !top->players.at(0).trade && !top->players.at(0).tourism &&
                   !top->players.at(0).faith);
            const auto composed = compositor.compose(frame, host.screens(), 1280, 960);
            assert(!composed.paints.empty());

            auto city_input = *input;
            city_input.frame_sources.selected_city = [] {
                return std::optional<std::int64_t>{8192};
            };
            city_input.frame_sources.localize_info = [&catalog](std::string_view key) {
                const auto found = catalog.rows.find(std::string(key));
                return found == catalog.rows.end() ? std::string(key) : found->second;
            };
            auto plain_city_input = city_input;
            plain_city_input.notification_owner = nullptr;
            auto plain_city_binding = ui::bind_live_bnw_preview(
                std::move(plain_city_input));
            const auto plain_city_frame = plain_city_binding.frames();
            const auto plain_choices = plain_city_binding.backend->capture_choices(
                plain_city_frame);
            const auto warrior_text = catalog.rows.find("TXT_KEY_UNIT_WARRIOR");
            assert(plain_choices && warrior_text != catalog.rows.end() &&
                   std::any_of(plain_choices->entries.begin(),
                       plain_choices->entries.end(), [&](const auto &choice) {
                           return choice.authored_type == "UNIT_WARRIOR" &&
                               choice.localized_name == warrior_text->second &&
                               choice.availability.status ==
                                   ui::UiCommandStatus::unavailable;
                       }));
            auto city_binding = ui::bind_live_bnw_preview(std::move(city_input));
            ui::UiScreenCompositor city_compositor(resources, registry,
                city_binding.backend.get(),
                [&catalog](std::string_view key) -> std::optional<std::string> {
                    const auto found = catalog.rows.find(std::string(key));
                    return found == catalog.rows.end() ? std::nullopt
                        : std::optional<std::string>(found->second);
                });
            const auto production_source = ui::read_city_screen_source(
                resources, "ProductionPopup");
            ui::LiveUiSources city_sources;
            city_sources.production_backend = city_binding.backend.get();
            city_sources.production_popup_source = &production_source;
            ui::LiveUiHost city_host(registry, city_binding.frames,
                                     city_binding.backend.get(), city_sources);
            auto city_frame = city_host.begin_frame(1280, 960);
            const auto *delhi = ui::selected_city(city_frame.snapshot);
            assert(delhi && delhi->owner == 0 && delhi->id == 8192 &&
                   delhi->x == 32 && delhi->y == 15 && delhi->population == 1);
            assert(city_host.dispatch(ui::city_open_command(), city_frame).status ==
                   ui::UiCommandStatus::navigation);
            city_frame = city_host.begin_frame(1280, 960);
            const auto *city_view_delhi = ui::selected_city(city_frame.snapshot);
            const auto city_composed = city_compositor.compose(city_frame,
                city_host.screens(), 1280, 960);
            const auto label_key = city_view_delhi && city_view_delhi->queue.empty()
                ? "TXT_KEY_CITYVIEW_CHOOSE_PROD"
                : "TXT_KEY_CITYVIEW_CHANGE_PROD";
            const auto label_text = catalog.rows.find(label_key);
            const auto has_authored_label = label_text != catalog.rows.end() &&
                std::any_of(city_composed.paints.begin(), city_composed.paints.end(),
                    [&](const ui::WorldViewPaint &paint) {
                        return paint.text == label_text->second;
                    });
            assert(city_composed.focused_context == "CityView" &&
                   city_view_delhi && !city_composed.paints.empty() &&
                   has_authored_label == city_view_delhi->queue_resolved);
            std::cout << "selected Delhi owned queue: "
                      << (city_view_delhi->queue_resolved ? "resolved" : "unresolved")
                      << ", orders=" << city_view_delhi->queue.size() << '\n';
            assert(city_host.dispatch(ui::city_production_popup_command(), city_frame).status ==
                   ui::UiCommandStatus::navigation);
            city_frame = city_host.begin_frame(1280, 960);
            const auto *choices = city_frame.snapshot.find<ui::ProductionPopupChoices>();
            const auto production_composed = compositor.compose(city_frame,
                city_host.screens(), 1280, 960);
            assert(choices && !choices->entries.empty() &&
                   production_composed.focused_context == "ProductionPopup");
            const auto warrior = std::find_if(choices->entries.begin(),
                choices->entries.end(), [](const ui::ProductionPopupChoice &choice) {
                    return choice.authored_type == "UNIT_WARRIOR";
                });
            assert(warrior != choices->entries.end() &&
                   warrior_text != catalog.rows.end() &&
                   warrior->localized_name == warrior_text->second &&
                   warrior->availability.status == ui::UiCommandStatus::unavailable);
            for (const auto [width, height] : {
                     std::pair{1280, 960}, std::pair{1920, 1080}}) {
                const auto presented = ui::present_live_ui_frame(city_host,
                    city_compositor, width, height,
                    [=](const ui::LiveUiFrame &live,
                        const ui::CityScreenSource &source) {
                        return ui::measure_civis_production_popup(
                            live, source, width, height);
                    });
                const auto *live_choices = presented.live.snapshot.find<
                    ui::ProductionPopupChoices>();
                const auto layout = ui::layout_production_popup(
                    production_source, width, height);
                assert(live_choices && live_choices->player_id == 0 &&
                       live_choices->city_id == 8192 && layout &&
                       live_choices->visible_rows ==
                           static_cast<std::size_t>(height == 960 ? 13 : 16) &&
                       presented.composed.production_scroll_panel &&
                       presented.composed.production_scroll_panel->x ==
                           layout->scroll_panel.x &&
                       presented.composed.production_scroll_panel->y ==
                           layout->scroll_panel.y &&
                       presented.composed.production_scroll_panel->height ==
                           layout->scroll_panel.height);
                for (const auto &hit : presented.composed.hits) {
                    const auto *gameplay = std::get_if<ui::UiGameplayCommand>(
                        &hit.command);
                    const auto *choice = gameplay
                        ? gameplay->as<ui::CityChooseProduction>() : nullptr;
                    if (choice) assert(choice->city_id == 8192 && !hit.enabled);
                }
            }
            city_frame = city_host.begin_frame(1280, 960);
            const auto *current_choices = city_frame.snapshot.find<
                ui::ProductionPopupChoices>();
            assert(current_choices && !current_choices->entries.empty());
            const auto &first = current_choices->entries.front();
            assert(first.availability.status != ui::UiCommandStatus::ready &&
                   city_host.dispatch(ui::city_choose_production(8192,
                       first.order_type, first.item_id), city_frame).status ==
                       ui::UiCommandStatus::unavailable);
            assert(city_host.dispatch(ui::UiCloseScreen{"ProductionPopup"}, city_frame).status ==
                   ui::UiCommandStatus::navigation);
            city_frame = city_host.begin_frame(1280, 960);
            assert(city_host.dispatch(ui::UiCloseScreen{"CityView"}, city_frame).status ==
                   ui::UiCommandStatus::navigation);
            verify_actual_debug_read_owner(input->session);
        });
        std::cout << "selected Gandhi read-only preview restored; actual Game debug reset/Read owner PASS; full-turn readiness remains blocked\n";
    }

    std::cout << "selected BNW preview host admission and owned live frame PASS\n";
}
