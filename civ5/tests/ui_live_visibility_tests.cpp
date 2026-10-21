#include "civ5/ui/live_session_frame.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace civ5;

namespace {
constexpr auto bnw = saves::GameStateLayout::expansion2_version1;

runtime::LiveSession synthetic() {
    saves::SavedSession source;
    runtime::LiveSessionState state{source.snapshot, runtime::ResearchContext{}, {},
        runtime::PlayerResearchContext{}, runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{}, runtime::MaintenanceContext{},
        runtime::TreasuryContext{}, runtime::ScienceContext{}, runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {1, 0, 0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {2, 0, 0})}};
    state.world.game_state.layout = state.world.map_header.layout = bnw;
    state.world.map_header.grid = {3, 1};
    state.world.pregame.emplace();
    state.world.pregame->layout = bnw;
    state.world.pregame->archive.integers["active_player"] = 7;
    auto &map = state.world.map_projection.emplace();
    map.grid = {3, 1};
    auto &plots = state.world.map_plots.emplace();
    plots.layout = bnw;
    plots.dimensions.teams = 80;
    auto &teams = state.world.teams.emplace();
    teams.layout = bnw;
    teams.teams.resize(64);
    for (std::uint64_t plot = 0; plot < 3; ++plot) {
        saves::SavedMapCellProjection cell;
        cell.index = cell.x = plot;
        cell.y = 0;
        cell.revealed_teams.emplace();
        if (plot == 1) cell.revealed_teams->insert(3);
        map.cells.push_back(std::move(cell));
        saves::PlotObservation saved;
        saved.index = saved.x = plot;
        saved.y = 0;
        saves::PlotArrayObservation raw{"visibility", 1, std::vector<std::int64_t>(80, 0)};
        raw.values[3] = plot == 0 ? -4 : plot == 2 ? 2 : 0;
        raw.values[7] = plot == 0 ? 9 : 0; // player ID must never be used as team
        saved.arrays.push_back(std::move(raw));
        plots.plots.push_back(std::move(saved));
    }
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    state.setup.players[7].team = 3;
    state.setup.players[8].alive = true;
    state.setup.players[8].team = 4;
    runtime::rebuild_bnw_live_plot_visibility(state);
    source.snapshot = state.world;
    return runtime::LiveSession::from_restored_bnw(std::move(source), std::move(state));
}

