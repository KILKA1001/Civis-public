#include "civ5/assets/asset_source.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/scene/saved_fog_of_war.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/saves/vp_all_players_profile.hpp"
#include "civ5/saves/vp_game_progression_profile.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include "civ5/saves/vp_team_profile.hpp"
#include "civ5/terrain/world_layout.hpp"
#include "civ5/ui/top_panel_saved_state.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>

namespace fs = std::filesystem;
namespace {
std::string json_string(const char *value) { return std::string("\"") + value + '"'; }
void random_preview_json(civ5::runtime::SimulationRandom random) {
    std::cout << "{\"seed_before\":\"" << random.state().seed
              << "\",\"calls_before\":\"" << random.state().calls
              << "\",\"resets\":\"" << random.state().resets
              << "\",\"bound\":1000,\"values\":[";
    for (unsigned i = 0; i < 8; ++i) {
        if (i)
            std::cout << ',';
        std::cout << random.draw(1000);
    }
    std::cout << "],\"seed_after\":\"" << random.state().seed
              << "\",\"calls_after\":\"" << random.state().calls << "\"}";
}
void optional_integer_json(const std::optional<std::int32_t> &value) {
    if (value) std::cout << *value;
    else std::cout << "null";
}
void top_panel_saved_summary_json(const civ5::ui::TopPanelSavedProjection &value,
                                  std::uint64_t active_player) {
    std::cout << "{\"active_player\":" << active_player << ",\"city_count\":";
    optional_integer_json(value.city_count);
    std::cout << ",\"gold\":";
    optional_integer_json(value.gold);
    std::cout << ",\"gold_per_turn_from_diplomacy\":";
    optional_integer_json(value.gold_per_turn_from_diplomacy);
    std::cout << ",\"total_happiness\":";
    optional_integer_json(value.total_happiness);
    std::cout << ",\"total_unhappiness\":";
    optional_integer_json(value.total_unhappiness);
    std::cout << ",\"golden_age_progress\":";
    if (value.golden_age_progress)
        std::cout << "{\"raw\":" << value.golden_age_progress->raw
                  << ",\"scale\":" << value.golden_age_progress->scale << '}';
    else std::cout << "null";
    std::cout << ",\"culture\":";
    if (value.culture)
        std::cout << "{\"raw\":" << value.culture->raw
                  << ",\"scale\":" << value.culture->scale << '}';
    else std::cout << "null";
    std::cout << ",\"next_policy_cost\":";
    optional_integer_json(value.next_policy_cost);
    std::cout << ",\"unit_production_maintenance_mod\":";
    optional_integer_json(value.unit_production_maintenance_mod);
    std::cout << '}';
}
void plot_art_summary_json(const civ5::saves::SavedMapTeamArtObservation &map) {
    std::uint64_t visible = 0, revealed = 0, unrevealed = 0, unknown_reveal = 0;
    std::uint64_t visible_unrevealed = 0;
    std::uint64_t numeric_improvements = 0, hashed_improvements = 0, routes = 0;
    std::uint64_t nonnegative_numeric_improvements = 0, nonzero_hashes = 0;
    std::uint64_t visible_numeric_improvements = 0, visible_nonzero_hashes = 0, visible_routes = 0;
    for (const auto &plot : map.plots) {
        const bool in_view = plot.visibility_count > 0 && plot.revealed == true;
        visible += plot.visibility_count > 0;
        if (!plot.revealed) ++unknown_reveal;
        else if (*plot.revealed) ++revealed;
        else ++unrevealed;
        visible_unrevealed += plot.visibility_count > 0 && plot.revealed == false;
        numeric_improvements += plot.revealed_improvement.has_value();
        hashed_improvements += plot.revealed_improvement_hash.has_value();
        routes += plot.revealed_route >= 0;
        const bool numeric = plot.revealed_improvement && *plot.revealed_improvement >= 0;
        const bool hashed = plot.revealed_improvement_hash && *plot.revealed_improvement_hash != 0;
        nonnegative_numeric_improvements += numeric;
        nonzero_hashes += hashed;
        visible_numeric_improvements += in_view && numeric;
        visible_nonzero_hashes += in_view && hashed;
        visible_routes += in_view && plot.revealed_route >= 0;
    }
    std::cout << "{\"active_player\":" << map.active_player << ",\"active_team\":" << map.active_team
              << ",\"plots\":" << map.plots.size() << ",\"visible\":" << visible
              << ",\"revealed\":" << revealed << ",\"unrevealed\":" << unrevealed
              << ",\"unknown_reveal\":" << unknown_reveal
              << ",\"visible_unrevealed\":" << visible_unrevealed
              << ",\"numeric_improvement_fields\":" << numeric_improvements
              << ",\"hashed_improvement_fields\":" << hashed_improvements
              << ",\"nonnegative_numeric_improvements\":" << nonnegative_numeric_improvements
              << ",\"nonzero_improvement_hashes\":" << nonzero_hashes
              << ",\"routes\":" << routes
              << ",\"visible_numeric_improvements\":" << visible_numeric_improvements
              << ",\"visible_nonzero_improvement_hashes\":" << visible_nonzero_hashes
              << ",\"visible_routes\":" << visible_routes
              << ",\"visible_nonzero_improvement_hash_samples\":[";
    unsigned samples = 0;
    for (const auto &plot : map.plots) {
        if (plot.visibility_count <= 0 || plot.revealed != true || !plot.revealed_improvement_hash ||
            *plot.revealed_improvement_hash == 0)
            continue;
        if (samples) std::cout << ',';
        std::cout << "{\"plot_index\":" << plot.plot_index
                  << ",\"hash\":" << *plot.revealed_improvement_hash << '}';
        if (++samples == 8) break;
    }
    std::cout << "]}";
}
}

