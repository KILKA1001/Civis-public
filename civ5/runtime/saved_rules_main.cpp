#include "civ5/assets/asset_source.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/mod_acquisition.hpp"
#include "civ5/content/mod_archive.hpp"
#include "civ5/content/saved_rules.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/content/save_dimensions.hpp"
#include "civ5/content/technology_rules.hpp"
#include "civ5/content/vp_profile.hpp"
#include "civ5/content/vp_content_profile.hpp"
#include "civ5/content/vp_lua_assets_profile.hpp"
#include "civ5/content/inno_archive.hpp"
#include "civ5/platform/steam_installation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/runtime/team_technologies.hpp"
#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include "civ5/runtime/research.hpp"
#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/production_checkpoint.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include "civ5/runtime/research_turn_checkpoint.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/runtime/research_ai_weights.hpp"
#include "civ5/runtime/vp_science_checkpoint.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_city_espionage.hpp"
#include "civ5/runtime/vp_espionage_checkpoint.hpp"
#include "civ5/runtime/vp_research_cost_profile.hpp"
#include "civ5/runtime/vp_research_cost.hpp"
#include "civ5/runtime/vp_research_selection.hpp"
#include "civ5/runtime/treasury_checkpoint.hpp"
#include "civ5/runtime/vp_rules_fingerprint_profile.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/save_header.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/saves/vp_all_players_profile.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include "civ5/scene/saved_scene_sources.hpp"
#include "civ5/terrain/terrain_source_grid.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"
#include "civ5/terrain/terrain_auxiliary.hpp"
#include "research_probe.hpp"
#include "city_yields_report.hpp"
#include "bnw_unhappiness_report.hpp"
#include "maintenance_report.hpp"
#include "route_economy_report.hpp"
#include "plot_water_report.hpp"
#include "plot_movement_report.hpp"
#include "improvement_yield_report.hpp"
#include "improvement_calculation_report.hpp"
#include "bnw_nature_yield_report.hpp"
#include "vp_nature_yield_report.hpp"
#include "vp_plot_religion_report.hpp"
#include "vp_player_plot_yield_report.hpp"
#include "vp_plot_yield_report.hpp"
#include "vp_city_food_report.hpp"
#include "vp_city_growth_report.hpp"
#include "vp_unit_supply_report.hpp"
#include "vp_great_works_report.hpp"
#include "vp_city_building_yields_report.hpp"
#include "vp_city_religion_minor_yields_report.hpp"
#include "vp_city_trait_yields_report.hpp"
#include "vp_city_happiness_report.hpp"
#include "vp_city_base_yields_report.hpp"
#include "vp_corporations_report.hpp"
#include "vp_city_production_report.hpp"
#include "vp_building_production_report.hpp"
#include "vp_unit_traits_report.hpp"
#include "vp_player_unit_production_report.hpp"
#include "vp_city_unit_production_report.hpp"
#include "production_progress_report.hpp"
#include "resource_visibility_report.hpp"
#include "treasury_report.hpp"
#include "science_report.hpp"
#include "treasury_turn_report.hpp"
#include "research_turn_report.hpp"
#include "player_economy_research_stage_report.hpp"
#include "research_selection_report.hpp"
#include "lua_city_bombard_report.hpp"
#include "lua_gameinfo_report.hpp"
#include "lua_save_data_report.hpp"
#include "lua_pregame_report.hpp"
#include "lua_map_report.hpp"
#include "lua_selected_entrypoints_report.hpp"
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <set>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>

namespace fs = std::filesystem;
namespace {
std::string json(const std::string &value) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (const auto byte : value) {
        const auto c = static_cast<unsigned char>(byte);
        if (c == '\\' || c == '"') {
            result += '\\';
            result += static_cast<char>(c);
        } else if (c < 32) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 15];
        } else
            result += static_cast<char>(c);
    }
    return result + '"';
}
std::uint64_t sum_dlc(const civ5::content::DlcGameplayReport &report,
                      std::uint64_t civ5::content::DlcGameplayPackageReport::*field) {
    std::uint64_t result = 0;
    for (const auto &package : report.packages) {
        if (std::numeric_limits<std::uint64_t>::max() - result < package.*field)
            throw std::runtime_error("DLC report counter overflow");
        result += package.*field;
    }
    return result;
}
std::string pregame_summary(const civ5::saves::PregameObservation *setup) {
    if (!setup) return "null";
    const auto &p = *setup;
    std::ostringstream out;
    out << "{\"slots_version\":" << p.slots_version << ",\"archive_version\":" << p.archive_version
        << ",\"slots_offset\":" << p.slots_offset << ",\"archive_offset\":" << p.archive_offset
        << ",\"archive_end_offset\":" << p.archive_end_offset << ",\"end_offset\":" << p.end_offset
        << ",\"known_player_mask_rows\":"
        << (p.wire_known_player_masks ? std::to_string(p.wire_known_player_masks->size()) : "null")
        << ",\"game_options\":" << p.game_options.size() << ",\"map_options\":" << p.map_options.size();
    // No names, passwords, email addresses, network IDs or opaque strings in logs.
    for (auto name : {"active_player", "game_speed", "world_size", "era", "game_turn", "game_type", "game_map_type"})
        out << ',' << json(name) << ':' << p.archive.integers.at(name);
    for (auto name : {"team_types", "handicaps", "last_human_handicaps", "slot_status", "slot_claims"}) {
        out << ',' << json(name) << ":[";
        bool first = true;
        for (const auto value : p.archive.integer_arrays.at(name)) {
            if (!first) out << ',';
            first = false;
            out << value;
        }
        out << ']';
    }
    out << ",\"world\":{\"id\":" << p.world.id;
    for (const auto &[name, value] : p.world.values.integers) out << ',' << json(name) << ':' << value;
    out << "}}";
    return out.str();
}
} // namespace