ui::UiContextRegistry registry() {
    return ui::UiContextRegistry::from_documents(
        content::read_xml_metadata(R"(<Context>
            <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
        </Context>)"), content::read_xml_metadata("<Context/>"));
}

void synthetic_visibility() {
    auto session = synthetic();
    auto provider = ui::make_live_session_frame_provider(session, 7);
    ui::LiveUiHost host(registry(), provider, nullptr);
    auto first = host.begin_frame(1280, 960);
    const auto *world = first.snapshot.find<ui::WorldViewState>();
    assert(world && world->map_visibility && !world->end_turn_available);
    const auto &fog = *world->map_visibility;
    assert(fog.player_id == 7 && fog.team_id == 3 && fog.width == 3 && fog.height == 1);
    assert(fog.plots == std::vector<ui::WorldViewPlotFog>({
        ui::WorldViewPlotFog::hidden, ui::WorldViewPlotFog::explored,
        ui::WorldViewPlotFog::visible}));
    ui::MinimapTileGrid minimap(3, 1, false);
    for (int x = 0; x < 3; ++x) assert(minimap.populate_terrain_cell(x, 0, 0));
    assert(ui::apply_live_bnw_minimap_fog(minimap, fog) == 2);
    assert((minimap.at(0, 0).byte2 & 1) == 1 &&
           (minimap.at(1, 0).byte2 & 1) == 0 &&
           (minimap.at(2, 0).byte2 & 1) == 0);

    bool rejected = false;
    try { (void)ui::capture_live_session_ui_frame(session, 5, 8); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
    session.mutable_state().setup.game_turn = 13;
    auto next = host.begin_frame(1280, 960);
    const auto *dirty = next.snapshot.find<ui::WorldViewState>();
    assert(next.snapshot.turn() == 13 && dirty && !dirty->map_visibility);
    assert(host.dispatch(ui::UiOpenScreen{"ProductionPopup"}, first).status ==
           ui::UiCommandStatus::invalid);
    assert(world->map_visibility->plots.front() == ui::WorldViewPlotFog::hidden);
}

void selected_visibility(const std::filesystem::path &save,
                         const std::filesystem::path &game_root,
                         const std::filesystem::path &assets_root) {
    auto bootstrap = ui::bootstrap_selected_bnw_preview(save, game_root, 0);
    if (bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    ui::SelectedBnwReadOnlyPreview preview(std::move(bootstrap.preview));
    const auto &state = preview.session().state();
    const auto player = state.setup.players.at(0);
    const auto source = [&](const std::filesystem::path &directory) {
        auto index = std::make_shared<assets::LooseResourceIndex>(assets_root);
        index->scan({directory, {}, false});
        return index;
    };
    auto base = source(assets_root / "Assets/UI");
    auto expansion = source(assets_root / "Assets/DLC/Expansion/UI");
    auto bnw_ui = source(assets_root / "Assets/DLC/Expansion2/UI");
    assets::ResourceMountTable resources(assets::resource_lookup(base));
    resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
    resources.mount({2, 0, 0, assets::resource_lookup(bnw_ui), true, true, {}, {}});
    auto contexts = ui::UiContextRegistry::from_resources(resources);
    auto skin = ui::WorldViewSkin::from_resources(resources, contexts);
    ui::LiveUiSources sources;
    sources.world_skin = &skin;
    ui::LiveUiHost host(contexts,
        ui::make_live_session_frame_provider(preview.session(), 0), nullptr, sources);
    auto frame = host.begin_frame(1280, 960);
    const auto *world = frame.snapshot.find<ui::WorldViewState>();
    assert(world && world->map_visibility && !world->end_turn_available);
    assert(frame.world_view && frame.world_view->map_visibility &&
           frame.world_view->map_visibility->plots == world->map_visibility->plots);
    const auto &fog = *frame.world_view->map_visibility;
    assert(fog.team_id == player.team && fog.player_id == 0 &&
           fog.plots.size() == state.world.map_projection->cells.size());
    std::size_t hidden = 0, visible = 0;
    const auto &cache = runtime::bnw_live_plot_visibility_cache(state);
    for (std::size_t plot = 0; plot < fog.plots.size(); ++plot) {
        const auto count = cache.counts.at(plot * cache.wire_slots +
                                            static_cast<std::uint64_t>(player.team));
        if (fog.plots[plot] == ui::WorldViewPlotFog::hidden) ++hidden;
        if (fog.plots[plot] == ui::WorldViewPlotFog::visible) ++visible;
        assert((fog.plots[plot] == ui::WorldViewPlotFog::visible) == (count > 0));
    }
    assert(hidden > 0 && visible > 0);
    ui::MinimapTileGrid minimap(static_cast<std::uint32_t>(fog.width),
                                static_cast<std::uint32_t>(fog.height),
                                state.world.map_projection->wrap_x);
    for (std::size_t plot = 0; plot < fog.plots.size(); ++plot) {
        const auto x = static_cast<std::int32_t>(plot % fog.width);
        const auto y = static_cast<std::int32_t>(plot / fog.width);
        assert(minimap.populate_terrain_cell(x, y, 0));
    }
    assert(ui::apply_live_bnw_minimap_fog(minimap, fog) >= visible);
    for (std::size_t plot = 0; plot < fog.plots.size(); ++plot) {
        const auto x = static_cast<std::int32_t>(plot % fog.width);
        const auto y = static_cast<std::int32_t>(plot / fog.width);
        assert(((minimap.at(x, y).byte2 & 1) == 1) ==
               (fog.plots[plot] == ui::WorldViewPlotFog::hidden));
    }
    std::cout << "selected BNW map visibility hidden=" << hidden
              << " visible=" << visible << " team=" << player.team << '\n';
}
} // namespace

int main(int argc, char **argv) {
    assert(argc == 4);
    synthetic_visibility();
    selected_visibility(argv[1], argv[2], argv[3]);
    std::cout << "live BNW UI visibility PASS\n";
}