int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save;
        std::string layout = "expansion2";
        bool all_players = false;
        bool random_preview = false;
        bool terrain_random_seed_summary = false;
        bool plot_art_summary = false;
        bool fow_state_summary = false;
        bool top_panel_saved_summary = false;
        bool map_after_non_sqlite_db = false;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout << "civis_session_snapshot --save FILE_OR_DIRECTORY "
                             "[--layout expansion2|vp542] [--all-players] [--random-preview] "
                             "[--terrain-random-seed-summary] "
                             "[--plot-art-summary] [--fow-state-summary] "
                             "[--top-panel-saved-summary] [--map-after-non-sqlite-db]\n"
                             "Reads one owned x64 saved-session snapshot. VP mode uses the "
                             "documented 5.4.2 wire dimensions. Random preview advances copies "
                             "of the saved generators, using the selected source reference.\n"
                             "--map-after-non-sqlite-db is a read-only diagnostic; it never "
                             "validates a missing SQLite image or enables game continuation.\n";
                return 0;
            }
            if (option == L"--all-players") {
                all_players = true;
                continue;
            }
            if (option == L"--random-preview") {
                random_preview = true;
                continue;
            }
            if (option == L"--terrain-random-seed-summary") {
                terrain_random_seed_summary = true;
                continue;
            }
            if (option == L"--plot-art-summary") {
                plot_art_summary = true;
                continue;
            }
            if (option == L"--fow-state-summary") {
                fow_state_summary = true;
                continue;
            }
            if (option == L"--top-panel-saved-summary") {
                top_panel_saved_summary = true;
                all_players = true;
                continue;
            }
            if (option == L"--map-after-non-sqlite-db") {
                map_after_non_sqlite_db = true;
                continue;
            }
            if (i + 1 >= argc)
                throw std::runtime_error("Missing option value");
            if (option == L"--save")
                save = argv[++i];
            else if (option == L"--layout")
                layout = assets::path_utf8(argv[++i]);
            else
                throw std::runtime_error("Unknown option");
        }
        if (save.empty())
            throw std::runtime_error("Supply --save FILE_OR_DIRECTORY");
        const auto selection = runtime::select_save(save);
        if (!selection.selected)
            throw std::runtime_error("No supported save header");
        const auto &candidate = selection.candidates[*selection.selected];
        saves::SavedSessionDecodeContext context;
        if (runtime::ascii_lower(layout) == "vp542") {
            context.layout = saves::GameStateLayout::vp542_corpus;
            context.roster = {64, 64, {}};
            context.roster.profile_extensions.emplace<saves::VpRosterWireDimensions>(
                saves::VpRosterWireDimensions{88});
            context.progression = {64, 5, {}};
            context.progression.profile_extensions.emplace<saves::VpGameProgressionWireDimensions>(
                saves::VpGameProgressionWireDimensions{{255, 147, 541, 10, 4, 5, 0}});
            context.trade = {43, 17};
            context.plots = {64, 64, 43, 17, 0};
            context.teams = {64, 64, 63, 80, 80, 5, 43, 17, {}};
            context.teams.profile_extensions.emplace<saves::VpTeamContentDimensions>(
                saves::VpTeamContentDimensions{2, 38, 83, 2, 75, 10, 147, 541, 617, 9, 59, 69, 5});
            if (all_players) {
                context.decode_map_regions = true;
                context.decode_teams = true;
                context.decode_all_players = true;
                context.decode_supporting_classes = true;
                saves::AllPlayersContext players;
                players.player_count = 64;
                players.strategy = {43, 63, 17, 4, {}};
                players.strategy.profile_extensions.emplace<saves::VpPlayerStrategyWireDimensions>(
                    saves::VpPlayerStrategyWireDimensions{267, 15, 38, 37, 26, 4});
                players.diplomacy = {43, 20, 63, 64, 7, 5, 113, 60};
                players.profile_extensions.emplace<saves::VpPlayerKnowledgeWireDimensions>(
                    saves::VpPlayerKnowledgeWireDimensions{83, 38});
                players.planning = {43, 63, 80};
                players.traits = saves::PlayerTraitsWireDimensions{17, 9};
                players.trade_culture = saves::PlayerTradeCultureWireDimensions{43, 17};
                players.profile_extensions.emplace<saves::VpAllPlayersWireDimensions>(
                    saves::VpAllPlayersWireDimensions{56, 5, 617});
                context.all_players = std::move(players);
            }
        } else if (runtime::ascii_lower(layout) != "expansion2") {
            throw std::runtime_error("Unknown layout; use expansion2 or vp542");
        } else if (all_players) {
            context.decode_map_regions = true;
            context.decode_teams = true;
            context.decode_all_players = true;
            context.decode_supporting_classes = true;
            context.all_players = saves::AllPlayersContext{};
        }
        context.skip_non_sqlite_image_for_map_diagnostic = map_after_non_sqlite_db;
        const auto session = saves::read_saved_session(candidate.path, context);
        const auto &loaded = session.source;
        const auto &snapshot = session.snapshot;
        if (terrain_random_seed_summary && !snapshot.pregame)
            throw std::runtime_error("Terrain random seed summary needs saved pregame state");
        std::optional<saves::SavedMapTeamArtObservation> plot_team_art;
        if (plot_art_summary || fow_state_summary || top_panel_saved_summary) {
            if (!snapshot.pregame || !snapshot.map_plots)
                throw std::runtime_error("Plot art summary needs saved setup and map plots");
            plot_team_art = saves::observe_saved_map_team_art(*snapshot.pregame, *snapshot.map_plots);
        }
        std::optional<scene::SavedFogOfWarCounts> fow_counts;
        std::size_t fow_visibility_cells = 0, fow_culling_kept = 0;
        int fow_grid_mode = 0;
        bool fow_dirty_after_tick = false;
        if (fow_state_summary) {
            const auto &header = snapshot.map_header;
            if (header.grid.width > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
                header.grid.height > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
                throw std::runtime_error("Saved FOW grid exceeds native world dimensions");
            fow_grid_mode = terrain::game_world_grid_mode(header.wrap_x);
            const auto world = terrain::build_world_layout(
                static_cast<std::int32_t>(header.grid.width), static_cast<std::int32_t>(header.grid.height),
                fow_grid_mode, {}, [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); });
            materials::FogOfWarState state(world.source_grid.width, world.source_grid.height, {},
                                           world.bounds[0], world.bounds[3]);
            fow_counts = scene::apply_saved_map_fog(header.grid, *plot_team_art, state);
            state.tick(0.f);
            fow_visibility_cells = state.visibility().size();
            fow_culling_kept = std::count(state.culling().begin(), state.culling().end(), 1u);
            fow_dirty_after_tick = state.dirty();
        }
        std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"session_snapshot\",\"can_continue\":false,\"container\":"
                  << json_string(saves::save_container_kind_name(loaded.container))
                  << ",\"turn\":" << loaded.header.turn
                  << ",\"payload_bytes\":" << loaded.bytes.size()
                  << ",\"game_state_end\":" << snapshot.game_state.end_offset
                  << ",\"map\":{\"width\":" << snapshot.map_header.grid.width
                  << ",\"height\":" << snapshot.map_header.grid.height
                  << ",\"plots\":"
                  << (snapshot.map_plots ? snapshot.map_plots->plots.size() : 0)
                  << ",\"terrain_cells\":"
                  << (snapshot.map_terrain ? snapshot.map_terrain->cells.size() : 0)
                  << ",\"known_terrain_masks\":"
                  << (snapshot.map_terrain ? snapshot.map_terrain->known_terrain_masks : 0)
                  << ",\"native_markup_complete\":"
                  << (snapshot.map_terrain && snapshot.map_terrain->native_markup_complete ? "true" : "false") << "}"
                  << ",\"teams\":" << (snapshot.teams ? snapshot.teams->teams.size() : 0)
                  << ",\"players\":"
                  << (snapshot.all_players ? snapshot.all_players->players.size() : 0)
                  << ",\"supporting_classes\":" << (snapshot.supporting_classes ? "true" : "false")
                  << ",\"payload_end_offset\":" << snapshot.payload_end_offset;
        if (random_preview) {
            const auto streams = runtime::restore_session_random(snapshot.progression);
            std::cout << ",\"random_preview\":{\"saved_gamecore_verified\":false,\"map\":";
            random_preview_json(streams.map);
            std::cout << ",\"gameplay\":";
            random_preview_json(streams.gameplay);
            std::cout << '}';
        }
        if (terrain_random_seed_summary) {
            const auto &integers = snapshot.pregame->archive.integers;
            std::cout << ",\"pregame_random_seeds\":{\"map_random_seed\":"
                      << integers.at("map_random_seed") << ",\"sync_random_seed\":"
                      << integers.at("sync_random_seed") << '}';
        }
        if (plot_art_summary) {
            std::cout << ",\"plot_team_art\":";
            plot_art_summary_json(*plot_team_art);
        }
        if (fow_state_summary) {
            std::cout << ",\"fow_state\":{\"visible\":" << fow_counts->visible
                      << ",\"explored\":" << fow_counts->explored
                      << ",\"unexplored\":" << fow_counts->unexplored
                      << ",\"visible_unrevealed\":" << fow_counts->visible_unrevealed
                      << ",\"visibility_cells\":" << fow_visibility_cells
                      << ",\"culling_kept\":" << fow_culling_kept
                      << ",\"grid_mode\":" << fow_grid_mode
                      << ",\"dirty_after_tick\":" << (fow_dirty_after_tick ? "true" : "false") << '}';
        }
        if (top_panel_saved_summary) {
            const auto projection = ui::project_top_panel_saved_player(
                snapshot, static_cast<std::int64_t>(plot_team_art->active_player));
            std::cout << ",\"top_panel_saved_summary\":";
            top_panel_saved_summary_json(projection, plot_team_art->active_player);
        }
        if (map_after_non_sqlite_db) {
            const auto &db = snapshot.saved_database;
            std::cout << ",\"saved_database_diagnostic\":{\"declared_image_bytes\":"
                      << db.declared_image_bytes << ",\"read_image_bytes\":" << db.image.size()
                      << ",\"quick_check_passed\":" << (db.quick_check_passed ? "true" : "false")
                      << ",\"non_sqlite_image_skipped\":" << (db.non_sqlite_image_skipped ? "true" : "false")
                      << '}';
        }
        std::cout << "}\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Civis session snapshot failed: " << error.what() << '\n';
        return 2;
    }
}
