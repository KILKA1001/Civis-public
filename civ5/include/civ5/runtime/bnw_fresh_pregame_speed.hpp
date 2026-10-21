#pragma once
#include "civ5/runtime/bnw_fresh_civilization_info_collection.hpp"
#include "civ5/runtime/bnw_creation_config.hpp"
#include <cstdint>
#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace civ5::runtime {
class BnwFreshPregameRoster;
struct BnwFreshPregameRosterObservation;
enum class BnwFreshPregameSpeedError { parent_missing, wrong_thread, retired,
    wrong_phase, native_source_unavailable };
class BnwFreshPregameSpeedRefusal : public std::runtime_error {
 public:
    BnwFreshPregameSpeedRefusal(BnwFreshPregameSpeedError,const std::string&);
    BnwFreshPregameSpeedError code() const noexcept {return code_;}
 private:BnwFreshPregameSpeedError code_;
};
enum class BnwFreshPregameSpeedBoundary { constructed, failed_prefix,
    selected_speed_setter_complete };
// Explicit new-game menu inputs, never an executed setter/initialization receipt.
enum class BnwFreshGameType : std::int32_t { single_player=0, network=1, hotseat=2, email=3 };
struct BnwFreshPregameInitRequest {
    std::uint64_t map_seed,sync_seed;
    BnwFreshGameType game_type;
};
struct BnwFreshPregameInitObservation {
    bool attempted,complete;
    std::int32_t era,game_type,game_start_type;
    std::uint32_t map_seed,sync_seed;
    bool internet_game;
};
struct BnwFreshPregameCustomOption {std::string name;std::int32_t value;};
struct BnwFreshPregameOptionsObservation {
    bool attempted,complete;
    std::array<bool,4> multiplayer;
    std::vector<BnwFreshPregameCustomOption> custom;
    std::map<std::uint32_t,std::size_t> hash_indices;
    std::array<std::int32_t,22> enum_values;
    bool quick_combat;
};
struct BnwFreshPregameTurnIncrement {
    std::optional<std::int32_t> month_increment,turns;
};
// This source subset executes resetGame before selected setting writes.
// It does not execute the platform Globals startup or stock settings/slots.
enum class BnwFreshPregameResetBoundary { constructor_diagnostic,
    reset_game_required, failed_prefix, reset_game_settings_assignment_required };
struct BnwFreshPregameResetObservation {
    BnwFreshPregameResetBoundary boundary;
    std::int32_t cached_standard_speed,cached_standard_era,victory_count;
    std::vector<bool> victories;
    std::map<std::string,std::int32_t,std::less<>> integer_stores;
    std::map<std::string,bool,std::less<>> boolean_stores;
    std::map<std::string,float,std::less<>> float_stores;
    std::map<std::string,std::string,std::less<>> string_stores;
};
enum class BnwFreshPregameMenuBoundary { menu_request_required, failed_prefix,
    constructor_slots_reset_complete, reset_game_options_required,
    load_pregame_settings_required };
enum class BnwFreshPregameCachedGlobalsBoundary { constructor_diagnostic,
    failed_prefix, selected_database_subset_stored };
struct BnwFreshPregameCachedGlobalsObservation {
    BnwFreshPregameCachedGlobalsBoundary boundary;
    std::int32_t standard_handicap,multiplayer_handicap,standard_speed,standard_era,ai_handicap;
};
struct BnwFreshPregameMenuObservation {
    BnwFreshPregameMenuBoundary boundary;
    bool private_game;
    bool options_reset_attempted;
    std::vector<BnwFreshPregameCustomOption> map_options;
};
enum class BnwFreshPregameWorldSetterBoundary { not_called, failed_prefix,
    cache_stored, slots_reset_returned };
struct BnwFreshPregameWorldSetterObservation {
    BnwFreshPregameWorldSetterBoundary boundary;
    std::uint64_t call_count;
    std::string requested_type;
};

// Observations of actual native Climate/SeaLevel caches, never a Map init grant.
enum class BnwFreshPregameEnvironmentSetterBoundary { not_called, failed_prefix,
    cache_and_selection_stored };
struct BnwFreshPregameEnvironmentInfoObservation {
    BnwFreshPregameEnvironmentSetterBoundary boundary;
    std::uint64_t call_count;
    std::string requested_type;
    std::int32_t selected_id;
    std::map<std::string,std::int32_t,std::less<>> numbers;
    std::map<std::string,float,std::less<>> floats;
    std::map<std::string,std::string,std::less<>> text;
    std::vector<std::string> order;
};
struct BnwFreshPregameEnvironmentObservation {
    BnwFreshPregameEnvironmentInfoObservation climate,sea_level;
};

