#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F action) {
    try { action(); } catch (const std::exception &) { return; }
    throw std::runtime_error("Required rejection missing");
}
runtime::LiveSession session() {
    using namespace runtime;
    LiveSessionState state{saves::SavedSessionSnapshot{}, ResearchContext{}, {},
        PlayerResearchContext{}, CityYieldContext{}, BnwUnhappinessContext{}, MaintenanceContext{},
        TreasuryContext{}, ScienceContext{}, ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw, {5, 0, 0}),
         SimulationRandom(RandomAlgorithm::bnw, {7, 0, 0})}};
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    state.setup.players[7].team = 3;
    state.maintenance.players[7].units[21].type = 4;
    auto &world = state.world;
    world.game_state.layout = world.map_header.layout = bnw;
    world.map_header.grid = {2, 1};
    world.map_header.guid[0] = 42;
    world.pregame.emplace().archive.integers["active_player"] = 7;
    world.pregame->archive.integers["game_turn"] = 12;
    auto &map = world.map_projection.emplace(); map.grid = {2, 1};
    auto &plots = world.map_plots.emplace(); plots.layout = bnw; plots.dimensions.teams = 80;
    auto &teams = world.teams.emplace(); teams.layout = bnw; teams.dimensions.teams = 64;
    teams.teams.resize(64);
    for (std::uint64_t i = 0; i < 2; ++i) {
        saves::SavedMapCellProjection cell; cell.index = cell.x = i; cell.y = 0;
        cell.terrain_type = 2; cell.revealed_teams = std::set<std::int64_t>{3};
        map.cells.push_back(cell);
        saves::PlotObservation plot; plot.index = plot.x = i; plot.y = 0;
        saves::PlotArrayObservation raw{"visibility", 1, std::vector<std::int64_t>(80, 0)};
        raw.values[3] = i == 0 ? 1 : 0;
        plot.arrays.push_back(std::move(raw)); plots.plots.push_back(std::move(plot));
    }
    rebuild_bnw_live_plot_visibility(state);
    saves::SavedSession source; source.snapshot = world; source.source.bytes = {1, 2, 3};
    return LiveSession::from_restored_bnw(std::move(source), std::move(state));
}
ui::UiContextRegistry registry() {
    return ui::UiContextRegistry::from_documents(content::read_xml_metadata(
        R"(<Context><LuaContext ID="WorldView" FileName="Assets/UI/InGame/WorldView/WorldView"/><LuaContext ID="TestModal" FileName="TestModal" Modal="1"/></Context>)"),
        content::read_xml_metadata("<Context/>"));
}
struct Backend final : ui::UiGameplayBackend {
    int executions = 0;
    bool throwing = false;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &,
                                     const ui::UiFrameSnapshot &) const override {
        return {ui::UiCommandStatus::ready, {}};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &,
                                const ui::UiFrameSnapshot &) override {
        ++executions;
        if (throwing) throw std::runtime_error("uncertain native effect");
        return {ui::UiCommandStatus::performed, {}};
    }
};
// Deliberate callback fault fixtures, not native city appearance evidence.
void callback_invalidation() {
    {
        auto replaced=session();
        const auto saved=replaced.checkpoint();
        std::weak_ptr<const runtime::LiveSessionState> original=replaced.read_state_lease();
        bool pinned_during_callback=false;
        ui::LiveSessionUiSources replacing_sources;
        replacing_sources.game_menu_quick_save=[&](const auto &,auto frame) {
            replaced.restore(saved);
            pinned_during_callback=!original.expired();
            ui::LiveGameMenuQuickSaveFeed value;
            value.player_id=7;value.turn=12;value.frame_number=frame;
            value.owner_sequence=1;value.visible=true;
            return std::optional(value);
        };
        rejects([&]{ (void)ui::capture_live_session_ui_frame(replaced,1,7,replacing_sources); });
        check(pinned_during_callback && original.expired(),
            "UI callback replacement lost its read lifetime or leaked retired state");
    }
    auto live=session();
    auto &city=live.mutable_state().cities.players[7].cities[9];
    city.owner=7;city.id=9;city.population=5;city.x=0;city.y=0;
    runtime::rebuild_bnw_live_plot_visibility(live.mutable_state());
    bool invalidate=false;
    ui::BnwClientFrameOwner *current_owner=nullptr;
    ui::LiveSessionUiSources sources;
    sources.city_banner_appearance=[](const auto &,auto active,auto player,auto id,auto frame) {
        ui::BnwCityBannerAppearance receipt;
        receipt.active_player_id=active;receipt.player_id=player;receipt.city_id=id;
        receipt.frame_number=frame;receipt.turn=12;
        return std::optional(receipt);
    };
    sources.city_banner_appearance_current=[&](const auto &,const auto &) {
        if(invalidate){invalidate=false;current_owner->notify_world_changed();}
        return true;
    };
    ui::BnwClientFrameOwner owner(live,"callback-fixture",7,sources);current_owner=&owner;
    const auto frame=owner.capture({800,600,800,600,96},{});
    check(frame->ui.find<ui::UiOwnerFences>(),"Fixture did not exercise retained owner fence");
    invalidate=true;int composed=0;
    check(!owner.present(frame,[&](const auto &){++composed;})&&composed==0&&!owner.current_frame(),
        "Publication invalidated during an owner check still composed");

    ui::LiveUiHost *current_host=nullptr;
    bool replace=false;
    auto guards=std::make_shared<ui::UiOwnerFences>();
    guards->checks.push_back([&] {
        if(replace){replace=false;(void)current_host->begin_frame(800,600);}
        return true;
    });
    const auto snapshot=ui::UiFrameSnapshot(1,7,12).with<ui::UiOwnerFences>(guards);
    ui::LiveUiHost host(registry(),[snapshot]{return snapshot;},nullptr);current_host=&host;
    const auto old=host.begin_frame(800,600);replace=true;
    check(!host.is_current_frame(old),"UI sequence replaced during an owner check remained current");
}
void run() {
    auto live = session();
    ui::BnwClientFrameOwner owner(live, "fixture-session", 7);
    const auto provider = owner.ui_provider();
    rejects([&] { provider(); });
    const ui::ClientViewport viewport{800, 600, 1200, 900, 144};
    auto frame = owner.capture(viewport, {});
    check(frame && frame->stamp.identity.player == 7 && frame->stamp.identity.team == 3 &&
          frame->stamp.identity.turn == 12 && frame->stamp.identity.map_guid[0] == 42,
          "Live identity was not published");
    check(frame->visibility.plots[0] == ui::WorldViewPlotFog::visible &&
          frame->visibility.plots[1] == ui::WorldViewPlotFog::explored,
          "UI and scene did not share live visibility");
    const auto ui_snapshot = provider();
    check(ui_snapshot.find<ui::BnwClientFrameStamp>()->identity == frame->stamp.identity,
          "UI provider recaptured a different world frame");
    Backend backend;
    ui::LiveUiHost host(registry(), provider, &backend);
    // Even an identical restore and no selected unit must expire the entire
    // published UI/world frame. Repeated allocation cannot revive it.
    const auto original_generation=live.state_generation();
    const auto saved=live.checkpoint();
    auto another=session();const auto foreign_saved=another.checkpoint();
    rejects([&]{live.restore(foreign_saved);});
    check(live.state_generation()==original_generation&&owner.is_current(frame),
        "Failed foreign restore changed generation/publication");
    for(int i=0;i<16;++i) {
        const auto previous=live.state_generation();live.restore(saved);
        check(live.state_generation()>previous&&!owner.is_current(frame)&&
            !ui::ui_owners_current(ui_snapshot),"Identical restore/ABA revived an old frame");
    }
    frame=owner.capture(viewport,{});
    auto host_frame = host.begin_frame(1200, 900);
    int composed = 0;
    check(owner.present(frame, [&](const auto &captured) {
        ++composed; check(&captured == frame.get(), "Presenter did not receive owned frame");
    }) && composed == 1, "Production presentation not consumed");
    owner.select({21, {}});
    check(owner.dispatch(host, host_frame, frame, ui::UiOpenScreen{"WorldView"}).status ==
          ui::UiCommandStatus::invalid, "Stale selection command executed");
    check(!owner.present(frame, [&](const auto &) { ++composed; }) && composed == 1,
          "Stale frame composed");
    rejects([&] { owner.select({999, {}}); });
    auto selected = owner.capture(viewport, {});
    check(selected->stamp.selection.unit == 21, "Selection owner did not publish unit");
    auto selected_ui = host.begin_frame(1200, 900);
    auto foreign = session(); ui::BnwClientFrameOwner other(foreign, "fixture-session", 7);
    auto foreign_frame = other.capture(viewport, {});
    check(other.dispatch(host, selected_ui, foreign_frame, ui::UiOpenScreen{"WorldView"}).status ==
          ui::UiCommandStatus::invalid, "Another owner accepted a matching fixture identity");
    const auto old_revision = selected->stamp.identity.revision;
    auto &changed = live.mutable_state(); changed.world.map_projection->cells[0].terrain_type = 77;
    runtime::rebuild_bnw_live_plot_visibility(changed);
    owner.notify_world_changed();
    check(selected->world.map_projection->cells[0].terrain_type == 2,
          "Published scene changed through live mutable alias");
    auto next = owner.capture(viewport, {});
    check(next->world.map_projection->cells[0].terrain_type == 77 &&
          next->stamp.identity.revision > old_revision, "Live scene did not republish mutation");
    check(!owner.present(next, [&](const auto &) { owner.notify_world_changed(); }),
          "World mutation during composition allowed swap");
    auto navigable = owner.capture(viewport, {});
    auto current_ui = host.begin_frame(1200, 900);
    auto navigation = owner.dispatch(host, current_ui, navigable, ui::UiOpenScreen{"WorldView"});
    check(navigation.status == ui::UiCommandStatus::navigation && !owner.current_frame(),
          "Navigation retained measured old UI frame");
    rejects([&] { provider(); });
    auto executable = owner.capture(viewport, {});
    auto executable_ui = host.begin_frame(1200, 900);
    const auto command = ui::UiGameplayCommand::make("WorldView", std::int64_t{123});
    const auto executed = owner.dispatch(host, executable_ui, executable, command);
    check(executed.status == ui::UiCommandStatus::performed && backend.executions == 1 &&
          !owner.current_frame(), "Performed gameplay command retained old world revision");
    check(owner.dispatch(host, executable_ui, executable, command).status == ui::UiCommandStatus::invalid &&
          backend.executions == 1, "Performed gameplay command repeated");
    auto uncertain = owner.capture(viewport, {});
    auto uncertain_ui = host.begin_frame(1200, 900);
    backend.throwing = true;
    rejects([&] { owner.dispatch(host, uncertain_ui, uncertain, command); });
    check(!owner.current_frame() && backend.executions == 2,
          "Throwing backend retained a retryable frame");
    backend.throwing = false;
    check(!owner.capture({0, 0, 0, 0, 96}, {}), "Minimized viewport published frame");
    owner.notify_world_changed();
    live.mutable_state();
    rejects([&] { owner.capture(viewport, {}); });
    check(!owner.current_frame(), "Missing current visibility retained old frame");
}
} // namespace
int main() {
    try { callback_invalidation(); run(); std::cout << "BNW production frame tests passed\n"; return 0; }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
