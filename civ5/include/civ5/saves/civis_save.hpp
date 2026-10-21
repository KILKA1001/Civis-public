#pragma once
#include "civ5/saves/save_header.hpp"
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

namespace civ5::saves {
inline constexpr std::string_view civis_save_extension = ".CivisSave";
enum class CivisSaveSectionId : std::uint32_t {
    imported_payload = 1,
    rules_snapshot = 2,
    session_state = 3,
    source_metadata = 4,
    source_prefix = 5,
    saved_database = 6,
    live_city_production = 7,
    live_team_research = 8,
    vp_research_profile = 9,
    live_treasury = 10,
    live_research_turn = 11,
    vp_science_profile = 12,
    live_simulation_random = 13,
    vp_espionage_profile = 14,
    live_research_selection = 15,
    live_bnw_game_clock = 16,
    live_bnw_map_plots = 17,
    live_bnw_units = 18,
    live_bnw_area_units = 19,
    live_bnw_player_proximity = 20,
    live_bnw_team_meet = 21,
    live_bnw_exploration_dirty = 22,
    live_bnw_player_strategy = 23,
    live_bnw_player_core = 24,
    live_bnw_city_core = 25,
    live_bnw_city_production = 26,
    live_bnw_city_management = 27,
    live_bnw_city_tail = 28,
    live_bnw_treasury_balance = 30,
    live_bnw_team_progress = 31,
    live_bnw_culture_bomb_timer = 32,
    live_bnw_research_queue = 33,
    live_bnw_maintenance_expense = 34,
    live_bnw_player_knowledge = 35,
    live_bnw_player_administration = 36,
    live_bnw_game_supporting = 37,
    live_bnw_game_score_replay = 38,
    live_bnw_game_deals = 39,
    live_bnw_team_turn = 40,
    live_bnw_ui_transients = 41,
    standalone_services = 42,
    live_bnw_unit_mission_continuation = 43,
    live_bnw_military_requests = 44,
    live_bnw_unit_serial = 45,
    live_bnw_population_totals = 46,
    live_bnw_food_growth_journal = 47,
};
struct CivisSaveSection {
    std::uint32_t id = 0;
    std::uint32_t flags = 0;
    std::vector<std::uint8_t> bytes;
};
// Opaque GameCore-owned state. The common save container keeps section identity
// and bytes; profile modules decide how to decode and apply them.
using ProfileSaveSections = std::map<std::uint32_t, std::vector<std::uint8_t>>;
inline const std::vector<std::uint8_t> *profile_section(const ProfileSaveSections &sections,
                                                        CivisSaveSectionId id) {
    const auto found = sections.find(static_cast<std::uint32_t>(id));
    return found == sections.end() ? nullptr : &found->second;
}
struct CivisSaveDocument {
    std::uint32_t version = 1;
    std::uint32_t flags = 0;
    std::vector<CivisSaveSection> sections;
};
struct CivisSaveMetadata {
    std::string source_sha256;
    SaveHeader header;
};
// Explicit standalone host configuration; never interpreted as original BNW
// Steam/logging flags. Other backends and schemas require their own implementation.
enum class StandaloneServiceBackend : std::uint32_t { backend_disabled = 1 };
struct StandaloneServicesConfig {
    std::uint32_t schema = 1;
    StandaloneServiceBackend steam_achievements = StandaloneServiceBackend::backend_disabled;
    StandaloneServiceBackend culture_diagnostics = StandaloneServiceBackend::backend_disabled;
    bool operator==(const StandaloneServicesConfig &) const = default;
};
void require_supported_standalone_services(const StandaloneServicesConfig &config);
CivisSaveSection make_standalone_services_section(const StandaloneServicesConfig &config);
StandaloneServicesConfig read_standalone_services_section(const CivisSaveSection &section);

struct CivisImportedSave {
    CivisSaveMetadata metadata;
    std::vector<std::uint8_t> payload;
    // Absent in legacy two-section imports; absence is not an empty setup.
    std::optional<std::vector<std::uint8_t>> source_prefix;
    // Current mod state supersedes the database retained inside payload.
    // Absence means use the imported image; an empty override is invalid.
    std::optional<std::vector<std::uint8_t>> saved_database_override;
    // Versioned x64 city production state. The original payload remains the
    // immutable import baseline; runtime consumers must apply this section.
    std::optional<std::vector<std::uint8_t>> live_city_production;
    std::optional<std::vector<std::uint8_t>> live_team_research;
    std::optional<std::vector<std::uint8_t>> live_treasury;
    std::optional<std::vector<std::uint8_t>> live_research_turn;
    std::optional<std::vector<std::uint8_t>> live_research_selection;
    std::optional<std::vector<std::uint8_t>> live_simulation_random;
    std::optional<std::vector<std::uint8_t>> live_bnw_game_clock;
    std::optional<std::vector<std::uint8_t>> live_bnw_map_plots;
    std::optional<std::vector<std::uint8_t>> live_bnw_units;
    std::optional<std::vector<std::uint8_t>> live_bnw_area_units;
    std::optional<std::vector<std::uint8_t>> live_bnw_player_proximity;
    std::optional<std::vector<std::uint8_t>> live_bnw_team_meet;
    std::optional<std::vector<std::uint8_t>> live_bnw_exploration_dirty;
    std::optional<std::vector<std::uint8_t>> live_bnw_player_strategy;
    std::optional<std::vector<std::uint8_t>> live_bnw_player_core;
    std::optional<std::vector<std::uint8_t>> live_bnw_city_core;
    std::optional<std::vector<std::uint8_t>> live_bnw_city_production;
    std::optional<std::vector<std::uint8_t>> live_bnw_city_management;
    std::optional<std::vector<std::uint8_t>> live_bnw_city_tail;
    std::optional<std::vector<std::uint8_t>> live_bnw_treasury_balance;
    std::optional<std::vector<std::uint8_t>> live_bnw_team_progress;
    std::optional<std::vector<std::uint8_t>> live_bnw_culture_bomb_timer;
    std::optional<std::vector<std::uint8_t>> live_bnw_research_queue;
    std::optional<std::vector<std::uint8_t>> live_bnw_maintenance_expense;
    std::optional<std::vector<std::uint8_t>> live_bnw_player_knowledge;
    std::optional<std::vector<std::uint8_t>> live_bnw_player_administration;
    std::optional<std::vector<std::uint8_t>> live_bnw_game_supporting;
    std::optional<std::vector<std::uint8_t>> live_bnw_game_score_replay;
    std::optional<std::vector<std::uint8_t>> live_bnw_game_deals;
    std::optional<std::vector<std::uint8_t>> live_bnw_team_turn;
    std::optional<std::vector<std::uint8_t>> live_bnw_ui_transients;
    ProfileSaveSections profile_sections;
    // Absence in legacy containers is retained, never inferred as backend_disabled.
    std::optional<StandaloneServicesConfig> standalone_services;
    // Exact canonical continuation journal, not applied world effects. Unit
    // Core must restore it after the SAME world, clock and RNG are restored.
    std::optional<std::vector<std::uint8_t>> live_bnw_unit_mission_continuation = {};
    std::optional<std::vector<std::uint8_t>> live_bnw_military_requests = {};
    // Parsed serial overlay remains quarantined until SAME canonical allocator restore.
    std::optional<std::vector<std::uint8_t>> live_bnw_unit_serial = {};
    // Unfinished native population totals; no growth continuation owner is inferred.
    std::optional<std::vector<std::uint8_t>> live_bnw_population_totals = {};
    // Ordered once-owner journal; bytes alone never mint a Food/Population owner.
    std::optional<std::vector<std::uint8_t>> live_bnw_food_growth_journal = {};
};
// These are host-side budgets. On-disk lengths and counters are always 64-bit;
// no original Civ V/x86 container capacity is reused.
struct CivisSaveBudget {
    std::uint64_t maximum_file_bytes = 8ULL * 1024 * 1024 * 1024;
    std::uint64_t maximum_sections = 1'000'000;
    std::uint64_t maximum_section_bytes = 8ULL * 1024 * 1024 * 1024;
    std::uint64_t maximum_total_section_bytes = 8ULL * 1024 * 1024 * 1024;
};
// Creates a new file exclusively. Never truncates an existing save or follows
// an existing output link; callers must choose a new output path.
void write_civis_save(const std::filesystem::path &path, const CivisSaveDocument &document,
                     CivisSaveBudget budget = {});
CivisSaveDocument read_civis_save(const std::filesystem::path &path, CivisSaveBudget budget = {});
CivisSaveSection make_save_metadata_section(const CivisSaveMetadata &metadata, HeaderBudget budget = {});
CivisSaveMetadata read_save_metadata_section(const CivisSaveSection &section, HeaderBudget budget = {});
// Imported payload + metadata, with optional retained original prefix. Legacy
// imports remain readable and explicitly lack setup bytes. A saved_database
// section replaces only mod database state. A live production section is
// retained for the runtime to apply; unknown session/rules sections reject.
// Structural import validation uses the SAME parser as read_imported_civis_save.
// Container version, uniqueness and byte budgets are checked by the writer or
// reader first. This borrows bytes, never installs runtime state or grants
// continuation, and does not validate opaque GameCore codec semantics.
void validate_civis_import_document(const CivisSaveDocument &document,
                                   HeaderBudget metadata_budget = {});
CivisImportedSave read_imported_civis_save(const std::filesystem::path &path,
                                         CivisSaveBudget budget = {}, HeaderBudget metadata_budget = {});
} // namespace civ5::saves
