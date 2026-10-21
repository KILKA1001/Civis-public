#include "civ5/assets/asset_source.hpp"
#include "civ5/content/mod_activation.hpp"
#include "civ5/platform/steam_installation.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/saves/all_players.hpp"
#include "civ5/saves/army_operations.hpp"
#include "civ5/saves/chunked_stream.hpp"
#include "civ5/saves/cities.hpp"
#include "civ5/saves/city_core.hpp"
#include "civ5/saves/city_management.hpp"
#include "civ5/saves/city_production.hpp"
#include "civ5/saves/city_tail.hpp"
#include "civ5/saves/expansion2_prefix.hpp"
#include "civ5/saves/game_history.hpp"
#include "civ5/saves/game_leagues.hpp"
#include "civ5/saves/game_progression.hpp"
#include "civ5/saves/vp_game_progression_profile.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include "civ5/saves/game_religion_culture.hpp"
#include "civ5/saves/game_state.hpp"
#include "civ5/saves/game_tail.hpp"
#include "civ5/saves/game_trade.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include "civ5/saves/inflate_stream.hpp"
#include "civ5/saves/map_regions.hpp"
#include "civ5/saves/map_state.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/player_administration.hpp"
#include "civ5/saves/player_core.hpp"
#include "civ5/saves/player_diplomacy.hpp"
#include "civ5/saves/player_espionage.hpp"
#include "civ5/saves/player_knowledge.hpp"
#include "civ5/saves/player_planning.hpp"
#include "civ5/saves/player_strategy.hpp"
#include "civ5/saves/player_tail.hpp"
#include "civ5/saves/player_trade_culture.hpp"
#include "civ5/saves/player_traits.hpp"
#include "civ5/saves/supporting_classes.hpp"
#include "civ5/saves/team_state.hpp"
#include "civ5/saves/vp_team_profile.hpp"
#include "civ5/saves/unit_core.hpp"
#include "civ5/saves/units.hpp"
#include "civ5/saves/vp_prefix.hpp"
#include "civ5/saves/vp_all_players_profile.hpp"
#include "civ5/saves/vp_cities_profile.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include "civ5/saves/vp_city_management_profile.hpp"
#include "civ5/saves/vp_city_tail_profile.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace {
std::string quote(const std::string &value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (const unsigned char c : value) {
        if (c == '\\' || c == '"') {
            result += '\\';
            result += static_cast<char>(c);
        } else if (c < 0x20) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 15];
        } else
            result += static_cast<char>(c);
    }
    return result + '"';
}
std::string path_json(const fs::path &value) { return quote(civ5::assets::path_utf8(value)); }
void strings(const std::vector<std::string> &values) {
    std::cout << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(values[i]);
    }
    std::cout << ']';
}
std::uint64_t dimension(const wchar_t *text) {
    std::uint64_t result = 0;
    if (!*text)
        throw std::runtime_error("Empty wire dimension");
    for (; *text; ++text) {
        if (*text < L'0' || *text > L'9' ||
            result > (std::numeric_limits<std::uint64_t>::max() - (*text - L'0')) / 10)
            throw std::runtime_error("Invalid 64-bit wire dimension");
        result = result * 10 + (*text - L'0');
    }
    return result;
}
std::string bytes_hex(std::span<const std::uint8_t> bytes) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (auto byte : bytes) {
        result += hex[byte >> 4];
        result += hex[byte & 15];
    }
    return result;
}
void game_state_json(const civ5::saves::GameStateObservation &state) {
    using namespace civ5::saves;
    std::cout << "{\"layout\":"
              << quote(state.layout == GameStateLayout::vp542_corpus ? "vp542_corpus_comparison"
                                                                     : "expansion2_version1")
              << ",\"controls_end_offset\":" << state.controls_end_offset
              << ",\"end_offset\":" << state.end_offset
              << ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false,\"flags\":{";
    for (std::size_t i = 0; i < state.flags.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(state.flags[i].name) << ':' << (state.flags[i].value ? "true" : "false");
    }
    std::cout << "},\"controls\":{";
    for (std::size_t i = 0; i < state.controls.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(state.controls[i].name) << ':' << state.controls[i].value;
    }
    std::cout << "},\"advisor_messages_hex\":[";
    for (std::size_t i = 0; i < state.advisor_messages.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(bytes_hex(state.advisor_messages[i]));
    }
    std::cout << "],\"advisor_hashes\":[";
    for (std::size_t i = 0; i < state.advisor_hashes.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << state.advisor_hashes[i];
    }
    std::cout << "],\"next_field\":"
              << quote(state.roster                                    ? "unit_created_count"
                       : state.layout == GameStateLayout::vp542_corpus ? "resource_monopoly_players"
                                                                       : "script_data")
              << ",\"roster\":";
    if (!state.roster) {
        std::cout << "null}";
        return;
    }
    const auto &roster = *state.roster;
    std::cout << "{\"dimensions_origin\":"
              << quote(state.layout == GameStateLayout::vp542_corpus ? "caller_supplied_unverified"
                                                                     : "retail_403694_expansion2_sdk")
              << ",\"player_slots\":" << roster.dimensions.player_slots
              << ",\"team_slots\":" << roster.dimensions.team_slots << ",\"vp_resource_slots\":";
    if (const auto *vp_dimensions = civ5::saves::vp_roster_dimensions(roster.dimensions))
        std::cout << vp_dimensions->resource_slots;
    else
        std::cout << "null";
    std::cout << ",\"rankings_offset\":" << roster.rankings_offset
              << ",\"script_data_hex\":" << quote(bytes_hex(roster.script_data))
              << ",\"resource_monopoly_players\":[";
    for (std::size_t i = 0; i < roster.resource_monopoly_players.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << roster.resource_monopoly_players[i];
    }
    std::cout << ']';
    for (std::size_t i = 0; i < roster.arrays.size(); ++i) {
        std::cout << ',' << quote(RosterObservation::array_names[i]) << ":[";
        for (std::size_t j = 0; j < roster.arrays[i].size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << roster.arrays[i][j];
        }
        std::cout << ']';
    }
    std::cout << "}}";
}
#include "all_players_json.hpp"
#include "supporting_classes_json.hpp"
#include "army_operations_json.hpp"
#include "cities_json.hpp"
#include "city_core_json.hpp"
#include "city_management_json.hpp"
#include "city_production_json.hpp"
#include "city_tail_json.hpp"
#include "game_history_json.hpp"
#include "game_leagues_json.hpp"
#include "game_progression_json.hpp"
#include "game_religion_culture_json.hpp"
#include "game_tail_json.hpp"
#include "game_trade_json.hpp"
#include "map_regions_json.hpp"
#include "map_state_json.hpp"
#include "player_administration_json.hpp"
#include "player_core_json.hpp"
#include "player_diplomacy_json.hpp"
#include "player_espionage_json.hpp"
#include "player_knowledge_json.hpp"
#include "player_planning_json.hpp"
#include "player_strategy_json.hpp"
#include "player_tail_json.hpp"
#include "player_trade_culture_json.hpp"
#include "player_traits_json.hpp"
#include "team_state_json.hpp"
#include "unit_core_json.hpp"
#include "units_json.hpp"
const char *status(civ5::runtime::RequirementStatus value) {
    using enum civ5::runtime::RequirementStatus;
    switch (value) {
    case located:
        return "located";
    case missing:
        return "missing";
    case version_mismatch:
        return "version_mismatch";
    case ambiguous:
        return "ambiguous";
    }
    throw std::logic_error("Unknown requirement status");
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save_path, game_root, steam_root, mod_cache;
        std::vector<fs::path> mod_roots;
        std::optional<saves::RosterWireDimensions> vp_dimensions;
        std::optional<saves::ProgressionWireDimensions> vp_progression;
        std::optional<saves::TradeWireDimensions> vp_trade;
        bool include_map_plots = false;
        bool include_map_regions = false;
        bool include_teams = false;
        bool include_player_core = false;
        bool include_player_strategy = false;
        bool include_player_diplomacy = false;
        bool include_player_knowledge = false;
        bool include_player_planning = false;
        bool include_player_traits = false;
        bool include_player_espionage = false;
        bool include_player_trade_culture = false;
        bool include_player_administration = false;
        bool include_city_core = false;
        bool include_city_production = false;
        bool include_city_management = false;
        std::optional<std::uint64_t> vp_city_strategies;
        bool include_city_tail = false;
        bool include_cities = false;
        bool include_unit_core = false;
        bool include_units = false;
        bool include_army_operations = false;
        bool include_player_tail = false;
        bool include_all_players = false;
        bool include_supporting_classes = false;
        std::optional<std::uint64_t> vp_city_emphases;
        std::optional<saves::PlayerDiplomacyWireDimensions> vp_diplomacy_dimensions;
        std::optional<saves::VpPlayerStrategyWireDimensions> vp_strategy_rows;
        std::optional<std::uint64_t> bnw_grand_strategies;
        std::optional<saves::VpTeamContentDimensions> vp_team_rows;
        std::optional<std::uint64_t> capture_budget;
        for (int i = 1; i < argc; ++i) {
            const std::wstring key = argv[i];
            if (key == L"--help") {
                std::cout
                    << "Civis 64-bit save/content inspector (read-only)\n"
                       "--save FILE_OR_DIRECTORY [--game-root DIRECTORY | --steam-root DIRECTORY] "
                       "[--mods DIRECTORY]... [--mod-cache CACHE]\n"
                       "[--vp-roster-dimensions RESOURCE_SLOTS PLAYER_SLOTS TEAM_SLOTS]\n"
                       "[--vp-progression-dimensions UNITS UNITCLASSES BUILDINGCLASSES PROJECTS "
                       "SPECIALUNITS VICTORIES CONTRACTS VICTORY_AWARDS]\n"
                       "[--vp-trade-dimensions MAJOR_SLOTS YIELD_SLOTS]\n"
                       "[--vp-team-dimensions VOTES SMALL_AWARDS TECHS ROUTES BUILDS BUILDINGS "
                       "TERRAINS FEATURES IMPROVEMENTS]\n"
                       "[--vp-strategy-dimensions POLICIES BRANCHES FLAVORS ECONOMIC MILITARY GRAND]\n"
                       "[--bnw-grand-strategies COUNT] [--player-strategy]\n"
                       "[--vp-diplomacy-dimensions MINORS CIV_PLAYERS PLAYERS APPROACHES STATEMENTS] "
                       "[--player-diplomacy]\n"
                       "[--player-knowledge] [--player-planning] [--player-traits] [--player-espionage] "
                       "[--player-trade-culture] [--player-administration] [--city-core] "
                       "[--city-production] [--city-management]\n"
                       "[--vp-city-strategies COUNT] [--city-tail] [--vp-city-emphases COUNT] [--cities] "
                       "[--unit-core] [--units] [--army-operations] [--player-tail] [--all-players] "
                       "[--supporting-classes]\n"
                       "[--map-plots] [--map-regions] [--teams] [--player-core] [--capture-bytes BYTES]\n"
                       "VP dimensions are explicit diagnostic context, not a verified rules profile.\n"
                       "Chooses maximum saved mod count, then latest turn. Outputs JSON.\n"
                       "Exit 0: inspection completed; 2: inspection/content problem. Neither means "
                       "game continuation.\n";
                return 0;
            }
            if (key == L"--map-plots") {
                if (include_map_plots)
                    throw std::runtime_error("Supply --map-plots only once");
                include_map_plots = true;
                continue;
            }
            if (key == L"--map-regions") {
                if (include_map_regions)
                    throw std::runtime_error("Supply --map-regions only once");
                include_map_regions = true;
                continue;
            }
            if (key == L"--player-core") {
                if (include_player_core)
                    throw std::runtime_error("Supply --player-core only once");
                include_player_core = true;
                continue;
            }
            if (key == L"--player-knowledge") {
                if (include_player_knowledge)
                    throw std::runtime_error("Supply --player-knowledge only once");
                include_player_knowledge = true;
                continue;
            }
            if (key == L"--player-planning") {
                if (include_player_planning)
                    throw std::runtime_error("Supply --player-planning only once");
                include_player_planning = true;
                continue;
            }
            if (key == L"--player-traits") {
                if (include_player_traits)
                    throw std::runtime_error("Supply --player-traits only once");
                include_player_traits = true;
                continue;
            }
            if (key == L"--player-espionage") {
                if (include_player_espionage)
                    throw std::runtime_error("Supply --player-espionage only once");
                include_player_espionage = true;
                continue;
            }
            if (key == L"--city-management") {
                if (include_city_management)
                    throw std::runtime_error("Supply --city-management only once");
                include_city_management = true;
                continue;
            }
            if (key == L"--city-tail") {
                if (include_city_tail)
                    throw std::runtime_error("Supply --city-tail only once");
                include_city_tail = true;
                continue;
            }
            if (key == L"--cities") {
                if (include_cities)
                    throw std::runtime_error("Supply --cities only once");
                include_cities = true;
                continue;
            }
            if (key == L"--units") {
                if (include_units)
                    throw std::runtime_error("Supply --units only once");
                include_units = true;
                continue;
            }
            if (key == L"--army-operations") {
                if (include_army_operations)
                    throw std::runtime_error("Supply --army-operations only once");
                include_army_operations = true;
                continue;
            }
            if (key == L"--player-tail") {
                if (include_player_tail)
                    throw std::runtime_error("Supply --player-tail only once");
                include_player_tail = true;
                continue;
            }
            if (key == L"--all-players") {
                if (include_all_players)
                    throw std::runtime_error("Supply --all-players only once");
                include_all_players = true;
                continue;
            }
            if (key == L"--supporting-classes") {
                if (include_supporting_classes)
                    throw std::runtime_error("Supply --supporting-classes only once");
                include_supporting_classes = true;
                continue;
            }
            if (key == L"--unit-core") {
                if (include_unit_core)
                    throw std::runtime_error("Supply --unit-core only once");
                include_unit_core = true;
                continue;
            }
            if (key == L"--vp-city-emphases") {
                if (vp_city_emphases || i + 1 >= argc)
                    throw std::runtime_error("Supply VP city emphasis count only once");
                vp_city_emphases = dimension(argv[++i]);
                continue;
            }
            if (key == L"--vp-city-strategies") {
                if (vp_city_strategies || i + 1 >= argc)
                    throw std::runtime_error("Supply VP city strategy count only once");
                vp_city_strategies = dimension(argv[++i]);
                continue;
            }
            if (key == L"--city-production") {
                if (include_city_production)
                    throw std::runtime_error("Supply --city-production only once");
                include_city_production = true;
                continue;
            }
            if (key == L"--city-core") {
                if (include_city_core)
                    throw std::runtime_error("Supply --city-core only once");
                include_city_core = true;
                continue;
            }
            if (key == L"--player-administration") {
                if (include_player_administration)
                    throw std::runtime_error("Supply --player-administration only once");
                include_player_administration = true;
                continue;
            }
            if (key == L"--player-trade-culture") {
                if (include_player_trade_culture)
                    throw std::runtime_error("Supply --player-trade-culture only once");
                include_player_trade_culture = true;
                continue;
            }
            if (key == L"--player-diplomacy") {
                if (include_player_diplomacy)
                    throw std::runtime_error("Supply --player-diplomacy only once");
                include_player_diplomacy = true;
                continue;
            }
            if (key == L"--vp-diplomacy-dimensions") {
                if (vp_diplomacy_dimensions || i + 5 >= argc)
                    throw std::runtime_error("Supply five VP diplomacy dimensions only once");
                auto &d = vp_diplomacy_dimensions.emplace();
                for (auto target : {&d.minor, &d.civ_players, &d.players, &d.approaches, &d.statements})
                    *target = dimension(argv[++i]);
                continue;
            }
            if (key == L"--player-strategy") {
                if (include_player_strategy)
                    throw std::runtime_error("Supply --player-strategy only once");
                include_player_strategy = true;
                continue;
            }
            if (key == L"--vp-strategy-dimensions") {
                if (vp_strategy_rows || i + 6 >= argc)
                    throw std::runtime_error("Supply VP strategy dimensions once, with six values");
                vp_strategy_rows.emplace();
                auto &c = *vp_strategy_rows;
                for (auto field : {&c.policies, &c.branches, &c.flavors, &c.economic, &c.military, &c.grand})
                    *field = dimension(argv[++i]);
                continue;
            }
            if (key == L"--bnw-grand-strategies") {
                if (bnw_grand_strategies || i + 1 >= argc)
                    throw std::runtime_error("Supply BNW grand strategy count once");
                bnw_grand_strategies = dimension(argv[++i]);
                continue;
            }
            if (key == L"--teams") {
                if (include_teams)
                    throw std::runtime_error("Supply --teams only once");
                include_teams = true;
                continue;
            }
            if (key == L"--vp-team-dimensions") {
                if (vp_team_rows || i + 9 >= argc)
                    throw std::runtime_error("Supply VP team dimensions once, with nine values");
                vp_team_rows.emplace();
                auto &c = *vp_team_rows;
                for (auto field : {&c.vote_sources, &c.small_awards, &c.techs, &c.routes, &c.builds,
                                   &c.buildings, &c.terrains, &c.features, &c.improvements})
                    *field = dimension(argv[++i]);
                continue;
            }
            if (key == L"--capture-bytes") {
                if (capture_budget || i + 1 >= argc)
                    throw std::runtime_error("Supply capture budget once with a value");
                capture_budget = dimension(argv[++i]);
                if (!*capture_budget)
                    throw std::runtime_error("Capture budget must be positive");
                continue;
            }
            if (key == L"--vp-roster-dimensions") {
                if (vp_dimensions || i + 3 >= argc)
                    throw std::runtime_error("Supply VP wire dimensions exactly once, with three values");
                const auto resources = dimension(argv[++i]);
                const auto players = dimension(argv[++i]);
                const auto teams = dimension(argv[++i]);
                vp_dimensions = saves::RosterWireDimensions{players, teams};
                vp_dimensions->profile_extensions.emplace<saves::VpRosterWireDimensions>(
                    saves::VpRosterWireDimensions{resources});
                continue;
            }
            if (key == L"--vp-progression-dimensions") {
                if (vp_progression || i + 8 >= argc)
                    throw std::runtime_error("Supply VP progression dimensions once, with eight values");
                vp_progression.emplace();
                auto &vp_rows = vp_progression->profile_extensions.emplace<saves::VpGameProgressionWireDimensions>();
                for (auto &rows : vp_rows.rows)
                    rows = dimension(argv[++i]);
                vp_progression->victory_award_slots = dimension(argv[++i]);
                continue;
            }
            if (key == L"--vp-trade-dimensions") {
                if (vp_trade || i + 2 >= argc)
                    throw std::runtime_error("Supply VP trade dimensions once, with two values");
                const auto major = dimension(argv[++i]);
                const auto yields = dimension(argv[++i]);
                vp_trade = saves::TradeWireDimensions{major, yields};
                continue;
            }
            if (i + 1 == argc)
                throw std::runtime_error("Missing option value");
            const fs::path value = argv[++i];
            if (key == L"--save")
                save_path = value;
            else if (key == L"--game-root")
                game_root = value;
            else if (key == L"--steam-root")
                steam_root = value;
            else if (key == L"--mods")
                mod_roots.push_back(value);
            else if (key == L"--mod-cache")
                mod_cache = value;
            else
                throw std::runtime_error("Unknown option");
        }
        if (save_path.empty())
            throw std::runtime_error("Supply --save FILE_OR_DIRECTORY");
        if (include_supporting_classes)
            include_all_players = true;
        if (include_all_players)
            include_player_tail = true;
        if (include_all_players)
            include_supporting_classes = true;
        if (include_player_tail)
            include_army_operations = true;
        if (include_army_operations)
            include_units = true;
        if (include_units)
            include_unit_core = true;
        if (include_unit_core)
            include_cities = true;
        if (include_cities)
            include_city_tail = true;
        if (include_city_tail)
            include_city_management = true;
        if (vp_city_emphases && !vp_city_strategies)
            throw std::runtime_error("VP city emphases require city strategy and prior context");
        if (include_city_management)
            include_city_production = true;
        if (vp_city_strategies && !vp_strategy_rows)
            throw std::runtime_error("VP city dimensions require strategy and prior context");
        if (include_city_production)
            include_city_core = true;
        if (include_city_core)
            include_player_administration = true;
        if (include_player_administration)
            include_player_trade_culture = true;
        if (include_player_trade_culture)
            include_player_espionage = true;
        if (include_player_espionage)
            include_player_traits = true;
        if (include_player_traits)
            include_player_planning = true;
        if (include_player_planning)
            include_player_knowledge = true;
        if (include_player_knowledge)
            include_player_diplomacy = true;
        if (include_player_diplomacy)
            include_player_strategy = true;
        if (vp_diplomacy_dimensions && !vp_strategy_rows)
            throw std::runtime_error("VP diplomacy dimensions require strategy and prior context");
        if (include_player_strategy)
            include_player_core = true;
        if (vp_strategy_rows && !vp_team_rows)
            throw std::runtime_error("VP strategy dimensions require team and prior context");
        if (bnw_grand_strategies && vp_dimensions)
            throw std::runtime_error("BNW grand strategy count supplied for VP context");
        if (vp_progression) {
            if (!vp_dimensions)
                throw std::runtime_error("VP progression requires explicit roster dimensions");
            vp_progression->vote_team_slots = vp_dimensions->team_slots;
        }
        if (vp_trade && (!vp_dimensions || !vp_progression))
            throw std::runtime_error("VP trade requires explicit roster and progression dimensions");
        if (vp_team_rows) {
            if (!vp_trade)
                throw std::runtime_error("VP team dimensions require roster, progression and trade context");
            const auto &rows = saves::vp_game_progression_dimensions(*vp_progression)->rows;
            vp_team_rows->projects = rows[3];
            vp_team_rows->unit_classes = rows[1];
            vp_team_rows->building_classes = rows[2];
            vp_team_rows->victories = rows[5];
        }
        std::vector<std::string> discovery_diagnostics;
        if (game_root.empty()) {
            const auto steam =
                platform::discover_civ5(steam_root.empty() ? platform::default_steam_root() : steam_root);
            discovery_diagnostics = steam.diagnostics;
            if (steam.installations.size() != 1)
                throw std::runtime_error("Expected one installed Civilization V; supply --game-root");
            game_root = steam.installations.front().game_root;
            if (fs::is_directory(steam.installations.front().workshop_root))
                mod_roots.push_back(steam.installations.front().workshop_root);
        }
        platform::validate_game_root(game_root);
        game_root = fs::canonical(game_root);
        runtime::SaveSelection selection;
        saves::LoadedSave loaded;
        const bool civis_save = fs::is_regular_file(save_path) &&
                                runtime::ascii_lower(assets::path_utf8(save_path.extension())) ==
                                    ".civissave";
        if (civis_save) {
            loaded = saves::read_loaded_save(save_path);
            selection.candidates.push_back({fs::canonical(save_path), loaded.header});
            selection.selected = 0;
        } else {
            selection = runtime::select_save(save_path);
        }
        if (!selection.selected)
            throw std::runtime_error("No supported save header in supplied path");
        const auto &selected = selection.candidates[*selection.selected];
        if (!civis_save)
            loaded = saves::read_loaded_save(selected.path);
        const auto identity = runtime::identify_file(selected.path);
        const auto &frame = loaded.frame;
        const auto &inflated = loaded.inflate;
        std::vector<std::uint8_t> payload_prefix;
        const auto capture_bytes = capture_budget.value_or(
            (include_map_plots || include_map_regions || include_teams || include_player_core)
                ? 256ull * 1024 * 1024
                : 8ull * 1024 * 1024);
        const auto count = std::min<std::uint64_t>(capture_bytes, loaded.bytes.size());
        payload_prefix.insert(payload_prefix.end(), loaded.bytes.begin(),
                              loaded.bytes.begin() + static_cast<std::size_t>(count));
        const auto prefix_hex =
            bytes_hex(std::span(payload_prefix).first(std::min<std::size_t>(256, payload_prefix.size())));
        std::optional<saves::VpPrefixObservation> vp_prefix;
        std::optional<saves::Expansion2PrefixObservation> expansion2_prefix;
        std::string prefix_diagnostic;
        std::optional<saves::GameStateObservation> game_state;
        std::string game_state_diagnostic;
        std::optional<saves::GameProgressionObservation> game_progression;
        std::string game_progression_diagnostic;
        std::optional<saves::GameHistoryObservation> game_history;
        std::string game_history_diagnostic;
        std::optional<saves::GameReligionCultureObservation> game_religion_culture;
        std::string game_religion_culture_diagnostic;
        std::optional<saves::GameLeaguesObservation> game_leagues;
        std::string game_leagues_diagnostic;
        std::optional<saves::GameTradeObservation> game_trade;
        std::string game_trade_diagnostic;
        std::optional<saves::GameFinalSystemsObservation> game_final_systems;
        std::string game_final_systems_diagnostic;
        std::optional<saves::SavedDatabaseObservation> saved_database;
        std::string saved_database_diagnostic;
        std::optional<saves::MapHeaderObservation> map_header;
        std::string map_header_diagnostic;
        std::optional<saves::MapPlotsObservation> map_plots;
        std::string map_plots_diagnostic;
        std::optional<saves::SavedMapProjection> saved_map_projection;
        std::optional<saves::SavedMapTerrainSource> saved_map_terrain;
        std::string saved_map_terrain_diagnostic;
        std::optional<saves::MapRegionsObservation> map_regions;
        std::string map_regions_diagnostic;
        std::optional<saves::TeamsObservation> teams;
        std::string teams_diagnostic;
        std::optional<saves::PlayerCoreObservation> player_core;
        std::string player_core_diagnostic;
        std::optional<saves::PlayerStrategyObservation> player_strategy;
        std::string player_strategy_diagnostic;
        std::optional<saves::PlayerDiplomacyObservation> player_diplomacy;
        std::string player_diplomacy_diagnostic;
        std::optional<saves::PlayerKnowledgeObservation> player_knowledge;
        std::string player_knowledge_diagnostic;
        std::optional<saves::PlayerPlanningObservation> player_planning;
        std::string player_planning_diagnostic;
        std::optional<saves::PlayerTraitsObservation> player_traits;
        std::string player_traits_diagnostic;
        std::optional<saves::PlayerEspionageObservation> player_espionage;
        std::string player_espionage_diagnostic;
        std::optional<saves::PlayerTradeCultureObservation> player_trade_culture;
        std::string player_trade_culture_diagnostic;
        std::optional<saves::PlayerAdministrationObservation> player_administration;
        std::string player_administration_diagnostic;
        std::optional<saves::CityCollectionHeader> city_collection;
        std::optional<saves::CityCoreObservation> city_core;
        std::string city_core_diagnostic;
        std::optional<saves::CityProductionObservation> city_production;
        std::string city_production_diagnostic;
        std::optional<saves::CityManagementObservation> city_management;
        std::string city_management_diagnostic;
        std::optional<saves::CityTailObservation> city_tail;
        std::string city_tail_diagnostic;
        std::optional<saves::CitiesObservation> cities;
        std::string cities_diagnostic;
        std::optional<saves::UnitCollectionHeader> unit_collection;
        std::optional<saves::UnitCoreObservation> unit_core;
        std::string unit_core_diagnostic;
        std::optional<saves::UnitsObservation> units;
        std::string units_diagnostic;
        std::optional<saves::ArmyOperationsObservation> army_operations;
        std::string army_operations_diagnostic;
        std::optional<saves::PlayerTailObservation> player_tail;
        std::string player_tail_diagnostic;
        std::optional<saves::AllPlayersObservation> all_players;
        std::string all_players_diagnostic;
        std::optional<saves::SupportingClassesObservation> supporting_classes;
        std::string supporting_classes_diagnostic;
        std::optional<std::uint64_t> players_start_offset;
        try {
            std::istringstream prefix_stream(std::string(payload_prefix.begin(), payload_prefix.end()));
            runtime::BinaryReader prefix_reader(prefix_stream, payload_prefix.size());
            const bool expansion2 =
                std::any_of(selected.header.dlcs.begin(), selected.header.dlcs.end(),
                            [](const auto &dlc) { return dlc.id == "6da07636-4123-4018-b643-6575b4ec336b"; });
            if (selected.header.mods.empty() && expansion2)
                expansion2_prefix = saves::observe_expansion2_prefix(prefix_reader);
            else
                vp_prefix = saves::observe_vp_prefix(prefix_reader);
            try {
                prefix_reader.seek(0);
                if (expansion2_prefix) {
                    if (vp_dimensions)
                        throw std::runtime_error("VP wire dimensions cannot be applied to Expansion2");
                    game_state =
                        saves::observe_game_state(prefix_reader, saves::GameStateLayout::expansion2_version1,
                                                  saves::RosterWireDimensions{64, 64});
                } else
                    game_state = saves::observe_game_state(
                        prefix_reader, saves::GameStateLayout::vp542_corpus, vp_dimensions);
            } catch (const std::exception &error) {
                game_state_diagnostic = error.what();
            }
            if (game_state && game_state->roster && (expansion2_prefix || vp_progression)) {
                try {
                    game_progression = saves::observe_game_progression(
                        prefix_reader, game_state->layout,
                        expansion2_prefix ? saves::ProgressionWireDimensions{63, 5} : *vp_progression);
                } catch (const std::exception &error) {
                    game_progression_diagnostic = error.what();
                }
            }
            if (game_progression) {
                try {
                    game_history = saves::observe_game_history(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    game_history_diagnostic = error.what();
                }
            }
            if (game_history) {
                try {
                    game_religion_culture =
                        saves::observe_game_religion_culture(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    game_religion_culture_diagnostic = error.what();
                }
            }
            if (game_religion_culture) {
                try {
                    game_leagues = saves::observe_game_leagues(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    game_leagues_diagnostic = error.what();
                }
            }
            if (game_leagues && (expansion2_prefix || vp_trade)) {
                try {
                    game_trade = saves::observe_game_trade(
                        prefix_reader, game_state->layout,
                        expansion2_prefix ? saves::TradeWireDimensions{22, 6} : *vp_trade);
                } catch (const std::exception &error) {
                    game_trade_diagnostic = error.what();
                }
            }
            if (game_trade) {
                try {
                    game_final_systems = saves::observe_game_final_systems(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    game_final_systems_diagnostic = error.what();
                }
            }
            if (game_final_systems) {
                try {
                    saved_database = saves::observe_saved_database(prefix_reader);
                } catch (const std::exception &error) {
                    saved_database_diagnostic = error.what();
                }
            }
            if (saved_database) {
                try {
                    const auto *vp_roster = vp_dimensions ? saves::vp_roster_dimensions(*vp_dimensions) : nullptr;
                    map_header = saves::observe_map_header(
                        prefix_reader, game_state->layout,
                        vp_roster ? std::optional<std::uint64_t>(vp_roster->resource_slots) : std::nullopt);
                } catch (const std::exception &error) {
                    map_header_diagnostic = error.what();
                }
            }
            if (map_header &&
                (include_map_plots || include_map_regions || include_teams || include_player_core)) {
                try {
                    // These are the source arrays' widths, not host capacities.
                    const auto dimensions =
                        vp_dimensions
                            ? saves::PlotWireDimensions{vp_dimensions->player_slots,
                                                        vp_dimensions->team_slots, vp_trade->major_slots,
                                                        vp_trade->yield_slots, 0}
                            : saves::PlotWireDimensions{80, 80, 22, 6, 1};
                    map_plots = saves::observe_map_plots(prefix_reader, *map_header, dimensions);
                } catch (const std::exception &error) {
                    map_plots_diagnostic = error.what();
                }
            }
            if (map_header && map_plots) {
                try {
                    saved_map_projection = saves::project_saved_map(*map_header, *map_plots);
                    saved_map_terrain = saves::project_saved_map_terrain(*saved_map_projection);
                } catch (const std::exception &error) {
                    saved_map_terrain_diagnostic = error.what();
                }
            }
            if (map_plots && (include_map_regions || include_teams || include_player_core)) {
                try {
                    map_regions = saves::observe_map_regions(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    map_regions_diagnostic = error.what();
                }
            }
            if (map_regions && (include_teams || include_player_core)) {
                try {
                    saves::TeamWireDimensions d;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (!vp_team_rows)
                            throw std::runtime_error("VP team import requires explicit content dimensions");
                        d.major = vp_trade->major_slots;
                        d.yields = vp_trade->yield_slots;
                        d.profile_extensions.emplace<saves::VpTeamContentDimensions>(*vp_team_rows);
                        // EXE iterates 64 teams. Firaxis::Array serializes all
                        // 80 storage slots, unlike the earlier roster spans.
                    } else if (vp_team_rows)
                        throw std::runtime_error("VP team dimensions supplied for BNW");
                    teams = saves::observe_teams(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    teams_diagnostic = error.what();
                }
            }
            if (teams && include_player_core) {
                try {
                    players_start_offset = prefix_reader.position();
                    player_core = saves::observe_player_core(
                        prefix_reader, game_state->layout,
                        game_state->layout == saves::GameStateLayout::vp542_corpus
                            ? std::optional<std::uint64_t>(teams->dimensions.yields)
                            : std::nullopt);
                } catch (const std::exception &error) {
                    player_core_diagnostic = error.what();
                }
            }
            if (player_core && include_player_strategy) {
                try {
                    saves::PlayerStrategyWireDimensions d;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (!vp_strategy_rows)
                            throw std::runtime_error("Missing VP strategy rules dimensions");
                        d.major = teams->dimensions.major;
                        d.yields = teams->dimensions.yields;
                        d.profile_extensions.emplace<saves::VpPlayerStrategyWireDimensions>(*vp_strategy_rows);
                    } else if (bnw_grand_strategies)
                        d.grand = *bnw_grand_strategies;
                    player_strategy = saves::observe_player_strategy(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    player_strategy_diagnostic = error.what();
                }
            }
            if (player_strategy && include_player_diplomacy) {
                try {
                    saves::PlayerDiplomacyWireDimensions d;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (!vp_diplomacy_dimensions)
                            throw std::runtime_error("Missing VP diplomacy wire dimensions");
                        d = *vp_diplomacy_dimensions;
                        d.major = teams->dimensions.major;
                    }
                    player_diplomacy = saves::observe_player_diplomacy(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    player_diplomacy_diagnostic = error.what();
                }
            }
            if (player_diplomacy && include_player_knowledge) {
                try {
                    saves::PlayerKnowledgeDecodeContext d;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus)
                        d.profile_extensions.emplace<saves::VpPlayerKnowledgeWireDimensions>(
                            saves::VpPlayerKnowledgeWireDimensions{
                            saves::vp_team_dimensions(teams->dimensions)->techs,
                            saves::vp_player_strategy_dimensions(player_strategy->dimensions)->flavors});
                    player_knowledge = saves::observe_player_knowledge(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    player_knowledge_diagnostic = error.what();
                }
            }
            if (player_knowledge && include_player_planning) {
                try {
                    const saves::PlayerPlanningWireDimensions d{
                        teams->dimensions.major, teams->dimensions.civ_teams, teams->dimensions.fixed_teams};
                    player_planning = saves::observe_player_planning(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    player_planning_diagnostic = error.what();
                }
            }
            if (player_planning && include_player_traits) {
                try {
                    std::optional<saves::PlayerTraitsWireDimensions> d;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus)
                        // NUM_TERRAIN_TYPES is a fixed enum span, not the size
                        // of the current Terrains SQL table.
                        d = saves::PlayerTraitsWireDimensions{player_core->yield_slots, 9};
                    player_traits = saves::observe_player_traits(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    player_traits_diagnostic = error.what();
                }
            }
            if (player_traits && include_player_espionage) {
                try {
                    player_espionage = saves::observe_player_espionage(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    player_espionage_diagnostic = error.what();
                }
            }
            if (player_espionage && include_player_trade_culture) {
                try {
                    std::optional<saves::PlayerTradeCultureWireDimensions> d;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus)
                        d = saves::PlayerTradeCultureWireDimensions{game_trade->dimensions.major_slots,
                                                                    game_trade->dimensions.yield_slots};
                    player_trade_culture =
                        saves::observe_player_trade_culture(prefix_reader, game_state->layout, d);
                } catch (const std::exception &error) {
                    player_trade_culture_diagnostic = error.what();
                }
            }
            if (player_trade_culture && include_player_administration) {
                try {
                    player_administration =
                        saves::observe_player_administration(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    player_administration_diagnostic = error.what();
                }
            }
            if (player_administration && include_cities) {
                try {
                    saves::CitiesContext context;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (vp_city_strategies && vp_city_emphases) {
                            const auto &fields = player_core->fields;
                            const auto promotions =
                                std::find_if(fields.begin(), fields.end(), [](const auto &f) {
                                    return std::string_view(f.name) == "m_paiFreePromotionCount";
                                });
                            if (promotions == fields.end() ||
                                promotions->encoding != saves::PlayerCoreEncoding::vector_i32)
                                throw std::runtime_error("Missing saved promotion slots");
                            context.profile_extensions.emplace<saves::VpCitiesWireDimensions>(
                                saves::VpCitiesWireDimensions{saves::vp_player_strategy_dimensions(player_strategy->dimensions)->flavors,
                                                              *vp_city_strategies,
                                                              promotions->values.size(),
                                                              saves::vp_team_dimensions(teams->dimensions)->buildings,
                                                              player_core->yield_slots,
                                                              *vp_city_emphases});
                        }
                    } else {
                        const auto &arrays = game_progression->arrays;
                        const auto projects = std::find_if(arrays.begin(), arrays.end(), [](const auto &a) {
                            return std::string_view(a.name) == "project_created_count";
                        });
                        if (projects == arrays.end())
                            throw std::runtime_error("Missing saved project slots");
                        context.bnw_projects = projects->rows.size();
                    }
                    cities = saves::observe_cities(prefix_reader, game_state->layout, context);
                } catch (const std::exception &error) {
                    cities_diagnostic = error.what();
                }
            }
            if (cities && include_units) {
                try {
                    units = saves::observe_units(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    units_diagnostic = error.what();
                }
            }
            if (units && include_army_operations) {
                try {
                    army_operations =
                        saves::observe_army_operations(prefix_reader, game_state->layout,
                                                       saves::ArmyOperationsContext{player_core->version});
                } catch (const std::exception &error) {
                    army_operations_diagnostic = error.what();
                }
            }
            if (army_operations && include_player_tail) {
                try {
                    std::optional<saves::PlayerTailVpContext> context;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        const auto &fields = player_core->fields;
                        const auto id = std::find_if(fields.begin(), fields.end(), [](const auto &f) {
                            return std::string_view(f.name) == "m_eID";
                        });
                        if (id == fields.end() || id->values.size() != 1)
                            throw std::runtime_error("Missing saved player ID for tail context");
                        context = saves::PlayerTailVpContext{
                            player_core->yield_slots, player_diplomacy->dimensions.players,
                            player_diplomacy->dimensions.major, id->values[0]};
                    }
                    player_tail = saves::observe_player_tail(prefix_reader, game_state->layout, context);
                } catch (const std::exception &error) {
                    player_tail_diagnostic = error.what();
                }
            }
            if (include_all_players) {
                try {
                    if (!players_start_offset || !player_core || !player_strategy || !player_diplomacy ||
                        !player_knowledge || !player_planning || !player_traits || !player_trade_culture ||
                        !game_state)
                        throw std::runtime_error("All-player import needs a complete first-player context");
                    saves::AllPlayersContext context;
                    context.player_count = player_diplomacy->dimensions.players;
                    context.strategy = player_strategy->dimensions;
                    context.diplomacy = player_diplomacy->dimensions;
                    if (const auto *knowledge=saves::vp_player_knowledge_dimensions(*player_knowledge))
                        context.profile_extensions.emplace<saves::VpPlayerKnowledgeWireDimensions>(*knowledge);
                    context.planning = player_planning->dimensions;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (!vp_city_strategies || !vp_city_emphases || !teams ||
                            !saves::vp_team_dimensions(teams->dimensions))
                            throw std::runtime_error("Missing VP all-player city dimensions");
                        context.traits = player_traits->dimensions;
                        context.trade_culture = player_trade_culture->dimensions;
                        context.profile_extensions.emplace<saves::VpAllPlayersWireDimensions>(
                            saves::VpAllPlayersWireDimensions{*vp_city_strategies, *vp_city_emphases,
                                                               saves::vp_team_dimensions(teams->dimensions)->buildings});
                    } else {
                        const auto &arrays = game_progression->arrays;
                        const auto projects = std::find_if(arrays.begin(), arrays.end(), [](const auto &a) {
                            return std::string_view(a.name) == "project_created_count";
                        });
                        if (projects == arrays.end())
                            throw std::runtime_error("Missing BNW all-player project dimensions");
                        context.bnw_projects = projects->rows.size();
                    }
                    prefix_reader.seek(*players_start_offset);
                    all_players = saves::observe_all_players(prefix_reader, game_state->layout, context);
                } catch (const std::exception &error) {
                    all_players_diagnostic = error.what();
                }
            }
            if (include_supporting_classes) {
                try {
                    if (!all_players || !map_header || !player_diplomacy)
                        throw std::runtime_error("Supporting classes need all players, map and diplomacy context");
                    supporting_classes = saves::observe_supporting_classes(
                        prefix_reader, game_state->layout, map_header->grid.size(),
                        player_diplomacy->dimensions.major);
                } catch (const std::exception &error) {
                    supporting_classes_diagnostic = error.what();
                }
            }
            if (cities && include_unit_core && !include_units) {
                try {
                    unit_collection =
                        saves::observe_unit_collection_header(prefix_reader, game_state->layout);
                    if (unit_collection->wire_unit_count > 0)
                        unit_core = saves::observe_unit_core(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    unit_core_diagnostic = error.what();
                }
            }
            if (player_administration && include_city_core && !include_cities) {
                try {
                    city_collection =
                        saves::observe_city_collection_header(prefix_reader, game_state->layout);
                    if (city_collection->wire_city_count > 0)
                        city_core = saves::observe_city_core(prefix_reader, game_state->layout);
                } catch (const std::exception &error) {
                    city_core_diagnostic = error.what();
                }
            }
            if (city_core && include_city_production) {
                try {
                    saves::CityProductionContext context;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus)
                        context.profile_extensions.emplace<saves::VpCityProductionWireDimensions>(
                            saves::VpCityProductionWireDimensions{saves::vp_team_dimensions(teams->dimensions)->buildings});
                    else
                        context.bnw_city_version = city_core->version;
                    city_production =
                        saves::observe_city_production(prefix_reader, game_state->layout, context);
                } catch (const std::exception &error) {
                    city_production_diagnostic = error.what();
                }
            }
            if (city_production && include_city_management) {
                try {
                    saves::CityManagementContext context;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (!vp_city_strategies)
                            throw std::runtime_error("City management needs VP city strategy count");
                        const auto &fields = player_core->fields;
                        const auto promotions = std::find_if(fields.begin(), fields.end(), [](const auto &f) {
                            return std::string_view(f.name) == "m_paiFreePromotionCount";
                        });
                        if (promotions == fields.end() ||
                            promotions->encoding != saves::PlayerCoreEncoding::vector_i32)
                            throw std::runtime_error("Missing saved promotion slots");
                        // Source Traits<SpecialistTypes> incorrectly counts promotions.
                        context.profile_extensions.emplace<saves::VpCityManagementWireDimensions>(
                            saves::VpCityManagementWireDimensions{
                            saves::vp_player_strategy_dimensions(player_strategy->dimensions)->flavors, *vp_city_strategies,
                            promotions->values.size(),
                            *saves::vp_city_production_building_slots(city_production->context)});
                    } else {
                        const auto &arrays = game_progression->arrays;
                        const auto it = std::find_if(arrays.begin(), arrays.end(), [](const auto &a) {
                            return std::string_view(a.name) == "project_created_count";
                        });
                        if (it == arrays.end())
                            throw std::runtime_error("Missing saved project slots");
                        context.bnw_projects = it->rows.size(); // Includes null-hash slots.
                    }
                    city_management =
                        saves::observe_city_management(prefix_reader, game_state->layout, context);
                } catch (const std::exception &error) {
                    city_management_diagnostic = error.what();
                }
            }
            if (city_management && include_city_tail) {
                try {
                    saves::CityTailContext context;
                    if (game_state->layout == saves::GameStateLayout::vp542_corpus) {
                        if (!vp_city_emphases)
                            throw std::runtime_error("City tail needs VP emphasis count");
                        context.profile_extensions.emplace<saves::VpCityTailWireDimensions>(
                            saves::VpCityTailWireDimensions{player_core->yield_slots, *vp_city_emphases});
                    } else
                        context.bnw_city_version = city_core->version;
                    city_tail = saves::observe_city_tail(prefix_reader, game_state->layout, context);
                } catch (const std::exception &error) {
                    city_tail_diagnostic = error.what();
                }
            }
        } catch (const std::exception &error) {
            prefix_diagnostic = error.what();
        }
        std::vector<fs::path> roots{game_root / "Assets/DLC"};
        roots.insert(roots.end(), mod_roots.begin(), mod_roots.end());
        if (!mod_cache.empty())
            roots.push_back(fs::canonical(mod_cache) / "packages");
        const auto catalog = content::discover_packages(roots);
        const auto profile = runtime::resolve_saved_content_profile(selected.header, catalog);
        const auto &requirements = profile.requirements;
        const auto database_plan = content::plan_saved_database_actions(profile, catalog);
        const auto ready_database_actions = std::count_if(
            database_plan.actions.begin(), database_plan.actions.end(), [](const auto &action) {
                return action.status == content::DatabaseActivationStatus::ready;
            });
        const auto missing_database_actions = std::count_if(
            database_plan.actions.begin(), database_plan.actions.end(), [](const auto &action) {
                return action.status == content::DatabaseActivationStatus::missing_source;
            });
        const auto unsupported_database_actions = std::count_if(
            database_plan.actions.begin(), database_plan.actions.end(), [](const auto &action) {
                return action.status == content::DatabaseActivationStatus::unsupported_extension;
            });
        // This is the same read-only asset implementation consumed by graphics.
        const assets::AssetEnvironment assets(game_root);
        const auto archives = assets.discover_fpk_sources();
        const bool located = profile.all_requirements_located;
        bool files_ok = true;
        std::cout << "{\n\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"inspect\",\"can_continue\":false,\"game_root\":"
                  << path_json(game_root)
                  << ",\"shared_asset_loader\":\"civ5_assets\",\"supported_fpk_archives\":"
                  << archives.supported.size()
                  << ",\"unresolved_fpk_archives\":" << archives.unresolved.size()
                  << ",\"save\":{\"path\":" << path_json(selected.path) << ",\"bytes\":" << identity.bytes
                  << ",\"sha256\":" << quote(identity.sha256) << ",\"build\":" << quote(selected.header.build)
                  << ",\"turn\":" << selected.header.turn << ",\"header_end\":" << selected.header.end_offset
                  << ",\"mod_count\":" << selected.header.mods.size()
                  << ",\"dlc_count\":" << selected.header.dlcs.size() << ",\"payload_format\":"
                  << quote(saves::save_container_kind_name(loaded.container))
                  << ",\"frame_offset\":" << frame.offset
                  << ",\"compressed_bytes\":" << frame.compressed_bytes
                  << ",\"chunks\":" << frame.chunks.size()
                  << ",\"sync_flush_suffix\":" << (frame.sync_flush_suffix ? "true" : "false")
                  << ",\"available_inflated_bytes\":" << inflated.available_bytes
                  << ",\"zlib_stream_end\":" << (inflated.stream_end ? "true" : "false")
                  << ",\"inflated_prefix_hex\":" << quote(prefix_hex)
                  << ",\"payload_validated\":false},\n\"saved_content_profile\":{\"kind\":\""
                  << runtime::content_profile_kind_name(profile.kind)
                  << "\",\"all_requirements_located\":"
                  << (profile.all_requirements_located ? "true" : "false")
                  << ",\"files_complete\":" << (profile.files_complete ? "true" : "false")
                  << ",\"native_mods_require_port\":"
                  << (profile.native_mods_require_port ? "true" : "false")
                  << ",\"enabled_dlc_count\":" << profile.enabled_dlc.size()
                  << ",\"enabled_mod_count\":" << profile.enabled_mods.size()
                  << ",\"can_activate\":" << (profile.can_activate ? "true" : "false")
                  << "},\n\"database_activation_plan\":{\"actions\":"
                  << database_plan.actions.size() << ",\"ready\":" << ready_database_actions
                  << ",\"missing_source\":" << missing_database_actions
                  << ",\"unsupported_extension\":" << unsupported_database_actions
                  << ",\"all_sources_present\":"
                  << (database_plan.all_sources_present ? "true" : "false")
                  << "},\n\"candidates\":[";
        for (std::size_t i = 0; i < selection.candidates.size(); ++i) {
            if (i)
                std::cout << ',';
            const auto &candidate = selection.candidates[i];
            std::cout << "{\"path\":" << path_json(candidate.path) << ",\"turn\":" << candidate.header.turn
                      << ",\"mods\":" << candidate.header.mods.size() << '}';
        }
        std::cout << "],\n\"all_saved_manifests_located\":" << (located ? "true" : "false")
                  << ",\"requirements\":[";
        for (std::size_t i = 0; i < requirements.size(); ++i) {
            if (i)
                std::cout << ',';
            const auto &r = requirements[i];
            std::cout << "{\"kind\":" << quote(r.kind == content::PackageKind::mod ? "mod" : "dlc")
                      << ",\"id\":" << quote(r.id) << ",\"name\":" << quote(r.name)
                      << ",\"wire_value\":" << r.wire_value << ",\"status\":" << quote(status(r.status));
            if (r.package) {
                const auto &package = catalog.packages[*r.package];
                const auto manifest_identity = runtime::identify_file(package.path);
                std::cout << ",\"manifest\":" << path_json(package.path)
                          << ",\"manifest_sha256\":" << quote(manifest_identity.sha256)
                          << ",\"manifest_version\":" << package.version;
                if (r.kind == content::PackageKind::mod) {
                    const auto inspected = runtime::inspect_mod_files(catalog, *r.package);
                    files_ok = files_ok && inspected.diagnostics.empty();
                    std::cout << ",\"declared_files\":" << package.files.size()
                              << ",\"present_files\":" << inspected.present_files << ",\"native_files\":[";
                    for (std::size_t j = 0; j < inspected.native_files.size(); ++j) {
                        if (j)
                            std::cout << ',';
                        const auto &native = inspected.native_files[j];
                        std::cout << "{\"path\":" << quote(native.path) << ",\"machine\":" << native.machine
                                  << ",\"sha256\":" << quote(native.identity.sha256)
                                  << ",\"requires_64bit_semantic_port\":true}";
                    }
                    std::cout << "],\"file_diagnostics\":";
                    strings(inspected.diagnostics);
                }
            }
            std::cout << '}';
        }
        std::cout << "],\n\"save_diagnostics\":";
        strings(selection.diagnostics);
        std::cout << ",\"vp_prefix_observation\":";
        if (vp_prefix) {
            std::cout << "{\"gamecore_build\":" << quote(vp_prefix->gamecore_build)
                      << ",\"save_version\":" << vp_prefix->save_version << ",\"game_data_hash\":[";
            for (std::size_t i = 0; i < vp_prefix->game_data_hash.size(); ++i) {
                if (i)
                    std::cout << ',';
                std::cout << vp_prefix->game_data_hash[i];
            }
            std::cout << "],\"end_offset\":" << vp_prefix->end_offset
                      << ",\"exact_saved_build_source_verified\":false,\"scalars\":{";
            for (std::size_t i = 0; i < vp_prefix->scalars.size(); ++i) {
                if (i)
                    std::cout << ',';
                std::cout << quote(vp_prefix->scalar_names[i]) << ':' << vp_prefix->scalars[i];
            }
            std::cout << "}}";
        } else
            std::cout << "null";
        std::cout << ",\"prefix_diagnostic\":" << quote(prefix_diagnostic);
        std::cout << ",\"game_state_observation\":";
        if (game_state)
            game_state_json(*game_state);
        else
            std::cout << "null";
        std::cout << ",\"game_state_diagnostic\":" << quote(game_state_diagnostic)
                  << ",\"game_state_captured_bytes\":" << payload_prefix.size();
        std::cout << ",\"game_progression_observation\":";
        if (game_progression)
            game_progression_json(*game_progression);
        else
            std::cout << "null";
        std::cout << ",\"game_progression_diagnostic\":" << quote(game_progression_diagnostic);
        std::cout << ",\"game_history_observation\":";
        if (game_history)
            game_history_json(*game_history);
        else
            std::cout << "null";
        std::cout << ",\"game_history_diagnostic\":" << quote(game_history_diagnostic);
        std::cout << ",\"game_religion_culture_observation\":";
        if (game_religion_culture)
            game_religion_culture_json(*game_religion_culture);
        else
            std::cout << "null";
        std::cout << ",\"game_religion_culture_diagnostic\":" << quote(game_religion_culture_diagnostic);
        std::cout << ",\"game_leagues_observation\":";
        if (game_leagues)
            game_leagues_json(*game_leagues);
        else
            std::cout << "null";
        std::cout << ",\"game_leagues_diagnostic\":" << quote(game_leagues_diagnostic);
        std::cout << ",\"game_trade_observation\":";
        if (game_trade)
            game_trade_json(*game_trade);
        else
            std::cout << "null";
        std::cout << ",\"game_trade_diagnostic\":" << quote(game_trade_diagnostic);
        std::cout << ",\"game_final_systems_observation\":";
        if (game_final_systems)
            game_final_systems_json(*game_final_systems);
        else
            std::cout << "null";
        std::cout << ",\"game_final_systems_diagnostic\":" << quote(game_final_systems_diagnostic);
        std::cout << ",\"saved_database_observation\":";
        if (saved_database)
            saved_database_json(*saved_database);
        else
            std::cout << "null";
        std::cout << ",\"saved_database_diagnostic\":" << quote(saved_database_diagnostic);
        std::cout << ",\"map_header_observation\":";
        if (map_header)
            map_header_json(*map_header);
        else
            std::cout << "null";
        std::cout << ",\"map_header_diagnostic\":" << quote(map_header_diagnostic);
        std::cout << ",\"map_plots_observation\":";
        if (map_plots)
            map_plots_json(*map_plots);
        else
            std::cout << "null";
        std::cout << ",\"map_plots_diagnostic\":" << quote(map_plots_diagnostic);
        std::cout << ",\"saved_map_terrain_source\":";
        if (saved_map_terrain)
            saved_map_terrain_json(*saved_map_terrain);
        else
            std::cout << "null";
        std::cout << ",\"saved_map_terrain_diagnostic\":" << quote(saved_map_terrain_diagnostic);
        std::cout << ",\"map_regions_observation\":";
        if (map_regions)
            map_regions_json(*map_regions);
        else
            std::cout << "null";
        std::cout << ",\"map_regions_diagnostic\":" << quote(map_regions_diagnostic);
        std::cout << ",\"teams_observation\":";
        if (teams)
            teams_json(*teams);
        else
            std::cout << "null";
        std::cout << ",\"teams_diagnostic\":" << quote(teams_diagnostic);
        std::cout << ",\"player_core_observation\":";
        if (player_core)
            player_core_json(*player_core);
        else
            std::cout << "null";
        std::cout << ",\"player_core_diagnostic\":" << quote(player_core_diagnostic);
        std::cout << ",\"player_strategy_observation\":";
        if (player_strategy)
            player_strategy_json(*player_strategy);
        else
            std::cout << "null";
        std::cout << ",\"player_strategy_diagnostic\":" << quote(player_strategy_diagnostic);
        std::cout << ",\"player_diplomacy_observation\":";
        if (player_diplomacy)
            player_diplomacy_json(*player_diplomacy);
        else
            std::cout << "null";
        std::cout << ",\"player_diplomacy_diagnostic\":" << quote(player_diplomacy_diagnostic);
        std::cout << ",\"player_knowledge_observation\":";
        if (player_knowledge)
            player_knowledge_json(*player_knowledge);
        else
            std::cout << "null";
        std::cout << ",\"player_knowledge_diagnostic\":" << quote(player_knowledge_diagnostic);
        std::cout << ",\"player_planning_observation\":";
        if (player_planning)
            player_planning_json(*player_planning);
        else
            std::cout << "null";
        std::cout << ",\"player_planning_diagnostic\":" << quote(player_planning_diagnostic);
        std::cout << ",\"player_traits_observation\":";
        if (player_traits)
            player_traits_json(*player_traits);
        else
            std::cout << "null";
        std::cout << ",\"player_traits_diagnostic\":" << quote(player_traits_diagnostic);
        std::cout << ",\"player_espionage_observation\":";
        if (player_espionage)
            player_espionage_json(*player_espionage);
        else
            std::cout << "null";
        std::cout << ",\"player_espionage_diagnostic\":" << quote(player_espionage_diagnostic);
        std::cout << ",\"player_trade_culture_observation\":";
        if (player_trade_culture)
            player_trade_culture_json(*player_trade_culture);
        else
            std::cout << "null";
        std::cout << ",\"player_trade_culture_diagnostic\":" << quote(player_trade_culture_diagnostic);
        std::cout << ",\"player_administration_observation\":";
        if (player_administration)
            player_administration_json(*player_administration);
        else
            std::cout << "null";
        std::cout << ",\"player_administration_diagnostic\":" << quote(player_administration_diagnostic);
        std::cout << ",\"city_collection_observation\":";
        if (city_collection)
            city_collection_json(*city_collection);
        else if (cities)
            city_collection_json(cities->header);
        else
            std::cout << "null";
        std::cout << ",\"city_core_observation\":";
        const auto *first_city = cities && !cities->cities.empty() ? &cities->cities.front() : nullptr;
        if (city_core)
            city_core_json(*city_core);
        else if (first_city)
            city_core_json(first_city->core);
        else
            std::cout << "null";
        std::cout << ",\"city_core_diagnostic\":" << quote(city_core_diagnostic);
        std::cout << ",\"city_production_observation\":";
        if (city_production)
            city_production_json(*city_production);
        else if (first_city)
            city_production_json(first_city->production);
        else
            std::cout << "null";
        std::cout << ",\"city_production_diagnostic\":" << quote(city_production_diagnostic);
        std::cout << ",\"city_management_observation\":";
        if (city_management)
            city_management_json(*city_management);
        else if (first_city)
            city_management_json(first_city->management);
        else
            std::cout << "null";
        std::cout << ",\"city_management_diagnostic\":" << quote(city_management_diagnostic);
        std::cout << ",\"city_tail_observation\":";
        if (city_tail)
            city_tail_json(*city_tail);
        else if (first_city)
            city_tail_json(first_city->tail);
        else
            std::cout << "null";
        std::cout << ",\"city_tail_diagnostic\":" << quote(city_tail_diagnostic);
        std::cout << ",\"cities_observation\":";
        if (cities)
            cities_json(*cities);
        else
            std::cout << "null";
        std::cout << ",\"cities_diagnostic\":" << quote(cities_diagnostic);
        std::cout << ",\"unit_collection_observation\":";
        if (unit_collection)
            unit_collection_json(*unit_collection);
        else if (units)
            unit_collection_json(units->header);
        else
            std::cout << "null";
        std::cout << ",\"unit_core_observation\":";
        const auto *first_unit = units && !units->units.empty() ? &units->units.front() : nullptr;
        if (unit_core)
            unit_core_json(*unit_core);
        else if (first_unit)
            unit_core_json(first_unit->core);
        else
            std::cout << "null";
        std::cout << ",\"unit_core_diagnostic\":" << quote(unit_core_diagnostic);
        std::cout << ",\"unit_tail_observation\":";
        if (first_unit)
            unit_tail_json(first_unit->tail);
        else
            std::cout << "null";
        std::cout << ",\"units_observation\":";
        if (units)
            units_json(*units);
        else
            std::cout << "null";
        std::cout << ",\"units_diagnostic\":" << quote(units_diagnostic);
        std::cout << ",\"army_operations_observation\":";
        if (army_operations)
            army_operations_json(*army_operations);
        else
            std::cout << "null";
        std::cout << ",\"army_operations_diagnostic\":" << quote(army_operations_diagnostic);
        std::cout << ",\"player_tail_observation\":";
        if (player_tail)
            player_tail_json(*player_tail);
        else
            std::cout << "null";
        std::cout << ",\"player_tail_diagnostic\":" << quote(player_tail_diagnostic);
        std::cout << ",\"all_players_observation\":";
        if (all_players)
            all_players_json(*all_players);
        else
            std::cout << "null";
        std::cout << ",\"all_players_diagnostic\":" << quote(all_players_diagnostic);
        std::cout << ",\"supporting_classes_observation\":";
        if (supporting_classes)
            supporting_classes_json(*supporting_classes);
        else
            std::cout << "null";
        std::cout << ",\"supporting_classes_diagnostic\":" << quote(supporting_classes_diagnostic);
        std::cout << ",\"expansion2_prefix_observation\":";
        if (expansion2_prefix) {
            std::cout << "{\"version\":" << expansion2_prefix->version
                      << ",\"end_offset\":" << expansion2_prefix->end_offset << ",\"scalars\":{";
            for (std::size_t i = 0; i < expansion2_prefix->scalars.size(); ++i) {
                if (i)
                    std::cout << ',';
                std::cout << quote(expansion2_prefix->scalar_names[i]) << ':'
                          << expansion2_prefix->scalars[i];
            }
            std::cout << "}}";
        } else
            std::cout << "null";
        std::cout << ",\"mod_cache\":"
                  << (mod_cache.empty() ? std::string("null") : quote(assets::path_utf8(fs::canonical(mod_cache))))
                  << ",\"content_acquisition_implemented\":true,\"content_acquisition_scope\":\"pinned_7z_zip_"
                     "sources_and_vp542_installer_bundle\",\"content_acquisition_performed\":false,"
                     "\"acquisition_required\":[";
        bool first_acquisition = true;
        for (const auto &requirement : requirements) {
            if (requirement.status != runtime::RequirementStatus::missing &&
                requirement.status != runtime::RequirementStatus::version_mismatch)
                continue;
            if (!first_acquisition)
                std::cout << ',';
            first_acquisition = false;
            std::cout << "{\"id\":" << quote(requirement.id) << ",\"name\":" << quote(requirement.name)
                      << ",\"wire_value\":" << requirement.wire_value << ",\"source_lookup\":"
                      << quote(requirement.kind == content::PackageKind::mod ? "workshop_or_external"
                                                                             : "steam_dlc_install")
                      << ",\"exact_manifest_verification_required\":true}";
        }
        std::cout << ']';
        std::cout << ",\"catalog_diagnostics\":";
        strings(catalog.diagnostics);
        std::cout << ",\"steam_diagnostics\":";
        strings(discovery_diagnostics);
        std::cout << ",\"continuation_missing\":[\"full_saved_state_decoder\",\"selected_rules_database_and_"
                     "lua_runtime\","
                     "\"64bit_gamecore_and_mod_mechanics\",\"gameplay_to_renderer_projection\"],"
                     "\"steamworks_initialized\":false}\n";
        return located && files_ok && catalog.diagnostics.empty() && discovery_diagnostics.empty() &&
                       selection.diagnostics.empty() && game_state_diagnostic.empty() &&
                       game_progression_diagnostic.empty() && game_history_diagnostic.empty() &&
                       game_religion_culture_diagnostic.empty() && game_leagues_diagnostic.empty() &&
                       game_trade_diagnostic.empty() && game_final_systems_diagnostic.empty() &&
                       saved_database_diagnostic.empty() && map_header_diagnostic.empty() &&
                       map_plots_diagnostic.empty() && saved_map_terrain_diagnostic.empty() &&
                       map_regions_diagnostic.empty() &&
                       teams_diagnostic.empty() && player_core_diagnostic.empty() &&
                       player_strategy_diagnostic.empty() && player_diplomacy_diagnostic.empty() &&
                       player_knowledge_diagnostic.empty() && player_planning_diagnostic.empty() &&
                       player_traits_diagnostic.empty() && player_espionage_diagnostic.empty() &&
                       player_trade_culture_diagnostic.empty() && player_administration_diagnostic.empty() &&
                       city_core_diagnostic.empty() && city_production_diagnostic.empty() &&
                       city_management_diagnostic.empty() && city_tail_diagnostic.empty() &&
                       cities_diagnostic.empty() && unit_core_diagnostic.empty() &&
                       units_diagnostic.empty() && army_operations_diagnostic.empty() &&
                       player_tail_diagnostic.empty() && all_players_diagnostic.empty() &&
                       supporting_classes_diagnostic.empty()
                   ? 0
                   : 2;
    } catch (const std::exception &error) {
        std::cerr << "Civis inspection failed: " << error.what() << '\n';
        return 2;
    }
}