int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save, game_root, steam_root, mount_plan, mod_cache, vp_bundle;
        std::string vp_component_name;
        fs::path sources = fs::absolute(argv[0]).parent_path() / "mod_sources.xml";
        fs::path decoder = fs::absolute(argv[0]).parent_path() / "inno-decoder";
        fs::path installer;
        fs::path remap_gamecore;
        std::string terrain_layout;
        std::string session_layout;
        std::optional<int> terrain_grid_mode;
        fs::path terrain_grid_output;
        fs::path terrain_rivers_output;
        fs::path terrain_relief_output;
        fs::path terrain_feature2_output;
        bool terrain_mode_from_save = false;
        bool acquire_missing = false;
        bool use_installed_mod_bytes = false;
        bool inspect_city_yields = false;
        bool inspect_lua_city_bombard = false;
        bool inspect_lua_gameinfo = false;
        bool inspect_lua_save_data = false;
        bool inspect_lua_pregame = false;
        bool inspect_lua_map = false;
        fs::path lua_registry_plan;
        fs::path lua_user_data_root;
        fs::path saved_value_fallback_plan;
        fs::path saved_value_fallback_output;
        std::string lua_language;
        std::string lua_entrypoint_type;
        std::vector<std::string> lua_ui_notifications;
        std::string lua_save_data_library_mod;
        fs::path lua_save_data_output;
        fs::path production_checkpoint_output;
        fs::path session_checkpoint_output;
        std::optional<validation::ResearchProbeInput> research_probe_input;
        std::vector<fs::path> mod_roots;
        std::vector<fs::path> explicit_mod_sources;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout
                     << "civis_saved_rules --save FILE_OR_DIRECTORY "
                        "[--game-root DIRECTORY | --steam-root DIRECTORY] "
                       "[--mount-plan PLAN.xml] [--mods DIRECTORY]... [--mod-cache CACHE]\n"
                       "                       [--mod-source FILE.modinfo]... (resolve duplicate mod versions)\n"
                       "                       [--use-installed-mod-bytes] (diagnostic local mod edits)\n"
                       "                       [--acquire-missing [--sources CATALOG.xml] "
                       "[--decoder DIRECTORY] [--installer FILE]]\n"
                       "                       [--vp-component NAME [--vp-bundle DIRECTORY]]\n"
                       "                       [--terrain-layout expansion2|vp542 [--vp-remap-gamecore FILE]]\n"
                       "                       [--session-layout expansion2|vp542]\n"
                       "                       [--city-yields]\n"
                       "                       [--production-checkpoint NEW.CivisSave] (saved city production state)\n"
                       "                       [--session-checkpoint NEW.CivisSave] (city production and research state)\n"
                       "                       [--lua-city-bombard]\n"
                       "                       [--lua-gameinfo] [--lua-save-data]\n"
                       "                       [--lua-pregame]\n"
                       "                       [--lua-map]\n"
                       "                       [--lua-registry PLAN.xml --lua-entrypoints TYPE]\n"
                       "                       [--lua-user-data-root DIRECTORY] (read-only source for mod preferences)\n"
                       "                       [--saved-value-fallback-plan PLAN.xml] (opt-in ModUserData or Lua preset repair)\n"
                       "                       [--saved-value-fallback-output NEW.CivisSave] (persist repaired mod settings)\n"
                       "                       [--lua-language TYPE] (selected language for Lua, e.g. ru_RU)\n"
                       "                       [--lua-ui-notification NAME]... (diagnostic UI capability)\n"
                       "                       [--lua-save-data-library-mod ID] (diagnostic only)\n"
                       "                       [--lua-save-data-output NEW.CivisSave] (diagnostic mod-data edit)\n"
                       "                       [--research-probe PLAYER,TECH,CHANGE_TIMES100,SCIENCE_TIMES100]\n"
                       "                       [--terrain-grid-mode auto|0|1|2 [--terrain-grid-output NEW_FILE]\n"
                       "                        [--terrain-rivers-output NEW_FILE]"
                       " [--terrain-relief-output NEW_FILE]"
                       " [--terrain-feature2-output NEW_FILE]]\n"
                       "Applies the selected save profile to one isolated x64 rules database.\n"
                       "--save accepts an original .Civ5Save or a Civis .CivisSave.\n"
                       "--research-probe uses controlled inputs on a disposable copy; it is not a game turn.\n"
                       "--acquire-missing downloads exact save mod versions and manifest "
                       "dependencies into --mod-cache before discovery.\n"
                       "Return 0 when rules are ready, 3 when content is incomplete, 2 on a fatal error.\n";
                return 0;
            }
            if (option == L"--acquire-missing") {
                acquire_missing = true;
                continue;
            }
            if (option == L"--use-installed-mod-bytes") { use_installed_mod_bytes = true; continue; }
            if (option == L"--city-yields") { inspect_city_yields = true; continue; }
            if (option == L"--lua-city-bombard") { inspect_lua_city_bombard = true; continue; }
            if (option == L"--lua-gameinfo") { inspect_lua_gameinfo = true; continue; }
            if (option == L"--lua-save-data") { inspect_lua_save_data = true; continue; }
            if (option == L"--lua-pregame") { inspect_lua_pregame = true; continue; }
            if (option == L"--lua-map") { inspect_lua_map = true; continue; }
            if (i + 1 >= argc)
                throw std::runtime_error("Missing option value");
            const auto value = fs::path(argv[++i]);
            if (option == L"--lua-save-data-library-mod")
                lua_save_data_library_mod=value.string();
            else if (option == L"--lua-save-data-output")
                lua_save_data_output=value;
            else if (option == L"--production-checkpoint")
                production_checkpoint_output=value;
            else if (option == L"--session-checkpoint")
                session_checkpoint_output=value;
            else if (option == L"--save")
                save = value;
            else if (option == L"--game-root")
                game_root = value;
            else if (option == L"--steam-root")
                steam_root = value;
            else if (option == L"--mount-plan")
                mount_plan = value;
            else if (option == L"--lua-registry")
                lua_registry_plan = value;
            else if (option == L"--lua-user-data-root")
                lua_user_data_root = value;
            else if (option == L"--saved-value-fallback-plan")
                saved_value_fallback_plan = value;
            else if (option == L"--saved-value-fallback-output")
                saved_value_fallback_output = value;
            else if (option == L"--lua-language")
                lua_language = assets::path_utf8(value);
            else if (option == L"--lua-entrypoints")
                lua_entrypoint_type = assets::path_utf8(value);
            else if (option == L"--lua-ui-notification")
                lua_ui_notifications.push_back(assets::path_utf8(value));
            else if (option == L"--mods")
                mod_roots.push_back(value);
            else if (option == L"--mod-source")
                explicit_mod_sources.push_back(value);
            else if (option == L"--mod-cache")
                mod_cache = value;
            else if (option == L"--sources")
                sources = value;
            else if (option == L"--decoder")
                decoder = value;
            else if (option == L"--installer")
                installer = value;
            else if (option == L"--vp-bundle")
                vp_bundle = value;
            else if (option == L"--vp-component")
                vp_component_name = assets::path_utf8(value);
            else if (option == L"--terrain-layout")
                terrain_layout = assets::path_utf8(value);
            else if (option == L"--session-layout")
                session_layout = assets::path_utf8(value);
            else if (option == L"--research-probe") {
                if (research_probe_input) throw std::runtime_error("Duplicate research probe");
                research_probe_input = validation::parse_research_probe(assets::path_utf8(value));
            }
            else if (option == L"--vp-remap-gamecore")
                remap_gamecore = value;
            else if (option == L"--terrain-grid-output")
                terrain_grid_output = value;
            else if (option == L"--terrain-rivers-output")
                terrain_rivers_output = value;
            else if (option == L"--terrain-relief-output")
                terrain_relief_output = value;
            else if (option == L"--terrain-feature2-output")
                terrain_feature2_output = value;
            else if (option == L"--terrain-grid-mode") {
                const auto mode = assets::path_utf8(value);
                if (mode != "auto" && mode != "0" && mode != "1" && mode != "2")
                    throw std::runtime_error("Invalid terrain grid mode");
                terrain_grid_mode = mode == "auto" ? -1 : mode[0] - '0';
            }
            else
                throw std::runtime_error("Unknown option");
        }
        if (save.empty())
            throw std::runtime_error("Supply --save");
        if (use_installed_mod_bytes && mod_roots.empty())
            throw std::runtime_error("--use-installed-mod-bytes requires --mods");
        if (!game_root.empty() && !steam_root.empty())
            throw std::runtime_error("Choose --game-root or --steam-root");
        bool steam_discovered = false;
        if (game_root.empty()) {
            const auto steam = platform::discover_civ5(
                steam_root.empty() ? platform::default_steam_root() : steam_root);
            if (steam.installations.size() != 1)
                throw std::runtime_error("Expected one installed Civilization V; supply --game-root");
            game_root = steam.installations.front().game_root;
            if (fs::is_directory(steam.installations.front().workshop_root))
                mod_roots.push_back(steam.installations.front().workshop_root);
            steam_discovered = true;
        }
        if (!terrain_layout.empty() && terrain_layout != "expansion2" && terrain_layout != "vp542")
            throw std::runtime_error("Unknown terrain layout");
        if (!session_layout.empty() && session_layout != "expansion2" && session_layout != "vp542")
            throw std::runtime_error("Unknown session layout");
        if (!session_layout.empty() && !terrain_layout.empty() && session_layout != terrain_layout)
            throw std::runtime_error("Session and terrain must use the same saved layout");
        const auto decode_layout = session_layout.empty() ? terrain_layout : session_layout;
        if (research_probe_input && session_layout.empty())
            throw std::runtime_error("Research probe requires --session-layout");
        if (inspect_city_yields && session_layout.empty())
            throw std::runtime_error("City yields require --session-layout");
        if (inspect_lua_city_bombard && session_layout.empty())throw std::runtime_error("Lua city bombard requires --session-layout");
        if (inspect_lua_gameinfo && session_layout.empty())throw std::runtime_error("Lua GameInfo requires --session-layout");
        if (inspect_lua_pregame && session_layout.empty())throw std::runtime_error("Lua pregame requires --session-layout");
        if (inspect_lua_map && session_layout.empty())throw std::runtime_error("Lua map requires --session-layout");
        if (inspect_lua_save_data && session_layout.empty())throw std::runtime_error("Lua saved database requires --session-layout");
        if(lua_registry_plan.empty()!=lua_entrypoint_type.empty()||
           (!lua_registry_plan.empty()&&session_layout.empty()))
            throw std::runtime_error("Selected Lua entrypoints require --lua-registry, --lua-entrypoints and --session-layout");
        if(lua_registry_plan.empty()&&!lua_ui_notifications.empty())
            throw std::runtime_error("Lua UI notification capabilities require --lua-registry");
        if(!saved_value_fallback_plan.empty()&&session_layout.empty())
            throw std::runtime_error("Saved-value fallback requires --session-layout");
        if(!saved_value_fallback_output.empty()&&
           (saved_value_fallback_plan.empty()||fs::exists(saved_value_fallback_output)||
            runtime::ascii_lower(assets::path_utf8(saved_value_fallback_output.extension()))!=".civissave"))
            throw std::runtime_error("Saved-value fallback output requires a plan and a new .CivisSave file");
        if(!lua_save_data_library_mod.empty()&&!inspect_lua_save_data)throw std::runtime_error("Diagnostic table loader requires --lua-save-data");
        if(!lua_save_data_output.empty()&&(!inspect_lua_save_data||fs::exists(lua_save_data_output)))
            throw std::runtime_error("Saved database output requires --lua-save-data and a new file");
        if(!production_checkpoint_output.empty()&&(!inspect_city_yields||fs::exists(production_checkpoint_output)||
            runtime::ascii_lower(assets::path_utf8(production_checkpoint_output.extension()))!=".civissave"))
            throw std::runtime_error("Production checkpoint requires --city-yields and a new .CivisSave file");
        if(!session_checkpoint_output.empty()&&(!production_checkpoint_output.empty()||!inspect_city_yields||
            fs::exists(session_checkpoint_output)||
            runtime::ascii_lower(assets::path_utf8(session_checkpoint_output.extension()))!=".civissave"))
            throw std::runtime_error("Session checkpoint requires --city-yields, no separate production output, and a new .CivisSave file");
        if (terrain_grid_mode && terrain_layout.empty())
            throw std::runtime_error("Terrain grid mode requires --terrain-layout");
        if (!terrain_grid_output.empty() && (!terrain_grid_mode || fs::exists(terrain_grid_output)))
            throw std::runtime_error("Terrain grid output requires a mode and a new file");
        if (!terrain_rivers_output.empty() && (!terrain_grid_mode || fs::exists(terrain_rivers_output)))
            throw std::runtime_error("Terrain rivers output requires a mode and a new file");
        if (!terrain_relief_output.empty() && (!terrain_grid_mode || fs::exists(terrain_relief_output)))
            throw std::runtime_error("Terrain relief output requires a mode and a new file");
        if (!terrain_feature2_output.empty() && (!terrain_grid_mode || fs::exists(terrain_feature2_output)))
            throw std::runtime_error("Terrain feature-2 output requires a mode and a new file");
        if (!terrain_grid_output.empty() && !terrain_rivers_output.empty() &&
            fs::weakly_canonical(terrain_grid_output) == fs::weakly_canonical(terrain_rivers_output))
            throw std::runtime_error("Terrain grid and rivers need different output files");
        for (const auto &other : {terrain_grid_output, terrain_rivers_output, terrain_relief_output})
            if (!terrain_feature2_output.empty() && !other.empty() &&
                fs::weakly_canonical(terrain_feature2_output) == fs::weakly_canonical(other))
                throw std::runtime_error("Terrain feature-2 output needs its own file");
        for (const auto &other : {terrain_grid_output, terrain_rivers_output})
            if (!terrain_relief_output.empty() && !other.empty() &&
                fs::weakly_canonical(terrain_relief_output) == fs::weakly_canonical(other))
                throw std::runtime_error("Terrain relief output needs its own file");
        if(!vp_bundle.empty()&&vp_component_name.empty())
            throw std::runtime_error("--vp-bundle requires --vp-component");
        const auto root = fs::canonical(game_root);
        runtime::SaveCandidate candidate;
        saves::LoadedSave loaded;
        runtime::FileIdentity save_identity;
        const bool civis_save = fs::is_regular_file(save) &&
                                runtime::ascii_lower(assets::path_utf8(save.extension())) ==
                                    ".civissave";
        if (civis_save) {
            save_identity=runtime::identify_file(save);
            loaded = saves::read_loaded_save(save);
            candidate = {fs::canonical(save), loaded.header};
        } else {
            const auto save_selection = runtime::select_save(save);
            if (!save_selection.selected)
                throw std::runtime_error("No supported save header");
            candidate = save_selection.candidates[*save_selection.selected];
            save_identity=runtime::identify_file(candidate.path);
            loaded = saves::read_loaded_save(candidate.path);
        }
        if (runtime::identify_file(candidate.path).sha256!=save_identity.sha256 || loaded.header!=candidate.header)
            throw std::runtime_error("Save payload header changed while opening");
        const auto *vp_research_profile=saves::profile_section(
            loaded.profile_sections,saves::CivisSaveSectionId::vp_research_profile);
        const auto *vp_science_profile=saves::profile_section(
            loaded.profile_sections,saves::CivisSaveSectionId::vp_science_profile);
        const auto *vp_espionage_profile=saves::profile_section(
            loaded.profile_sections,saves::CivisSaveSectionId::vp_espionage_profile);
        const auto checkpoint_output=session_checkpoint_output.empty()?production_checkpoint_output:session_checkpoint_output;
        if(!checkpoint_output.empty()&&fs::weakly_canonical(checkpoint_output)==candidate.path)
            throw std::runtime_error("Checkpoint output must differ from input save");
        if(!saved_value_fallback_output.empty()&&fs::weakly_canonical(saved_value_fallback_output)==candidate.path)
            throw std::runtime_error("Saved-value fallback output must differ from input save");
        std::optional<content::SavedModAcquisitionReport> acquisition;
        if (acquire_missing && !candidate.header.mods.empty()) {
            if (mod_cache.empty())
                throw std::runtime_error("--acquire-missing requires --mod-cache");
            std::vector<content::ModTarget> targets;
            targets.reserve(candidate.header.mods.size());
            for (const auto &mod : candidate.header.mods)
                targets.push_back({runtime::ascii_lower(mod.id), mod.name, mod.wire_value});
            const auto source_catalog = content::read_mod_sources(sources);
            acquisition = content::acquire_saved_mods(targets, source_catalog, mod_cache, decoder,
                                                      installer);
        }
        std::optional<content::VpProfile> installer_profile;
        fs::path vp_view;
        if(!vp_component_name.empty()){
            if(mod_cache.empty())throw std::runtime_error("--vp-component requires --mod-cache");
            const auto cp=std::string("d1b6328c-ff44-4b0d-aad7-c657f83610cd");
            const auto vp=std::string("8411a7a8-dad3-4622-a18e-fcc18324c799");
            const auto needs_profile=std::any_of(candidate.header.mods.begin(),candidate.header.mods.end(),
                [&](const auto &mod){const auto id=runtime::ascii_lower(mod.id);return id==cp||id==vp;});
            if(!needs_profile)throw std::runtime_error("Save does not request a CP/VP GameCore profile");
            content::initialize_mod_cache(mod_cache);
            const auto cache=fs::canonical(mod_cache);
            const auto bundle=vp_bundle.empty()?cache/"bundles"/content::vp_542_installer_sha256:vp_bundle;
            installer_profile=content::build_vp_542_profile(bundle,content::vp_component(vp_component_name));
            const auto views=cache/"profiles";
            fs::create_directory(views);
            content::require_plain_directory(views);
            vp_view=content::publish_vp_542_profile_view(*installer_profile,
                views/(std::string(content::vp_542_installer_sha256)+"-"+vp_component_name));
            if(decode_layout=="vp542"&&remap_gamecore.empty())
                remap_gamecore=vp_view/"MODS/(1) Community Patch/CvGameCore_Expansion2.dll";
        }
        if ((decode_layout == "vp542") != !remap_gamecore.empty())
            throw std::runtime_error("VP saved-state decoding requires a verified GameCore ID map; other layouts do not");
        if (!remap_gamecore.empty()) {
            const auto identity = runtime::identify_file(remap_gamecore);
            if (identity.bytes != 16980992 ||
                identity.sha256 != "30872ae3ecce6607613aaca76084d19d96785ba2156bd21fc304612143e2ef25")
                throw std::runtime_error("Unverified GameCore ID-remapping implementation");
        }
        std::vector<fs::path> roots{root / "Assets/DLC"};
        roots.insert(roots.end(), mod_roots.begin(), mod_roots.end());
        if (!mod_cache.empty())
            roots.push_back(fs::canonical(mod_cache) / "packages");
        if(!vp_view.empty())roots.push_back(vp_view/"MODS");
        auto catalog = content::discover_packages(roots);
        if(installer_profile){
            std::set<std::pair<std::string,std::uint64_t>> selected;
            for(const auto &mod:installer_profile->installed_mods)
                selected.emplace(mod.manifest.id,mod.manifest.version);
            std::erase_if(catalog.packages,[&](const auto &package){
                if(package.kind!=content::PackageKind::mod||
                   !selected.contains({package.id,package.version}))return false;
                const auto relative=package.path.lexically_relative(vp_view/"MODS");
                return relative.empty()||relative.is_absolute()||*relative.begin()==fs::path("..");
            });
        }
        const auto vp_catalog_overlays=installer_profile?
            content::apply_vp_542_catalog_overlays(catalog,*installer_profile,root,vp_view):0;
        runtime::select_saved_mod_sources(catalog,candidate.header,explicit_mod_sources);
        const auto verified_cache_mirrors=mod_cache.empty()?std::uint64_t{0}:
            content::prefer_verified_cached_mods(catalog,fs::canonical(mod_cache)/"packages");
        auto profile = runtime::resolve_saved_content_profile(candidate.header, catalog);
        std::vector<runtime::VerifiedManifestHash> verified_archive_hashes;
        std::vector<runtime::ManifestFileOmission> verified_archive_omissions;
        if (!mod_cache.empty())
            for (const auto index : profile.enabled_mods) {
                const auto verified = content::verify_cached_mod_package(
                    mod_cache, catalog.packages.at(index));
                if (!verified) continue;
                for (const auto &hash : verified->stale_manifest_hashes)
                    verified_archive_hashes.push_back(
                        {index, hash.path, hash.declared_md5, hash.actual_md5});
                for(const auto &missing:verified->missing_manifest_assets)
                    verified_archive_omissions.push_back(
                        {verified->manifest.id,missing.path,missing.declared_md5,
                         verified->manifest.version});
            }
        if(installer_profile&&profile.all_requirements_located){
            auto omissions=content::vp_manifest_file_omissions(*installer_profile,catalog,profile);
            omissions.insert(omissions.end(),verified_archive_omissions.begin(),
                             verified_archive_omissions.end());
            profile=runtime::resolve_saved_content_profile(candidate.header,catalog,omissions,
                                                           verified_archive_hashes);
        } else if (!verified_archive_hashes.empty()||!verified_archive_omissions.empty()) {
            profile=runtime::resolve_saved_content_profile(candidate.header,catalog,
                                                           verified_archive_omissions,
                                                           verified_archive_hashes);
        }
        std::vector<std::size_t> installed_byte_packages;
        if(use_installed_mod_bytes)for(const auto index:profile.enabled_mods){
            const auto package_root=fs::canonical(catalog.packages.at(index).path.parent_path());
            for(const auto &mod_root:mod_roots){
                const auto relative=package_root.lexically_relative(fs::canonical(mod_root));
                if(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=fs::path("..")){
                    installed_byte_packages.push_back(index);break;
                }
            }
        }
        std::optional<runtime::FileIdentity> mount_identity;
        if (!mount_plan.empty()) mount_identity = runtime::identify_file(mount_plan);
        auto mounts = mount_plan.empty()
            ? content::build_selected_loose_resource_plan(root, profile.enabled_dlc, catalog)
            : content::read_loose_resource_plan(mount_plan);
        if (mount_identity && runtime::identify_file(mount_plan).sha256 != mount_identity->sha256)
            throw std::runtime_error("Resource mount plan changed while indexing");
        std::uint64_t selected_cache_resource_files = 0;
        if (!mod_cache.empty() && !profile.enabled_mods.empty()) {
            const auto packages_root = fs::canonical(mod_cache) / "packages";
            auto source = std::make_shared<assets::LooseResourceIndex>(packages_root);
            for (const auto index : content::order_mod_actions(profile.enabled_mods, catalog)) {
                const auto package_root = fs::canonical(catalog.packages.at(index).path.parent_path());
                const auto relative = package_root.lexically_relative(packages_root);
                if (!relative.empty() && !relative.is_absolute() && *relative.begin() != fs::path(".."))
                    source->scan({package_root, {}, false});
            }
            selected_cache_resource_files = source->indexed_files();
            if (selected_cache_resource_files)
                mounts.resources.mount({UINT64_MAX, 1, 0, assets::resource_lookup(source), true, true,
                                        assets::resource_lookup_all(source)});
        }
        std::uint64_t vp_view_selected_mods=0, vp_view_resource_files=0;
        if(installer_profile){
            for(const auto index:profile.enabled_mods){
                const auto relative=catalog.packages.at(index).path.lexically_relative(vp_view/"MODS");
                if(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=fs::path(".."))
                    ++vp_view_selected_mods;
            }
            for(const auto &saved_mod:candidate.header.mods){
                const auto id=runtime::ascii_lower(saved_mod.id);
                if(id!="d1b6328c-ff44-4b0d-aad7-c657f83610cd"&&
                   id!="8411a7a8-dad3-4622-a18e-fcc18324c799")continue;
                const auto found=std::find_if(profile.enabled_mods.begin(),profile.enabled_mods.end(),
                    [&](auto index){
                        const auto &package=catalog.packages.at(index);
                        const auto relative=package.path.lexically_relative(vp_view/"MODS");
                        return package.id==id&&package.version==saved_mod.wire_value&&
                            !relative.empty()&&!relative.is_absolute()&&*relative.begin()!=fs::path("..");
                    });
                if(found==profile.enabled_mods.end())
                    throw std::runtime_error("Saved CP/VP package was not selected from installer profile");
            }
            auto source=std::make_shared<assets::LooseResourceIndex>(vp_view);
            source->scan({vp_view/"Game",{},false});
            source->scan({vp_view/"User",{},false});
            for(const auto index:content::order_mod_actions(profile.enabled_mods,catalog)){
                const auto &package=catalog.packages.at(index);
                const auto relative=package.path.lexically_relative(vp_view/"MODS");
                if(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!=fs::path(".."))
                    source->scan({package.path.parent_path(),{},false});
            }
            vp_view_resource_files=source->indexed_files();
            mounts.resources.mount({UINT64_MAX-1,1,0,assets::resource_lookup(source),true,true,
                                     assets::resource_lookup_all(source)});
        }
        content::RulesDatabase database;
        const auto report = content::activate_saved_rules(database, root, profile, catalog,
                                                          mounts.resources,64ULL*1024*1024,{},
                                                          installed_byte_packages,
                                                          verified_archive_hashes);
        std::optional<saves::SavedSessionSnapshot> terrain_snapshot;
        std::optional<saves::SavedSessionDecodeContext> session_context;
        std::optional<content::GameplayPostprocessReport> postprocessing;
        std::optional<content::InfoTypeCatalog> info_types;
        std::optional<content::GlobalDefineCache> global_defines;
        std::optional<content::TechnologyRules> technologies;
        std::optional<content::CustomModOptions> custom_options;
        std::optional<content::ResearchRules> research_rules;
        std::optional<runtime::ResearchContext> research_context;
        std::map<std::int64_t, runtime::BoundTeamResearch> research_bindings;
        bool research_fully_resolved = true;
        std::optional<runtime::PlayerResearchContext> player_research;
        std::map<std::int64_t, std::vector<std::pair<std::int64_t, runtime::PlayerResearchQuote>>> player_quotes;
        std::map<std::int64_t, std::pair<std::int64_t, std::int64_t>> player_research_before;
        bool player_research_resolved = true;
        std::optional<std::string> research_probe_report;
        std::optional<std::string> city_yields_report;
        std::optional<std::string> bnw_unhappiness_report;
        std::optional<std::string> maintenance_report;
        std::optional<std::string> route_economy_report;
        std::optional<std::string> plot_water_report;
        std::optional<std::string> plot_movement_report;
        std::optional<std::string> improvement_yield_report;
        std::optional<std::string> improvement_calculation_report;
        std::optional<std::string> bnw_nature_yield_report;
        std::optional<std::string> vp_nature_yield_report;
        std::optional<std::string> vp_plot_religion_report;
        std::optional<std::string> vp_player_plot_yield_report;
        std::optional<std::string> vp_plot_yield_report;
        std::optional<std::string> vp_city_food_report;
        std::optional<std::string> vp_city_growth_report;
        std::optional<std::string> vp_unit_supply_report;
        std::optional<std::string> vp_great_works_report;
        std::optional<std::string> vp_city_building_yields_report;
        std::optional<std::string> vp_city_religion_minor_yields_report;
        std::optional<std::string> vp_city_trait_yields_report;
        std::optional<std::string> vp_city_happiness_report;
        std::optional<std::string> vp_city_base_yields_report;
        std::optional<std::string> vp_corporations_report;
        std::optional<std::string> vp_city_production_report;
        std::optional<std::string> vp_building_production_report;
        std::optional<std::string> vp_unit_traits_report;
        std::optional<std::string> vp_player_unit_production_report;
        std::optional<std::string> vp_city_unit_production_report;
        std::optional<std::string> city_production_progress_report;
        std::optional<saves::CivisSaveSection> production_checkpoint_section;
        std::optional<runtime::ResearchCheckpointIdentity> research_checkpoint_identity;
        std::optional<runtime::ResearchCheckpointSections> research_checkpoint_sections;
        std::optional<saves::CivisSaveSection> treasury_checkpoint_section;
        std::optional<saves::CivisSaveSection> research_turn_checkpoint_section;
        std::optional<saves::CivisSaveSection> random_checkpoint_section;
        std::optional<saves::CivisSaveSection> vp_science_checkpoint_section;
        std::optional<saves::CivisSaveSection> vp_espionage_checkpoint_section;
        std::optional<std::string> resource_visibility_report;
        std::optional<std::string> treasury_report;
        std::optional<std::string> science_report;
        std::optional<std::string> treasury_turn_report;
        std::optional<std::string> research_turn_report;
        std::optional<std::string> player_economy_research_stage_report;
        std::optional<std::string> research_selection_report;
        std::optional<runtime::ResearchAiWeights> research_ai_weights;
        std::optional<std::string> lua_city_bombard_report;
        std::optional<std::string> lua_gameinfo_report;
        std::optional<std::string> lua_pregame_report;
        std::optional<std::string> lua_map_report;
        std::optional<std::string> lua_save_data_report;
        std::optional<std::string> lua_selected_entrypoints_report;
        std::optional<runtime::SavedValueFallbackReport> saved_value_fallback_report;
        std::optional<std::string> saved_value_fallback_source_kind;
        std::optional<std::string> saved_value_fallback_output_sha256;
        std::optional<std::vector<runtime::TeamTechnologies>> team_technologies;
        bool team_technologies_mapped = true;
        std::uint64_t feature_definitions = 0;
        std::optional<terrain::TerrainSourceGrid> terrain_grid;
        std::vector<terrain::RiverEdge> terrain_rivers;
        std::optional<terrain::RiverNetwork> terrain_river_network;
        std::optional<terrain::TerrainAuxiliarySources> terrain_auxiliary;
        std::optional<scene::SavedSceneSources> saved_scene_sources;
        if (!decode_layout.empty()) {
            if (!profile.all_requirements_located)
                throw std::runtime_error("Saved-state decoding requires all saved content manifests");
            // This explicit diagnostic projects the database actually produced
            // above. Unrelated missing UI/art files and action errors stay in
            // the activation report; a known cell mask does not certify that
            // the complete content profile or game session is ready.
            saves::SavedSessionDecodeContext context;
            if (decode_layout == "vp542" && !database.remap_vp_ids().all_succeeded)
                throw std::runtime_error("VP saved-state ID remapping failed");
            // Initial dimensions of this imported session, not Civis allocation
            // ceilings. Wider sessions may supply wider values to the same API.
            std::vector<content::GameplayDefine> runtime_defines{
                {"MAX_CIV_PLAYERS", 63}, {"MAX_CIV_TEAMS", 63},
                {"MAX_MAJOR_CIVS", decode_layout == "vp542" ? 43 : 22},
                {"MAX_MINOR_CIVS", decode_layout == "vp542" ? 20 : 41},
                {"MAX_PLAYERS", 64}, {"MAX_TEAMS", 64},
                {"BARBARIAN_PLAYER", 63}, {"BARBARIAN_TEAM", 63}, {"NUM_CITY_PLOTS", 37}};
            if (decode_layout == "vp542") {
                runtime_defines.push_back({"MIN_CITY_RADIUS", 2});
                runtime_defines.push_back({"MAX_CITY_RADIUS", 5});
            }
            runtime_defines.push_back({"CITY_HOME_PLOT", 0});
            if (decode_layout == "vp542") runtime_defines.push_back({"MAX_CITY_DIAMETER", 11});
            runtime_defines.push_back({"CITY_PLOTS_RADIUS", 3});
            runtime_defines.push_back({"CITY_PLOTS_DIAMETER", 7});
            postprocessing = database.postprocess_gameplay(
                decode_layout == "vp542" ? content::GameplayPostprocessProfile::vp542
                                         : content::GameplayPostprocessProfile::bnw,
                runtime_defines);
            if (decode_layout == "vp542") custom_options = content::load_custom_mod_options(database);
            info_types = database.cache_info_types();
            global_defines = database.cache_global_defines(decode_layout == "vp542"
                ? content::GlobalDefineProfile::vp542 : content::GlobalDefineProfile::bnw);
            technologies = content::load_technology_rules(database, *info_types,
                decode_layout == "vp542" ? content::TechnologyProfile::vp542 : content::TechnologyProfile::bnw, 5);
            research_rules = content::load_research_rules(database, technologies->profile);
            if (decode_layout == "vp542") {
                // These slot dimensions belong to the explicitly selected,
                // pinned 5.4.2 source GameCore, not to host allocation limits.
                if (!session_layout.empty()) {
                    const content::VpSessionSourceDimensions source{
                        64, 64, 63, 63, 43, 20, 80, 80, 17, 5, 9, 7, 5, 113, 60};
                    context = content::vp_saved_session_context(database, source);
                } else {
                    const auto dimensions = content::vp_save_wire_dimensions(database, 64, 64);
                    context.layout = saves::GameStateLayout::vp542_corpus;
                    context.roster = dimensions.roster;
                    context.progression = dimensions.progression;
                    context.trade = {43, 17};
                    context.plots = {64, 64, 43, 17, 0};
                }
            } else if (!session_layout.empty()) {
                context.decode_map_regions = true;
                context.decode_teams = true;
                context.decode_all_players = true;
                context.decode_supporting_classes = true;
                context.all_players = saves::AllPlayersContext{};
            }
            auto features = content::saved_terrain_features(database, *info_types);
            feature_definitions = features.definitions;
            context.terrain_features = std::move(features.resolve);
            context.permit_live_production_overlay=inspect_city_yields;
            context.permit_live_research_overlay=!session_layout.empty();
            context.permit_live_treasury_overlay=inspect_city_yields;
            context.permit_live_research_turn_overlay=inspect_city_yields;
            context.permit_live_random_overlay=!session_layout.empty();
            if(!session_layout.empty())context.permitted_profile_sections.insert(
                static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_research_profile));
            if(inspect_city_yields)context.permitted_profile_sections.insert(
                static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_science_profile));
            if(inspect_city_yields&&decode_layout=="vp542")context.permitted_profile_sections.insert(
                static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_espionage_profile));
            if(loaded.live_team_research||loaded.live_treasury||loaded.live_research_turn||
               loaded.live_simulation_random||vp_science_profile||vp_espionage_profile||
               !session_checkpoint_output.empty()){
                const auto normalizations=session_layout=="vp542"?
                    runtime::vp_rules_fingerprint_normalizations():
                    std::span<const content::RulesFingerprintNormalization>{};
                const auto fingerprint=database.gameplay_fingerprint_data(normalizations);
                research_checkpoint_identity=runtime::ResearchCheckpointIdentity{
                    runtime::identify_bytes(loaded.bytes).sha256,
                    runtime::identify_bytes(fingerprint).sha256};
            }
            terrain_snapshot = saves::observe_loaded_session(loaded, context);
            if(!session_layout.empty()){
                auto random_streams=runtime::restore_session_random(terrain_snapshot->progression);
                if(loaded.live_simulation_random){
                    if(!research_checkpoint_identity)throw std::runtime_error("Missing simulation RNG checkpoint identity");
                    runtime::restore_simulation_random_checkpoint(
                        {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_simulation_random),0,
                            *loaded.live_simulation_random},random_streams,*research_checkpoint_identity);
                }
                if(!session_checkpoint_output.empty()){
                    if(!research_checkpoint_identity)throw std::runtime_error("Missing simulation RNG checkpoint identity");
                    random_checkpoint_section=runtime::make_simulation_random_checkpoint(
                        random_streams,*research_checkpoint_identity);
                }
            }
            if(!saved_value_fallback_plan.empty()){
                auto &saved_data=terrain_snapshot->saved_database;
                if(saved_data.image.empty()||!saved_data.quick_check_passed||saved_data.non_sqlite_image_skipped)
                    throw std::runtime_error("Saved-value fallback requires a verified saved database");
                const auto plan=runtime::read_saved_value_fallback_plan(saved_value_fallback_plan);
                const auto selected=std::find_if(profile.enabled_mods.begin(),profile.enabled_mods.end(),[&](auto index){
                    const auto &package=catalog.packages.at(index);
                    return package.id==plan.mod_id&&package.version==plan.mod_version;
                });
                if(selected==profile.enabled_mods.end())
                    throw std::runtime_error("Saved-value fallback mod/version is not selected by the save");
                std::shared_ptr<content::RulesDatabase> source;
                if(plan.lua_preset){
                    source=runtime::load_saved_value_fallback_lua_source(
                        catalog.packages.at(*selected),database,plan,lua_language);
                    saved_value_fallback_source_kind="lua_preset";
                }else{
                    if(lua_user_data_root.empty())
                        throw std::runtime_error("ModUserData fallback requires --lua-user-data-root");
                    source=runtime::load_saved_value_fallback_source(lua_user_data_root,plan);
                    saved_value_fallback_source_kind="mod_user_data";
                }
                auto private_database=content::RulesDatabase::from_saved_image(saved_data.image);
                saved_value_fallback_report=runtime::fill_missing_saved_values(*private_database,*source,plan);
                saved_data.image=private_database->saved_image();
                if(!saved_value_fallback_output.empty()){
                    if(runtime::identify_file(candidate.path).sha256!=save_identity.sha256)
                        throw std::runtime_error("Input save changed before repaired database write");
                    const auto document=saves::make_civis_import_document(loaded,save_identity.sha256,
                        std::span<const std::uint8_t>(saved_data.image));
                    saves::write_civis_save(saved_value_fallback_output,document);
                    const auto verified=saves::read_loaded_save(saved_value_fallback_output);
                    if(verified.bytes!=loaded.bytes||verified.header!=loaded.header||
                       verified.source_prefix!=loaded.source_prefix||
                       verified.source_sha256!=loaded.source_sha256.value_or(save_identity.sha256)||
                       verified.saved_database_override!=saved_data.image)
                        throw std::runtime_error("Repaired Civis save did not retain its source and database");
                    const auto reopened=saves::observe_loaded_session(verified,context);
                    if(!reopened.saved_database.image_from_civis_section||
                       reopened.saved_database.image!=saved_data.image)
                        throw std::runtime_error("Repaired saved database did not reload from Civis save");
                    saved_value_fallback_output_sha256=runtime::identify_file(saved_value_fallback_output).sha256;
                }
            }
            if (terrain_snapshot->teams && technologies->all_succeeded() && info_types->report().all_succeeded()) {
                team_technologies = runtime::restore_team_technologies(*terrain_snapshot->teams, *technologies, *info_types);
                for (const auto &team : *team_technologies)
                    team_technologies_mapped = team_technologies_mapped && team.remap().all_mapped();
            }
            if (terrain_snapshot->pregame && terrain_snapshot->all_players && team_technologies &&
                research_rules->all_succeeded() && (!custom_options || custom_options->all_succeeded())) {
                research_context = runtime::restore_research_context(*terrain_snapshot, database);
                if(research_context->profile==content::TechnologyProfile::vp542)
                    runtime::restore_vp_espionage_profile(*terrain_snapshot,*research_context);
                research_ai_weights=runtime::restore_research_ai_weights(
                    *terrain_snapshot,*research_context,*technologies,*info_types);
                if(inspect_lua_map)
                    lua_map_report=validation::lua_map_report(*terrain_snapshot,database,*research_context,*team_technologies,*info_types,catalog,profile,custom_options?&*custom_options:nullptr);
                if(inspect_lua_pregame)
                    lua_pregame_report=validation::lua_pregame_report(*terrain_snapshot->pregame,database,*research_context,*team_technologies,*info_types,catalog,profile);
                if (inspect_lua_save_data)
                    lua_save_data_report=validation::lua_save_data_report(terrain_snapshot->saved_database,database,*research_context,*team_technologies,*info_types,catalog,profile,lua_save_data_library_mod,
                        {&loaded,&context,candidate.path,lua_save_data_output,save_identity.sha256});
                if (inspect_lua_gameinfo)
                    lua_gameinfo_report=validation::lua_gameinfo_report(database,*research_context,*team_technologies,*info_types,catalog,profile);
                if (inspect_lua_city_bombard)
                    lua_city_bombard_report=validation::lua_city_bombard_report(database,*research_context,*team_technologies,catalog,profile,*info_types);
                player_research = runtime::restore_player_research_context(*terrain_snapshot,*research_context,*global_defines);
                if(loaded.live_team_research){
                    if(!research_checkpoint_identity)throw std::runtime_error("Missing research checkpoint identity");
                    std::optional<saves::CivisSaveSection> vp_profile;
                    if(vp_research_profile)vp_profile=saves::CivisSaveSection{
                        static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_research_profile),0,
                        *vp_research_profile};
                    runtime::restore_research_checkpoint(
                        {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research),0,
                            *loaded.live_team_research},vp_profile,*team_technologies,*player_research,
                        *research_context,*technologies,*research_checkpoint_identity);
                }
                if (inspect_city_yields) {
                    auto rules = runtime::load_city_yield_rules(database, research_context->profile);
                    auto yield_setup = *research_context;
                    auto state = runtime::restore_city_yields(*terrain_snapshot, yield_setup, database, rules, *info_types);
                    if(yield_setup.profile==content::TechnologyProfile::vp542){
                        runtime::restore_vp_city_espionage(*terrain_snapshot,state);
                        if(vp_espionage_profile){
                            if(!research_checkpoint_identity)
                                throw std::runtime_error("Missing VP espionage checkpoint identity");
                            runtime::restore_vp_espionage_checkpoint(
                                {static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_espionage_profile),0,
                                    *vp_espionage_profile},yield_setup,state,*research_checkpoint_identity);
                        }
                        if(!session_checkpoint_output.empty()){
                            if(!research_checkpoint_identity)
                                throw std::runtime_error("Missing VP espionage checkpoint identity");
                            vp_espionage_checkpoint_section=runtime::make_vp_espionage_checkpoint(
                                yield_setup,state,*research_checkpoint_identity);
                        }
                    }
                    plot_water_report=validation::plot_water_report(*terrain_snapshot,database,*info_types,yield_setup,state,
                        *global_defines,custom_options?&*custom_options:nullptr);
                    improvement_yield_report=validation::improvement_yield_report(*terrain_snapshot->map_projection,database,*info_types,yield_setup.profile);
                    resource_visibility_report=validation::resource_visibility_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,*global_defines);
                    plot_movement_report=validation::plot_movement_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*global_defines);
                    if (yield_setup.profile == content::TechnologyProfile::bnw) {
                        const auto unhappiness_rules = runtime::load_bnw_unhappiness_rules(database);
                        const auto unhappiness = runtime::restore_bnw_unhappiness(*terrain_snapshot, yield_setup, state, unhappiness_rules, *info_types);
                        const auto quotes = runtime::bind_bnw_unhappiness(unhappiness, yield_setup, state, unhappiness_rules, *global_defines);
                        bnw_unhappiness_report = validation::bnw_unhappiness_report(unhappiness, yield_setup, unhappiness_rules, *global_defines, quotes);
                    }
                    city_yields_report = validation::city_yields_report(state, rules, *global_defines);
                    const runtime::MaintenanceWorld maintenance_world{yield_setup,*player_research,state,*team_technologies};
                    const auto maintenance_rules=runtime::load_maintenance_rules(database,yield_setup.profile,*info_types);
                    auto maintenance=runtime::restore_maintenance(*terrain_snapshot,maintenance_world,maintenance_rules,*info_types);
                    improvement_calculation_report=validation::improvement_calculation_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,maintenance.team_eras,*global_defines,custom_options?&*custom_options:nullptr);
                    bnw_nature_yield_report=validation::bnw_nature_yield_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,*global_defines);
                    vp_nature_yield_report=validation::vp_nature_yield_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,maintenance,maintenance_rules,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_plot_religion_report=validation::vp_plot_religion_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_player_plot_yield_report=validation::vp_player_plot_yield_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,maintenance,*global_defines,custom_options?&*custom_options:nullptr,maintenance.team_eras);
                    vp_plot_yield_report=validation::vp_plot_yield_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,maintenance,maintenance_rules,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_food_report=validation::vp_city_food_report(*terrain_snapshot,database,state,yield_setup,maintenance.team_eras,*global_defines);
                    route_economy_report=validation::route_economy_report(*terrain_snapshot,database,*info_types,maintenance,
                        custom_options?&*custom_options:nullptr,terrain_snapshot->pregame->archive.integers.at("active_player"));
                    maintenance_report=validation::maintenance_report(maintenance,maintenance_world,maintenance_rules,
                        *global_defines,custom_options?&*custom_options:nullptr);
                    const auto treasury_rules=runtime::load_treasury_rules(database,yield_setup.profile,*info_types);
                    auto treasury=runtime::restore_treasury(*terrain_snapshot,maintenance_world,treasury_rules,*info_types);
                    if(loaded.live_treasury){
                        if(!research_checkpoint_identity)throw std::runtime_error("Missing treasury checkpoint identity");
                        runtime::restore_treasury_checkpoint(
                            {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_treasury),0,*loaded.live_treasury},
                            treasury,yield_setup,*research_checkpoint_identity);
                    }
                    treasury_report=validation::treasury_report(treasury,maintenance_world,maintenance,treasury_rules,
                        rules,maintenance_rules,*global_defines,custom_options?&*custom_options:nullptr);
                    if(!session_checkpoint_output.empty()){
                        if(!research_checkpoint_identity)throw std::runtime_error("Missing treasury checkpoint identity");
                        treasury_checkpoint_section=runtime::make_treasury_checkpoint(
                            treasury,yield_setup,*research_checkpoint_identity);
                    }
                    auto science=runtime::restore_science(*terrain_snapshot,yield_setup,maintenance_world,database);
                    if(vp_science_profile){
                        if(!research_checkpoint_identity)throw std::runtime_error("Missing VP science checkpoint identity");
                        runtime::restore_vp_science_checkpoint(
                            {static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_science_profile),0,
                                *vp_science_profile},science,yield_setup,*research_checkpoint_identity);
                    }
                    if(!session_checkpoint_output.empty()&&science.profile==content::TechnologyProfile::vp542){
                        if(!research_checkpoint_identity)throw std::runtime_error("Missing VP science checkpoint identity");
                        vp_science_checkpoint_section=runtime::make_vp_science_checkpoint(
                            science,yield_setup,*research_checkpoint_identity);
                    }
                    vp_city_growth_report=validation::vp_city_growth_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,science,state,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_unit_supply_report=validation::vp_unit_supply_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,maintenance,science,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_great_works_report=validation::vp_great_works_report(*terrain_snapshot,database,*info_types,yield_setup,state,rules,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_building_yields_report=validation::vp_city_building_yields_report(*terrain_snapshot,database,*info_types,yield_setup,state,rules,maintenance.team_eras,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_religion_minor_yields_report=validation::vp_city_religion_minor_yields_report(*terrain_snapshot,database,*info_types,yield_setup,state,treasury,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_trait_yields_report=validation::vp_city_trait_yields_report(*terrain_snapshot,database,*info_types,yield_setup,state,treasury,maintenance.team_eras,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_happiness_report=validation::vp_city_happiness_report(*terrain_snapshot,database,*info_types,yield_setup,state,rules,maintenance,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_base_yields_report=validation::vp_city_base_yields_report(*terrain_snapshot,database,*info_types,yield_setup,state,rules,maintenance,treasury,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_corporations_report=validation::vp_corporations_report(*terrain_snapshot,database,*info_types,yield_setup,state,rules,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_production_report=validation::vp_city_production_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,maintenance,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_building_production_report=validation::vp_building_production_report(*terrain_snapshot,database,*info_types,yield_setup,*player_research,*team_technologies,state,rules,maintenance,*global_defines,custom_options?&*custom_options:nullptr);
                    auto research_turn=runtime::restore_research_turn(*terrain_snapshot,yield_setup,*technologies,*info_types,database);
                    if(loaded.live_research_turn){
                        if(!research_checkpoint_identity)throw std::runtime_error("Missing research-turn checkpoint identity");
                        runtime::restore_research_turn_checkpoint(
                            {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_research_turn),0,
                                *loaded.live_research_turn},research_turn,yield_setup,*technologies,
                            *research_checkpoint_identity);
                    }
                    if(!session_checkpoint_output.empty()){
                        if(!research_checkpoint_identity)throw std::runtime_error("Missing research-turn checkpoint identity");
                        research_turn_checkpoint_section=runtime::make_research_turn_checkpoint(
                            research_turn,yield_setup,*technologies,*research_checkpoint_identity);
                    }
                    vp_unit_traits_report=validation::vp_unit_traits_report(*terrain_snapshot,database,*info_types,yield_setup,research_turn,*player_research,*team_technologies,state,treasury,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_player_unit_production_report=validation::vp_player_unit_production_report(*terrain_snapshot,database,*info_types,yield_setup,research_turn,*player_research,*team_technologies,state,maintenance,treasury,*global_defines,custom_options?&*custom_options:nullptr);
                    vp_city_unit_production_report=validation::vp_city_unit_production_report(*terrain_snapshot,database,*info_types,yield_setup,research_turn,*player_research,*team_technologies,state,rules,maintenance,science,treasury,*global_defines,custom_options?&*custom_options:nullptr);
                    const auto progress_rules=runtime::load_production_progress_rules(database,state.profile);
                    runtime::restore_production_progress(*terrain_snapshot,progress_rules,*info_types,state);
                    if(loaded.live_city_production)runtime::restore_production_checkpoint(
                        {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_city_production),0,*loaded.live_city_production},
                        progress_rules,*info_types,state);
                    city_production_progress_report=validation::production_progress_report(
                        state,progress_rules,loaded.live_city_production.has_value());
                    if(!checkpoint_output.empty())production_checkpoint_section=
                        runtime::make_production_checkpoint(state,progress_rules,*info_types);
                    const auto research_selection=runtime::restore_research_selection(*terrain_snapshot,yield_setup,database);
                    research_selection_report=validation::research_selection_report(research_selection,research_turn,*player_research,yield_setup,
                        *technologies,custom_options?&*custom_options:nullptr,*team_technologies,maintenance.team_eras,
                        runtime::research_prerequisite_limits(*global_defines),science.no_science);
                    research_turn_report=validation::research_turn_report(research_turn,*player_research,yield_setup,*research_rules,*technologies,
                        *global_defines,custom_options?&*custom_options:nullptr,*team_technologies,science,state,treasury,maintenance,
                        treasury_rules,rules,maintenance_rules,terrain_snapshot->pregame->archive.integers.at("active_player"));
                    treasury_turn_report=validation::treasury_turn_report(treasury,maintenance_world,maintenance,treasury_rules,
                        rules,maintenance_rules,*global_defines,custom_options?&*custom_options:nullptr,
                        terrain_snapshot->pregame->archive.integers.at("active_player"));
                    player_economy_research_stage_report=validation::player_economy_research_stage_report(
                        *terrain_snapshot,database,research_turn,*player_research,yield_setup,*research_rules,*technologies,*global_defines,
                        custom_options?&*custom_options:nullptr,*team_technologies,science,state,treasury,maintenance,
                        treasury_rules,rules,maintenance_rules,
                        info_types->find("YIELD_CULTURE")?info_types->find("YIELD_CULTURE")->id:-1,
                        terrain_snapshot->pregame->archive.integers.at("active_player"));
                    science_report=validation::science_report(science,maintenance_world,treasury,std::move(maintenance),treasury_rules,
                        rules,maintenance_rules,*global_defines,custom_options?&*custom_options:nullptr);
                }
                for (const auto &team : *team_technologies) {
                    auto binding = runtime::bind_team_research(*research_context, *research_rules, *global_defines,
                        custom_options ? &*custom_options : nullptr, team);
                    research_fully_resolved = research_fully_resolved && binding.missing_infos.empty();
                    research_bindings.emplace(team.team(), std::move(binding));
                }
                for (const auto &[id, player] : player_research->players) {
                    player_research_before[id] = {player.max_effective_cities,
                        research_context->profile==content::TechnologyProfile::vp542?
                            runtime::vp_player_research_profile(player).science_from_league:0};
                    for (auto queued : player.queue)
                        if (queued < 0 || !technologies->find(static_cast<std::uint64_t>(queued))) player_research_resolved = false;
                    auto &quotes = player_quotes[id];
                    for (const auto &[slot,info] : technologies->entries) {
                        if (slot > INT64_MAX) throw std::overflow_error("Player research technology exceeds int64");
                        auto q = runtime::player_research_quote(*player_research,*research_context,*research_rules,*technologies,
                            *global_defines,custom_options ? &*custom_options : nullptr,*team_technologies,id,static_cast<std::int64_t>(slot));
                        player_research_resolved = player_research_resolved && q.modifier.missing_infos.empty() && q.team.missing_infos.empty();
                        quotes.emplace_back(static_cast<std::int64_t>(slot),std::move(q));
                    }
                }
                if (research_probe_input)
                    research_probe_report = validation::research_probe(*research_probe_input,*terrain_snapshot,*research_context,
                        database,*research_rules,*technologies,*global_defines,custom_options?&*custom_options:nullptr,*team_technologies);
                if(!session_checkpoint_output.empty()){
                    if(!research_checkpoint_identity)throw std::runtime_error("Missing research checkpoint identity");
                    research_checkpoint_sections=runtime::make_research_checkpoint(
                        *team_technologies,*player_research,*research_context,*technologies,
                        *research_checkpoint_identity);
                }
            }
            if (!session_layout.empty())
                session_context = context;
            if (terrain_grid_mode) {
                const auto &source = *terrain_snapshot->map_terrain;
                if (source.grid.width > INT32_MAX || source.grid.height > INT32_MAX)
                    throw std::runtime_error("Source dimensions exceed native coordinate range");
                if (*terrain_grid_mode == -1) {
                    terrain_grid_mode = terrain::game_world_grid_mode(source.wrap_x);
                    terrain_mode_from_save = true;
                    saved_scene_sources = scene::build_saved_scene_sources(*terrain_snapshot, 0, {},
                        [](float value) { return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(value))); });
                    terrain_grid = saved_scene_sources->terrain_grid;
                    terrain_auxiliary = saved_scene_sources->auxiliary;
                    terrain_rivers = saved_scene_sources->river_edges;
                    terrain_river_network = saved_scene_sources->river_network;
                } else {
                    const auto grids = terrain::build_world_grid_layout(static_cast<std::int32_t>(source.grid.width),
                        static_cast<std::int32_t>(source.grid.height), *terrain_grid_mode, {});
                    std::vector<terrain::TerrainSourceCell> classified;
                    std::vector<terrain::TerrainRiverPlot> rivers;
                    std::vector<terrain::TerrainAuxiliaryPlot> auxiliary;
                    classified.reserve(source.cells.size());
                    rivers.reserve(source.cells.size());
                    auxiliary.reserve(source.cells.size());
                    for (const auto &cell : source.cells) {
                        if (!cell.native_terrain_mask_known || !cell.native_category)
                            throw std::runtime_error("Unresolved terrain cannot populate the native source grid");
                        classified.push_back({cell.native_terrain_mask, *cell.native_category});
                        if (!cell.rivers)
                            throw std::runtime_error("Unresolved river source plot");
                        const auto &r = *cell.rivers;
                        rivers.push_back({r.is_river, r.west, r.northwest, r.northeast,
                                          r.flow_e, r.flow_se, r.flow_sw});
                        if (!cell.auxiliary)
                            throw std::runtime_error("Unresolved auxiliary source plot");
                        const auto &a = *cell.auxiliary;
                        auxiliary.push_back({a.terrain, a.plot, a.continent, a.feature});
                    }
                    // Reserved bytes have explicit host-owned zero storage. They
                    // do not claim to reproduce an uninitialized original stack word.
                    terrain_grid = terrain::build_terrain_source_grid(grids, classified, 0);
                    terrain_auxiliary = terrain::build_terrain_auxiliary_sources(grids.source_grid, auxiliary);
                    terrain_rivers = terrain::build_terrain_river_edges(grids.source_grid, rivers);
                    terrain_river_network = terrain::RiverNetwork::build(grids.source_grid.width,
                        grids.source_grid.height, terrain_rivers);
                }
            }
        }
        if (runtime::identify_file(candidate.path).sha256 != save_identity.sha256)
            throw std::runtime_error("Save changed while applying rules");
        const auto write_new = [](const fs::path &path, std::span<const std::uint8_t> bytes) {
            const auto output = _wfopen(path.c_str(), L"wbx");
            if (!output) throw std::runtime_error("Cannot create new terrain source output");
            const auto written = std::fwrite(bytes.data(), 1, bytes.size(), output);
            const auto closed = std::fclose(output);
            if (written != bytes.size() || closed != 0)
                throw std::runtime_error("Cannot write terrain source output");
        };
        if (!terrain_grid_output.empty()) write_new(terrain_grid_output, terrain_grid->cells);
        if (!terrain_rivers_output.empty()) {
            std::vector<std::uint8_t> bytes;
            bytes.reserve(terrain_rivers.size()*16);
            for (const auto &edge : terrain_rivers)
                for (const auto v : {edge.x, edge.y, edge.z, edge.direction})
                    for (unsigned b = 0; b < 4; ++b)
                        bytes.push_back(static_cast<std::uint8_t>(static_cast<std::uint32_t>(v) >> (b*8)));
            write_new(terrain_rivers_output, bytes);
        }
        if (!terrain_relief_output.empty()) {
            std::vector<std::uint8_t> bytes;
            bytes.reserve(terrain_auxiliary->relief.size() * 5);
            for (const auto &source : terrain_auxiliary->relief) {
                for (const auto value : {static_cast<std::uint16_t>(source.x),
                                         static_cast<std::uint16_t>(source.y)}) {
                    bytes.push_back(static_cast<std::uint8_t>(value));
                    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
                }
                bytes.push_back(source.kind);
            }
            write_new(terrain_relief_output, bytes);
        }
        if (!terrain_feature2_output.empty()) {
            std::vector<std::uint8_t> bytes;
            bytes.reserve(terrain_auxiliary->feature2.size() * 20);
            for (const auto &source : terrain_auxiliary->feature2)
                for (const auto value : {source.position.x, source.position.y, source.position.z,
                                         source.terrain, source.continent})
                    for (unsigned b = 0; b < 4; ++b)
                        bytes.push_back(static_cast<std::uint8_t>(static_cast<std::uint32_t>(value) >> (b * 8)));
            write_new(terrain_feature2_output, bytes);
        }
        std::uint64_t ready_actions = 0, failed_actions = 0;
        std::vector<const content::DatabaseActivationResult *> failure_samples;
        std::uint64_t manifest_md5_mismatches = 0;
        std::vector<const content::DatabaseActivationResult *> manifest_md5_mismatch_samples;
        for (const auto &action : report.mod_plan.actions) {
            if (action.status == content::DatabaseActivationStatus::ready)
                ++ready_actions;
        }
        for (const auto &action : report.mods.actions) {
            failed_actions += action.applied ? 0 : 1;
            if (!action.applied && failure_samples.size() < 8)
                failure_samples.push_back(&action);
            if (!action.manifest_md5_match){
                ++manifest_md5_mismatches;
                if(manifest_md5_mismatch_samples.size()<8)manifest_md5_mismatch_samples.push_back(&action);
            }
        }
        const auto dlc_requested = sum_dlc(report.dlc, &content::DlcGameplayPackageReport::requested);
        const auto dlc_resolved = sum_dlc(report.dlc, &content::DlcGameplayPackageReport::resolved);
        const auto dlc_missing = sum_dlc(report.dlc, &content::DlcGameplayPackageReport::missing);
        const auto dlc_failed = sum_dlc(report.dlc, &content::DlcGameplayPackageReport::failed);
        std::string dlc_missing_samples = "[";
        for (const auto &package : report.dlc.packages)
            for (const auto &request : package.missing_requests) {
                if (dlc_missing_samples.size() != 1) dlc_missing_samples += ',';
                dlc_missing_samples += "{\"package\":" + json(package.id) +
                                       ",\"request\":" + json(request) + '}';
            }
        dlc_missing_samples += ']';
        std::uint64_t localization_failed = report.localization.base_failed;
        for (const auto &package : report.localization.packages)
            localization_failed += package.failed;
        if(!lua_registry_plan.empty()&&terrain_snapshot&&research_context&&team_technologies&&info_types){
            const auto omissions=installer_profile?
                content::vp_lua_import_omissions(*installer_profile,catalog,profile):
                std::vector<content::LuaImportOmission>{};
            lua_selected_entrypoints_report=validation::lua_selected_entrypoints_report(
                lua_registry_plan,lua_entrypoint_type,root,mounts.resources,catalog,profile,database,
                *terrain_snapshot,*research_context,*team_technologies,*info_types,
                custom_options?&*custom_options:nullptr,omissions,installed_byte_packages,lua_ui_notifications,
                lua_user_data_root,lua_language,saved_value_fallback_report,verified_archive_hashes);
        }
        if(!lua_registry_plan.empty()&&!lua_selected_entrypoints_report)
            throw std::runtime_error("Selected Lua entrypoint inputs could not be resolved");
        if (research_probe_input && !research_probe_report) throw std::runtime_error("Research probe inputs could not be resolved");
        if (inspect_lua_city_bombard && !lua_city_bombard_report)throw std::runtime_error("Lua gameplay inputs could not be resolved");
        if (inspect_lua_gameinfo && !lua_gameinfo_report)throw std::runtime_error("Lua GameInfo inputs could not be resolved");
        if (inspect_lua_pregame && !lua_pregame_report)throw std::runtime_error("Lua pregame inputs could not be resolved");
        if (inspect_lua_map && !lua_map_report)throw std::runtime_error("Lua map inputs could not be resolved");
        if (inspect_lua_save_data && !lua_save_data_report)throw std::runtime_error("Lua saved database inputs could not be resolved");
        if (inspect_city_yields && !city_yields_report) throw std::runtime_error("City yield inputs could not be resolved");
        const bool rules_ready = report.rules_database_ready && (!postprocessing || postprocessing->all_succeeded)
                                 && (!info_types || info_types->report().all_succeeded())
                                 && (!global_defines || global_defines->errors() == 0)
                                 && (!custom_options || custom_options->all_succeeded())
                                 && (!research_rules || research_rules->all_succeeded()) && research_fully_resolved
                                 && player_research_resolved
                                 && (!technologies || technologies->all_succeeded()) && team_technologies_mapped;
        if(!checkpoint_output.empty()&&!production_checkpoint_section)
            throw std::runtime_error("City production checkpoint inputs could not be resolved");
        if(!session_checkpoint_output.empty()&&!research_checkpoint_sections)
            throw std::runtime_error("Research checkpoint inputs could not be resolved");
        if(!session_checkpoint_output.empty()&&!treasury_checkpoint_section)
            throw std::runtime_error("Treasury checkpoint inputs could not be resolved");
        if(!session_checkpoint_output.empty()&&!research_turn_checkpoint_section)
            throw std::runtime_error("Research-turn checkpoint inputs could not be resolved");
        if(!session_checkpoint_output.empty()&&!random_checkpoint_section)
            throw std::runtime_error("Simulation RNG checkpoint inputs could not be resolved");
        if(!session_checkpoint_output.empty()&&decode_layout=="vp542"&&!vp_science_checkpoint_section)
            throw std::runtime_error("VP science checkpoint inputs could not be resolved");
        if(!session_checkpoint_output.empty()&&decode_layout=="vp542"&&!vp_espionage_checkpoint_section)
            throw std::runtime_error("VP espionage checkpoint inputs could not be resolved");
        if(production_checkpoint_section){
            auto document=saved_value_fallback_report?
                saves::make_civis_import_document(loaded,save_identity.sha256,
                    std::span<const std::uint8_t>(terrain_snapshot->saved_database.image)):
                saves::make_civis_import_document(loaded,save_identity.sha256);
            const auto upsert=[&](const saves::CivisSaveSection &section){
                const auto old=std::find_if(document.sections.begin(),document.sections.end(),
                    [&](const auto &s){return s.id==section.id;});
                if(old==document.sections.end())document.sections.push_back(section);
                else *old=section;
            };
            upsert(*production_checkpoint_section);
            if(research_checkpoint_sections){
                upsert(research_checkpoint_sections->common);
                if(research_checkpoint_sections->profile)upsert(*research_checkpoint_sections->profile);
            }
            if(treasury_checkpoint_section)upsert(*treasury_checkpoint_section);
            if(research_turn_checkpoint_section)upsert(*research_turn_checkpoint_section);
            if(random_checkpoint_section)upsert(*random_checkpoint_section);
            if(vp_science_checkpoint_section)upsert(*vp_science_checkpoint_section);
            if(vp_espionage_checkpoint_section)upsert(*vp_espionage_checkpoint_section);
            saves::write_civis_save(checkpoint_output,document);
            const auto verified=saves::read_loaded_save(checkpoint_output);
            if(verified.live_city_production!=std::optional{production_checkpoint_section->bytes}||
               verified.bytes!=loaded.bytes||verified.header!=loaded.header)
                throw std::runtime_error("Production checkpoint verification failed");
            if(saved_value_fallback_report&&
               verified.saved_database_override!=terrain_snapshot->saved_database.image)
                throw std::runtime_error("Production checkpoint lost repaired mod settings");
            if(research_checkpoint_sections&&
               (verified.live_team_research!=std::optional{research_checkpoint_sections->common.bytes}||
                ([&]{const auto *bytes=saves::profile_section(verified.profile_sections,
                    saves::CivisSaveSectionId::vp_research_profile);
                    return research_checkpoint_sections->profile?
                        !bytes||*bytes!=research_checkpoint_sections->profile->bytes:bytes!=nullptr;}())))
                throw std::runtime_error("Research checkpoint verification failed");
            if(treasury_checkpoint_section&&
               verified.live_treasury!=std::optional{treasury_checkpoint_section->bytes})
                throw std::runtime_error("Treasury checkpoint verification failed");
            if(research_turn_checkpoint_section&&
               verified.live_research_turn!=std::optional{research_turn_checkpoint_section->bytes})
                throw std::runtime_error("Research-turn checkpoint verification failed");
            if(random_checkpoint_section&&
               verified.live_simulation_random!=std::optional{random_checkpoint_section->bytes})
                throw std::runtime_error("Simulation RNG checkpoint verification failed");
            if(vp_science_checkpoint_section&&
               ([&]{const auto *bytes=saves::profile_section(verified.profile_sections,
                    saves::CivisSaveSectionId::vp_science_profile);
                    return !bytes||*bytes!=vp_science_checkpoint_section->bytes;}()))
                throw std::runtime_error("VP science checkpoint verification failed");
            if(vp_espionage_checkpoint_section&&
               ([&]{const auto *bytes=saves::profile_section(verified.profile_sections,
                    saves::CivisSaveSectionId::vp_espionage_profile);
                    return !bytes||*bytes!=vp_espionage_checkpoint_section->bytes;}()))
                throw std::runtime_error("VP espionage checkpoint verification failed");
        }
        std::cout << "{\"schema\":1,\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"operation\":\"saved_rules\",\"can_continue\":false"
                  << ",\"production_checkpoint_output\":"
                  << (production_checkpoint_output.empty()?std::string("null"):
                      json(assets::path_utf8(fs::absolute(production_checkpoint_output))))
                  << ",\"session_checkpoint_output\":"
                  << (session_checkpoint_output.empty()?std::string("null"):
                      json(assets::path_utf8(fs::absolute(session_checkpoint_output))))
                  << ",\"research_checkpoint_loaded\":"
                  << (loaded.live_team_research?"true":"false")
                  << ",\"treasury_checkpoint_loaded\":"
                  << (loaded.live_treasury?"true":"false")
                  << ",\"research_turn_checkpoint_loaded\":"
                  << (loaded.live_research_turn?"true":"false")
                  << ",\"random_checkpoint_loaded\":"
                  << (loaded.live_simulation_random?"true":"false")
                  << ",\"vp_science_checkpoint_loaded\":"
                  << (vp_science_profile?"true":"false")
                  << ",\"save\":\"" << json(assets::path_utf8(candidate.path)).substr(1)
                  << ",\"save_sha256\":" << json(save_identity.sha256)
                  << ",\"game_root\":" << json(assets::path_utf8(root))
                  << ",\"steam_discovered\":" << (steam_discovered ? "true" : "false")
                  << ",\"mount_plan\":"
                  << (mount_plan.empty() ? std::string("null") : json(assets::path_utf8(mount_plan)))
                  << ",\"mount_plan_sha256\":"
                  << (mount_identity ? json(mount_identity->sha256) : std::string("null"))
                  << ",\"mod_cache\":"
                  << (mod_cache.empty() ? std::string("null") : json(assets::path_utf8(fs::canonical(mod_cache))))
                  << ",\"verified_cache_mirrors\":" << verified_cache_mirrors
                  << ",\"explicit_mod_sources\":" << explicit_mod_sources.size()
                  << ",\"verified_archive_md5_exceptions\":" << verified_archive_hashes.size()
                  << ",\"verified_archive_asset_omissions\":" << verified_archive_omissions.size()
                  << ",\"use_installed_mod_bytes\":" << (use_installed_mod_bytes?"true":"false")
                  << ",\"installed_byte_packages\":" << installed_byte_packages.size()
                  << ",\"selected_cache_resource_files\":" << selected_cache_resource_files
                  << ",\"vp_installer_profile\":";
        if(installer_profile)
            std::cout << "{\"component\":" << json(vp_component_name)
                      << ",\"view\":" << json(assets::path_utf8(vp_view))
                      << ",\"selected_mods\":" << vp_view_selected_mods
                      << ",\"resource_files\":" << vp_view_resource_files
                      << ",\"catalog_overlays\":" << vp_catalog_overlays << '}';
        else std::cout << "null";
        std::cout
                  << ",\"acquisition\":";
        if (!acquisition) {
            std::cout << "null";
        } else {
            std::cout << "{\"performed\":true,\"acquired\":" << acquisition->acquired
                      << ",\"reused\":" << acquisition->reused
                      << ",\"retained_installer_bundles\":"
                      << acquisition->retained_installer_bundles << ",\"all_acquired\":"
                      << (acquisition->all_acquired ? "true" : "false") << ",\"diagnostics\":[";
            for (std::size_t i = 0; i < acquisition->diagnostics.size(); ++i) {
                if (i)
                    std::cout << ',';
                std::cout << json(acquisition->diagnostics[i]);
            }
            std::cout << "]}";
        }
        std::cout
                  << ",\"profile\":{\"kind\":"
                  << json(runtime::content_profile_kind_name(profile.kind))
                  << ",\"requirements\":" << profile.requirements.size()
                  << ",\"enabled_dlc\":" << profile.enabled_dlc.size()
                  << ",\"enabled_mods\":" << profile.enabled_mods.size()
                  << ",\"all_requirements_located\":"
                  << (profile.all_requirements_located ? "true" : "false")
                  << ",\"files_complete\":" << (profile.files_complete ? "true" : "false")
                  << ",\"native_mods_require_port\":"
                  << (profile.native_mods_require_port ? "true" : "false")
                  << ",\"can_activate\":" << (profile.can_activate ? "true" : "false") << "}"
                  << ",\"payload\":{\"format\":"
                  << json(saves::save_container_kind_name(loaded.container))
                  << ",\"compressed_bytes\":"
                  << (civis_save ? 0 : loaded.inflate.compressed_bytes)
                  << ",\"inflated_bytes\":" << loaded.bytes.size()
                  << ",\"stream_end\":"
                  << (civis_save ? "true" : (loaded.inflate.stream_end ? "true" : "false"))
                  << ",\"sync_flush_suffix\":"
                  << (civis_save ? "false" : (loaded.inflate.sync_flush_suffix ? "true" : "false"))
                  << "}"
                  << ",\"source_prefix\":{\"retained\":" << (loaded.source_prefix ? "true" : "false")
                  << ",\"bytes\":" << (loaded.source_prefix ? std::to_string(loaded.source_prefix->size()) : "null")
                  << ",\"setup_bytes\":" << (loaded.source_prefix ? std::to_string(loaded.source_prefix->size() - loaded.header.end_offset) : "null")
                  << ",\"sha256\":" << (loaded.source_prefix ? json(runtime::identify_bytes(*loaded.source_prefix).sha256) : "null")
                  << ",\"setup_decoded\":" << (terrain_snapshot && terrain_snapshot->pregame ? "true" : "false") << "}"
                  << ",\"pregame\":" << pregame_summary(terrain_snapshot && terrain_snapshot->pregame ? &*terrain_snapshot->pregame : nullptr)
                  << ",\"localization\":{\"base_schema_applied\":"
                  << (report.localization.base_schema_applied ? "true" : "false")
                  << ",\"base_requested\":" << report.localization.base_requested
                  << ",\"base_resolved\":" << report.localization.base_resolved
                  << ",\"base_applied\":" << report.localization.base_applied
                  << ",\"failed\":" << localization_failed
                  << ",\"dlc_packages\":" << report.localization.packages.size()
                  << ",\"attached\":" << (report.localization.attached ? "true" : "false") << "}"
                  << ",\"base\":{\"requests\":" << report.base.sources.size()
                  << ",\"selected\":" << report.base.selected << ",\"missing\":"
                  << report.base.missing << ",\"applied\":" << report.base.applied
                  << ",\"failed\":" << report.base.failed << ",\"schema_applied\":"
                  << (report.base.schema_applied ? "true" : "false") << "}"
                  << ",\"dlc\":{\"packages\":" << report.dlc.packages.size()
                  << ",\"requested\":" << dlc_requested << ",\"resolved\":" << dlc_resolved
                  << ",\"missing\":" << dlc_missing << ",\"failed\":" << dlc_failed
                  << ",\"missing_samples\":" << dlc_missing_samples << "}"
                  << ",\"mods\":{\"actions\":" << report.mod_plan.actions.size()
                  << ",\"ready\":" << ready_actions << ",\"failed\":" << failed_actions
                  << ",\"manifest_md5_mismatches\":" << manifest_md5_mismatches
                  << ",\"all_sources_present\":"
                  << (report.mod_plan.all_sources_present ? "true" : "false")
                  << ",\"all_succeeded\":" << (report.mods.all_succeeded ? "true" : "false")
                  << ",\"action_order\":[";
        for (std::size_t i=0;i<report.mod_plan.ordered_mods.size();++i) {
            if(i)std::cout<<',';
            const auto &mod=catalog.packages.at(report.mod_plan.ordered_mods[i]);
            std::cout<<'['<<json(mod.id)<<','<<mod.version<<']';
        }
        std::cout << "],\"failure_samples\":[";
        for (std::size_t i = 0; i < failure_samples.size(); ++i) {
            if (i)
                std::cout << ',';
            const auto &failure = *failure_samples[i];
            std::cout << "{\"mod\":" << json(failure.action.mod_id)
                      << ",\"source\":" << json(failure.action.requested_path)
                      << ",\"error\":" << json(failure.error) << '}';
        }
        std::cout << "],\"manifest_md5_mismatch_samples\":[";
        for(std::size_t i=0;i<manifest_md5_mismatch_samples.size();++i){
            if(i)std::cout<<',';
            const auto &mismatch=*manifest_md5_mismatch_samples[i];
            std::cout << "{\"mod\":" << json(mismatch.action.mod_id)
                      << ",\"source\":" << json(mismatch.action.requested_path)
                      << ",\"applied\":" << (mismatch.applied?"true":"false") << '}';
        }
        std::cout << "]}";
        if (postprocessing) {
            std::uint64_t resolved = 0, applied = 0, errors = postprocessing->errors.size();
            for (const auto &entry : postprocessing->post_defines) {
                resolved += entry.rowid.has_value() ? 1 : 0;
                applied += entry.applied ? 1 : 0;
                errors += entry.errors.size();
            }
            std::cout << ",\"postprocessing\":{\"post_defines\":" << postprocessing->post_defines.size()
                      << ",\"resolved\":" << resolved << ",\"applied\":" << applied
                      << ",\"errors\":" << errors
                      << ",\"strategic_resources_applied\":" << (postprocessing->strategic_resources_applied ? "true" : "false")
                      << ",\"luxury_resources_applied\":" << (postprocessing->luxury_resources_applied ? "true" : "false")
                      << ",\"runtime_defines_applied\":" << postprocessing->runtime_defines_applied
                      << ",\"transaction_released\":" << (postprocessing->transaction_released ? "true" : "false")
                      << ",\"all_succeeded\":" << (postprocessing->all_succeeded ? "true" : "false")
                      << ",\"unresolved_samples\":[";
            std::size_t samples = 0;
            for (const auto &entry : postprocessing->post_defines) {
                if (entry.rowid || !entry.errors.empty() || samples == 16) continue;
                if (samples++) std::cout << ',';
                std::cout << "{\"name\":" << json(entry.name) << ",\"key\":" << json(entry.key)
                          << ",\"table\":" << json(entry.table) << '}';
            }
            std::cout << "],\"error_samples\":[";
            samples = 0;
            const auto show_error = [&](const content::GameplayPostprocessError &error,
                                         const std::string &name) {
                if (samples == 8) return;
                if (samples++) std::cout << ',';
                std::cout << "{\"name\":" << json(name) << ",\"stage\":" << json(error.stage)
                          << ",\"error\":" << json(error.message) << '}';
            };
            for (const auto &error : postprocessing->errors) show_error(error, {});
            for (const auto &entry : postprocessing->post_defines)
                for (const auto &error : entry.errors) show_error(error, entry.name);
            std::cout << "]}";
        }
        if (info_types) {
            const auto &types = info_types->report();
            std::cout << ",\"info_types\":{\"tables\":" << types.tables
                      << ",\"eligible_tables\":" << types.eligible_tables
                      << ",\"rows\":" << types.rows << ",\"null_types\":" << types.null_types
                      << ",\"names\":" << info_types->names() << ",\"hashes\":" << info_types->hashes()
                      << ",\"name_overwrites\":" << types.name_overwrites
                      << ",\"hash_collisions\":" << types.hash_collisions
                      << ",\"errors\":" << types.errors.size()
                      << ",\"all_succeeded\":" << (types.all_succeeded() ? "true" : "false")
                      << ",\"collision_samples\":[";
            bool first_collision = true;
            for (const auto &collision : types.collision_samples) {
                if (!first_collision) std::cout << ',';
                first_collision = false;
                std::cout << "{\"previous_type\":" << json(collision.previous.type)
                          << ",\"previous_table\":" << json(collision.previous.table)
                          << ",\"previous_id\":" << collision.previous.id
                          << ",\"type\":" << json(collision.replacement.type)
                          << ",\"table\":" << json(collision.replacement.table)
                          << ",\"id\":" << collision.replacement.id << '}';
            }
            std::cout << "]}";
        }
        if (global_defines) {
            std::cout << ",\"global_defines\":{\"cached\":" << global_defines->entries().size()
                      << ",\"missing\":" << global_defines->missing()
                      << ",\"errors\":" << global_defines->errors()
                      << ",\"missing_samples\":[";
            std::size_t shown = 0;
            for (const auto &entry : global_defines->entries()) {
                if (entry.lookup.found || !entry.lookup.error.empty() || shown == 16) continue;
                if (shown++) std::cout << ',';
                std::cout << json(entry.name);
            }
            std::cout << "]}";
        }
        if (custom_options) {
            const auto &options = *custom_options;
            std::cout << ",\"custom_mod_options\":{\"rows\":" << options.query.rows
                      << ",\"entries\":" << options.entries.size() << ",\"errors\":" << options.errors
                      << ",\"query_error\":" << json(options.query.error)
                      << ",\"prefetch_succeeded\":" << (options.prefetch.all_succeeded ? "true" : "false")
                      << ",\"rejected_updates\":" << options.rejected_updates
                      << ",\"all_succeeded\":" << (options.all_succeeded() ? "true" : "false")
                      << ",\"human_uses_ai_handicap\":" << (options.enabled("HUMAN_USES_AI_HANDICAP") ? "true" : "false")
                      << ",\"civ6_eurekas\":" << (options.enabled("CIV6_EUREKAS") ? "true" : "false") << '}';
        }
        if (research_rules) {
            std::cout << ",\"research_rules\":{\"all_succeeded\":" << (research_rules->all_succeeded() ? "true" : "false");
            const auto collection = [&](const char *name, const auto &infos, const auto &fields) {
                std::cout << ',' << json(name) << ":{\"slots\":" << infos.report.slots
                          << ",\"error\":" << json(infos.report.error) << ",\"entries\":[";
                bool first = true;
                for (const auto &[slot, info] : infos.entries) {
                    if (!first) std::cout << ',';
                    first = false;
                    std::cout << "{\"slot\":" << slot << ",\"authored_id\":" << info.base.authored_id
                              << ",\"percent\":" << info.percent;
                    fields(info);
                    std::cout << '}';
                }
                std::cout << "]}";
            };
            collection("handicaps", research_rules->handicaps, [&](const auto &info) {
                std::cout << ",\"per_era\":" << info.per_era_modifier << ",\"ai_percent\":" << info.ai_percent
                          << ",\"ai_per_era\":" << info.ai_per_era_modifier
                          << ",\"tech_catch_up\":" << info.tech_catch_up << ",\"ai_tech_catch_up\":" << info.ai_tech_catch_up;
            });
            collection("speeds", research_rules->speeds, [](const auto &) {});
            collection("eras", research_rules->eras, [](const auto &) {});
            std::cout << '}';
        }
        if (research_context) {
            const auto &context = *research_context;
            std::cout << ",\"research_context\":{\"fully_resolved\":" << (research_fully_resolved ? "true" : "false")
                      << ",\"game_type\":" << context.game_type << ",\"speed\":" << context.speed
                      << ",\"start_era\":" << context.start_era << ",\"current_era\":" << context.current_era
                      << ",\"game_handicap\":" << context.game_handicap << ",\"world_percent\":" << context.world_percent
                      << ",\"pitboss\":" << (context.pitboss ? (*context.pitboss ? "true" : "false") : "null")
                      << ",\"defines\":{";
            bool first = true;
            for (const auto *name : {"STANDARD_HANDICAP", "AI_HANDICAP", "TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER"}) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << json(name) << ':' << std::get<std::int64_t>(global_defines->find(name)->lookup.value);
            }
            std::cout << "},\"team_members\":[";
            first = true;
            for (const auto &[id, members] : context.team_members) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << '[' << id << ',' << members << ']';
            }
            std::cout << "],\"player_columns\":[\"id\",\"team\",\"handicap\",\"slot_status\",\"slot_claim\",\"alive\"],\"players\":[";
            first = true;
            for (const auto &[id, player] : context.players) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << '[' << id << ',' << player.team << ',' << player.handicap << ','
                          << player.slot_status << ',' << player.slot_claim << ',' << player.alive << ']';
            }
            std::cout << "]}";
        }
        if (technologies) {
            const auto &techs = *technologies;
            std::cout << ",\"technologies\":{\"entries\":" << techs.entries.size()
                      << ",\"slots\":" << techs.collection.slots << ",\"relation_rows\":" << techs.relation_rows
                      << ",\"errors\":" << techs.errors << ",\"collection_error\":" << json(techs.collection.error)
                      << ",\"localization_supplied\":" << (techs.localization_supplied ? "true" : "false")
                      << ",\"error_samples\":[";
            for (std::size_t i = 0; i < techs.error_samples.size(); ++i) {
                if (i) std::cout << ',';
                const auto &error = techs.error_samples[i];
                std::cout << "{\"slot\":" << error.slot << ",\"stage\":" << json(error.stage)
                          << ",\"message\":" << json(error.message) << ",\"sqlite_code\":" << error.sqlite_code << '}';
            }
            std::cout << "],\"or_prerequisites\":[";
            bool first_prereq = true;
            for (const auto &[slot, info] : techs.entries) {
                if (!first_prereq) std::cout << ',';
                first_prereq = false;
                std::cout << "{\"slot\":" << slot << ",\"extent\":" << info.or_prerequisites.extent
                          << ",\"default\":" << info.or_prerequisites.default_value << ",\"values\":[";
                bool first_value = true;
                for (const auto &[index, prerequisite] : info.or_prerequisites.values) {
                    if (!first_value) std::cout << ',';
                    first_value = false;
                    std::cout << '[' << index << ',' << prerequisite << ']';
                }
                std::cout << "]}";
            }
            std::cout << "]}";
        }
        if (team_technologies) {
            const auto limits = runtime::research_prerequisite_limits(*global_defines);
            std::cout << ",\"team_technologies\":{\"teams\":" << team_technologies->size()
                      << ",\"all_mapped\":" << (team_technologies_mapped ? "true" : "false")
                      << ",\"prerequisite_limits\":{\"and\":" << limits.and_count << ",\"or\":" << limits.or_count << '}'
                      << ",\"value_columns\":[\"slot\",\"has_tech\",\"no_trade\",\"progress_times100\",\"eureka\",\"count\"],\"states\":[";
            bool first_team = true;
            for (const auto &team : *team_technologies) {
                if (!first_team) std::cout << ',';
                first_team = false;
                const auto &r = team.remap();
                std::cout << "{\"team\":" << team.team() << ",\"slots\":" << team.slots()
                          << ",\"last_tech\":" << team.last_tech() << ",\"known_count\":" << team.known_count()
                          << ",\"all_researched\":" << (team.all_researched() ? "true" : "false")
                          << ",\"saved\":" << r.saved << ",\"mapped\":" << r.mapped
                          << ",\"sentinels\":" << r.sentinels << ",\"unknown\":" << r.unknown
                          << ",\"outside\":" << r.outside << ",\"duplicates\":" << r.duplicates
                          << ",\"holes\":" << r.holes << ",\"last_tech_outside\":" << (r.last_tech_outside ? "true" : "false")
                          << ",\"values\":[";
                bool first_value = true;
                for (const auto slot : team.occupied_indices()) {
                    if (!first_value) std::cout << ',';
                    first_value = false;
                    const auto value = team.at(static_cast<std::int64_t>(slot));
                    std::cout << '[' << slot << ',' << value.has_tech << ',' << value.no_trade << ','
                              << value.progress_times100 << ','
                              << (team.profile()==content::TechnologyProfile::vp542
                                  ? vp_team_eureka(team, static_cast<std::int64_t>(slot)) : 0)
                              << ',' << value.count << ']';
                }
                std::cout << "]";
                for (const bool trading : {false, true}) {
                    std::cout << (trading ? ",\"trade_prerequisite_candidates\":[" : ",\"research_prerequisite_candidates\":[");
                    bool first = true;
                    for (const auto &[slot, info] : technologies->entries) {
                        if (slot > INT64_MAX) throw std::overflow_error("Research technology ID exceeds int64");
                        const auto tech = static_cast<std::int64_t>(slot);
                        if (!team.has_tech(tech) && runtime::research_prerequisites_met(*technologies, team, tech, limits, trading)) {
                            if (!first) std::cout << ',';
                            first = false;
                            std::cout << tech;
                        }
                    }
                    std::cout << ']';
                }
                if (team.profile() == content::TechnologyProfile::vp542) {
                    std::cout << ",\"vp_ai_frontier\":[";
                    bool first = true;
                    for (const auto tech : runtime::vp_tech_frontier(*technologies, team, limits)) {
                        if (!first) std::cout << ',';
                        first = false;
                        std::cout << tech;
                    }
                    std::cout << ']';
                }
                if (research_context) {
                    const auto &binding = research_bindings.at(team.team());
                    const auto &f = binding.factors;
                    std::cout << ",\"research_factors\":{\"handicap_id\":" << binding.handicap_id
                              << ",\"alive_players\":" << binding.alive_players
                              << ",\"team_percent\":" << (f.team_handicap_percent ? std::to_string(*f.team_handicap_percent) : "null")
                              << ",\"members\":" << f.members << ",\"world_percent\":" << f.world_percent
                              << ",\"speed_percent\":" << f.speed_percent << ",\"start_era_percent\":" << f.start_era_percent
                              << ",\"extra_member_modifier\":" << f.extra_team_member_modifier;
                    if(team.profile()==content::TechnologyProfile::vp542){
                        const auto &vp=runtime::vp_research_cost_profile(f);
                        std::cout << ",\"team_per_era\":" << (f.team_handicap_percent ? std::to_string(vp.team_handicap_per_era_modifier) : "null")
                                  << ",\"ai_percent\":" << vp.game_ai_handicap_percent
                                  << ",\"ai_per_era\":" << vp.game_ai_per_era_modifier
                                  << ",\"human_for_handicap\":" << (vp.human_for_handicap ? "true" : "false")
                                  << ",\"eurekas_enabled\":" << (vp.eurekas_enabled ? "true" : "false")
                                  << ",\"current_era\":" << vp.current_era;
                    }
                    std::cout << ",\"missing_infos\":[";
                    for (std::size_t i = 0; i < binding.missing_infos.size(); ++i) {
                        if (i) std::cout << ',';
                        std::cout << json(binding.missing_infos[i]);
                    }
                    std::cout << "]},\"research_cost_columns\":[\"slot\",\"base\",\"cost\",\"left\",\"eureka_per_million\"]"
                              << ",\"research_left_unit\":" << json(team.profile() == content::TechnologyProfile::vp542 ? "hundredths" : "beakers")
                              << ",\"research_costs\":[";
                    bool first = true;
                    for (const auto &[slot, info] : technologies->entries) {
                        if (slot > INT64_MAX) throw std::overflow_error("Research technology ID exceeds int64");
                        if (!first) std::cout << ',';
                        first = false;
                        const auto tech = static_cast<std::int64_t>(slot);
                        const auto left = team.profile() == content::TechnologyProfile::vp542
                            ? runtime::vp_research_left_times100(*technologies, team, tech, f)
                            : runtime::bnw_research_left(*technologies, team, tech, f);
                        std::cout << '[' << slot << ',' << info.integers.at("Cost") << ','
                                  << runtime::team_research_cost(*technologies, team, tech, f) << ',' << left << ','
                                  << (team.profile() == content::TechnologyProfile::vp542 ? info.integers.at("EurekaPerMillion") : 0) << ']';
                    }
                    std::cout << ']';
                }
                std::cout << '}';
            }
            std::cout << "]}";
        }
        if (player_research) {
            const auto &s = *player_research;
            const bool vp = s.profile == content::TechnologyProfile::vp542;
            std::cout << ",\"player_research\":{\"fully_resolved\":" << (player_research_resolved ? "true" : "false")
                      << ",\"world_city_modifier\":" << s.world_city_modifier
                      << ",\"progress_unit\":" << json(vp ? "hundredths" : "beakers") << ",\"defines\":{";
            bool first = true;
            for (const auto *name : {"TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER", "TECH_COST_KNOWN_PREREQ_MODIFIER",
                                     "NUM_CITIES_COST_MOD_SCALING", "SCHOLAR_MINOR_ALLY_MULTIPLIER"}) {
                const auto *entry = global_defines->find(name);
                if (!entry) continue; // VP-only defines are absent in BNW's source cache.
                if (!first) std::cout << ',';
                first = false;
                std::cout << json(name) << ':' << std::get<std::int64_t>(entry->lookup.value);
            }
            std::cout << "},\"options\":{";
            first = true;
            for (const auto *name : {"DIPLOMACY_TECH_BONUSES", "BALANCE_VP", "BALANCE_PUPPET_CHANGES"}) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << json(name) << ':' << (vp && custom_options->enabled(name) ? "true" : "false");
            }
            std::cout << "},\"leagues\":[";
            first = true;
            for (const auto &league : s.leagues) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << "{\"members\":[";
                for (std::size_t i=0; i<league.members.size(); ++i) { if (i) std::cout << ','; std::cout << league.members[i]; }
                std::cout << "],\"modifiers\":[";
                for (std::size_t i=0; i<league.discovered_tech_modifiers.size(); ++i) { if (i) std::cout << ','; std::cout << league.discovered_tech_modifiers[i]; }
                std::cout << "]}";
            }
            std::cout << "],\"teams\":[";
            first = true;
            for (const auto &[id,t] : s.teams) {
                if (!first) std::cout << ',';
                first = false;
                std::cout << "{\"id\":" << id << ",\"alive_count\":" << t.alive_count
                          << ",\"minor\":" << (s.team_is_minor(*research_context,id) ? "true" : "false")
                          << ",\"major\":" << (s.team_is_major(*research_context,id) ? "true" : "false")
                          << ",\"civ_loop\":" << (s.civ_teams.contains(id) ? "true" : "false") << '}';
            }
            std::cout << "],\"cost_columns\":[\"tech\",\"known_teams\",\"possible_teams\",\"known_or_prerequisites\",\"catch_up\",\"league\",\"modifier\",\"city_modifier_times100\",\"team_cost\",\"cost\",\"progress\",\"completion_threshold_times100\"],\"players\":[";
            first = true;
            for (const auto &[id,p] : s.players) {
                if (!first) std::cout << ',';
                first = false;
                const auto vp_values=vp?runtime::vp_player_research_profile(p):runtime::VpPlayerResearchProfileState{};
                std::int64_t puppets=0, ignored=0, ignored_or_razing=0;
                for (const auto &[city,c] : p.cities) { puppets+=c.puppet; ignored+=c.ignore_happiness; ignored_or_razing+=c.ignore_happiness||c.razing; }
                std::cout << "{\"id\":" << id << ",\"current_research\":" << p.current_research() << ",\"queue\":[";
                for (std::size_t i=0; i<p.queue.size(); ++i) { if (i) std::cout << ','; std::cout << p.queue[i]; }
                std::cout << "],\"overflow_times100\":" << p.overflow_times100 << ",\"cities\":" << p.cities.size()
                          << ",\"puppets\":" << puppets << ",\"ignored\":" << ignored << ",\"ignored_or_razing\":" << ignored_or_razing
                          << ",\"max_effective_before\":" << player_research_before.at(id).first
                          << ",\"max_effective_after\":" << p.max_effective_cities << ",\"city_cost_modifier\":" << vp_values.city_cost_modifier
                          << ",\"cs_allies\":" << vp_values.cs_allies << ",\"science_from_league_before\":" << player_research_before.at(id).second
                          << ",\"science_from_league_after\":" << vp_values.science_from_league << ",\"spies\":" << vp_values.spies << ",\"borrowable_teams\":[";
                bool first_team=true;
                const auto own = research_context->players.at(id).team;
                for (auto other : s.civ_teams) {
                    const bool borrow = vp && custom_options->enabled("DIPLOMACY_TECH_BONUSES")
                        ? (vp_values.spies ? s.spy_at(*research_context,own,other) : s.embassy_at(own,other)) : s.teams.at(own).met.contains(other);
                    if (borrow) { if (!first_team) std::cout << ','; first_team=false; std::cout << other; }
                }
                std::cout << "],\"costs\":[";
                bool first_cost=true;
                for (const auto &[tech,q] : player_quotes.at(id)) {
                    if (!first_cost) std::cout << ',';
                    first_cost=false;
                    const auto &m=q.modifier;
                    std::cout << '[' << tech << ',' << m.known_teams << ',' << m.possible_teams << ',' << m.known_or_prerequisites
                              << ',' << m.catch_up << ',' << m.league << ',' << m.value << ',' << q.city_modifier_times100
                              << ',' << q.team_cost << ',' << q.cost << ',' << q.progress << ',' << q.completion_threshold_times100 << ']';
                }
                std::cout << "]}";
            }
            std::cout << "]}";
        }
        if (research_probe_report) std::cout << ",\"research_probe\":" << *research_probe_report;
        if (city_yields_report) std::cout << ",\"city_yields\":" << *city_yields_report;
        if (bnw_unhappiness_report) std::cout << ",\"bnw_unhappiness\":" << *bnw_unhappiness_report;
        if (maintenance_report) std::cout << ",\"maintenance\":" << *maintenance_report;
        if (route_economy_report) std::cout << ",\"route_economy\":" << *route_economy_report;
        if (plot_water_report) std::cout << ",\"plot_water\":" << *plot_water_report;
        if (plot_movement_report) std::cout << ",\"plot_movement\":" << *plot_movement_report;
        if (improvement_yield_report) std::cout << ",\"improvement_yields\":" << *improvement_yield_report;
        if (improvement_calculation_report) std::cout << ",\"improvement_calculation\":" << *improvement_calculation_report;
        if (bnw_nature_yield_report) std::cout << ",\"bnw_nature_yield\":" << *bnw_nature_yield_report;
        if (vp_nature_yield_report) std::cout << ",\"vp_nature_yield\":" << *vp_nature_yield_report;
        if (vp_plot_religion_report) std::cout << ",\"vp_plot_religion\":" << *vp_plot_religion_report;
        if (vp_player_plot_yield_report) std::cout << ",\"vp_player_plot_yield\":" << *vp_player_plot_yield_report;
        if (vp_plot_yield_report) std::cout << ",\"vp_plot_yield\":" << *vp_plot_yield_report;
        if (vp_city_food_report) std::cout << ",\"vp_city_food\":" << *vp_city_food_report;
        if (vp_city_growth_report) std::cout << ",\"vp_city_growth\":" << *vp_city_growth_report;
        if (vp_unit_supply_report) std::cout << ",\"vp_unit_supply\":" << *vp_unit_supply_report;
        if (vp_great_works_report) std::cout << ",\"vp_great_works\":" << *vp_great_works_report;
        if (vp_city_building_yields_report) std::cout << ",\"vp_city_building_yields\":" << *vp_city_building_yields_report;
        if (vp_city_religion_minor_yields_report) std::cout << ",\"vp_city_religion_minor_yields\":" << *vp_city_religion_minor_yields_report;
        if (vp_city_trait_yields_report) std::cout << ",\"vp_city_trait_yields\":" << *vp_city_trait_yields_report;
        if (vp_city_happiness_report) std::cout << ",\"vp_city_happiness\":" << *vp_city_happiness_report;
        if (vp_city_base_yields_report) std::cout << ",\"vp_city_base_yields\":" << *vp_city_base_yields_report;
        if (vp_corporations_report) std::cout << ",\"vp_corporations\":" << *vp_corporations_report;
        if (vp_city_production_report) std::cout << ",\"vp_city_production\":" << *vp_city_production_report;
        if (vp_building_production_report) std::cout << ",\"vp_building_production\":" << *vp_building_production_report;
        if (vp_unit_traits_report) std::cout << ",\"vp_unit_traits\":" << *vp_unit_traits_report;
        if (vp_player_unit_production_report) std::cout << ",\"vp_player_unit_production\":" << *vp_player_unit_production_report;
        if (vp_city_unit_production_report) std::cout << ",\"vp_city_unit_production\":" << *vp_city_unit_production_report;
        if (city_production_progress_report) std::cout << ",\"city_production_progress\":" << *city_production_progress_report;
        if (resource_visibility_report) std::cout << ",\"resource_visibility\":" << *resource_visibility_report;
        if (treasury_report) std::cout << ",\"treasury\":" << *treasury_report;
        if (science_report) std::cout << ",\"science\":" << *science_report;
        if (treasury_turn_report) std::cout << ",\"treasury_turn\":" << *treasury_turn_report;
        if (research_turn_report) std::cout << ",\"research_turn\":" << *research_turn_report;
        if (player_economy_research_stage_report)
            std::cout << ",\"player_economy_research_stage\":" << *player_economy_research_stage_report;
        if (research_selection_report) std::cout << ",\"research_selection\":" << *research_selection_report;
        if (research_ai_weights) {
            std::cout << ",\"research_ai_weights\":{\"profile\":\""
                      << (research_ai_weights->profile==content::TechnologyProfile::vp542?"vp542":"bnw")
                      << "\",\"players\":[";
            bool first=true;
            for(const auto &[id,weights]:research_ai_weights->players) {
                if(!first)std::cout << ',';
                first=false;
                std::cout << "{\"id\":" << id << ",\"weight_slots\":" << weights.weight_slots
                          << ",\"nonzero_weights\":" << weights.weights.size()
                          << ",\"unresolved_rows\":" << weights.unresolved_rows << '}';
            }
            std::cout << "]}";
        }
        if (lua_city_bombard_report) std::cout << ",\"lua_city_bombard\":" << *lua_city_bombard_report;
        if (lua_gameinfo_report) std::cout << ",\"lua_gameinfo\":" << *lua_gameinfo_report;
        if (lua_pregame_report) std::cout << ",\"lua_pregame\":" << *lua_pregame_report;
        if (lua_map_report) std::cout << ",\"lua_map\":" << *lua_map_report;
        if (lua_save_data_report) std::cout << ",\"lua_save_data\":" << *lua_save_data_report;
        if (saved_value_fallback_report) {
            std::cout << ",\"saved_value_fallback\":{\"copied\":" << saved_value_fallback_report->copied
                      << ",\"source\":" << json(*saved_value_fallback_source_kind)
                      << ",\"present\":" << saved_value_fallback_report->present
                      << ",\"present_null\":" << saved_value_fallback_report->present_null
                      << ",\"matching_source\":" << saved_value_fallback_report->matching_source
                      << ",\"missing_source\":" << saved_value_fallback_report->missing_source
                      << ",\"output\":" << (saved_value_fallback_output.empty()?std::string("null"):
                          json(assets::path_utf8(fs::absolute(saved_value_fallback_output))))
                      << ",\"output_sha256\":" << (saved_value_fallback_output_sha256?
                          json(*saved_value_fallback_output_sha256):std::string("null")) << '}';
        }
        if (lua_selected_entrypoints_report) std::cout << ",\"lua_selected_entrypoints\":" << *lua_selected_entrypoints_report;
        if (session_context) {
            const auto &snapshot = *terrain_snapshot;
            const auto &players = *snapshot.all_players;
            std::uint64_t cities = 0, units = 0;
            for (const auto &player : players.players) {
                cities += player.cities.cities.size();
                units += player.units.units.size();
            }
            std::cout << ",\"session\":{\"layout\":" << json(session_layout)
                      << ",\"dimension_source\":"
                      << json(session_layout == "vp542" ? "observed_rules_database_and_source_contract"
                                                       : "saved_collections_and_source_contract")
                      << ",\"full_profile_verified\":false,\"players\":" << players.players.size()
                      << ",\"teams\":" << snapshot.teams->teams.size()
                      << ",\"cities\":" << cities << ",\"units\":" << units
                      << ",\"payload_end_offset\":" << snapshot.payload_end_offset
                      << ",\"payload_fully_consumed\":"
                      << (snapshot.payload_end_offset == loaded.bytes.size() ? "true" : "false");
            if (session_layout == "vp542") {
                const auto &team = *saves::vp_team_dimensions(session_context->teams);
                const auto &p = players.context;
                const auto &vp = *p.profile_extensions.find<saves::VpAllPlayersWireDimensions>();
                std::cout << ",\"rules_dimensions\":{\"resources\":"
                          << saves::vp_roster_dimensions(session_context->roster)->resource_slots
                          << ",\"techs\":" << team.techs << ",\"buildings\":" << team.buildings
                          << ",\"policies\":" << saves::vp_player_strategy_dimensions(p.strategy)->policies
                          << ",\"city_strategies\":" << vp.city_strategies
                          << ",\"city_emphases\":" << vp.city_emphases << '}';
            }
            std::cout << '}';
        }
        if (terrain_snapshot) {
            const auto &map = *terrain_snapshot->map_terrain;
            std::cout << ",\"terrain_map\":{\"width\":" << map.grid.width
                      << ",\"height\":" << map.grid.height << ",\"cells\":" << map.cells.size()
                      << ",\"feature_definitions\":" << feature_definitions
                      << ",\"known_terrain_masks\":" << map.known_terrain_masks
                      << ",\"native_markup_complete\":" << (map.native_markup_complete ? "true" : "false")
                      << ",\"source\":\"observed_rules_database\",\"full_profile_verified\":false}";
        }
        if (terrain_grid) {
            std::cout << ",\"terrain_grid\":{\"width\":" << terrain_grid->width
                      << ",\"height\":" << terrain_grid->height << ",\"bytes\":" << terrain_grid->cells.size()
                      << ",\"mode\":" << *terrain_grid_mode
                      << ",\"mode_source\":" << json(terrain_mode_from_save ? "save_wrap_x" : "explicit")
                      << ",\"reserved_word\":0,\"auxiliary_pass_complete\":false,\"output\":"
                      << (terrain_grid_output.empty() ? std::string("null")
                          : json(assets::path_utf8(fs::absolute(terrain_grid_output)))) << '}';
            std::cout << ",\"terrain_rivers\":{\"edges\":" << terrain_rivers.size()
                      << ",\"chains\":" << terrain_river_network->chains().size()
                      << ",\"network_complete\":true,\"source_edges_complete\":true,\"geometry_complete\":false,\"output\":"
                      << (terrain_rivers_output.empty() ? std::string("null")
                          : json(assets::path_utf8(fs::absolute(terrain_rivers_output)))) << '}';
            std::cout << ",\"terrain_auxiliary\":{\"relief_records\":" << terrain_auxiliary->relief.size()
                      << ",\"feature_2_records\":" << terrain_auxiliary->feature2.size()
                      << ",\"feature_0_records\":" << terrain_auxiliary->feature0.size()
                      << ",\"feature_4_records\":" << terrain_auxiliary->feature4.size()
                      << ",\"source_records_complete\":true,\"append_owners_complete\":false,\"relief_output\":"
                      << (terrain_relief_output.empty() ? std::string("null")
                          : json(assets::path_utf8(fs::absolute(terrain_relief_output))))
                      << ",\"feature_2_output\":"
                      << (terrain_feature2_output.empty() ? std::string("null")
                          : json(assets::path_utf8(fs::absolute(terrain_feature2_output)))) << '}';
        }
        if (saved_scene_sources) {
            const auto &fog = saved_scene_sources->fog_counts;
            std::cout << ",\"saved_scene_sources\":{\"source\":\"one_saved_session\",\"fog\":{\"visible\":"
                      << fog.visible << ",\"explored\":" << fog.explored
                      << ",\"unexplored\":" << fog.unexplored
                      << ",\"visible_unrevealed\":" << fog.visible_unrevealed
                      << "},\"rendered\":false}";
        }
        std::cout << ",\"rules_database_ready\":"
                  << (rules_ready ? "true" : "false") << "}\n";
        return rules_ready ? 0 : 3;
    } catch (const std::exception &error) {
        std::cerr << "saved_rules=FAIL " << error.what() << '\n';
        return 2;
    }
}