struct BnwFreshPregameSpeedObservation {
    BnwFreshPregameSpeedBoundary boundary;
    std::int32_t speed,timer;
    std::map<std::string,std::int32_t,std::less<>> timer_numbers;
    std::map<std::string,std::string,std::less<>> timer_text;
    std::vector<std::string> order;
    BnwFreshPregameInitObservation init_inputs;
    BnwFreshPregameOptionsObservation options;
    std::int32_t game_turn;
    std::string admin_password;
    std::int32_t turn_count;
    std::optional<std::vector<BnwFreshPregameTurnIncrement>> turn_cache;
    BnwFreshPregameResetObservation reset;
    BnwFreshPregameMenuObservation menu;
    BnwFreshPregameCachedGlobalsObservation cached_globals;
    BnwFreshPregameWorldSetterObservation world_setter;
};
// Genuine owned storage for this narrow native setter slice. Never a full
// loadFromIni, player roster, localization or fresh Game grant. The separate
// reset factory executes only resetGame and preserves later mandatory setup.
class BnwFreshPregameSpeed {
 public:
    static std::shared_ptr<BnwFreshPregameSpeed> from_creation_config(
        std::shared_ptr<const BnwCreationConfig>, std::filesystem::path selected_config_ini);
    // Genuine SAME Config/DB reset-consumed cache subset; no caller values.
    static std::shared_ptr<BnwFreshPregameSpeed> from_creation_config_for_reset_setup(
        std::shared_ptr<const BnwCreationConfig>, std::filesystem::path selected_config_ini);
    void advance_reset_game();
    // Actual single-player menu setters then SAME privately owned ResetSlots.
    // This source subset refuses at an unavailable reached Globals operand.
    void advance_single_player_menu_prefix();
    // Closed continuation only after the actual constructor-slot return.
    // Executes options and stops at actual LoadPreGameSettings; no full menu.
    void advance_single_player_menu_options_prefix();
    // A real explicit native string-setting request, not a settings receipt.
    // Repeated completed calls issue new ordered resets; failed calls quarantine.
    void set_selected_world_size(std::string selected_type);
    // Explicit native string setters. Each executes selected SQL/CacheResult;
    // neither completes LoadPreGameSettings nor grants Map init authority.
    void set_selected_climate(std::string selected_type);
    void set_selected_sea_level(std::string selected_type);
    BnwFreshPregameEnvironmentObservation environment_observation()const;
    // Actual native cache getters: equal-ID returns existing Info; otherwise
    // genuine numeric ROWID SelectAt/CacheResult. Never Map/Game authority.
    BnwFreshPregameEnvironmentInfoObservation selected_climate_info()const;
    BnwFreshPregameEnvironmentInfoObservation selected_sea_level_info()const;
    // SAME owned WorldInfo getter. Copied diagnostics never authorize Map
    // initialization; equal IDs preserve every cache/roster write count.
    BnwFreshPregameRosterObservation selected_world_info()const;
    std::shared_ptr<const BnwFreshPregameRoster> roster() const;
    void advance_selected_speed();
    // Actual fresh request setters on SAME owned mutable Pregame; once.
    // No imported seed, timer/default clock, roster, or readiness substitution.
    void advance_selected_init_inputs(BnwFreshPregameInitRequest);
    void require_current() const;
    BnwFreshPregameSpeedObservation observe() const;
 private:
    friend class BnwFreshGame;
    friend class BnwFreshCivilizationInfoCollection;
    void advance_ordered_globals_info_prefix_from_game();
    BnwFreshOrderedInfoObservation ordered_info_observation_from_game()const;
    std::string localized_for_ordered_info(std::string_view);
    friend class BnwFreshPregameRoster;
    std::int32_t current_game_type_for_roster() const;
    std::int32_t current_cached_standard_handicap_for_roster() const;
    std::int32_t current_cached_ai_handicap_for_roster() const;
    std::string current_world_localized_for_roster(std::string_view) const;
    std::uint64_t current_world_reset_operation_for_roster() const;
    std::weak_ptr<BnwFreshPregameSpeed> current_parent_for_roster() const;
    void require_same_roster(const BnwFreshPregameRoster&) const;
    std::int32_t current_train_percent() const;
    BnwFreshPregameInitObservation current_init_inputs() const;
    void verify_start_era_from_game();
    void clear_single_player_options_from_game();
    bool current_shuffle_teams_from_game() const;
    // Closed Game-only reached getters/setter; no public turn/password grant.
    bool current_lock_mods_from_game() const;
    std::int32_t current_game_turn_from_game() const;
    void set_admin_password_from_game(std::string);
    const std::vector<BnwFreshPregameTurnIncrement>& current_turn_increments_from_game();
    const BnwCreationConfig* actual_config() const;
    struct Impl;
    explicit BnwFreshPregameSpeed(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
