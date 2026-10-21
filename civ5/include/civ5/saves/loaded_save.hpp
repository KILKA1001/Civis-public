#pragma once
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/save_payload.hpp"
#include <optional>
#include <span>

namespace civ5::saves {
enum class SaveContainerKind { civ5, civis };

const char *save_container_kind_name(SaveContainerKind kind);

// One immutable input snapshot for rules, graphics and diagnostics. Original
// Civ V framing is retained when available; a Civis container already stores
// the validated decompressed payload and therefore has no zlib frame.
struct LoadedSave {
    SaveContainerKind container = SaveContainerKind::civ5;
    SaveHeader header;
    std::optional<std::string> source_sha256;
    ChunkedFrame frame;
    InflateObservation inflate;
    std::vector<std::uint8_t> bytes;
    // Original uncompressed prefix, including all setup/engine bytes. Old
    // Civis imports may lack it; consumers must not infer setup from the header.
    std::optional<std::vector<std::uint8_t>> source_prefix;
    std::optional<std::vector<std::uint8_t>> saved_database_override;
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
    std::optional<StandaloneServicesConfig> standalone_services;
    std::optional<std::vector<std::uint8_t>> live_bnw_unit_mission_continuation = {};
    std::optional<std::vector<std::uint8_t>> live_bnw_military_requests = {};
    std::optional<std::vector<std::uint8_t>> live_bnw_unit_serial = {};
    std::optional<std::vector<std::uint8_t>> live_bnw_population_totals = {};
    // Ordered once-owner journal; bytes alone never mint a Food/Population owner.
    std::optional<std::vector<std::uint8_t>> live_bnw_food_growth_journal = {};
};

LoadedSave read_loaded_save(const std::filesystem::path &path,
                            SavePayloadBudget payload_budget = {},
                            CivisSaveBudget civis_budget = {},
                            HeaderBudget metadata_budget = {});
// Retains original payload/setup and source provenance. The optional image
// replaces current mod data; omission preserves an existing Civis override.
// input_sha256 is the identity of the input file captured with this snapshot;
// an imported Civis container retains its original source hash instead.
CivisSaveDocument make_civis_import_document(const LoadedSave &source,std::string_view input_sha256,
    std::optional<std::span<const std::uint8_t>> database_override = {});
} // namespace civ5::saves
